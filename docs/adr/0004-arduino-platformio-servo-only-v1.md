# Arduino-ESP32 on PlatformIO; PWM servos only in V1

The firmware is written with the Arduino-ESP32 framework on PlatformIO, like WLED, for its library ecosystem and its accessibility to contributors; ESP-IDF calls are used directly where low-level control is needed. V1 targets ESP32, ESP32-S3 and ESP32-C3 (one binary per chip in the web flasher), drives two axes with hobby PWM servos only, and lets the user set pins from the Device interface. Motor drivers sit behind their own layer so steppers or feedback servos can be added later without touching the motion logic.
