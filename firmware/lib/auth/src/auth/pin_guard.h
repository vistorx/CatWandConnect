#pragma once

// The optional settings PIN. When set, changing settings that could damage
// the robot or take it over needs a session token, obtained by entering the
// PIN. Manual control and Stow never need it.
//
// Wrong PINs lock unlocking out for a growing delay, so a 4-digit PIN can't
// be brute-forced over the network. Tokens live in RAM: a restart logs
// everyone out.

#include <cstdint>
#include <functional>
#include <string>

namespace auth {

class PinGuard {
 public:
  static constexpr size_t kMinDigits = 4;
  static constexpr size_t kMaxDigits = 8;
  static constexpr int kFreeAttempts = 5;
  static constexpr uint32_t kFirstLockoutMs = 30000;
  static constexpr uint32_t kMaxLockoutMs = 15 * 60 * 1000;
  static constexpr uint32_t kSessionIdleMs = 12 * 60 * 60 * 1000;
  static constexpr size_t kMaxSessions = 4;

  // `random` returns uniformly random 32-bit values (hardware RNG on the robot).
  explicit PinGuard(std::function<uint32_t()> random) : random_(std::move(random)) {}

  // 4 to 8 digits.
  static bool validPin(const std::string& pin);

  // An empty PIN disables protection. Changing the PIN ends every session.
  // Returns false (and changes nothing) for an invalid PIN.
  bool setPin(const std::string& pin);
  const std::string& pin() const { return pin_; }
  bool enabled() const { return !pin_.empty(); }

  enum class Unlock { Ok, Wrong, LockedOut };

  // Checks the PIN; on success `token` receives a new session token.
  Unlock unlock(const std::string& pin, uint32_t nowMs, std::string& token);

  // How long unlocking stays refused after too many wrong PINs.
  uint32_t lockedForMs(uint32_t nowMs) const;

  // True when protection is off, or `token` is a live session (which it
  // keeps alive).
  bool authorized(const std::string& token, uint32_t nowMs);

 private:
  struct Session {
    std::string token;
    uint32_t lastUsedMs = 0;
  };

  std::string newToken();

  std::function<uint32_t()> random_;
  std::string pin_;
  Session sessions_[kMaxSessions];
  int failures_ = 0;
  uint32_t lockedUntilMs_ = 0;
  bool locked_ = false;
};

}  // namespace auth
