#pragma once

// The robot's name, as the owner types it ("CatWandConnect salon"), and the
// names derived from it: the mDNS hostname and the hotspot SSID.

#include <cstddef>
#include <string>

namespace identity {

constexpr const char* kDefaultName = "CatWandConnect";
constexpr size_t kMaxNameBytes = 32;
constexpr size_t kMaxHostnameLength = 32;

// 1 to 32 bytes of UTF-8 without control characters, with at least one
// letter or digit, so a hostname can be derived from it.
bool validName(const std::string& name);

// Lowercase ASCII letters, digits and single hyphens: "Salon d'été" gives
// "salon-d-ete". Falls back to "catwandconnect" when nothing usable remains.
std::string hostnameFor(const std::string& name);

// "<name>-<suffix>", with the name shortened to fit an SSID's 32 bytes
// without cutting a UTF-8 character in half.
std::string hotspotName(const std::string& name, const std::string& suffix);

}  // namespace identity
