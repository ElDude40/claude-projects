#include "display.h"

#include "config.h"
#include "dtc_descriptions.h"

namespace {

constexpr int kWidth = 320;
constexpr int kHeaderH = 26;
constexpr int kFooterY = 222;
constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kLabel = TFT_LIGHTGREY;
constexpr uint16_t kAccent = TFT_CYAN;

const char* const kPageTitles[] = {"Dashboard", "Engine Data", "Trouble Codes", "Vehicle Info"};

String fmt(const Reading& r, int decimals, const char* suffix = "") {
  if (!r.valid) return "--";
  return String(r.value, decimals) + suffix;
}

String fmtTemp(const Reading& r) {
  if (!r.valid) return "--";
#if USE_IMPERIAL_UNITS
  return String(r.value * 9.0f / 5.0f + 32.0f, 0) + "F";
#else
  return String(r.value, 0) + "C";
#endif
}

String fmtSpeed(const Reading& r) {
  if (!r.valid) return "--";
#if USE_IMPERIAL_UNITS
  return String(r.value * 0.621371f, 0);
#else
  return String(r.value, 0);
#endif
}

String fmtDuration(uint32_t seconds) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%u:%02u:%02u", (unsigned)(seconds / 3600), (unsigned)(seconds / 60 % 60),
           (unsigned)(seconds % 60));
  return buf;
}

const char* linkText(LinkState s) {
  switch (s) {
    case LinkState::Starting:     return "Starting";
    case LinkState::Connecting:   return "Finding adapter";
    case LinkState::Initializing: return "Setting up adapter";
    case LinkState::SearchingEcu: return "Contacting ECU";
    case LinkState::Online:       return "Online";
    case LinkState::NoEcu:        return "No ECU (ign. off?)";
  }
  return "";
}

uint16_t linkColor(LinkState s) {
  switch (s) {
    case LinkState::Online: return TFT_GREEN;
    case LinkState::NoEcu:  return TFT_RED;
    default:                return TFT_YELLOW;
  }
}

}  // namespace

void Display::begin() {
  tft_.init();
  tft_.setRotation(1);  // landscape, connector on the left
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif
  tft_.fillScreen(kBg);
  tft_.setTextColor(kAccent, kBg);
  tft_.setTextDatum(MC_DATUM);
  tft_.drawString("Vehicle Monitor", kWidth / 2, 100, 4);
  tft_.setTextColor(kLabel, kBg);
  tft_.drawString("Connecting to OBD-II adapter...", kWidth / 2, 140, 2);
}

String Display::fit(const String& text, int font, int maxWidth) {
  if (tft_.textWidth(text, font) <= maxWidth) return text;
  String s = text;
  while (s.length() > 1 && tft_.textWidth(s + "..", font) > maxWidth) s.remove(s.length() - 1);
  return s + "..";
}

void Display::render(Page page, const VehicleData& d) {
  const bool full = page != lastPage_;
  if (full) {
    tft_.fillScreen(kBg);
    lastHeaderKey_ = "";
    lastCodesKey_ = "";
    lastPage_ = page;
  }
  drawHeader(page, d);
  switch (page) {
    case Page::Dashboard: drawDashboard(d, full); break;
    case Page::Engine:    drawEngine(d, full); break;
    case Page::Codes:     drawCodes(d, full); break;
    case Page::Info:      drawInfo(d, full); break;
    default: break;
  }
}

void Display::drawHeader(Page page, const VehicleData& d) {
  String key = String(static_cast<int>(page)) + linkText(d.link);
  if (key == lastHeaderKey_) return;
  lastHeaderKey_ = key;

  tft_.fillRect(0, 0, kWidth, kHeaderH, TFT_NAVY);
  tft_.setTextColor(TFT_WHITE, TFT_NAVY);
  tft_.setTextPadding(0);
  tft_.setTextDatum(ML_DATUM);
  tft_.drawString(kPageTitles[static_cast<int>(page)], 6, kHeaderH / 2, 2);

  const uint16_t color = linkColor(d.link);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextColor(color, TFT_NAVY);
  const char* status = linkText(d.link);
  tft_.drawString(status, kWidth - 48, kHeaderH / 2, 2);
  tft_.fillCircle(kWidth - 48 - tft_.textWidth(status, 2) - 10, kHeaderH / 2, 4, color);

  tft_.setTextColor(kLabel, TFT_NAVY);
  tft_.drawString(String(static_cast<int>(page) + 1) + "/" + String(static_cast<int>(Page::Count)), kWidth - 6,
                  kHeaderH / 2, 2);
}

