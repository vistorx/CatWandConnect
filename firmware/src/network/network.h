#pragma once

// Gets the robot onto the owner's Wi-Fi and keeps it reachable:
// - credentials from Improv over serial (web flasher) or from the setup page;
// - a CatWandConnect-XXXX hotspot with a captive setup page whenever no known
//   network is reachable, retrying the home network in the background;
// - mDNS at <hostname>.local once connected.

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <atomic>
#include <improv/improv.h>

class Network {
 public:
  // `deviceName` is shown to Improv clients; `hostname` feeds DHCP and mDNS.
  Network(const char* deviceName, const char* hostname);

  void begin();
  void loop();

 private:
  enum class Mode { Connecting, Connected, Hotspot };
  enum class Source { Saved, Improv, SetupPage };

  void startAttempt(const String& ssid, const String& password, Source source);
  void onConnected();
  void onAttemptFailed();
  void openHotspot();
  void closeHotspot();
  void startMdns();

  void handleImprov();
  void answerImprov(const improv::Request& request);
  void sendImprov(const std::vector<uint8_t>& packet);
  void sendImprovConnected();

  void setupRoutes();
  String statusJson() const;
  String networksJson();
  String deviceUrl() const;

  const char* deviceName_;
  const char* hostname_;
  String hotspotSsid_;

  Preferences prefs_;
  String savedSsid_;
  String savedPassword_;

  Mode mode_ = Mode::Hotspot;
  uint32_t modeSince_ = 0;
  uint32_t lostSince_ = 0;
  uint32_t nextRetryAt_ = 0;

  String attemptSsid_;
  String attemptPassword_;
  Source attemptSource_ = Source::Saved;
  bool lastAttemptFailed_ = false;

  // A setup-page request waiting for loop(), since web handlers must not block.
  std::atomic<bool> setupRequested_{false};
  String setupSsid_;
  String setupPassword_;

  bool hotspotActive_ = false;
  uint32_t hotspotCloseAt_ = 0;
  bool mdnsStarted_ = false;

  DNSServer dns_;
  AsyncWebServer server_{80};
  improv::Parser improv_;
};
