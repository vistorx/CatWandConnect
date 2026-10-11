#include "auth/pin_guard.h"

namespace auth {

namespace {

// Constant-time comparison, so response timing doesn't reveal the PIN.
bool sameSecret(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); ++i) diff |= static_cast<unsigned char>(a[i] ^ b[i]);
  return diff == 0;
}

bool reached(uint32_t nowMs, uint32_t deadlineMs) { return static_cast<int32_t>(nowMs - deadlineMs) >= 0; }

}  // namespace

bool PinGuard::validPin(const std::string& pin) {
  if (pin.size() < kMinDigits || pin.size() > kMaxDigits) return false;
  for (char c : pin) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

bool PinGuard::setPin(const std::string& pin) {
  if (!pin.empty() && !validPin(pin)) return false;
  pin_ = pin;
  for (Session& session : sessions_) session = Session{};
  failures_ = 0;
  locked_ = false;
  return true;
}

uint32_t PinGuard::lockedForMs(uint32_t nowMs) const {
  if (!locked_ || reached(nowMs, lockedUntilMs_)) return 0;
  return lockedUntilMs_ - nowMs;
}

PinGuard::Unlock PinGuard::unlock(const std::string& pin, uint32_t nowMs, std::string& token) {
  if (lockedForMs(nowMs) > 0) return Unlock::LockedOut;

  if (!enabled() || !sameSecret(pin, pin_)) {
    if (++failures_ >= kFreeAttempts) {
      // 30 s, then doubling for each further failure, up to 15 min.
      uint32_t lockout = kFirstLockoutMs;
      for (int i = kFreeAttempts; i < failures_ && lockout < kMaxLockoutMs; ++i) lockout *= 2;
      if (lockout > kMaxLockoutMs) lockout = kMaxLockoutMs;
      lockedUntilMs_ = nowMs + lockout;
      locked_ = true;
    }
    return Unlock::Wrong;
  }

  failures_ = 0;
  locked_ = false;
  token = newToken();

  // Reuse an expired slot, or else the least recently used one.
  Session* slot = &sessions_[0];
  for (Session& session : sessions_) {
    if (session.token.empty() || reached(nowMs, session.lastUsedMs + kSessionIdleMs)) {
      slot = &session;
      break;
    }
    if (session.lastUsedMs < slot->lastUsedMs) slot = &session;
  }
  *slot = Session{token, nowMs};
  return Unlock::Ok;
}

bool PinGuard::authorized(const std::string& token, uint32_t nowMs) {
  if (!enabled()) return true;
  if (token.empty()) return false;
  for (Session& session : sessions_) {
    if (session.token.empty() || !sameSecret(token, session.token)) continue;
    if (reached(nowMs, session.lastUsedMs + kSessionIdleMs)) {
      session = Session{};
      return false;
    }
    session.lastUsedMs = nowMs;
    return true;
  }
  return false;
}

std::string PinGuard::newToken() {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string token;
  for (int word = 0; word < 4; ++word) {  // 128 bits
    uint32_t bits = random_();
    for (int i = 0; i < 8; ++i, bits >>= 4) token += kHex[bits & 0xF];
  }
  return token;
}

}  // namespace auth
