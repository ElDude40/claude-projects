// User configuration for the ESP32 Vehicle Information Monitor.
// Edit the values in this file to match your adapter and wiring.
#pragma once

// ---------------------------------------------------------------------------
// Bluetooth OBD-II adapter
// ---------------------------------------------------------------------------
// Most ELM327-compatible Bluetooth Classic adapters advertise as "OBDII",
// "OBD2", "V-LINK" or "OBDLink LX". Check your phone's Bluetooth list with the
// adapter plugged into the car to find the exact name.
#define OBD_ADAPTER_NAME "OBDII"

// Optional: connect by MAC address instead of name (faster, more reliable).
// Leave empty ("") to connect by name. Format: "AA:BB:CC:DD:EE:FF"
#define OBD_ADAPTER_MAC ""

// Pairing PIN used by the adapter. Almost always "1234" or "0000".
#define OBD_ADAPTER_PIN "1234"

// Name this ESP32 shows while acting as the Bluetooth master.
#define DEVICE_BT_NAME "ESP32-VehicleMonitor"

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
#define ELM_COMMAND_TIMEOUT_MS   2000   // normal command timeout
#define ELM_SEARCH_TIMEOUT_MS    12000  // first query while protocol auto-detects
#define LIVE_DATA_INTERVAL_MS    250    // delay between live-data polling rounds
#define DTC_REFRESH_INTERVAL_MS  30000  // automatic trouble-code re-read
#define RECONNECT_DELAY_MS       5000   // wait before retrying a lost connection
#define DISPLAY_REFRESH_MS       200    // screen redraw period

// ---------------------------------------------------------------------------
// Buttons (wired between the GPIO and GND, internal pull-ups are used)
// ---------------------------------------------------------------------------
#define BUTTON_NEXT_PIN    32   // short press: next page
#define BUTTON_SELECT_PIN  33   // short press: page action / long press: clear codes
#define BUTTON_LONG_PRESS_MS 3000

// ---------------------------------------------------------------------------
// Units
// ---------------------------------------------------------------------------
// 1 = mph / °F, 0 = km/h / °C
#define USE_IMPERIAL_UNITS 1
