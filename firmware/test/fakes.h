#pragma once

// In-memory stand-ins for the Robot core's hardware ports.

#include <robot_core/ports.h>

#include <cstring>
#include <deque>
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
  void reverse(Axis axis) override { reversals.push_back(axis); }

  std::vector<Move> moves;
  std::vector<Axis> releases;
  std::vector<Axis> reversals;
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
    ++writes;
    return true;
  }

  std::map<std::string, std::vector<uint8_t>> blobs;
  int writes = 0;
};

// Plays back scripted draws, each a fraction of the drawn range (0 = its
// low end, 1 = its high end), then a seeded pseudo-random sequence.
class FakeRandom : public Random {
 public:
  uint32_t next() override {
    ++draws;
    if (!script.empty()) {
      double fraction = script.front();
      script.pop_front();
      return static_cast<uint32_t>(fraction * 4294967295.0);
    }
    seed = seed * 1664525u + 1013904223u;
    return seed;
  }

  std::deque<double> script;
  uint32_t seed = 12345;
  int draws = 0;
};

}  // namespace cwc::test
