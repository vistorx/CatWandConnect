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

// Defaults until device naming (#11) and hardware configuration (#10) land.
constexpr const char* kDeviceName = "CatWandConnect";
constexpr const char* kHostname = "catwandconnect";

// Pulse ranges from the ESPHome proof of concept (2-11 % and 3-12 % of 20 ms).
#if CONFIG_IDF_TARGET_ESP32
constexpr ServoConfig kHorizontalServo{18, 400, 2200};
constexpr ServoConfig kVerticalServo{19, 600, 2400};
#else
// GPIO 18 and 19 are the USB pins on the S3 and C3.
constexpr ServoConfig kHorizontalServo{4, 400, 2200};
constexpr ServoConfig kVerticalServo{5, 600, 2400};
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
PwmServoDriver motors(kHorizontalServo, kVerticalServo);
cwc::Robot robot(motors, clock, storage);

AsyncWebServer server(80);
Network network(kDeviceName, kHostname);
DeviceApi api(robot);

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.printf("CatWandConnect %s starting\n", CWC_VERSION);
  storage.begin();
  motors.begin();
  robot.begin();

  api.begin(server);
  registerWebUi(server, [] { return network.inSetup(); });
  network.begin(server);
  server.begin();
}

void loop() {
  network.loop();
  api.loop();  // also ticks the robot
  delay(5);
}
