#pragma once

// The robot's log: every line goes to the serial port and into a small
// in-memory buffer for the Diagnostic report (GET /api/diagnostics).
// Safe from any task.

#include <Arduino.h>
#include <diagnostics/report.h>

#include <string>
#include <utility>
#include <vector>

// printf-style; the line must not end with "\n".
void logLine(const char* format, ...) __attribute__((format(printf, 1, 2)));

// Marks a personal value that may appear in log lines (a Wi-Fi network typed
// in the setup page…) so the redacted report hides it.
void notePersonal(const String& value, const char* label);

// Recent log lines and noted personal values, for the report.
std::string recentLog();
std::vector<std::pair<std::string, std::string>> notedPersonal();

// Why the chip last started, in plain words ("power-on", "crash (panic)"…).
const char* resetReason();
