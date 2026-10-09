#pragma once

// In-memory stand-ins for the Robot core's hardware ports.

#include <robot_core/ports.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cwc::test {

class FakeMotorDriver : public MotorDriver {
 public:
  struct Move {
    Axis axis;
    float degrees;
  };

  void moveTo(Axis axis, float degrees) override { moves.push_back({axis, degrees}); }
  void release(Axis axis) override { releases.push_back(axis); }

  std::vector<Move> moves;
  std::vector<Axis> releases;
};

class FakeClock : public Clock {
 public:
  uint32_t nowMs() const override { return now; }
  void advance(uint32_t ms) { now += ms; }

  uint32_t now = 0;
};

class FakeStorage : public Storage {
 public:
  bool read(const char* key, void* data, size_t size) override {
    auto it = blobs.find(key);
    if (it == blobs.end() || it->second.size() != size) return false;
    std::memcpy(data, it->second.data(), size);
    return true;
  }

  bool write(const char* key, const void* data, size_t size) override {
    auto bytes = static_cast<const uint8_t*>(data);
    blobs[key].assign(bytes, bytes + size);
    return true;
  }

  std::map<std::string, std::vector<uint8_t>> blobs;
};

}  // namespace cwc::test
