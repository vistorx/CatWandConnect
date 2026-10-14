#pragma once

// How the robot makes itself known to the Native integration (docs/api.md):
// the API version, and the mDNS service `_catwandconnect._tcp` with its TXT
// records. Pure: the firmware registers the records with ESPmDNS.

#include <string>
#include <vector>

namespace discovery {

// The version of the documented API contract. Bump it only on a change that
// would break an existing integration; additive changes keep it.
constexpr int kApiVersion = 1;

// Advertised as `_catwandconnect._tcp` on port 80, beside `_http._tcp`.
constexpr const char* kServiceName = "catwandconnect";
constexpr const char* kServiceProtocol = "tcp";

struct TxtRecord {
  std::string key;
  std::string value;
};

// id (Device ID), api (API version), version (firmware version), name (the
// robot's name, as the owner typed it).
std::vector<TxtRecord> txtRecords(const std::string& deviceId, const std::string& version, const std::string& name);

}  // namespace discovery
