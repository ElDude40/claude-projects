# ESP32 Vehicle Information Monitor

A small dashboard display for your car, built on an ESP32. It connects over
Bluetooth to a standard ELM327-compatible OBD-II scanner plugged into the
vehicle's diagnostic port. It shows live engine data, reads stored and pending
trouble codes (with descriptions), and can clear them.

```
┌──────────────────────────────────────────┐
│ Dashboard                 ● Online   1/4 │
│      RPM          │         MPH          │
│      2150         │          45          │
│        CHECK ENGINE - 1 code(s)          │
│ ┌Coolant┐┌Throttle┐┌ Load ┐┌Battery┐     │
│ │ 195F  ││  18%   ││ 32%  ││ 14.1V │     │
│ └───────┘└────────┘└──────┘└───────┘     │
│   NEXT: page    SELECT: re-read codes    │
└──────────────────────────────────────────┘
```

## Features

| Page | Shows |
|------|-------|
| **1. Dashboard** | RPM, speed, coolant temp, throttle, engine load, battery voltage, check-engine warning |
| **2. Engine Data** | Intake/oil/ambient temp, MAF, MAP, ignition timing, fuel level, short/long fuel trims, ECU voltage, run time |
| **3. Trouble Codes** | Check-engine light (MIL) status, **stored** and **pending** DTCs with plain-English descriptions, clear codes |
| **4. Vehicle Info** | VIN, OBD protocol, adapter version, number of supported PIDs, uptime |

- Automatic protocol detection (CAN, ISO 9141, KWP2000, J1850 PWM/VPW).
- Only polls the PIDs your car reports as supported.
- Reconnects automatically if the adapter drops out or the ignition is turned off.
- Runs the Bluetooth/OBD work in a separate FreeRTOS task, so the screen and buttons stay responsive.
- Trouble codes are re-read every 30 seconds and on demand. They are also printed to the serial monitor.
- Imperial or metric units (`USE_IMPERIAL_UNITS` in `src/config.h`).

---

## Needed hardware

| # | Item | Notes | Approx. cost |
|---|------|-------|--------------|
| 1 | **ESP32 development board**: ESP32-WROOM-32 / ESP32-DevKitC (38-pin or 30-pin) | **Must be an original ESP32.** The ESP32-S2, S3, C3 and C6 have **no Bluetooth Classic** and cannot talk to most OBD-II scanners. | $6–12 |
| 2 | **Bluetooth OBD-II scanner** (ELM327 compatible, **Bluetooth Classic / SPP**) | See the recommended scanners below. **BLE-only / "iPhone" versions will NOT work.** | $15–60 |
| 3 | **2.8" ILI9341 SPI TFT display** (320×240) | The common red-PCB module with SPI pins (touch version is fine; touch is not used). A 2.4" or 3.2" ILI9341 also works. | $8–15 |
| 4 | **2× momentary push buttons** (6 mm or 12 mm tactile, or panel-mount) | NEXT and SELECT | $1–3 |
| 5 | **12 V → 5 V USB car adapter + USB cable** (micro-USB or USB-C to match your ESP32 board) | Any 1 A+ cigarette-lighter USB charger. Or use a 12 V→5 V buck converter (e.g. MP1584 / LM2596) hard-wired to an ignition-switched fuse. | $5–10 |
| 6 | **Jumper wires and a breadboard or perfboard** | Female-female Dupont wires make the display hookup easy | $5 |
| 7 | **Enclosure / mount** (optional) | 3D-printed case, project box, or a dash phone mount | — |

### Recommended Bluetooth OBD-II scanners

These use Bluetooth Classic (SPP), which this firmware requires:

| Scanner | Comments |
|---------|----------|
| **OBDLink LX Bluetooth** | Best choice. Fast, reliable, genuine chip, sleeps when the car is off. |
| **OBDLink MX+** | Also works (supports Classic Bluetooth). It costs more and has features this project doesn't use. |
| **Vgate iCar Pro Bluetooth 3.0** (the *non*-BLE version) | Good, inexpensive, has an auto-sleep mode. |
| **Veepeak Mini Bluetooth** (Android version) | Budget option, works well. |
| Generic "ELM327 v1.5 / v2.1 Bluetooth" | The cheapest choice. Many work, but quality varies a lot. Pick one that pairs with an Android phone using PIN `1234`. |

> ⚠️ **Do not buy:** adapters sold as "BLE", "Bluetooth 4.0", "for iPhone/iOS"
> (e.g. Veepeak BLE+, Vgate iCar Pro BLE 4.0, OBDLink CX), or Wi-Fi adapters.
> They use a different radio protocol and will not connect.

Your vehicle must support OBD-II: every car and light truck sold in the US
since 1996, petrol cars in the EU since 2001, and diesel cars in the EU since 2004.

---

## Wiring

| ILI9341 display pin | ESP32 GPIO |
|---------------------|------------|
| VCC                 | 3V3 |
| GND                 | GND |
| CS                  | GPIO 15 |
| RESET               | GPIO 4 |
| DC / RS             | GPIO 2 |
| SDI (MOSI)          | GPIO 23 |
| SCK                 | GPIO 18 |
| LED (backlight)     | GPIO 21 (or straight to 3V3 for always-on) |
| SDO (MISO)          | GPIO 19 |

