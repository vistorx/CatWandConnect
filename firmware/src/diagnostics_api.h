#pragma once

// GET /api/diagnostics: the Diagnostic report as plain text (docs/api.md).
// Redacted by default; "?full=1" keeps personal data, for local use only.

#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>

class DeviceApi;
class MqttBridge;
class Network;

void registerDiagnostics(AsyncWebServer& server, DeviceApi& api, MqttBridge& mqtt, Network& network,
                         auth::PinGuard& pin, const String& name, const String& hostname);
