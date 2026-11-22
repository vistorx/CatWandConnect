#pragma once

// Remote access (CONTEXT.md, docs/adr/0015): the robot joins the owner's
// private network through a Remote access provider, so its page opens from
// anywhere. Off by default; the owner turns it on with a key from his
// provider account, and turning it off disconnects and forgets the key.
//
// Pure: the provider client sits behind the Provider port, the settings go
// through the Storage port, and nothing here blocks. Everything that may take
// time (joining, leaving) happens in tick(), which the firmware runs on its
// own task.

#include <robot_core/ports.h>

#include <cstdint>
#include <string>

namespace remote_access {

// What the provider's client says about its connection.
enum class Link : uint8_t {
  Stopped,
  Connecting,   // joining, registering or reconnecting
  Connected,
  KeyRejected,  // the key expired or was revoked: a new one is needed
  Failed,
};

struct ProviderStatus {
  Link link = Link::Stopped;
  bool direct = false;     // when connected: a direct path to a device, not only the provider's relay
  uint32_t address = 0;    // the robot's IPv4 address on the private network, 0 = none yet
  int64_t keyExpiry = 0;   // Unix time the robot's key expires at, 0 = never or not known
};

// One Remote access provider's client (Tailscale first).
class Provider {
 public:
  virtual ~Provider() = default;

  virtual const char* id() const = 0;  // "tailscale"
  // False when this firmware has no client for the chip it runs on.
  virtual bool available() const = 0;
  // Whether it can offer routes to other devices of the home network (#271).
  virtual bool routes() const = 0;

  // Joins the private network with `key`, as `deviceName`. False when the
  // client could not even start (out of memory).
  virtual bool start(const std::string& key, const std::string& deviceName) = 0;
  // Disconnects. May take a few seconds.
  virtual void stop() = 0;
  // Erases what the client kept to rejoin as the same device.
  virtual void forget() = 0;
  // The Wi-Fi came back after a drop: reopen the connections.
  virtual void networkChanged() = 0;
  virtual ProviderStatus status() const = 0;
};

enum class State : uint8_t {
  Off,
  Connecting,
  ConnectedDirect,
  ConnectedRelay,
  KeyExpired,
  Error,
  Unsupported,  // no client for this chip in this firmware
};

// "off", "connecting", "connected_direct", "connected_relay", "key_expired",
// "error", "unsupported": the API's and the Diagnostic report's words.
const char* stateName(State state);

// What the owner sees. Never carries the key.
struct Status {
  State state = State::Off;
  bool enabled = false;
  const char* provider = "";
  bool routes = false;
  std::string address;     // "100.101.102.103", empty until known
  std::string deviceName;  // the robot's name on the private network
  int64_t keyExpiry = 0;   // Unix time, 0 = never or not known
};

enum class Result : uint8_t { Ok, InvalidKey, Unsupported, StorageFailed };

// The API's error code for a refused change: "invalid_key", "unsupported",
// "storage"; nullptr for Ok.
const char* errorCode(Result result);

// Whether `key` looks like a Tailscale auth key: "tskey-" then letters,
// digits and dashes, at most kMaxKeyLength characters in all.
constexpr size_t kMaxKeyLength = 120;
bool validKey(const std::string& key);

// Whether an IPv4 address (host byte order) is on a tailnet: 100.64.0.0/10,
// where Tailscale gives each device its address.
bool isTailnetAddress(uint32_t address);

// "a.b.c.d" for an IPv4 address in host byte order, empty for 0.
std::string addressText(uint32_t address);

class RemoteAccess {
 public:
  RemoteAccess(Provider& provider, cwc::Storage& storage, const cwc::Clock& clock)
      : provider_(provider), storage_(storage), clock_(clock) {}

  // Reads the saved setting; `deviceName` is the robot's name on the private
  // network (its hostname).
  void begin(const std::string& deviceName);

  // The owner turns it on with `key` (Settings → Network → Remote access).
  // Saved at once; joining happens in the next tick() with Wi-Fi.
  Result turnOn(const std::string& key);
  // Disconnects and forgets the key, in the next tick().
  Result turnOff();

  // Joins, leaves or reconnects as the setting and the Wi-Fi require.
  // `wifiConnected`: the robot is on the home network.
  void tick(bool wifiConnected);

  Status status() const;

  // Wait before trying again when the client could not start.
  static constexpr uint32_t kRetryMs = 30000;

 private:
  bool save();

  Provider& provider_;
  cwc::Storage& storage_;
  const cwc::Clock& clock_;
  std::string deviceName_;

  bool enabled_ = false;
  std::string key_;

  bool started_ = false;     // the provider was started and not stopped since
  bool leave_ = false;       // stop and forget before anything else
  bool startFailed_ = false;
  uint32_t failedAt_ = 0;
  bool wifiWas_ = false;     // Wi-Fi at the previous tick
};

}  // namespace remote_access
