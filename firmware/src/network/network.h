#pragma once

// Gets the robot onto the owner's Wi-Fi and keeps it reachable:
// - credentials from Improv over serial (web flasher) or from the setup page,
//   from the hotspot or, to change network, from the current Wi-Fi;
// - a failed change made from the current Wi-Fi rejoins that network
//   (lib/wifi_fallback, ADR 0011);
// - a CatWandConnect-XXXX hotspot with a captive setup page whenever no known
//   network is reachable, retrying the home network in the background;
// - mDNS at <hostname>.local once connected.

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <atomic>
#include <auth/pin_guard.h>
#include <improv/improv.h>
#include <wifi_fallback/wifi_fallback.h>

class Network {
 public:
  // Registers the Wi-Fi API and captive-portal routes on `server`.
  // `deviceName` names the hotspot and is shown to Improv clients;
  // `hostname` feeds DHCP and mDNS.
  // `pin` protects changing the Wi-Fi from the setup page.
  void begin(AsyncWebServer& server, const String& deviceName, const String& hostname, auth::PinGuard& pin);
  void loop();

  // True until the robot is on the owner's Wi-Fi: "/" should show the setup page.
  bool inSetup() const { return mode_ != Mode::Connected; }

  // The saved home network's name, empty when none is saved.
  String savedNetwork() const { return savedSsid_; }

 private:
  enum class Mode { Connecting, Connected, Hotspot };
  using Source = wifi_fallback::Source;

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

  void setupRoutes(AsyncWebServer& server);
  String statusJson() const;
  String networksJson();
  String deviceUrl() const;

  auth::PinGuard* pin_ = nullptr;
  String deviceName_;
  String hostname_;
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
  // The network a failed change tried, while the robot rejoins the saved one.
  String failedSsid_;

  // A setup-page request waiting for loop(), since web handlers must not block.
  std::atomic<bool> setupRequested_{false};
  // When to start it: a little later off the hotspot, so the 202 reaches the
  // phone before the radio leaves the current network.
  uint32_t setupAt_ = 0;
  String setupSsid_;
  String setupPassword_;

  bool hotspotActive_ = false;
  uint32_t hotspotCloseAt_ = 0;
  bool mdnsStarted_ = false;

  DNSServer dns_;
  improv::Parser improv_;
};
