#pragma once

// The Diagnostic report (CONTEXT.md): what the robot knows about itself, as
// plain text a user can read, copy and attach to a bug report.

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace diagnostics {

struct ReportData {
  std::string name;      // the owner's name for the robot
  std::string hostname;  // without ".local"
  std::string version;
  std::string chip;      // e.g. "ESP32 rev 3, 2 x 240 MHz, 4 MB flash"
  std::string resetReason;
  uint32_t uptimeS = 0;
  uint32_t freeHeap = 0;
  uint32_t minFreeHeap = 0;
  float temperature = 0;
  bool pinEnabled = false;

  bool wifiConnected = false;  // false: setup hotspot or connecting
  std::string wifiNetwork;     // saved network, connected or not
  std::string ip;
  std::string mac;
  int rssi = 0;

  bool mqttEnabled = false;
  bool mqttConnected = false;
  std::string mqttHost;
  uint16_t mqttPort = 0;
  std::string mqttUser;
  std::string mqttError;

  std::string settings;  // JSON, as GET /api/settings
  std::string hardware;  // JSON, as GET /api/hardware
  std::string log;       // recent log lines, LogBuffer::text()

  // Other personal values that may appear in the log, with their label, such
  // as a Wi-Fi network typed in the setup page but never saved.
  std::vector<std::pair<std::string, std::string>> notedPersonal;
};

// `redacted`: personal data replaced (see Redactor), for anything sent to
// support. The MQTT password is never part of the report.
std::string reportText(const ReportData& data, bool redacted);

}  // namespace diagnostics
