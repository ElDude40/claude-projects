#include "obd_client.h"

#include "config.h"
#include "dtc_descriptions.h"
#include "obd_parser.h"

namespace {

// Polled every cycle: values that change quickly while driving.
const uint8_t kFastPids[] = {0x0C /*RPM*/, 0x0D /*speed*/, 0x11 /*throttle*/, 0x04 /*load*/};

// Polled round-robin, a couple per cycle. 0x00 is a placeholder for ATRV.
const uint8_t kSlowPids[] = {0x01 /*MIL/DTC count*/, 0x05 /*coolant*/, 0x0F /*intake*/, 0x2F /*fuel*/,
                             0x42 /*module V*/,      0x10 /*MAF*/,     0x0B /*MAP*/,    0x0E /*timing*/,
                             0x06 /*STFT*/,          0x07 /*LTFT*/,    0x5C /*oil*/,    0x46 /*ambient*/,
                             0x1F /*run time*/,      0x00 /*ATRV*/};
const size_t kSlowPerCycle = 2;
const uint8_t kMaxFailedCycles = 5;

std::string toStd(const String& s) { return std::string(s.c_str()); }

void invalidateLive(VehicleData& d) {
  Reading* readings[] = {&d.rpm,          &d.speedKmh,     &d.coolantC,   &d.intakeTempC,  &d.oilTempC,
                         &d.ambientTempC, &d.engineLoadPct, &d.throttlePct, &d.fuelLevelPct, &d.mafGs,
                         &d.mapKpa,       &d.timingDeg,    &d.shortTrimPct, &d.longTrimPct, &d.runTimeSec,
                         &d.moduleVoltage};
  for (Reading* r : readings) r->valid = false;
}

}  // namespace

void ObdClient::start(int core) {
  mutex_ = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(taskEntry, "obd", 8192, this, 1, nullptr, core);
}

VehicleData ObdClient::snapshot() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  VehicleData copy = data_;
  xSemaphoreGive(mutex_);
  return copy;
}

void ObdClient::taskEntry(void* self) { static_cast<ObdClient*>(self)->run(); }

void ObdClient::setLink(LinkState s) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.link = s;
  if (s != LinkState::Online) invalidateLive(data_);
  xSemaphoreGive(mutex_);
}

String ObdClient::query(const char* cmd, uint32_t timeoutMs) {
  return elm_.send(cmd, timeoutMs ? timeoutMs : ELM_COMMAND_TIMEOUT_MS);
}

void ObdClient::run() {
  setLink(LinkState::Starting);
  while (!elm_.begin(DEVICE_BT_NAME, OBD_ADAPTER_PIN)) {
    Serial.println("[BT] Failed to start Bluetooth, retrying");
    vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAY_MS));
  }

  LinkState link = LinkState::Connecting;
  for (;;) {
    if (!elm_.isConnected()) {
      if (link == LinkState::Online || link == LinkState::NoEcu || link == LinkState::SearchingEcu) {
        Serial.println("[BT] Adapter connection lost");
      }
      link = LinkState::Connecting;
      setLink(link);
      if (!connectAndInit()) {
        vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAY_MS));
        continue;
      }
      link = LinkState::SearchingEcu;
      setLink(link);
    }

    if (link != LinkState::Online) {
      if (!detectEcu()) {
        link = LinkState::NoEcu;
        setLink(link);
        readBatteryVoltage();  // still useful with the ignition off
        vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAY_MS));
        continue;
      }
      readSupportedPids();
      readVehicleInfo();
      readDtcs();
      consecutiveFailures_ = 0;
      link = LinkState::Online;
      setLink(link);
    }

    if (dtcClearRequested_) {
      dtcClearRequested_ = false;
      clearDtcs();
    }
    if (dtcReadRequested_ || millis() - lastDtcReadMs_ > DTC_REFRESH_INTERVAL_MS) {
      dtcReadRequested_ = false;
      readDtcs();
    }

    pollLiveData();
    if (consecutiveFailures_ >= kMaxFailedCycles) {
      Serial.println("[OBD] ECU stopped answering");
      link = LinkState::NoEcu;
      setLink(link);
    }

    vTaskDelay(pdMS_TO_TICKS(LIVE_DATA_INTERVAL_MS));
  }
}

bool ObdClient::connectAndInit() {
  if (!elm_.connect(OBD_ADAPTER_NAME, OBD_ADAPTER_MAC)) {
    Serial.println("[BT] Adapter not found");
    return false;
  }
  setLink(LinkState::Initializing);
  if (!elm_.initialize()) {
    Serial.println("[ELM] Initialization failed");
    elm_.disconnect();
    return false;
  }
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.adapterVersion = elm_.version();
  xSemaphoreGive(mutex_);
  return true;
}

bool ObdClient::detectEcu() {
  // The first request triggers the adapter's protocol search, which can take
  // several seconds.
  String r = query("0100", ELM_SEARCH_TIMEOUT_MS);
  memset(supported_, 0, sizeof(supported_));
  if (!obd::parseSupportedPids(toStd(r), 0x00, supported_)) {
    Serial.printf("[OBD] No ECU response: \"%s\"\n", r.c_str());
    return false;
  }

  // ATDPN returns the protocol number, prefixed with 'A' when auto-detected
  // (e.g. "A6" = auto-detected ISO 15765-4 CAN). Protocols 6-C are CAN.
  String num = query("ATDPN");
  num.trim();
  char p = num.length() ? num.charAt(num.length() - 1) : '0';
  isCan_ = (p >= '6' && p <= '9') || (p >= 'A' && p <= 'C');

  String desc = query("ATDP");
  desc.replace("AUTO, ", "");
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.protocol = desc;
  xSemaphoreGive(mutex_);
  Serial.printf("[OBD] ECU found, protocol %s (%s)\n", desc.c_str(), isCan_ ? "CAN" : "legacy");
  return true;
}

