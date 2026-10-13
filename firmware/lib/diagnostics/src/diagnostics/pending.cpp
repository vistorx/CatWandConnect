#include "diagnostics/pending.h"

#include <cstring>

namespace diagnostics {

namespace {

constexpr const char* kKey = "pendingRep";
constexpr uint32_t kMagic = 0x50524550;  // "PREP"
constexpr const char* kLogHeader = "\nRecent log:\n";

// Fixed size, so NVS reads check it like the other records.
struct Record {
  uint32_t magic;
  uint16_t descriptionLength;
  uint16_t reportLength;
  char text[kPendingBytes];  // description then report, not terminated
};

// Drops the oldest log lines until the report fits in `room` bytes.
std::string shorten(const std::string& report, size_t room) {
  if (report.size() <= room) return report;
  size_t at = report.find(kLogHeader);
  if (at == std::string::npos) return report.substr(0, room);
  std::string head = report.substr(0, at) + kLogHeader + "(older log lines left out)\n";
  if (head.size() >= room) return report.substr(0, room);
  std::string log = report.substr(at + std::strlen(kLogHeader));
  size_t keep = room - head.size();
  size_t start = log.size() - keep;
  size_t line = log.find('\n', start == 0 ? 0 : start - 1);  // start on a whole line
  start = line == std::string::npos ? log.size() : line + 1;
  return head + log.substr(start);
}

}  // namespace

bool savePending(BlobStore& store, const std::string& description, const std::string& report) {
  std::string kept = description.substr(0, kMaxDescription);
  std::string body = shorten(report, kPendingBytes - kept.size());
  Record* record = new Record{};
  record->magic = kMagic;
  record->descriptionLength = static_cast<uint16_t>(kept.size());
  record->reportLength = static_cast<uint16_t>(body.size());
  std::memcpy(record->text, kept.data(), kept.size());
  std::memcpy(record->text + kept.size(), body.data(), body.size());
  bool ok = store.write(kKey, record, sizeof(Record));
  delete record;
  return ok;
}

bool loadPending(BlobStore& store, PendingReport& out) {
  Record* record = new Record{};
  bool ok = store.read(kKey, record, sizeof(Record)) && record->magic == kMagic &&
            size_t(record->descriptionLength) + record->reportLength <= kPendingBytes;
  if (ok) {
    out.description.assign(record->text, record->descriptionLength);
    out.report.assign(record->text + record->descriptionLength, record->reportLength);
  }
  delete record;
  return ok;
}

void clearPending(BlobStore& store) {
  uint32_t none = 0;
  // A record of another size reads as missing.
  store.write(kKey, &none, sizeof(none));
}

}  // namespace diagnostics
