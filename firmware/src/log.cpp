#include "log.h"

#include <diagnostics/log_buffer.h>
#include <esp_system.h>

#include <algorithm>
#include <cstdarg>
#include <mutex>

namespace {

constexpr size_t kMaxLines = 200;
constexpr size_t kMaxBytes = 12 * 1024;
constexpr size_t kMaxNoted = 8;

std::mutex lock;
diagnostics::LogBuffer buffer(kMaxLines, kMaxBytes);
std::vector<std::pair<std::string, std::string>> noted;

}  // namespace

void logLine(const char* format, ...) {
  char text[192];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof text, format, args);
  va_end(args);
  Serial.println(text);
  std::lock_guard<std::mutex> guard(lock);
  buffer.add(millis(), text);
}

void notePersonal(const String& value, const char* label) {
  if (value.isEmpty()) return;
  std::lock_guard<std::mutex> guard(lock);
  std::pair<std::string, std::string> entry{value.c_str(), label};
  if (std::find(noted.begin(), noted.end(), entry) != noted.end()) return;
  if (noted.size() == kMaxNoted) noted.erase(noted.begin());
  noted.push_back(entry);
}

std::string recentLog() {
  std::lock_guard<std::mutex> guard(lock);
  return buffer.text();
}

std::vector<std::pair<std::string, std::string>> notedPersonal() {
  std::lock_guard<std::mutex> guard(lock);
  return noted;
}

const char* resetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "reset pin";
    case ESP_RST_SW: return "software restart";
    case ESP_RST_PANIC: return "crash (panic)";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "wake from deep sleep";
    case ESP_RST_BROWNOUT: return "brownout (power supply too weak)";
    case ESP_RST_SDIO: return "SDIO";
    default: return "unknown";
  }
}
