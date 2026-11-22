#include "remote_access/remote_access.h"

#include <cstring>

namespace remote_access {

namespace {

// Saved under kRecordKey as one fixed-size record: kRecordVersion, enabled
// (0/1), the key's length, then the key padded with zeros. Turned off, the
// whole record is zeros: the key is gone.
constexpr const char* kRecordKey = "remote";
constexpr uint8_t kRecordVersion = 1;

struct Record {
  uint8_t version = 0;
  uint8_t enabled = 0;
  uint8_t keyLength = 0;
  char key[kMaxKeyLength] = {};
};

bool keyCharacter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
}

void wipe(std::string& text) {
  text.assign(text.size(), '\0');
  text.clear();
}

}  // namespace

const char* stateName(State state) {
  switch (state) {
    case State::Off: return "off";
    case State::Connecting: return "connecting";
    case State::ConnectedDirect: return "connected_direct";
    case State::ConnectedRelay: return "connected_relay";
    case State::KeyExpired: return "key_expired";
    case State::Error: return "error";
    case State::Unsupported: return "unsupported";
  }
  return "off";
}

const char* errorCode(Result result) {
  switch (result) {
    case Result::Ok: return nullptr;
    case Result::InvalidKey: return "invalid_key";
    case Result::Unsupported: return "unsupported";
    case Result::StorageFailed: return "storage";
  }
  return nullptr;
}

bool validKey(const std::string& key) {
  static const std::string prefix = "tskey-";
  if (key.size() <= prefix.size() || key.size() > kMaxKeyLength) return false;
  if (key.compare(0, prefix.size(), prefix) != 0) return false;
  for (char c : key) {
    if (!keyCharacter(c)) return false;
  }
  return true;
}

bool isTailnetAddress(uint32_t address) { return (address & 0xffc00000u) == ((100u << 24) | (64u << 16)); }

std::string addressText(uint32_t address) {
  if (address == 0) return "";
  return std::to_string(address >> 24) + "." + std::to_string(address >> 16 & 0xff) + "." +
         std::to_string(address >> 8 & 0xff) + "." + std::to_string(address & 0xff);
}

void RemoteAccess::begin(const std::string& deviceName) {
  deviceName_ = deviceName;
  Record record;
  if (storage_.read(kRecordKey, &record, sizeof(record)) && record.version == kRecordVersion &&
      record.enabled == 1 && record.keyLength <= kMaxKeyLength) {
    std::string key(record.key, record.keyLength);
    if (validKey(key)) {
      enabled_ = true;
      key_ = key;
    }
    wipe(key);
  }
  record = Record{};
}

Result RemoteAccess::turnOn(const std::string& key) {
  if (!provider_.available()) return Result::Unsupported;
  if (!validKey(key)) return Result::InvalidKey;
  // A new key replaces the device the old one joined as.
  if (started_ || enabled_) leave_ = true;
  enabled_ = true;
  wipe(key_);
  key_ = key;
  startFailed_ = false;
  return save() ? Result::Ok : Result::StorageFailed;
}

Result RemoteAccess::turnOff() {
  enabled_ = false;
  wipe(key_);
  leave_ = true;
  startFailed_ = false;
  return save() ? Result::Ok : Result::StorageFailed;
}

bool RemoteAccess::save() {
  Record record;
  if (enabled_) {
    record.version = kRecordVersion;
    record.enabled = 1;
    record.keyLength = static_cast<uint8_t>(key_.size());
    std::memcpy(record.key, key_.data(), key_.size());
  }
  bool saved = storage_.write(kRecordKey, &record, sizeof(record));
  record = Record{};
  return saved;
}

void RemoteAccess::tick(bool wifiConnected) {
  const bool wifiCameBack = wifiConnected && !wifiWas_;
  wifiWas_ = wifiConnected;

  if (leave_) {
    leave_ = false;
    provider_.stop();
    provider_.forget();
    started_ = false;
  }
  if (!enabled_ || !provider_.available() || !wifiConnected) return;

  if (started_) {
    if (wifiCameBack) provider_.networkChanged();
    return;
  }
  const uint32_t now = clock_.nowMs();
  if (startFailed_ && static_cast<int32_t>(now - (failedAt_ + kRetryMs)) < 0) return;
  started_ = provider_.start(key_, deviceName_);
  startFailed_ = !started_;
  failedAt_ = now;
}

Status RemoteAccess::status() const {
  Status s;
  s.enabled = enabled_;
  s.provider = provider_.id();
  s.routes = provider_.routes();
  s.deviceName = deviceName_;
  if (!provider_.available()) {
    s.state = State::Unsupported;
    return s;
  }
  if (!enabled_) return s;
  if (startFailed_) {
    s.state = State::Error;
    return s;
  }
  s.state = State::Connecting;
  if (!started_) return s;
  ProviderStatus p = provider_.status();
  s.address = addressText(p.address);
  s.keyExpiry = p.keyExpiry;
  switch (p.link) {
    case Link::Connected: s.state = p.direct ? State::ConnectedDirect : State::ConnectedRelay; break;
    case Link::KeyRejected: s.state = State::KeyExpired; break;
    case Link::Failed: s.state = State::Error; break;
    case Link::Stopped:
    case Link::Connecting: s.state = State::Connecting; break;
  }
  return s;
}

}  // namespace remote_access
