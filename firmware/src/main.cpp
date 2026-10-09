// CatWandConnect firmware entry point: wires the Robot core to Arduino-ESP32.

#include <Arduino.h>
#include <Preferences.h>
#include <robot_core/robot.h>

namespace {

class ArduinoClock : public cwc::Clock {
 public:
  uint32_t nowMs() const override { return millis(); }
};

// Settings live in the "cwc" NVS namespace, which survives OTA updates.
class NvsStorage : public cwc::Storage {
 public:
  void begin() { prefs_.begin("cwc", false); }

  bool read(const char* key, void* data, size_t size) override {
    if (prefs_.getBytesLength(key) != size) return false;
    return prefs_.getBytes(key, data, size) == size;
  }

  bool write(const char* key, const void* data, size_t size) override {
    return prefs_.putBytes(key, data, size) == size;
  }

 private:
  Preferences prefs_;
};

// Placeholder until the PWM servo driver lands with #5.
class NullMotorDriver : public cwc::MotorDriver {
 public:
  void moveTo(cwc::Axis, float) override {}
  void release(cwc::Axis) override {}
};

ArduinoClock clock;
NvsStorage storage;
NullMotorDriver motors;
cwc::Robot robot(motors, clock, storage);

}  // namespace

void setup() {
  Serial.begin(115200);
  storage.begin();
  Serial.println("CatWandConnect starting");
}

void loop() {
  robot.tick();
  delay(10);
}
