#include "wifi_fallback/wifi_fallback.h"

namespace wifi_fallback {

Next afterFailedAttempt(Source source, bool hasSaved, bool fromHotspot) {
  if (source == Source::Saved || !hasSaved || fromHotspot) return Next::OpenHotspot;
  return Next::RejoinSaved;
}

}  // namespace wifi_fallback
