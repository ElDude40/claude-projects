#include "elm327.h"

#include "config.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error "Bluetooth Classic is required. Use an original ESP32 (not S2/S3/C3/C6)."
#endif

bool Elm327::begin(const char* deviceName, const char* pin) {
  if (!bt_.begin(deviceName, true)) {  // true = master (we initiate the connection)
    return false;
  }
  bt_.setPin(pin);
  return true;
}

bool Elm327::connect(const char* adapterName, const char* adapterMac) {
  bool ok = false;
  if (adapterMac != nullptr && strlen(adapterMac) == 17) {
    uint8_t mac[6];
    if (sscanf(adapterMac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4],
               &mac[5]) == 6) {
      Serial.printf("[BT] Connecting to %s\n", adapterMac);
      ok = bt_.connect(mac);
    }
  } else {
    Serial.printf("[BT] Connecting to \"%s\"\n", adapterName);
    ok = bt_.connect(String(adapterName));
  }
  return ok && bt_.connected(5000);
}

void Elm327::disconnect() { bt_.disconnect(); }

bool Elm327::isConnected() { return bt_.connected(0); }

void Elm327::flushInput() {
  while (bt_.available()) bt_.read();
}

String Elm327::send(const char* command, uint32_t timeoutMs) {
  flushInput();
  bt_.print(command);
  bt_.print('\r');

  String response;
  const uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (bt_.available()) {
      char c = static_cast<char>(bt_.read());
      if (c == '>') {
        response.trim();
        return response;
      }
      if (c != '\0') response += c;
    }
    delay(2);
  }
  Serial.printf("[ELM] Timeout on \"%s\"\n", command);
  return String();
}

bool Elm327::initialize() {
  // ATZ restarts the chip; some clones answer slowly, so give it extra time.
  String reset = send("ATZ", 3000);
  delay(500);
  if (reset.length() == 0) {
    reset = send("ATZ", 3000);  // the first command after pairing is often lost
  }

  static const char* const kSetup[] = {
      "ATE0",   // echo off
      "ATL0",   // no line feeds
      "ATS0",   // no spaces between bytes
      "ATH0",   // no CAN headers
      "ATAT1",  // adaptive timing
      "ATSP0",  // automatic protocol detection
  };
  for (const char* cmd : kSetup) {
    String r = send(cmd, ELM_COMMAND_TIMEOUT_MS);
    if (r.indexOf("OK") < 0) {
      Serial.printf("[ELM] \"%s\" -> \"%s\"\n", cmd, r.c_str());
      return false;
    }
  }

  version_ = send("ATI", ELM_COMMAND_TIMEOUT_MS);
  version_.trim();
  Serial.printf("[ELM] Adapter: %s\n", version_.c_str());
  return true;
}
