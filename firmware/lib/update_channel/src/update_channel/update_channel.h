#pragma once

// The robot's Update channel and firmware version comparison (docs/adr/0012).
// Pure: the Device API stores the channel through the Storage port.

#include <robot_core/ports.h>

#include <string>

namespace update_channel {

enum class Channel { Stable, Beta };

const char* channelName(Channel channel);  // "stable" / "beta"
bool parseChannel(const std::string& name, Channel& channel);

// The channel of a firmware version: a "-beta" pre-release is Beta, anything
// else Stable.
Channel channelOf(const std::string& version);

// The saved Update channel, or the running firmware's when never set.
Channel loadChannel(cwc::Storage& storage, const std::string& running);
bool saveChannel(cwc::Storage& storage, Channel channel);

// Semantic versions with pre-release, a leading "v" stripped and build
// metadata ("+abc") or anything after a space ignored:
// 0.2.0-beta.3 < 0.2.0-beta.10 < 0.2.0. Negative, zero or positive.
int compareVersions(const std::string& a, const std::string& b);

// Whether `candidate` is strictly newer than `running`.
bool isNewer(const std::string& candidate, const std::string& running);

}  // namespace update_channel
