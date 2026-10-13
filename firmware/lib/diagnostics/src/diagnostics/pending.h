#pragma once

// A Pending report (CONTEXT.md): a problem described while the phone had no
// internet (on the setup hotspot), kept in flash until the owner sends or
// discards it once the robot is back on their network. Never sent by itself.

#include <cstddef>
#include <cstdint>
#include <string>

namespace diagnostics {

// Where the report is kept: NVS on the robot, a map in tests.
class BlobStore {
 public:
  virtual ~BlobStore() = default;
  // Returns false when the key is missing or its size differs from `size`.
  virtual bool read(const char* key, void* data, size_t size) = 0;
  virtual bool write(const char* key, const void* data, size_t size) = 0;
};

// Room for the description and the report together, in flash.
constexpr size_t kPendingBytes = 3000;  // stays inside one NVS blob on every chip
constexpr size_t kMaxDescription = 600;

struct PendingReport {
  std::string description;
  std::string report;  // redacted Diagnostic report, possibly shortened
};

// Saves the report, replacing any previous one. A report too long for flash
// keeps its summary and its most recent log lines.
bool savePending(BlobStore& store, const std::string& description, const std::string& report);

bool loadPending(BlobStore& store, PendingReport& out);

void clearPending(BlobStore& store);

}  // namespace diagnostics
