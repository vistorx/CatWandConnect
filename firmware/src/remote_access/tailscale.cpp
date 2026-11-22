#include "tailscale.h"

#include <esp_log.h>
#include <microlink.h>

extern "C" {
// lib/microlink/extra/key_expiry.c
int64_t cwc_microlink_key_expiry(const microlink_t* ml);
bool cwc_microlink_key_expired(const microlink_t* ml);
}

using remote_access::Link;

namespace {

void wipe(std::string& text) {
  text.assign(text.size(), '\0');
  text.clear();
}

}  // namespace

bool TailscaleProvider::start(const std::string& key, const std::string& deviceName) {
  if (ml_ != nullptr) stop();
  key_ = key;
  deviceName_ = deviceName;

  microlink_config_t config = {};
  config.auth_key = key_.c_str();
  config.device_name = deviceName_.c_str();
  config.enable_derp = true;
  config.enable_stun = true;
  config.enable_disco = true;
  config.max_peers = 8;  // CONFIG_ML_MAX_PEERS, lib/microlink/config/microlink_kconfig.h
  config.advertise_routes = "";  // the camera's route comes with #271

  ml_ = microlink_init(&config);
  if (ml_ == nullptr) {
    wipe(key_);
    return false;
  }
  if (microlink_start(ml_) != ESP_OK) {
    microlink_destroy(ml_);
    ml_ = nullptr;
    wipe(key_);
    return false;
  }
  return true;
}

void TailscaleProvider::stop() {
  if (ml_ == nullptr) return;
  microlink_destroy(ml_);  // stops first; waits up to CONFIG_ML_STOP_TIMEOUT_MS
  ml_ = nullptr;
  wipe(key_);
}

// The device's own keys (machine, node, disco) and its cached peers: without
// them the robot joins as a new device next time.
void TailscaleProvider::forget() { microlink_factory_reset(); }

void TailscaleProvider::networkChanged() {
  if (ml_ != nullptr) microlink_rebind(ml_);
}

remote_access::ProviderStatus TailscaleProvider::status() const {
  remote_access::ProviderStatus s;
  if (ml_ == nullptr) return s;
  s.address = microlink_get_vpn_ip(ml_);
  s.keyExpiry = cwc_microlink_key_expiry(ml_);
  switch (microlink_get_state(ml_)) {
    case ML_STATE_CONNECTED: s.link = Link::Connected; break;
    case ML_STATE_AUTH_FAILED:
    case ML_STATE_NEEDS_LOGIN: s.link = Link::KeyRejected; break;
    case ML_STATE_ERROR: s.link = Link::Failed; break;
    case ML_STATE_IDLE:
    case ML_STATE_WIFI_WAIT:
    case ML_STATE_CONNECTING:
    case ML_STATE_REGISTERING:
    case ML_STATE_RECONNECTING: s.link = Link::Connecting; break;
  }
  if (cwc_microlink_key_expired(ml_)) s.link = Link::KeyRejected;
  // Direct when at least one device reaches the robot without the relay.
  int peers = microlink_get_peer_count(ml_);
  for (int i = 0; i < peers && !s.direct; ++i) {
    microlink_peer_info_t peer;
    if (microlink_get_peer_info(ml_, i, &peer) == ESP_OK && peer.direct_path) s.direct = true;
  }
  return s;
}
