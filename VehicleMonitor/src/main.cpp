// ESP32 Vehicle Information Monitor
//
// Connects to an ELM327-compatible Bluetooth OBD-II adapter, shows live engine
// data on a TFT display and reads / clears diagnostic trouble codes.
//
// Controls:
//   NEXT   short press  -> next page (cancels the clear-codes prompt)
//   SELECT short press  -> re-read trouble codes (confirms the clear prompt)
//   SELECT long press   -> on the Trouble Codes page: clear codes (asks first)
#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "obd_client.h"

namespace {

class Button {
 public:
  enum Event { None, Short, Long };

  explicit Button(uint8_t pin) : pin_(pin) {}
  void begin() { pinMode(pin_, INPUT_PULLUP); }

  Event poll() {
    const bool down = digitalRead(pin_) == LOW;
    const uint32_t now = millis();
    if (down != lastRaw_) {
      lastRaw_ = down;
      changedAt_ = now;
    }
    if (now - changedAt_ < 30) return None;  // debounce

    Event ev = None;
    if (down && !pressed_) {
      pressed_ = true;
      longFired_ = false;
      pressedAt_ = now;
    } else if (down && pressed_ && !longFired_ && now - pressedAt_ >= BUTTON_LONG_PRESS_MS) {
      longFired_ = true;
      ev = Long;
    } else if (!down && pressed_) {
      pressed_ = false;
      if (!longFired_) ev = Short;
    }
    return ev;
  }

 private:
  uint8_t pin_;
  bool lastRaw_ = false;
  bool pressed_ = false;
  bool longFired_ = false;
  uint32_t changedAt_ = 0;
  uint32_t pressedAt_ = 0;
};

constexpr uint32_t kConfirmTimeoutMs = 10000;

ObdClient obdClient;
Display display;
Button nextButton(BUTTON_NEXT_PIN);
Button selectButton(BUTTON_SELECT_PIN);

Page page = Page::Dashboard;
bool confirmingClear = false;
uint32_t confirmStartedMs = 0;
uint32_t lastConfirmSecond = 0;
uint32_t lastRenderMs = 0;

void nextPage() {
  page = static_cast<Page>((static_cast<int>(page) + 1) % static_cast<int>(Page::Count));
}

void startClearConfirm() {
  confirmingClear = true;
  confirmStartedMs = millis();
  lastConfirmSecond = 0;
}

void endClearConfirm() {
  confirmingClear = false;
  display.invalidate();
}

void handleButtons() {
  const Button::Event next = nextButton.poll();
  const Button::Event select = selectButton.poll();

  if (confirmingClear) {
    if (select == Button::Short) {
      Serial.println("[UI] Clearing trouble codes");
      obdClient.requestDtcClear();
      page = Page::Codes;
      endClearConfirm();
    } else if (next != Button::None) {
      endClearConfirm();
    }
    return;
  }

  if (next == Button::Short) nextPage();
  if (select == Button::Short) obdClient.requestDtcRead();
  if (select == Button::Long && page == Page::Codes) startClearConfirm();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Vehicle Information Monitor");

  nextButton.begin();
  selectButton.begin();
  display.begin();

  // Bluetooth runs on core 0; keep the OBD task there and the UI on core 1.
  obdClient.start(0);
}

void loop() {
  handleButtons();

  const uint32_t now = millis();
  if (confirmingClear) {
    const uint32_t elapsed = now - confirmStartedMs;
    if (elapsed >= kConfirmTimeoutMs) {
      endClearConfirm();
    } else {
      const uint32_t secondsLeft = (kConfirmTimeoutMs - elapsed + 999) / 1000;
      if (secondsLeft != lastConfirmSecond) {
        lastConfirmSecond = secondsLeft;
        display.showClearConfirm(secondsLeft);
      }
    }
  } else if (now - lastRenderMs >= DISPLAY_REFRESH_MS) {
    lastRenderMs = now;
    display.render(page, obdClient.snapshot());
  }

  delay(10);
}