void Display::drawFooter(const char* hint) {
  tft_.fillRect(0, kFooterY, kWidth, 240 - kFooterY, kBg);
  tft_.drawFastHLine(0, kFooterY, kWidth, TFT_DARKGREY);
  tft_.setTextColor(TFT_DARKGREY, kBg);
  tft_.setTextPadding(0);
  tft_.setTextDatum(MC_DATUM);
  tft_.drawString(hint, kWidth / 2, kFooterY + 10, 2);
}

void Display::drawTile(int x, int y, int w, const char* label, const String& value, uint16_t color, bool full) {
  if (full) {
    tft_.drawRoundRect(x + 2, y, w - 4, 56, 6, TFT_DARKGREY);
    tft_.setTextColor(kLabel, kBg);
    tft_.setTextPadding(0);
    tft_.setTextDatum(TC_DATUM);
    tft_.drawString(label, x + w / 2, y + 5, 2);
  }
  tft_.setTextColor(color, kBg);
  tft_.setTextDatum(TC_DATUM);
  tft_.setTextPadding(w - 10);
  tft_.drawString(value, x + w / 2, y + 24, 4);
}

void Display::drawRow(int x, int y, int w, const char* label, const String& value, bool full) {
  if (full) {
    tft_.setTextColor(kLabel, kBg);
    tft_.setTextPadding(0);
    tft_.setTextDatum(ML_DATUM);
    tft_.drawString(label, x, y, 2);
  }
  tft_.setTextColor(TFT_WHITE, kBg);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextPadding(w / 2);
  tft_.drawString(value, x + w - 4, y, 4);
}

void Display::drawDashboard(const VehicleData& d, bool full) {
  if (full) {
    tft_.setTextColor(kLabel, kBg);
    tft_.setTextPadding(0);
    tft_.setTextDatum(TC_DATUM);
    tft_.drawString("RPM", 80, 34, 2);
#if USE_IMPERIAL_UNITS
    tft_.drawString("MPH", 240, 34, 2);
#else
    tft_.drawString("KM/H", 240, 34, 2);
#endif
    tft_.drawFastVLine(160, 34, 70, TFT_DARKGREY);
    drawFooter("NEXT: page    SELECT: re-read codes");
  }

  // Big seven-segment numbers (font 7 supports digits, '.', ':' and '-').
  tft_.setTextDatum(TC_DATUM);
  const uint16_t rpmColor = d.rpm.valid && d.rpm.value > 5500 ? TFT_RED : TFT_GREEN;
  tft_.setTextColor(rpmColor, kBg);
  tft_.setTextPadding(150);
  tft_.drawString(d.rpm.valid ? String(d.rpm.value, 0) : String("--"), 80, 54, 7);
  tft_.setTextColor(TFT_GREEN, kBg);
  tft_.drawString(fmtSpeed(d.speedKmh), 240, 54, 7);

  // Check-engine banner.
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextPadding(kWidth);
  const size_t codes = d.storedDtcs.size();
  if (d.milOn || codes > 0) {
    tft_.setTextColor(TFT_ORANGE, kBg);
    tft_.drawString("CHECK ENGINE - " + String(codes) + " code(s)", kWidth / 2, 124, 2);
  } else if (d.dtcsRead) {
    tft_.setTextColor(TFT_DARKGREY, kBg);
    tft_.drawString("No trouble codes", kWidth / 2, 124, 2);
  } else {
    tft_.drawString(" ", kWidth / 2, 124, 2);
  }

  const uint16_t coolantColor = d.coolantC.valid && d.coolantC.value > 110 ? TFT_RED : TFT_WHITE;
  const Reading& volts = d.batteryVoltage.valid ? d.batteryVoltage : d.moduleVoltage;
  const uint16_t voltColor = volts.valid && (volts.value < 11.8f || volts.value > 15.0f) ? TFT_ORANGE : TFT_WHITE;
  drawTile(0, 150, 80, "Coolant", fmtTemp(d.coolantC), coolantColor, full);
  drawTile(80, 150, 80, "Throttle", fmt(d.throttlePct, 0, "%"), TFT_WHITE, full);
  drawTile(160, 150, 80, "Load", fmt(d.engineLoadPct, 0, "%"), TFT_WHITE, full);
  drawTile(240, 150, 80, "Battery", fmt(volts, 1, "V"), voltColor, full);
}

