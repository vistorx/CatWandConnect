# ESP8266 is not supported, and almost certainly never will be

CatWandConnect runs on the ESP32 family only (ESP32, ESP32-S3, ESP32-C3). We considered the ESP8266, since WLED and Tasmota run on it, and rejected it for these reasons:

- **Memory.** It has about 40 KB of free RAM once Wi-Fi is up, against more than 250 KB on an ESP32. The Device interface, the WebSocket, MQTT and OTA updates would not fit comfortably together, and 1 MB boards leave too little flash for OTA.
- **Servo quality.** It has no hardware PWM, so servo pulses would come from software timing and jitter.
- **Missing features.** It has no FPU and no temperature sensor, and its pins are scarce: about five are safe to use. It has no Bluetooth, which is planned.
- **Maintenance.** Supporting it would need a separate servo driver, storage, Wi-Fi and mDNS layer, and locking model, all behind `#ifdef`s, plus a fourth binary to test on real hardware before every release.
- **No saving.** An ESP32-C3 board costs the same as an ESP8266 one, about €2–3.

Issues and pull requests asking for ESP8266 support will be closed with a link to this ADR. The Robot core is plain C++ and the motor layer sits behind an interface, so a community fork could still port it. Reopening this decision would need a supersede ADR showing a real need that a cheap ESP32 cannot meet.