| Button | ESP32 GPIO | Other leg |
|--------|------------|-----------|
| NEXT   | GPIO 32    | GND |
| SELECT | GPIO 33    | GND |

The buttons use the ESP32's internal pull-up resistors, so no external
resistors are needed. Power the ESP32 from the USB car adapter.

To use different pins, edit the `-DTFT_*` flags in `platformio.ini` and the
`BUTTON_*_PIN` values in `src/config.h`.

---

## Software setup

1. Install [VS Code](https://code.visualstudio.com/) with the
   [PlatformIO extension](https://platformio.org/install/ide?install=vscode),
   or the PlatformIO CLI (`pip install platformio`).
2. Find your scanner's Bluetooth name. Plug it into the car, turn the ignition
   on, and look at the Bluetooth device list on a phone. Common names are
   `OBDII`, `OBD2`, `V-LINK`, `OBDLink LX` and `Android-Vlink`.
   **Do not pair it to the phone.** Many adapters accept only one connection at a time.
3. Edit `src/config.h`:
   ```cpp
   #define OBD_ADAPTER_NAME "OBDII"   // your adapter's name
   #define OBD_ADAPTER_MAC  ""        // optional, e.g. "00:1D:A5:68:98:8B" (faster)
   #define OBD_ADAPTER_PIN  "1234"    // or "0000"
   #define USE_IMPERIAL_UNITS 1       // 0 for km/h and °C
   ```
4. Connect the ESP32 by USB and flash:
   ```bash
   cd VehicleMonitor
   pio run -t upload
   pio device monitor        # optional: view the debug log at 115200 baud
   ```

The TFT_eSPI display library is configured entirely from `platformio.ini`.
You do **not** need to edit its `User_Setup.h`.

### Running the parser tests

The OBD-II response parsing (PIDs, trouble codes, VIN, multi-frame CAN
responses) is unit-tested on your computer with no hardware:

```bash
pio test -e native
```

---

## Using it

1. Plug the OBD-II scanner into the port under the dashboard (usually near the
   steering column).
2. Turn the ignition **on** (engine running or not) and power the ESP32.
3. The header shows the connection progress: *Finding adapter → Setting up
   adapter → Contacting ECU → Online*. The first connection can take 10–15
   seconds while the adapter detects the protocol.

| Button | Action |
|--------|--------|
| **NEXT** (short press) | Go to the next page |
| **SELECT** (short press) | Re-read trouble codes now |
| **SELECT** (hold 3 s on the *Trouble Codes* page) | Clear trouble codes. A confirmation screen appears: press SELECT to confirm or NEXT to cancel. It cancels itself after 10 s. |

### About clearing codes

Clearing codes (OBD Mode 04) turns off the check-engine light. It also erases
freeze-frame data and **resets the emissions readiness monitors**, so the car
will fail an emissions or smog inspection until it has been driven enough to
re-run them. If the fault is still present, the code will come back. Clear
codes with the ignition **on** and the engine **off**.

Codes starting with `P1`, `P3`, `B1`, `C1` or `U1` are manufacturer-specific.
The display shows them as such; look them up in a service manual for your
vehicle.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Stuck on **Finding adapter** | Check `OBD_ADAPTER_NAME` (it is case-sensitive). Make sure no phone is connected to the adapter. Try setting `OBD_ADAPTER_MAC` instead. Confirm the adapter is Bluetooth Classic and not BLE. |
| Stuck on **Setting up adapter** | Try PIN `0000`. Some cheap clones need a power cycle: unplug the adapter for 10 s. |
| **No ECU (ign. off?)** | Turn the ignition on. Some cars only answer with the engine running. |
| Some values show `--` | Your car doesn't support that PID. This is normal. Many cars don't report oil temperature or ambient temperature. |
| VIN shows "Not reported" | Many pre-2005 vehicles don't support Mode 09 VIN requests. |
| Display blank or white | Check the wiring. Make sure the backlight (LED) pin is connected. Confirm the module uses an ILI9341 driver. |
| Build error about Bluetooth | You are building for an ESP32-S2/S3/C3/C6. Use an original ESP32 board. |

## Safety notes

- Mount the display where it does not block your view, and don't operate the
  buttons while driving.
- Most OBD-II scanners draw a small current even with the car off. Unplug
  yours (or use one with auto-sleep, such as the OBDLink LX or Vgate iCar Pro)
  if the car will sit for days.
- This project only *reads* data and sends the standard clear-codes command. It
  does not modify any vehicle settings.

## Project layout

```
VehicleMonitor/
├── platformio.ini          build config (board, libraries, display pins)
├── src/
│   ├── config.h            adapter name/PIN, buttons, timing, units
│   ├── main.cpp            setup/loop, buttons, page navigation
│   ├── elm327.*            Bluetooth SPP link + ELM327 AT commands
│   ├── obd_client.*        connection state machine, PID polling, DTC read/clear (FreeRTOS task)
│   ├── obd_parser.*        hardware-independent response parsing (unit-tested)
│   ├── dtc_descriptions.*  descriptions for common generic trouble codes
│   ├── vehicle_data.h      shared data model
│   └── display.*           TFT user interface (TFT_eSPI)
└── test/test_obd_parser/   host unit tests
```
