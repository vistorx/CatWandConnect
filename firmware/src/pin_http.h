#pragma once

// HTTP side of the settings PIN: protected handlers call requirePin() first.

#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>

// True when the request may go on: protection is off or it carries a live
// session token ("Authorization: Bearer <token>"). Otherwise answers 401
// {"error":"pin_required"} and returns false.
inline bool requirePin(auth::PinGuard& guard, AsyncWebServerRequest* request) {
  std::string token;
  if (request->hasHeader("Authorization")) {
    String value = request->header("Authorization");
    if (value.startsWith("Bearer ")) token = value.substring(7).c_str();
  }
  if (guard.authorized(token, millis())) return true;
  request->send(401, "application/json", "{\"error\":\"pin_required\"}");
  return false;
}
