#pragma once

// Improv Wi-Fi serial protocol (https://www.improv-wifi.com/serial/), used by
// the web flasher to hand Wi-Fi credentials to the robot over USB.
// Pure C++: framing and encoding only, no I/O.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace improv {

enum class State : uint8_t { Authorized = 0x02, Provisioning = 0x03, Provisioned = 0x04 };

enum class Error : uint8_t {
  None = 0x00,
  InvalidRpc = 0x01,
  UnknownRpc = 0x02,
  UnableToConnect = 0x03,
  Unknown = 0xFF,
};

enum class Command : uint8_t {
  WifiSettings = 0x01,
  GetCurrentState = 0x02,
  GetDeviceInfo = 0x03,
  GetWifiNetworks = 0x04,
};

struct Request {
  Command command = Command::GetCurrentState;
  std::string ssid;      // WifiSettings only
  std::string password;  // WifiSettings only
};

// Reassembles RPC command packets from a serial stream that also carries
// other traffic, such as log lines, which it skips.
class Parser {
 public:
  enum class Status { Pending, Request, Invalid };

  // Consumes one byte. Returns Request when a full command has arrived (read
  // it with request()), Invalid when a packet was rejected (see error()).
  Status feed(uint8_t byte);

  const Request& request() const { return request_; }
  Error error() const { return error_; }

 private:
  Status finishPacket();
  Status decodeRpc(const uint8_t* rpc, size_t size);

  std::vector<uint8_t> buffer_;
  Request request_;
  Error error_ = Error::None;
};

std::vector<uint8_t> statePacket(State state);
std::vector<uint8_t> errorPacket(Error error);

// Answer to `command`, carrying each string length-prefixed.
std::vector<uint8_t> rpcResultPacket(Command command, const std::vector<std::string>& strings);

}  // namespace improv
