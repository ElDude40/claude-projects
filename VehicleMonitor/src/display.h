// TFT user interface (ILI9341 320x240, landscape).
#pragma once

#include <TFT_eSPI.h>

#include "vehicle_data.h"

enum class Page : uint8_t { Dashboard, Engine, Codes, Info, Count };

class Display {
 public:
  void begin();

  // Draws the given page. Static parts are only redrawn when the page changes.
  void render(Page page, const VehicleData& d);

  // Full-screen confirmation prompt shown before clearing trouble codes.
  void showClearConfirm(uint32_t secondsLeft);

  // Forces a full redraw on the next render() call.
  void invalidate() { lastPage_ = Page::Count; }

 private:
  void drawHeader(Page page, const VehicleData& d);
  void drawFooter(const char* hint);
  void drawDashboard(const VehicleData& d, bool full);
  void drawEngine(const VehicleData& d, bool full);
  void drawCodes(const VehicleData& d, bool full);
  void drawInfo(const VehicleData& d, bool full);

  void drawTile(int x, int y, int w, const char* label, const String& value, uint16_t color, bool full);
  void drawRow(int x, int y, int w, const char* label, const String& value, bool full);
  String fit(const String& text, int font, int maxWidth);

  TFT_eSPI tft_;
  Page lastPage_ = Page::Count;
  String lastCodesKey_;
  String lastHeaderKey_;
};
