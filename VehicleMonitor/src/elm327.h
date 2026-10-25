// Minimal ELM327 client over Bluetooth Classic (SPP).
#pragma once

#include <Arduino.h>
#include <BluetoothSerial.h>

class Elm327 {
 public:
  // Starts the ESP32 Bluetooth stack in master mode.
  bool begin(const char* deviceName, const char* pin);

  // Connects to the adapter by MAC (if non-empty) or by advertised name.
  bool connect(const char* adapterName, const char* adapterMac);
  void disconnect();
  bool isConnected();

  // Resets and configures the adapter (echo off, spaces off, auto protocol).
  bool initialize();

  // Sends a command and returns everything up to the '>' prompt.
  // Returns an empty string on timeout.
  String send(const char* command, uint32_t timeoutMs);

  const String& version() const { return version_; }

 private:
  void flushInput();

  BluetoothSerial bt_;
  String version_;
};
