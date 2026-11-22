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
  // The default name says nothing about its owner and helps reading logs; so
  // does its hostname, plain or with part of the Device ID (#195).
  if (d.name != kDefaultName) r.add(d.name, "name");
  if (d.name != kDefaultName && d.hostname != kDefaultHostname) r.add(d.hostname, "hostname");
  r.add(d.wifiNetwork, "wifi-network");
  r.add(d.mqttHost, "broker");
  r.add(d.mqttUser, "mqtt-user");
  r.add(d.ip, "ip");
  r.add(d.mac, "mac");
  r.add(d.remoteAddress, "tailnet-ip");
  if (d.remoteDeviceName != d.hostname) r.add(d.remoteDeviceName, "tailnet-device");
  for (const auto& [value, label] : d.notedPersonal) r.add(value, label);
  return r;
}

// "2027-01-01" for a Unix time (UTC).
std::string dateText(int64_t unixTime) {
  int64_t z = unixTime / 86400 + 719468;  // days since 0000-03-01
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  int64_t doe = z - era * 146097;
  int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int64_t mp = (5 * doy + 2) / 153;
  int64_t day = doy - (153 * mp + 2) / 5 + 1;
  int64_t month = mp < 10 ? mp + 3 : mp - 9;
  int64_t year = yoe + era * 400 + (month <= 2 ? 1 : 0);
  return format("%04d-%02d-%02d", int(year), int(month), int(day));
}

std::string remoteStateText(const std::string& state) {
  if (state == "connecting") return "connecting";
  if (state == "connected_direct") return "connected directly";
  if (state == "connected_relay") return "connected through the relay";
  if (state == "key_expired") return "key expired, a new key is needed";
  if (state == "error") return "error, retrying";
  if (state == "unsupported") return "not available on this chip";
  return "off";
}

void remoteAccessLines(std::string& out, const ReportData& d) {
  if (d.remoteState.empty() || d.remoteState == "off" || d.remoteState == "unsupported") {
    line(out, "Remote access", remoteStateText(d.remoteState));
    return;
  }
  line(out, "Remote access", remoteStateText(d.remoteState) + " (" + d.remoteProvider + ")");
  if (!d.remoteAddress.empty()) line(out, "Remote access address", d.remoteAddress + " (" + d.remoteDeviceName + ")");
  if (d.remoteKeyExpiry > 0) line(out, "Remote access key expires", dateText(d.remoteKeyExpiry));
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
  remoteAccessLines(out, d);
  out += '\n';

  line(out, "Settings", d.settings);
  line(out, "Hardware", d.hardware);
  if (!d.journal.empty()) out += "\nPlay session journal:\n" + d.journal;
  out += "\nRecent log:\n";
  out += d.log.empty() ? std::string("(empty)\n") : d.log;

  return redacted ? redactorFor(d).apply(out) : out;
}

std::string deviceIdFrom(uint32_t random) { return format("%08x", unsigned(random)); }

}  // namespace diagnostics
