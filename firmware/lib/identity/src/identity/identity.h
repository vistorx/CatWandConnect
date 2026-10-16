#pragma once

// The robot's name, as the owner types it ("CatWandConnect salon"), and the
// names derived from it: the mDNS hostname and the hotspot SSID.

#include <robot_core/ports.h>

#include <cstddef>
#include <string>

namespace identity {

constexpr const char* kDefaultName = "CatWandConnect";
constexpr const char* kDefaultHostname = "catwandconnect";
constexpr size_t kMaxNameBytes = 32;
constexpr size_t kMaxHostnameLength = 32;

// 1 to 32 bytes of UTF-8 without control characters, with at least one
// letter or digit, so a hostname can be derived from it.
bool validName(const std::string& name);

// Lowercase ASCII letters, digits and single hyphens: "Salon d'été" gives
// "salon-d-ete". Falls back to "catwandconnect" when nothing usable remains.
std::string hostnameFor(const std::string& name);

// The robot's actual hostname (#195). A name of its own gives hostnameFor(name).
// The default name ("catwandconnect") gets the first 4 hex digits of the
// Device ID, lowercased, so two default-named robots don't clash on .local:
// "catwandconnect-5894". `legacy` (see legacyHostname()) keeps plain
// "catwandconnect" for robots that already answered there.
std::string hostnameFor(const std::string& name, const std::string& deviceId, bool legacy);

// Whether this robot keeps the plain "catwandconnect" hostname: decided once,
// at the first boot of a firmware with the suffix, and kept in `storage`.
// True when the robot already had Wi-Fi credentials then (installed before
// the suffix existed) and its name gave "catwandconnect"; false otherwise,
// fresh installs included. Never changes afterwards, renames included.
bool legacyHostname(cwc::Storage& storage, bool hasWifiCredentials, const std::string& name);

// "<name>-<suffix>", with the name shortened to fit an SSID's 32 bytes
// without cutting a UTF-8 character in half.
std::string hotspotName(const std::string& name, const std::string& suffix);

}  // namespace identity
