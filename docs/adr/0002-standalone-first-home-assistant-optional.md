# Standalone first, Home Assistant optional

The first prototype was built with Home Assistant as its control surface. CatWandConnect must instead work with no Home Assistant at all: all play logic runs on the ESP32 and the robot serves its own Device interface. Home Assistant integration is kept as an optional extra, never a requirement, so that owners without a home-automation setup can use it.
