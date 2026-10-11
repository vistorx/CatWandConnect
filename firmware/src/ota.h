#pragma once

// Firmware update by file upload (POST /api/update, docs/api.md).
//
// The new image is written to the inactive OTA slot and only marked bootable
// once complete and verified, so a failed or interrupted upload leaves the
// running firmware in place. Settings live in NVS, outside the app slots, and
// survive.

#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>
#include <robot_core/hardware.h>

#include <functional>

// `onStart` runs when a valid upload begins (to stop the robot); `onDone` when
// it succeeded and the board should restart.
void registerOta(AsyncWebServer& server, auth::PinGuard& pin, cwc::Chip chip, std::function<void()> onStart,
                 std::function<void()> onDone);
