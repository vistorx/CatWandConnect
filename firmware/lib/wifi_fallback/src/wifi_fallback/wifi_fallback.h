#pragma once

// What the robot does after a Wi-Fi attempt fails, so a wrong network or
// password never leaves it unreachable (docs/adr/0011). Pure: the network
// glue in src/network/ applies the answer.

namespace wifi_fallback {

// Where the credentials of the attempt came from.
enum class Source { Saved, Improv, SetupPage };

enum class Next {
  RejoinSaved,  // try the saved network again at once
  OpenHotspot,  // open (or keep) the CatWandConnect-XXXX hotspot
};

// `hasSaved`: a network is saved (only a successful attempt replaces it).
// `fromHotspot`: the hotspot was up when the attempt started.
Next afterFailedAttempt(Source source, bool hasSaved, bool fromHotspot);

}  // namespace wifi_fallback
