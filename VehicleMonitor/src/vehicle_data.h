// Snapshot of everything the monitor knows about the vehicle.
// All values are stored in metric units; conversion happens on display.
#pragma once

#include <Arduino.h>

#include <string>
#include <vector>

enum class LinkState : uint8_t {
  Starting,      // Bluetooth stack starting
  Connecting,    // looking for / pairing with the adapter
  Initializing,  // adapter found, sending AT setup commands
  SearchingEcu,  // adapter ready, waiting for the car to answer
  Online,        // receiving data from the ECU
  NoEcu,         // adapter connected but the car is not answering (ignition off?)
};

// A numeric reading plus whether the vehicle supplied it.
struct Reading {
  float value = 0;
  bool valid = false;
  void set(float v) { value = v; valid = true; }
};

struct VehicleData {
  LinkState link = LinkState::Starting;

  // Live data (Mode 01)
  Reading rpm;             // rev/min
  Reading speedKmh;        // km/h
  Reading coolantC;        // °C
  Reading intakeTempC;     // °C
  Reading oilTempC;        // °C
  Reading ambientTempC;    // °C
  Reading engineLoadPct;   // %
  Reading throttlePct;     // %
  Reading fuelLevelPct;    // %
  Reading mafGs;           // g/s
  Reading mapKpa;          // kPa
  Reading timingDeg;       // ° before TDC
  Reading shortTrimPct;    // % (bank 1)
  Reading longTrimPct;     // % (bank 1)
  Reading runTimeSec;      // s since engine start
  Reading moduleVoltage;   // V reported by the ECU (PID 42)
  Reading batteryVoltage;  // V measured by the adapter (ATRV)

  // Diagnostics
  bool milOn = false;            // check-engine light
  int  reportedDtcCount = -1;    // from PID 01, -1 = unknown
  std::vector<std::string> storedDtcs;
  std::vector<std::string> pendingDtcs;
  bool dtcsRead = false;
  uint32_t dtcReadAtMs = 0;
  String lastActionMsg;          // e.g. "Codes cleared"

  // Vehicle / adapter info
  String vin;
  String protocol;
  String adapterVersion;
  int supportedPidCount = 0;
};
