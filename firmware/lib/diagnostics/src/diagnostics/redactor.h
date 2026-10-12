#pragma once

// Removes personal data from text sent to support: the values it is told
// about (Wi-Fi name, broker address, user names…), and every IPv4 and MAC
// address it finds, wherever they appear, log lines included.

#include <string>
#include <utility>
#include <vector>

namespace diagnostics {

class Redactor {
 public:
  // Replaces `value` with "<label>". Empty values are ignored.
  void add(const std::string& value, const std::string& label);

  std::string apply(const std::string& text) const;

 private:
  std::vector<std::pair<std::string, std::string>> secrets_;
};

}  // namespace diagnostics
