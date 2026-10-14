#pragma once

// GET /api/diagnostics: the Diagnostic report as plain text (docs/api.md).
// Redacted by default; "?full=1" keeps personal data, for local use only.
// /api/diagnostics/pending: the Pending report (GET, PUT, DELETE).

#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>

class DeviceApi;
class MqttBridge;
class Network;

// The Device ID (CONTEXT.md), drawn once and kept in NVS. Call it once from
// setup() before the server starts; later calls only read the cached value.
const String& deviceId();

void registerDiagnostics(AsyncWebServer& server, DeviceApi& api, MqttBridge& mqtt, Network& network,
                         auth::PinGuard& pin, const String& name, const String& hostname);
