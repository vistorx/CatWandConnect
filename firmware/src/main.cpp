// CatWandConnect firmware entry point: wires the Robot core to Arduino-ESP32.

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <robot_core/robot.h>

#include "device_api.h"
#include "network/network.h"
#include "servo_driver.h"
#include "web/web_ui.h"

namespace {

// Defaults until device naming lands (#11).
constexpr const char* kDeviceName = "CatWandConnect";
constexpr const char* kHostname = "catwandconnect";

#if CONFIG_IDF_TARGET_ESP32S3
constexpr cwc::Chip kChip = cwc::Chip::Esp32S3;
#elif CONFIG_IDF_TARGET_ESP32C3
constexpr cwc::Chip kChip = cwc::Chip::Esp32C3;
#else
constexpr cwc::Chip kChip = cwc::Chip::Esp32;
#endif

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

ArduinoClock clock;
NvsStorage storage;
PwmServoDriver motors;
cwc::Robot robot(motors, clock, storage);

AsyncWebServer server(80);
Network network(kDeviceName, kHostname);
DeviceApi api(robot, storage, kChip);

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.printf("CatWandConnect %s starting\n", CWC_VERSION);
  storage.begin();
  cwc::HardwareConfig hardware = cwc::loadHardware(storage, kChip);
  motors.begin(hardware);
  robot.begin();

  api.begin(server, hardware);
  registerWebUi(server, [] { return network.inSetup(); });
  network.begin(server);
  server.begin();
}

void loop() {
  network.loop();
  api.loop();  // also ticks the robot
  delay(5);
}
