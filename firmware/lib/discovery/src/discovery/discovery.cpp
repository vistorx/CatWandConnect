#include "discovery/discovery.h"

namespace discovery {

std::vector<TxtRecord> txtRecords(const std::string& deviceId, const std::string& version, const std::string& name) {
  return {
      {"id", deviceId},
      {"api", std::to_string(kApiVersion)},
      {"version", version},
      {"name", name},
  };
}

}  // namespace discovery
