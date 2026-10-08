# Dedicated firmware replaces ESPHome

ESPHome was only the proof of concept. CatWandConnect gets its own firmware, modelled on WLED: an in-browser web flasher, a Device interface served over local Wi-Fi, and hardware (board, pins) chosen by the user rather than fixed at build time. We give up ESPHome's free Home Assistant integration, OTA and captive portal and must rebuild them, in exchange for full control over the interface, safety limits and behaviour. No cloud and no mobile app in V1; Bluetooth may come later.
