#include "redactor.h"

#include <algorithm>
#include <cctype>

namespace diagnostics {

namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isHex(char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; }
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0; }

// Length of an IPv4 address starting at `i`, or 0. Four numbers of 0–255
// joined by dots, not part of a longer number or dotted sequence.
size_t ipv4At(const std::string& s, size_t i) {
  if (i > 0 && (isWordChar(s[i - 1]) || s[i - 1] == '.')) return 0;
  size_t p = i;
  for (int part = 0; part < 4; ++part) {
    if (part > 0) {
      if (p >= s.size() || s[p] != '.') return 0;
      ++p;
    }
    size_t start = p;
    unsigned value = 0;
    while (p < s.size() && isDigit(s[p]) && p - start < 3) value = value * 10 + unsigned(s[p++] - '0');
    if (p == start || value > 255) return 0;
  }
  if (p < s.size() && (isWordChar(s[p]) || (s[p] == '.' && p + 1 < s.size() && isDigit(s[p + 1])))) return 0;
  return p - i;
}

// Length of a MAC address (six hex pairs joined by ':' or '-') at `i`, or 0.
size_t macAt(const std::string& s, size_t i) {
  if (i > 0 && (isWordChar(s[i - 1]) || s[i - 1] == ':')) return 0;
  if (i + 17 > s.size()) return 0;
  char sep = s[i + 2];
  if (sep != ':' && sep != '-') return 0;
  for (size_t k = 0; k < 6; ++k) {
    size_t p = i + k * 3;
    if (!isHex(s[p]) || !isHex(s[p + 1])) return 0;
    if (k < 5 && s[p + 2] != sep) return 0;
  }
  if (i + 17 < s.size() && (isWordChar(s[i + 17]) || s[i + 17] == sep)) return 0;
  return 17;
}

}  // namespace

void Redactor::add(const std::string& value, const std::string& label) {
  if (value.empty()) return;
  secrets_.emplace_back(value, label);
  // Longest first, so a value inside a longer one never leaves part of it.
  std::stable_sort(secrets_.begin(), secrets_.end(),
                   [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
}

std::string Redactor::apply(const std::string& text) const {
  std::string out;
  out.reserve(text.size());
  size_t i = 0;
  while (i < text.size()) {
    bool replaced = false;
    for (const auto& [value, label] : secrets_) {
      // Whole words only, so a short name like "C" never eats "Chip". The
      // edge check applies only where the value itself starts or ends with
      // a letter or digit.
      size_t end = i + value.size();
      bool startsClean = !isWordChar(value.front()) || i == 0 || !isWordChar(text[i - 1]);
      bool endsClean = !isWordChar(value.back()) || end >= text.size() || !isWordChar(text[end]);
      if (startsClean && endsClean && text.compare(i, value.size(), value) == 0) {
        out += "<" + label + ">";
        i += value.size();
        replaced = true;
        break;
      }
    }
    if (replaced) continue;
    if (size_t n = macAt(text, i)) {
      out += "<mac>";
      i += n;
    } else if (size_t n = ipv4At(text, i)) {
      out += "<ip>";
      i += n;
    } else {
      out += text[i++];
    }
  }
  return out;
}

}  // namespace diagnostics
