#pragma once

// Remote access on the robot (CONTEXT.md, docs/adr/0015): the
// remote_access::RemoteAccess core with the Tailscale provider, run on its
// own task, and its API (GET/PUT /api/remote-access, docs/api.md).
//
// Joining and leaving can take seconds (leaving waits for the client's
// tasks), so HTTP handlers never call the core: they check the request, leave
// it for the task, and answer from the status the task last published.

#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>
#include <remote_access/remote_access.h>

#include <mutex>
#include <string>

class RemoteAccessService {
 public:
  void begin(AsyncWebServer& server, auth::PinGuard& pin, const String& hostname);

  // As last published by the task. Never carries the key.
  remote_access::Status status();

  // Whether `request` came from a device on the owner's private network
  // (100.64.0.0/10): over remote access, not from home.
  static bool viaRemoteAccess(AsyncWebServerRequest* request);

 private:
  static void taskEntry(void* self);
  void run();
  void getStatus(AsyncWebServerRequest* request);
  void putStatus(AsyncWebServerRequest* request, JsonVariant& body);

  auth::PinGuard* pin_ = nullptr;
  std::string hostname_;

  std::mutex lock_;  // guards everything below
  remote_access::Status published_;
  // The owner's last change, for the task: kNone, or turn on (with a key) or off.
  enum class Change : uint8_t { None, On, Off };
  Change pending_ = Change::None;
  std::string pendingKey_;
};
