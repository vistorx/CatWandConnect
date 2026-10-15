#include "update_channel/update_channel.h"

#include <cstdint>
#include <vector>

namespace update_channel {

namespace {

// Stored as {kRecordVersion, 0 = stable / 1 = beta} under kKey in the
// settings storage. No record means "never set".
constexpr const char* kKey = "channel";
constexpr uint8_t kRecordVersion = 1;

struct Version {
  bool valid = false;
  unsigned long core[3] = {0, 0, 0};
  std::vector<std::string> pre;  // pre-release identifiers, empty for a release
};

// Digits only, short enough for stoul.
bool numeric(const std::string& text) {
  if (text.empty() || text.size() > 9) return false;
  for (char c : text) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

std::vector<std::string> split(const std::string& text, char separator) {
  std::vector<std::string> parts;
  size_t start = 0;
  for (;;) {
    size_t at = text.find(separator, start);
    parts.push_back(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
    if (at == std::string::npos) return parts;
    start = at + 1;
  }
}

Version parse(std::string text) {
  Version version;
  if (!text.empty() && (text[0] == 'v' || text[0] == 'V')) text.erase(0, 1);
  text = text.substr(0, text.find_first_of("+ "));  // build metadata, " (simulator)"
  size_t dash = text.find('-');
  std::vector<std::string> core = split(text.substr(0, dash), '.');
  if (core.size() != 3) return version;
  for (size_t i = 0; i < 3; ++i) {
    if (!numeric(core[i])) return version;
    version.core[i] = std::stoul(core[i]);
  }
  if (dash != std::string::npos) {
    version.pre = split(text.substr(dash + 1), '.');
    for (const std::string& id : version.pre) {
      if (id.empty()) return version;
    }
  }
  version.valid = true;
  return version;
}

int sign(long long value) { return value < 0 ? -1 : value > 0 ? 1 : 0; }

// SemVer 2.0 precedence of two pre-release identifiers: numbers compare as
// numbers and come before words, words compare as text.
int compareIdentifiers(const std::string& a, const std::string& b) {
  bool an = numeric(a);
  bool bn = numeric(b);
  if (an && bn) return sign(static_cast<long long>(std::stoul(a)) - static_cast<long long>(std::stoul(b)));
  if (an != bn) return an ? -1 : 1;
  return sign(a.compare(b));
}

int compare(const Version& a, const Version& b) {
  for (size_t i = 0; i < 3; ++i) {
    if (a.core[i] != b.core[i]) return a.core[i] < b.core[i] ? -1 : 1;
  }
  // A release outranks its pre-releases.
  if (a.pre.empty() != b.pre.empty()) return a.pre.empty() ? 1 : -1;
  for (size_t i = 0; i < a.pre.size() && i < b.pre.size(); ++i) {
    int order = compareIdentifiers(a.pre[i], b.pre[i]);
    if (order != 0) return order;
  }
  return sign(static_cast<long long>(a.pre.size()) - static_cast<long long>(b.pre.size()));
}

}  // namespace

const char* channelName(Channel channel) { return channel == Channel::Beta ? "beta" : "stable"; }

bool parseChannel(const std::string& name, Channel& channel) {
  if (name == "stable") channel = Channel::Stable;
  else if (name == "beta") channel = Channel::Beta;
  else return false;
  return true;
}

Channel channelOf(const std::string& version) {
  return version.find("-beta") != std::string::npos ? Channel::Beta : Channel::Stable;
}

Channel loadChannel(cwc::Storage& storage, const std::string& running) {
  uint8_t record[2] = {0, 0};
  if (storage.read(kKey, record, sizeof(record)) && record[0] == kRecordVersion && record[1] <= 1) {
    return record[1] == 1 ? Channel::Beta : Channel::Stable;
  }
  return channelOf(running);
}

bool saveChannel(cwc::Storage& storage, Channel channel) {
  uint8_t record[2] = {kRecordVersion, static_cast<uint8_t>(channel == Channel::Beta ? 1 : 0)};
  return storage.write(kKey, record, sizeof(record));
}

int compareVersions(const std::string& a, const std::string& b) {
  Version va = parse(a);
  Version vb = parse(b);
  if (!va.valid || !vb.valid) return va.valid == vb.valid ? 0 : (va.valid ? 1 : -1);
  return compare(va, vb);
}

bool isNewer(const std::string& candidate, const std::string& running) {
  Version c = parse(candidate);
  Version r = parse(running);
  return c.valid && r.valid && compare(c, r) > 0;
}

}  // namespace update_channel
