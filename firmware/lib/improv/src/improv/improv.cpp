#include "improv/improv.h"

namespace improv {

namespace {

constexpr uint8_t kHeader[] = {'I', 'M', 'P', 'R', 'O', 'V'};
constexpr size_t kHeaderSize = sizeof(kHeader);
constexpr uint8_t kVersion = 1;

// Offsets after the header.
constexpr size_t kVersionAt = kHeaderSize;
constexpr size_t kTypeAt = kHeaderSize + 1;
constexpr size_t kLengthAt = kHeaderSize + 2;
constexpr size_t kDataAt = kHeaderSize + 3;

enum class PacketType : uint8_t { CurrentState = 0x01, ErrorState = 0x02, Rpc = 0x03, RpcResult = 0x04 };

std::vector<uint8_t> packet(PacketType type, const std::vector<uint8_t>& data) {
  std::vector<uint8_t> out(kHeader, kHeader + kHeaderSize);
  out.push_back(kVersion);
  out.push_back(static_cast<uint8_t>(type));
  out.push_back(static_cast<uint8_t>(data.size()));
  out.insert(out.end(), data.begin(), data.end());
  uint8_t checksum = 0;
  for (uint8_t b : out) checksum += b;
  out.push_back(checksum);
  out.push_back('\n');
  return out;
}

}  // namespace

Parser::Status Parser::feed(uint8_t byte) {
  size_t at = buffer_.size();

  if (at < kHeaderSize) {
    if (byte == kHeader[at]) {
      buffer_.push_back(byte);
    } else {
      buffer_.clear();
      if (byte == kHeader[0]) buffer_.push_back(byte);
    }
    return Status::Pending;
  }

  if (at == kVersionAt && byte != kVersion) {
    buffer_.clear();
    return Status::Pending;
  }

  buffer_.push_back(byte);
  if (buffer_.size() <= kLengthAt) return Status::Pending;

  size_t complete = kDataAt + buffer_[kLengthAt] + 1;  // data plus checksum
  if (buffer_.size() < complete) return Status::Pending;

  Status status = finishPacket();
  buffer_.clear();
  return status;
}

Parser::Status Parser::finishPacket() {
  uint8_t checksum = 0;
  for (size_t i = 0; i + 1 < buffer_.size(); ++i) checksum += buffer_[i];
  if (checksum != buffer_.back()) {
    error_ = Error::InvalidRpc;
    return Status::Invalid;
  }
  if (buffer_[kTypeAt] != static_cast<uint8_t>(PacketType::Rpc)) return Status::Pending;
  return decodeRpc(&buffer_[kDataAt], buffer_[kLengthAt]);
}

Parser::Status Parser::decodeRpc(const uint8_t* rpc, size_t size) {
  error_ = Error::InvalidRpc;
  if (size < 2 || rpc[1] != size - 2) return Status::Invalid;

  const uint8_t* data = rpc + 2;
  size_t dataSize = rpc[1];
  Request request;

  switch (rpc[0]) {
    case static_cast<uint8_t>(Command::WifiSettings): {
      if (dataSize < 1) return Status::Invalid;
      size_t ssidSize = data[0];
      if (1 + ssidSize + 1 > dataSize) return Status::Invalid;
      size_t passwordSize = data[1 + ssidSize];
      if (1 + ssidSize + 1 + passwordSize != dataSize) return Status::Invalid;
      request.command = Command::WifiSettings;
      request.ssid.assign(reinterpret_cast<const char*>(data + 1), ssidSize);
      request.password.assign(reinterpret_cast<const char*>(data + 2 + ssidSize), passwordSize);
      break;
    }
    case static_cast<uint8_t>(Command::GetCurrentState):
    case static_cast<uint8_t>(Command::GetDeviceInfo):
    case static_cast<uint8_t>(Command::GetWifiNetworks):
      request.command = static_cast<Command>(rpc[0]);
      break;
    default:
      error_ = Error::UnknownRpc;
      return Status::Invalid;
  }

  request_ = request;
  error_ = Error::None;
  return Status::Request;
}

std::vector<uint8_t> statePacket(State state) {
  return packet(PacketType::CurrentState, {static_cast<uint8_t>(state)});
}

std::vector<uint8_t> errorPacket(Error error) {
  return packet(PacketType::ErrorState, {static_cast<uint8_t>(error)});
}

std::vector<uint8_t> rpcResultPacket(Command command, const std::vector<std::string>& strings) {
  std::vector<uint8_t> payload;
  for (const auto& s : strings) {
    payload.push_back(static_cast<uint8_t>(s.size()));
    payload.insert(payload.end(), s.begin(), s.end());
  }
  std::vector<uint8_t> data = {static_cast<uint8_t>(command), static_cast<uint8_t>(payload.size())};
  data.insert(data.end(), payload.begin(), payload.end());
  return packet(PacketType::RpcResult, data);
}

}  // namespace improv
