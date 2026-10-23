// CatWandConnect firmware entry point: wires the Robot core to Arduino-ESP32.

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <auth/pin_guard.h>
#include <esp_random.h>
#include <identity/identity.h>
#include <robot_core/robot.h>

#include "device_api.h"
#include "diagnostics_api.h"
#include "log.h"
#include "mqtt_bridge.h"
#include "network/network.h"
#include "ota.h"
#include "play_log.h"
#include "servo_driver.h"
#include "web/web_ui.h"

namespace {

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

class HardwareRandom : public cwc::Random {
 public:
  uint32_t next() override { return esp_random(); }
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
HardwareRandom random;
cwc::Robot robot(motors, clock, storage, random);
PlayLog play(clock, storage);  // the Play session journal and the Robot clock

auth::PinGuard pin([] { return esp_random(); });
AsyncWebServer server(80);
Network network;
DeviceApi api(robot, storage, kChip, pin, play);
MqttBridge mqtt;

// The owner's name for the robot, set from the Device interface.
String loadDeviceName() {
  Preferences prefs;
  prefs.begin("device", true);
  String name = prefs.getString("name", identity::kDefaultName);
  prefs.end();
  return identity::validName(name.c_str()) ? name : String(identity::kDefaultName);
}

String loadPin() {
  Preferences prefs;
  prefs.begin("auth", true);
  String stored = prefs.getString("pin", "");
  prefs.end();
  return stored;
}

// Whether a Wi-Fi network was saved: set up before this boot (#195).
bool hasWifiCredentials() {
  Preferences prefs;
  prefs.begin("wifi", true);
  bool saved = !prefs.getString("ssid", "").isEmpty();
  prefs.end();
  return saved;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  logLine("CatWandConnect %s starting, last reset: %s", CWC_VERSION, resetReason());
  storage.begin();
  logLine("Device ID %s", deviceId().c_str());
  cwc::HardwareConfig hardware = cwc::loadHardware(storage, kChip);
  String name = loadDeviceName();
  pin.setPin(loadPin().c_str());
  // Robots that already answered at catwandconnect.local keep it (#195).
  bool legacyHostname = identity::legacyHostname(storage, hasWifiCredentials(), name.c_str());
  String hostname = identity::hostnameFor(name.c_str(), deviceId().c_str(), legacyHostname).c_str();
  motors.begin(hardware);
  play.journal.begin();
  robot.setPlayObserver(&play);
  robot.begin();

  api.begin(server, hardware, name, hostname, legacyHostname);
  registerOta(server, pin, kChip, [] { api.stopForUpdate(); }, [] { api.restartSoon(); });
  registerWebUi(server, [] { return network.inSetup(); });
  network.begin(server, name, hostname, pin);
  mqtt.begin(server, pin, api, name, hostname);
  registerDiagnostics(server, api, mqtt, network, pin, name, hostname);
  server.begin();
}

void loop() {
  network.loop();
  api.loop();  // also ticks the robot
  delay(5);
}
