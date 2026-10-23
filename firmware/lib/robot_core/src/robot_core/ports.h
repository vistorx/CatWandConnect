#pragma once

// The interfaces through which the Robot core reaches hardware. Firmware
// implements them with Arduino-ESP32; tests implement them with fakes.

#include <cstddef>
#include <cstdint>

namespace cwc {

enum class Axis : uint8_t { Horizontal, Vertical };

constexpr size_t kAxisCount = 2;
constexpr Axis kAxes[kAxisCount] = {Axis::Horizontal, Axis::Vertical};

class MotorDriver {
 public:
  virtual ~MotorDriver() = default;

  // Drives the axis to `degrees` and holds it there.
  virtual void moveTo(Axis axis, float degrees) = 0;

  // Stops driving the axis so the servo rests.
  virtual void release(Axis axis) = 0;

  // Swaps the axis's direction from now on: `degrees` becomes 180 - `degrees`.
  virtual void reverse(Axis axis) = 0;
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

// What happened to a Play session, as the Robot core reports it.
enum class PlayEvent : uint8_t {
  Started,
  Ended,    // the Session duration is over
  Stopped,  // ended early, see PlayCause
  Refused,  // a start was refused, see PlayCause
};

// Why a Play session stopped or a start was refused; None for the others.
enum class PlayCause : uint8_t {
  None,
  Stop,           // stopPlaySession()
  Stow,
  ManualControl,
  Calibration,    // a Calibration started
  SpeedTuning,    // a Speed or Acceleration tuning started
  Busy,           // stowing, calibrating or tuning a speed
  NotCalibrated,
};

// Told about each Play session event, e.g. to keep the Play session journal.
// Called from inside the Robot core's calls, on the caller's task.
class PlayObserver {
 public:
  virtual ~PlayObserver() = default;

  virtual void playSession(PlayEvent event, PlayCause cause) = 0;
};

// Source of randomness for the Play session's draws.
class Random {
 public:
  virtual ~Random() = default;

  // A uniformly distributed 32-bit value.
  virtual uint32_t next() = 0;
};

}  // namespace cwc