void Display::drawEngine(const VehicleData& d, bool full) {
  if (full) {
    tft_.drawFastVLine(160, kHeaderH + 6, 186, TFT_DARKGREY);
    drawFooter("NEXT: page");
  }
  const int y0 = kHeaderH + 20;
  const int dy = 31;
  drawRow(6, y0 + 0 * dy, 150, "Intake air", fmtTemp(d.intakeTempC), full);
  drawRow(6, y0 + 1 * dy, 150, "Oil temp", fmtTemp(d.oilTempC), full);
  drawRow(6, y0 + 2 * dy, 150, "Ambient", fmtTemp(d.ambientTempC), full);
  drawRow(6, y0 + 3 * dy, 150, "MAF g/s", fmt(d.mafGs, 1), full);
  drawRow(6, y0 + 4 * dy, 150, "MAP kPa", fmt(d.mapKpa, 0), full);
  drawRow(6, y0 + 5 * dy, 150, "Timing", fmt(d.timingDeg, 1), full);

  drawRow(166, y0 + 0 * dy, 150, "Fuel level", fmt(d.fuelLevelPct, 0, "%"), full);
  drawRow(166, y0 + 1 * dy, 150, "STFT B1", fmt(d.shortTrimPct, 1, "%"), full);
  drawRow(166, y0 + 2 * dy, 150, "LTFT B1", fmt(d.longTrimPct, 1, "%"), full);
  drawRow(166, y0 + 3 * dy, 150, "ECU volts", fmt(d.moduleVoltage, 1, "V"), full);
  drawRow(166, y0 + 4 * dy, 150, "Battery", fmt(d.batteryVoltage, 1, "V"), full);
  drawRow(166, y0 + 5 * dy, 150, "Run time", d.runTimeSec.valid ? fmtDuration(static_cast<uint32_t>(d.runTimeSec.value)) : String("--"),
          full);
}

