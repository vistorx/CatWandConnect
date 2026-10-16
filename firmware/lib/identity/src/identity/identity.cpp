#include "identity/identity.h"

namespace identity {

namespace {

// ASCII stand-ins for the Latin-1 letters U+00C0 to U+00FF, encoded in UTF-8
// as 0xC3 followed by 0x80 to 0xBF. '-' marks symbols with no letter.
constexpr char kLatin1[] =
    "aaaaaaaceeeeiiii"  // À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï
    "dnooooo-ouuuuyts"  // Ð Ñ Ò Ó Ô Õ Ö × Ø Ù Ú Û Ü Ý Þ ß
    "aaaaaaaceeeeiiii"  // à á â ã ä å æ ç è é ê ë ì í î ï
    "dnooooo-ouuuuyty"; // ð ñ ò ó ô õ ö ÷ ø ù ú û ü ý þ ÿ

bool isAlnum(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }

char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

bool isContinuation(unsigned char c) { return (c & 0xC0) == 0x80; }

std::string slug(const std::string& name);

}  // namespace

bool validName(const std::string& name) {
  if (name.empty() || name.size() > kMaxNameBytes) return false;
  for (unsigned char c : name) {
    if (c < 0x20 || c == 0x7F) return false;
  }
  return !slug(name).empty();
}

std::string hostnameFor(const std::string& name) {
  std::string host = slug(name);
  return host.empty() ? kDefaultHostname : host;
}

std::string hostnameFor(const std::string& name, const std::string& deviceId, bool legacy) {
  std::string host = hostnameFor(name);
  if (host != kDefaultHostname || legacy) return host;
  std::string suffix;
  for (char c : deviceId.substr(0, 4)) suffix += lower(c);
  return suffix.empty() ? host : host + "-" + suffix;
}

namespace {
constexpr const char* kLegacyKey = "legacyHost";
constexpr uint8_t kLegacyRecordVersion = 1;
}  // namespace

bool legacyHostname(cwc::Storage& storage, bool hasWifiCredentials, const std::string& name) {
  uint8_t record[2] = {0, 0};
  if (storage.read(kLegacyKey, record, sizeof(record)) && record[0] == kLegacyRecordVersion) {
    return record[1] == 1;
  }
  bool legacy = hasWifiCredentials && hostnameFor(name) == kDefaultHostname;
  record[0] = kLegacyRecordVersion;
  record[1] = legacy ? 1 : 0;
  storage.write(kLegacyKey, record, sizeof(record));
  return legacy;
}

namespace {

std::string slug(const std::string& name) {
  std::string host;
  bool pendingHyphen = false;
  auto append = [&](char c) {
    if (pendingHyphen && !host.empty()) host += '-';
    pendingHyphen = false;
    host += c;
  };

  for (size_t i = 0; i < name.size(); ++i) {
    unsigned char c = name[i];
    if (isAlnum(c)) {
      append(lower(c));
    } else if (c == 0xC3 && i + 1 < name.size() && isContinuation(name[i + 1])) {
      char ascii = kLatin1[static_cast<unsigned char>(name[++i]) - 0x80];
      if (ascii == '-') pendingHyphen = true;
      else append(ascii);
    } else {
      pendingHyphen = true;
    }
  }

  if (host.size() > kMaxHostnameLength) host.resize(kMaxHostnameLength);
  while (!host.empty() && host.back() == '-') host.pop_back();
  return host;
}

}  // namespace

std::string hotspotName(const std::string& name, const std::string& suffix) {
  constexpr size_t kMaxSsid = 32;
  size_t room = kMaxSsid - 1 - suffix.size();
  std::string base = name;
  if (base.size() > room) {
    base.resize(room);
    while (!base.empty() && isContinuation(static_cast<unsigned char>(base.back()))) base.pop_back();
    if (!base.empty() && static_cast<unsigned char>(base.back()) >= 0xC0) base.pop_back();
  }
  return base + "-" + suffix;
}

}  // namespace identity