void ObdClient::readSupportedPids() {
  for (uint8_t base = 0x20; base <= 0x60; base += 0x20) {
    if (!supported_[base]) break;  // the previous range says whether this one exists
    char cmd[5];
    snprintf(cmd, sizeof(cmd), "01%02X", base);
    obd::parseSupportedPids(toStd(query(cmd)), base, supported_);
  }
  int count = 0;
  for (int i = 1; i < 256; ++i) {
    if (supported_[i] && i % 0x20 != 0) ++count;  // don't count the range markers
  }
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.supportedPidCount = count;
  xSemaphoreGive(mutex_);
}

void ObdClient::readVehicleInfo() {
  std::string vin = obd::parseVin(toStd(query("0902", 5000)));
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.vin = vin.empty() ? String("Not reported") : String(vin.c_str());
  xSemaphoreGive(mutex_);
}

void ObdClient::pollLiveData() {
  bool anyOk = false;
  for (uint8_t pid : kFastPids) anyOk |= pollPid(pid);

  for (size_t i = 0; i < kSlowPerCycle; ++i) {
    uint8_t pid = kSlowPids[slowIndex_];
    slowIndex_ = (slowIndex_ + 1) % (sizeof(kSlowPids) / sizeof(kSlowPids[0]));
    if (pid == 0x00) {
      readBatteryVoltage();
    } else {
      anyOk |= pollPid(pid);
    }
  }
  consecutiveFailures_ = anyOk ? 0 : consecutiveFailures_ + 1;
}

bool ObdClient::pollPid(uint8_t pid) {
  if (!supported_[pid]) return false;

  char cmd[5];
  snprintf(cmd, sizeof(cmd), "01%02X", pid);
  std::vector<uint8_t> b;
  if (!obd::parsePidBytes(toStd(query(cmd)), pid, b)) return false;

  const float A = b[0];
  const float B = b.size() > 1 ? b[1] : 0;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  VehicleData& d = data_;
  switch (pid) {
    case 0x01:
      d.milOn = b[0] & 0x80;
      d.reportedDtcCount = b[0] & 0x7F;
      break;
    case 0x04: d.engineLoadPct.set(A * 100.0f / 255.0f); break;
    case 0x05: d.coolantC.set(A - 40); break;
    case 0x06: d.shortTrimPct.set((A - 128) * 100.0f / 128.0f); break;
    case 0x07: d.longTrimPct.set((A - 128) * 100.0f / 128.0f); break;
    case 0x0B: d.mapKpa.set(A); break;
    case 0x0C: d.rpm.set((A * 256 + B) / 4.0f); break;
    case 0x0D: d.speedKmh.set(A); break;
    case 0x0E: d.timingDeg.set(A / 2.0f - 64); break;
    case 0x0F: d.intakeTempC.set(A - 40); break;
    case 0x10: d.mafGs.set((A * 256 + B) / 100.0f); break;
    case 0x11: d.throttlePct.set(A * 100.0f / 255.0f); break;
    case 0x1F: d.runTimeSec.set(A * 256 + B); break;
    case 0x2F: d.fuelLevelPct.set(A * 100.0f / 255.0f); break;
    case 0x42: d.moduleVoltage.set((A * 256 + B) / 1000.0f); break;
    case 0x46: d.ambientTempC.set(A - 40); break;
    case 0x5C: d.oilTempC.set(A - 40); break;
  }
  xSemaphoreGive(mutex_);
  return true;
}

void ObdClient::readBatteryVoltage() {
  String r = query("ATRV");  // e.g. "12.6V"
  float v = r.toFloat();
  if (v > 0) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    data_.batteryVoltage.set(v);
    xSemaphoreGive(mutex_);
  }
}

void ObdClient::readDtcs() {
  lastDtcReadMs_ = millis();

  String stored = query("03", 5000);
  String pending = query("07", 5000);
  if (stored.length() == 0) return;  // timeout: keep the previous list

  // "NO DATA" on mode 03 simply means there are no stored codes.
  std::vector<std::string> storedCodes = obd::parseDtcResponse(toStd(stored), 0x03, isCan_);
  std::vector<std::string> pendingCodes = obd::parseDtcResponse(toStd(pending), 0x07, isCan_);

  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.storedDtcs = storedCodes;
  data_.pendingDtcs = pendingCodes;
  data_.dtcsRead = true;
  data_.dtcReadAtMs = lastDtcReadMs_;
  xSemaphoreGive(mutex_);
  Serial.printf("[OBD] %u stored, %u pending trouble codes\n", (unsigned)storedCodes.size(),
                (unsigned)pendingCodes.size());
  for (const auto& c : storedCodes) Serial.printf("  stored  %s  %s\n", c.c_str(), describeDtc(c.c_str()));
  for (const auto& c : pendingCodes) Serial.printf("  pending %s  %s\n", c.c_str(), describeDtc(c.c_str()));
}

void ObdClient::clearDtcs() {
  // Mode 04 clears stored codes, freeze frames and resets readiness monitors.
  String r = query("04", 5000);
  String msg = r.indexOf("44") >= 0 ? "Codes cleared" : "Clear failed (ignition on, engine off?)";
  Serial.printf("[OBD] Clear DTCs -> \"%s\"\n", r.c_str());
  xSemaphoreTake(mutex_, portMAX_DELAY);
  data_.lastActionMsg = msg;
  xSemaphoreGive(mutex_);
  delay(500);
  readDtcs();
}
