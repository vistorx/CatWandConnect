#pragma once

// The interfaces through which the Robot core reaches hardware. Firmware
// implements them with Arduino-ESP32; tests implement them with fakes.

#include <cstddef>
#include <cstdint>

namespace cwc {

enum class Axis : uint8_t { Horizontal, Vertical };

constexpr size_t kAxisCount = 2;

class MotorDriver {
 public:
  virtual ~MotorDriver() = default;

  // Drives the axis to `degrees` and holds it there.
  virtual void moveTo(Axis axis, float degrees) = 0;

  // Stops driving the axis so the servo rests.
  virtual void release(Axis axis) = 0;
};

class Clock {
 public:
  virtual ~Clock() = default;

  virtual uint32_t nowMs() const = 0;
};

// Persistent key/value blobs that survive reboots and firmware updates.
class Storage {
 public:
  virtual ~Storage() = default;

  // Returns false when the key is missing or its size differs from `size`.
  virtual bool read(const char* key, void* data, size_t size) = 0;

  virtual bool write(const char* key, const void* data, size_t size) = 0;
};

}  // namespace cwc
