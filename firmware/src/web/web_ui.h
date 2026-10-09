#pragma once

// Serves the Device interface (firmware/web/, embedded gzip-compressed by
// tools/embed_web.py). "/" shows the Wi-Fi setup page until the robot is on
// the owner's network, then the main screen.

#include <ESPAsyncWebServer.h>

#include <functional>

void registerWebUi(AsyncWebServer& server, std::function<bool()> inSetup);
