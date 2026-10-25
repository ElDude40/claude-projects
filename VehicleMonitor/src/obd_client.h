// High-level OBD-II client: connection management, live data polling and
// diagnostic trouble code handling. Runs in its own FreeRTOS task so a slow
// adapter never freezes the display or buttons.
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "elm327.h"
#include "vehicle_data.h"

class ObdClient {
 public:
  // Starts the background task on the given core.
  void start(int core);

  // Thread-safe copy of the latest data for the UI.
  VehicleData snapshot();

  // Requests from the UI, executed by the OBD task.
  void requestDtcRead() { dtcReadRequested_ = true; }
  void requestDtcClear() { dtcClearRequested_ = true; }

 private:
  static void taskEntry(void* self);
  void run();

  bool connectAndInit();
  bool detectEcu();
  void readSupportedPids();
  void readVehicleInfo();
  void pollLiveData();
  bool pollPid(uint8_t pid);
  void readBatteryVoltage();
  void readDtcs();
  void clearDtcs();

  void setLink(LinkState s);
  String query(const char* cmd, uint32_t timeoutMs = 0);

  Elm327 elm_;
  SemaphoreHandle_t mutex_ = nullptr;
  VehicleData data_;  // guarded by mutex_

  bool supported_[256] = {false};
  bool isCan_ = true;
  uint8_t slowIndex_ = 0;
  uint8_t consecutiveFailures_ = 0;
  uint32_t lastDtcReadMs_ = 0;

  volatile bool dtcReadRequested_ = false;
  volatile bool dtcClearRequested_ = false;
};
