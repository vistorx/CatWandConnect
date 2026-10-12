#include "log_buffer.h"

#include <cstdio>

namespace diagnostics {

LogBuffer::LogBuffer(size_t maxLines, size_t maxBytes) : maxLines_(maxLines), maxBytes_(maxBytes) {}

void LogBuffer::add(uint32_t uptimeMs, const std::string& line) {
  char stamp[24];
  std::snprintf(stamp, sizeof stamp, "[%5u.%03u] ", unsigned(uptimeMs / 1000), unsigned(uptimeMs % 1000));
  lines_.push_back(stamp + line);
  bytes_ += lines_.back().size() + 1;
  while (!lines_.empty() && (lines_.size() > maxLines_ || bytes_ > maxBytes_)) {
    bytes_ -= lines_.front().size() + 1;
    lines_.pop_front();
  }
}

std::string LogBuffer::text() const {
  std::string out;
  out.reserve(bytes_);
  for (const std::string& line : lines_) {
    out += line;
    out += '\n';
  }
  return out;
}

}  // namespace diagnostics
