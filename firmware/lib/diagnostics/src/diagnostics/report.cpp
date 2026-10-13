#include "report.h"

#include <cstdarg>
#include <cstdio>

#include "redactor.h"

namespace diagnostics {

namespace {

constexpr const char* kDefaultName = "CatWandConnect";
constexpr const char* kDefaultHostname = "catwandconnect";

std::string format(const char* pattern, ...) __attribute__((format(printf, 1, 2)));

std::string format(const char* pattern, ...) {
  char buffer[160];
  va_list args;
  va_start(args, pattern);
  std::vsnprintf(buffer, sizeof buffer, pattern, args);
  va_end(args);
  return buffer;
}

void line(std::string& out, const char* label, const std::string& value) {
  out += label;
  out += ": ";
  out += value;
  out += '\n';
}

Redactor redactorFor(const ReportData& d) {
  Redactor r;
  // The default name says nothing about its owner and helps reading logs.
  if (d.name != kDefaultName) r.add(d.name, "name");
  if (d.hostname != kDefaultHostname) r.add(d.hostname, "hostname");
  r.add(d.wifiNetwork, "wifi-network");
  r.add(d.mqttHost, "broker");
  r.add(d.mqttUser, "mqtt-user");
  r.add(d.ip, "ip");
  r.add(d.mac, "mac");
  for (const auto& [value, label] : d.notedPersonal) r.add(value, label);
  return r;
}

}  // namespace

std::string reportText(const ReportData& d, bool redacted) {
  std::string out;
  out += "CatWandConnect diagnostic report\n";
  line(out, "Redacted", redacted ? "yes" : "no");
  out += '\n';

  line(out, "Name", d.name);
  line(out, "Address", "http://" + d.hostname + ".local/");
  line(out, "Firmware", d.version);
  line(out, "Chip", d.chip);
  if (!d.deviceId.empty()) line(out, "Device ID", d.deviceId);
  line(out, "Last reset", d.resetReason);
  line(out, "Running for", format("%u d %u h %u min (%u s)", unsigned(d.uptimeS / 86400), unsigned(d.uptimeS % 86400 / 3600),
                                  unsigned(d.uptimeS % 3600 / 60), unsigned(d.uptimeS)));
  line(out, "Free memory", format("%u KB (lowest %u KB)", unsigned(d.freeHeap / 1024), unsigned(d.minFreeHeap / 1024)));
  line(out, "Chip temperature", format("%.1f C", double(d.temperature)));
  line(out, "Settings PIN", d.pinEnabled ? "on" : "off");
  out += '\n';

  line(out, "Wi-Fi", d.wifiConnected ? format("connected, signal %d dBm", d.rssi)
                                     : std::string("not connected (setup hotspot or connecting)"));
  line(out, "Wi-Fi network", d.wifiNetwork.empty() ? std::string("none saved") : d.wifiNetwork);
  if (!d.ip.empty()) line(out, "IP address", d.ip);
  line(out, "MAC address", d.mac);
  if (!d.mqttEnabled) {
    line(out, "Home Assistant (MQTT)", "disabled");
  } else {
    std::string state = d.mqttConnected ? "enabled, connected" : "enabled, not connected";
    if (!d.mqttConnected && !d.mqttError.empty()) state += ", last error: " + d.mqttError;
    line(out, "Home Assistant (MQTT)", state);
    std::string broker = d.mqttHost + format(":%u", unsigned(d.mqttPort));
    if (!d.mqttUser.empty()) broker += ", user " + d.mqttUser;
    line(out, "MQTT broker", broker);
  }
  out += '\n';

  line(out, "Settings", d.settings);
  line(out, "Hardware", d.hardware);
  out += "\nRecent log:\n";
  out += d.log.empty() ? std::string("(empty)\n") : d.log;

  return redacted ? redactorFor(d).apply(out) : out;
}

std::string deviceIdFrom(uint32_t random) { return format("%08x", unsigned(random)); }

}  // namespace diagnostics
