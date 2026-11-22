#pragma once

// The Tailscale client (microlink, lib/microlink) as a Remote access
// provider. Every call comes from the remote access task (remote_access.cpp):
// microlink is not called from anywhere else.

#include <remote_access/remote_access.h>

#include <string>

struct microlink_s;

class TailscaleProvider : public remote_access::Provider {
 public:
  const char* id() const override { return "tailscale"; }
  bool available() const override { return true; }
  bool routes() const override { return true; }
  bool start(const std::string& key, const std::string& deviceName) override;
  void stop() override;
  void forget() override;
  void networkChanged() override;
  remote_access::ProviderStatus status() const override;

 private:
  microlink_s* ml_ = nullptr;
  // microlink keeps pointers to these until it is destroyed.
  std::string key_;
  std::string deviceName_;
};
