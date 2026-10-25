#include "dtc_descriptions.h"

#include <cstring>

namespace {

struct DtcEntry {
  const char* code;
  const char* text;
};

// Generic powertrain codes most often seen in the field. Manufacturer-specific
// codes (P1xxx, P3xxx, B/C/U 1xxx...) are not listed: look those up in a
// service manual for your vehicle.
const DtcEntry kDtcTable[] = {
    {"P0010", "Camshaft Position Actuator Circuit (Bank 1)"},
    {"P0011", "Camshaft Timing Over-Advanced (Bank 1)"},
    {"P0012", "Camshaft Timing Over-Retarded (Bank 1)"},
    {"P0016", "Crank/Cam Position Correlation (Bank 1 Sensor A)"},
    {"P0030", "O2 Sensor Heater Control Circuit (B1S1)"},
    {"P0036", "O2 Sensor Heater Control Circuit (B1S2)"},
    {"P0068", "MAP/MAF - Throttle Position Correlation"},
    {"P0087", "Fuel Rail/System Pressure Too Low"},
    {"P0100", "Mass Air Flow Circuit Malfunction"},
    {"P0101", "Mass Air Flow Circuit Range/Performance"},
    {"P0102", "Mass Air Flow Circuit Low Input"},
    {"P0103", "Mass Air Flow Circuit High Input"},
    {"P0106", "MAP/Baro Pressure Circuit Range/Performance"},
    {"P0107", "MAP/Baro Pressure Circuit Low Input"},
    {"P0108", "MAP/Baro Pressure Circuit High Input"},
    {"P0110", "Intake Air Temperature Circuit Malfunction"},
    {"P0112", "Intake Air Temperature Circuit Low Input"},
    {"P0113", "Intake Air Temperature Circuit High Input"},
    {"P0115", "Engine Coolant Temperature Circuit Malfunction"},
    {"P0116", "Engine Coolant Temp Circuit Range/Performance"},
    {"P0117", "Engine Coolant Temp Circuit Low Input"},
    {"P0118", "Engine Coolant Temp Circuit High Input"},
    {"P0120", "Throttle Position Sensor A Circuit Malfunction"},
    {"P0121", "Throttle Position Sensor A Range/Performance"},
    {"P0122", "Throttle Position Sensor A Circuit Low Input"},
    {"P0123", "Throttle Position Sensor A Circuit High Input"},
    {"P0125", "Insufficient Coolant Temp for Closed Loop"},
    {"P0128", "Coolant Thermostat Below Regulating Temperature"},
    {"P0130", "O2 Sensor Circuit Malfunction (B1S1)"},
    {"P0131", "O2 Sensor Circuit Low Voltage (B1S1)"},
    {"P0132", "O2 Sensor Circuit High Voltage (B1S1)"},
    {"P0133", "O2 Sensor Circuit Slow Response (B1S1)"},
    {"P0134", "O2 Sensor Circuit No Activity (B1S1)"},
    {"P0135", "O2 Sensor Heater Circuit Malfunction (B1S1)"},
    {"P0136", "O2 Sensor Circuit Malfunction (B1S2)"},
    {"P0137", "O2 Sensor Circuit Low Voltage (B1S2)"},
    {"P0138", "O2 Sensor Circuit High Voltage (B1S2)"},
    {"P0140", "O2 Sensor Circuit No Activity (B1S2)"},
    {"P0141", "O2 Sensor Heater Circuit Malfunction (B1S2)"},
    {"P0151", "O2 Sensor Circuit Low Voltage (B2S1)"},
    {"P0155", "O2 Sensor Heater Circuit Malfunction (B2S1)"},
    {"P0171", "System Too Lean (Bank 1)"},
    {"P0172", "System Too Rich (Bank 1)"},
    {"P0174", "System Too Lean (Bank 2)"},
    {"P0175", "System Too Rich (Bank 2)"},
    {"P0191", "Fuel Rail Pressure Sensor Range/Performance"},
    {"P0200", "Injector Circuit Malfunction"},
    {"P0201", "Injector Circuit Malfunction - Cylinder 1"},
    {"P0202", "Injector Circuit Malfunction - Cylinder 2"},
    {"P0203", "Injector Circuit Malfunction - Cylinder 3"},
    {"P0204", "Injector Circuit Malfunction - Cylinder 4"},
    {"P0217", "Engine Overheat Condition"},
    {"P0219", "Engine Overspeed Condition"},
    {"P0234", "Turbo/Supercharger Overboost Condition"},
    {"P0299", "Turbo/Supercharger Underboost"},
    {"P0300", "Random/Multiple Cylinder Misfire Detected"},
    {"P0301", "Cylinder 1 Misfire Detected"},
    {"P0302", "Cylinder 2 Misfire Detected"},
    {"P0303", "Cylinder 3 Misfire Detected"},
    {"P0304", "Cylinder 4 Misfire Detected"},
    {"P0305", "Cylinder 5 Misfire Detected"},
    {"P0306", "Cylinder 6 Misfire Detected"},
    {"P0307", "Cylinder 7 Misfire Detected"},
    {"P0308", "Cylinder 8 Misfire Detected"},
    {"P0316", "Misfire Detected on Startup (First 1000 Revs)"},
    {"P0325", "Knock Sensor 1 Circuit Malfunction (Bank 1)"},
    {"P0327", "Knock Sensor 1 Circuit Low Input (Bank 1)"},
    {"P0335", "Crankshaft Position Sensor A Circuit Malfunction"},
    {"P0336", "Crankshaft Position Sensor A Range/Performance"},
    {"P0340", "Camshaft Position Sensor Circuit Malfunction"},
    {"P0341", "Camshaft Position Sensor Range/Performance"},
    {"P0351", "Ignition Coil A Primary/Secondary Circuit"},
    {"P0352", "Ignition Coil B Primary/Secondary Circuit"},
    {"P0353", "Ignition Coil C Primary/Secondary Circuit"},
    {"P0354", "Ignition Coil D Primary/Secondary Circuit"},
    {"P0400", "Exhaust Gas Recirculation Flow Malfunction"},
    {"P0401", "Exhaust Gas Recirculation Flow Insufficient"},
    {"P0402", "Exhaust Gas Recirculation Flow Excessive"},
    {"P0403", "Exhaust Gas Recirculation Circuit Malfunction"},
    {"P0411", "Secondary Air Injection Incorrect Flow"},
    {"P0420", "Catalyst Efficiency Below Threshold (Bank 1)"},
    {"P0421", "Warm Up Catalyst Efficiency Below Threshold (Bank 1)"},
    {"P0430", "Catalyst Efficiency Below Threshold (Bank 2)"},
    {"P0440", "Evaporative Emission System Malfunction"},
    {"P0441", "EVAP System Incorrect Purge Flow"},
    {"P0442", "EVAP System Leak Detected (Small Leak)"},
    {"P0443", "EVAP Purge Control Valve Circuit"},
    {"P0446", "EVAP Vent Control Circuit Malfunction"},
    {"P0449", "EVAP Vent Valve/Solenoid Circuit"},
    {"P0451", "EVAP Pressure Sensor Range/Performance"},
    {"P0452", "EVAP Pressure Sensor Low Input"},
    {"P0455", "EVAP System Leak Detected (Large Leak)"},
    {"P0456", "EVAP System Leak Detected (Very Small Leak)"},
    {"P0457", "EVAP Leak Detected (Fuel Cap Loose/Off)"},
    {"P0460", "Fuel Level Sensor Circuit Malfunction"},
    {"P0480", "Cooling Fan 1 Control Circuit"},
    {"P0496", "EVAP High Purge Flow"},
    {"P0500", "Vehicle Speed Sensor Malfunction"},
    {"P0505", "Idle Control System Malfunction"},
    {"P0506", "Idle Control System RPM Lower Than Expected"},
    {"P0507", "Idle Control System RPM Higher Than Expected"},
    {"P0520", "Engine Oil Pressure Sensor/Switch Circuit"},
    {"P0562", "System Voltage Low"},
    {"P0563", "System Voltage High"},
    {"P0571", "Brake Switch A Circuit"},
    {"P0600", "Serial Communication Link Malfunction"},
    {"P0601", "Internal Control Module Memory Checksum Error"},
    {"P0606", "Control Module Processor Fault"},
    {"P0700", "Transmission Control System Malfunction"},
    {"P0705", "Transmission Range Sensor Circuit Malfunction"},
    {"P0715", "Input/Turbine Speed Sensor Circuit"},
    {"P0720", "Output Speed Sensor Circuit Malfunction"},
    {"P0730", "Incorrect Gear Ratio"},
    {"P0740", "Torque Converter Clutch Circuit Malfunction"},
    {"P0741", "Torque Converter Clutch Performance or Stuck Off"},
    {"P0750", "Shift Solenoid A Malfunction"},
    {"P0755", "Shift Solenoid B Malfunction"},
    {"P2096", "Post Catalyst Fuel Trim Too Lean (Bank 1)"},
    {"P2097", "Post Catalyst Fuel Trim Too Rich (Bank 1)"},
    {"P2135", "Throttle Position Sensor A/B Voltage Correlation"},
    {"P2187", "System Too Lean at Idle (Bank 1)"},
    {"P2188", "System Too Rich at Idle (Bank 1)"},
    {"P2195", "O2 Sensor Signal Stuck Lean (B1S1)"},
    {"P2196", "O2 Sensor Signal Stuck Rich (B1S1)"},
    {"P2270", "O2 Sensor Signal Stuck Lean (B1S2)"},
    {"P2271", "O2 Sensor Signal Stuck Rich (B1S2)"},
    {"U0001", "High Speed CAN Communication Bus"},
    {"U0073", "Control Module Communication Bus A Off"},
    {"U0100", "Lost Communication With ECM/PCM"},
    {"U0101", "Lost Communication With TCM"},
    {"U0121", "Lost Communication With ABS Module"},
    {"U0140", "Lost Communication With Body Control Module"},
    {"U0155", "Lost Communication With Instrument Cluster"},
};

}  // namespace

const char* describeDtc(const char* code) {
  if (code == nullptr || std::strlen(code) != 5) return "Invalid code";

  for (const DtcEntry& e : kDtcTable) {
    if (std::strcmp(e.code, code) == 0) return e.text;
  }

  // Second character: 0 = generic (SAE), 1 = manufacturer, 2/3 = mixed.
  const bool manufacturer = code[1] == '1' || (code[0] == 'P' && code[1] == '3');
  if (manufacturer) return "Manufacturer specific code";

  switch (code[0]) {
    case 'P': return "Powertrain fault (see service manual)";
    case 'C': return "Chassis fault (ABS/traction/suspension)";
    case 'B': return "Body fault (airbag/lighting/comfort)";
    case 'U': return "Network / communication fault";
    default:  return "Unknown code";
  }
}
