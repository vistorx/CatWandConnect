#pragma once

// The robot's most recent log lines, kept in memory for the Diagnostic report.

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace diagnostics {

class LogBuffer {
 public:
  LogBuffer(size_t maxLines, size_t maxBytes);

  // Adds one line stamped with the uptime it was logged at.
  void add(uint32_t uptimeMs, const std::string& line);

  // The kept lines, oldest first, one per line.
  std::string text() const;

 private:
  size_t maxLines_;
  size_t maxBytes_;
  size_t bytes_ = 0;
  std::deque<std::string> lines_;
};

}  // namespace diagnostics
