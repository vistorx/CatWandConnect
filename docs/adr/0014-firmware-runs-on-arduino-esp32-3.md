# Firmware runs on Arduino-ESP32 3.x

Remote access (#251) puts a VPN client inside the robot. The Tailscale client for the ESP32 (microlink), like the NetBird one, needs ESP-IDF 5, and the firmware ran on Arduino-ESP32 2.x, which sits on ESP-IDF 4.4 and gets no new features. The official `espressif32` PlatformIO platform does not ship Arduino-ESP32 3.x, so staying on PlatformIO (ADR 0004) means another platform.

We decided the firmware builds with the community [pioarduino](https://github.com/pioarduino/platform-espressif32) platform, pinned in `firmware/platformio.ini` to the exact release zip URL of **55.03.313**: Arduino-ESP32 3.3.13 on ESP-IDF 5.5. pioarduino is a drop-in `espressif32` platform (same board names, `framework = arduino`, `native` untouched), it packages each Arduino-ESP32 3.x release, and it is the usual way PlatformIO projects get Arduino-ESP32 3.x. The alternatives were worse: the official platform has no 3.x; building with plain ESP-IDF plus Arduino as a component would rewrite the build and drop PlatformIO's `native` test environment that the Robot core relies on. pioarduino's 61.x line (Arduino-ESP32 4.0 on ESP-IDF 6) was left for later: 3.x is what the VPN clients are written for.

Nothing changes for owners: same partition layout (`min_spiffs`, two 1.9 MB app slots, NVS at `0x9000`), same NVS namespaces and keys, same API, MQTT topics and Device interface.

## Consequences

- The platform is a pinned zip URL, not a registry version: moving it is a deliberate firmware change, retested on hardware with the *Arduino-ESP32 3.x migration* section of the acceptance checklist.
- The app grows by 190 to 240 KB depending on the chip (classic ESP32: 1.13 MB → 1.32 MB, 67 % of the 1.9 MB slot; ESP32-C3, the largest: 73 %). Remote access must fit in what is left.
- Robots updated over the air keep their ESP-IDF 4.4 bootloader; only a USB install (web flasher) writes the new one. The ESP-IDF 5 app is built to boot from both, and the hardware retest covers the OTA path from the current stable.
- Code follows the 3.x APIs: LEDC is addressed by pin (`ledcAttachChannel`, `ledcWrite(pin, …)`), keeping one channel per axis on a shared 50 Hz, 14-bit timer; the hotspot name reads the MAC from eFuse, because `WiFi.macAddress()` returns nothing before the Wi-Fi driver starts; the Wi-Fi glue class is `RobotNetwork`, since 3.x declares a global `Network`.
- New USB installs get an ESP-IDF 5 bootloader with app rollback enabled. The Arduino core marks each app valid as soon as it starts, so an update rolls back only if the new firmware crashes before it starts.
- ESP-IDF 5.5 requires ESP32-C3 chip revision 0.3 or newer; the earliest C3 boards (revisions 0.1 and 0.2, from 2021) can no longer run the firmware.
- `esptool` 5 is the one PlatformIO now runs; `tools/merge_firmware.py` uses its `merge-bin` spelling. The factory image layout the web flasher splits (bootloader at `0x1000` on the ESP32, `0x0` on the S3 and C3; partition table at `0x8000`) is unchanged.