void Display::drawCodes(const VehicleData& d, bool full) {
  if (full) drawFooter("NEXT: page  SEL: re-read  HOLD SEL: clear");

  // Only redraw the list when something in it changed.
  String key = String(d.dtcsRead) + d.milOn + d.reportedDtcCount + d.lastActionMsg + static_cast<int>(d.link);
  for (const auto& c : d.storedDtcs) key += c.c_str();
  key += "|";
  for (const auto& c : d.pendingDtcs) key += c.c_str();
  if (key == lastCodesKey_) return;
  lastCodesKey_ = key;

  tft_.fillRect(0, kHeaderH, kWidth, kFooterY - kHeaderH, kBg);
  tft_.setTextPadding(0);
  tft_.setTextDatum(TL_DATUM);

  int y = kHeaderH + 6;
  if (!d.dtcsRead) {
    tft_.setTextColor(kLabel, kBg);
    tft_.drawString(d.link == LinkState::Online ? "Reading trouble codes..." : "Waiting for vehicle connection...",
                    8, y + 60, 4);
    return;
  }

  // MIL status line.
  tft_.setTextColor(d.milOn ? TFT_ORANGE : TFT_GREEN, kBg);
  String mil = d.milOn ? "Check engine light: ON" : "Check engine light: OFF";
  tft_.drawString(mil, 8, y, 2);
  y += 20;
  if (d.lastActionMsg.length()) {
    tft_.setTextColor(kAccent, kBg);
    tft_.drawString(d.lastActionMsg, 8, y, 2);
    y += 20;
  }

  const int rowH = 18;
  const int maxY = kFooterY - rowH;
  auto drawList = [&](const char* title, const std::vector<std::string>& codes, uint16_t codeColor) {
    tft_.setTextColor(kAccent, kBg);
    tft_.drawString(String(title) + " (" + String(codes.size()) + ")", 8, y, 2);
    y += rowH;
    if (codes.empty()) {
      tft_.setTextColor(TFT_DARKGREY, kBg);
      tft_.drawString("None", 20, y, 2);
      y += rowH;
      return;
    }
    for (size_t i = 0; i < codes.size(); ++i) {
      if (y > maxY) return;
      if (y + rowH > maxY && i + 1 < codes.size()) {  // last free row, more to show
        tft_.setTextColor(TFT_DARKGREY, kBg);
        tft_.drawString("+" + String(codes.size() - i) + " more (see serial log)", 20, y, 2);
        y += rowH;
        return;
      }
      tft_.setTextColor(codeColor, kBg);
      tft_.drawString(codes[i].c_str(), 20, y, 2);
      tft_.setTextColor(TFT_WHITE, kBg);
      tft_.drawString(fit(describeDtc(codes[i].c_str()), 2, kWidth - 80), 72, y, 2);
      y += rowH;
    }
  };

  drawList("Stored", d.storedDtcs, TFT_RED);
  y += 4;
  if (y <= maxY) drawList("Pending", d.pendingDtcs, TFT_YELLOW);
}

void Display::drawInfo(const VehicleData& d, bool full) {
  if (full) drawFooter("NEXT: page");
  const int y0 = kHeaderH + 18;
  const int dy = 28;
  auto row = [&](int i, const char* label, const String& value) {
    if (full) {
      tft_.setTextColor(kLabel, kBg);
      tft_.setTextPadding(0);
      tft_.setTextDatum(ML_DATUM);
      tft_.drawString(label, 8, y0 + i * dy, 2);
    }
    tft_.setTextColor(TFT_WHITE, kBg);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextPadding(kWidth - 100);
    tft_.drawString(fit(value.length() ? value : String("--"), 2, kWidth - 104), 96, y0 + i * dy, 2);
  };
  row(0, "VIN", d.vin);
  row(1, "Protocol", d.protocol);
  row(2, "Adapter", d.adapterVersion);
  row(3, "PIDs", d.supportedPidCount ? String(d.supportedPidCount) + " supported" : String());
  row(4, "Link", linkText(d.link));
  row(5, "Battery", fmt(d.batteryVoltage, 1, "V"));
  row(6, "Uptime", fmtDuration(millis() / 1000));
}

void Display::showClearConfirm(uint32_t secondsLeft) {
  tft_.fillRect(10, 40, 300, 170, TFT_MAROON);
  tft_.drawRect(10, 40, 300, 170, TFT_RED);
  tft_.setTextPadding(0);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_WHITE, TFT_MAROON);
  tft_.drawString("Clear trouble codes?", kWidth / 2, 62, 4);
  tft_.drawString("Turns off the check engine light and", kWidth / 2, 96, 2);
  tft_.drawString("resets emissions readiness monitors.", kWidth / 2, 114, 2);
  tft_.drawString("Ignition ON, engine OFF recommended.", kWidth / 2, 132, 2);
  tft_.setTextColor(TFT_YELLOW, TFT_MAROON);
  tft_.drawString("SELECT = clear     NEXT = cancel", kWidth / 2, 164, 2);
  tft_.setTextColor(kLabel, TFT_MAROON);
  tft_.setTextPadding(60);
  tft_.drawString(String(secondsLeft) + "s", kWidth / 2, 190, 2);
  lastPage_ = Page::Count;  // the page underneath must be redrawn afterwards
}
