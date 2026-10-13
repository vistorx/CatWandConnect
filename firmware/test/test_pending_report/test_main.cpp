#include <diagnostics/pending.h>
#include <unity.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace diagnostics;

namespace {

class MemoryStore : public BlobStore {
 public:
  bool read(const char* key, void* data, size_t size) override {
    auto it = blobs.find(key);
    if (it == blobs.end() || it->second.size() != size) return false;
    std::memcpy(data, it->second.data(), size);
    return true;
  }
  bool write(const char* key, const void* data, size_t size) override {
    auto bytes = static_cast<const uint8_t*>(data);
    blobs[key].assign(bytes, bytes + size);
    return true;
  }
  std::map<std::string, std::vector<uint8_t>> blobs;
};

std::string reportWithLog(int lines) {
  std::string text = "CatWandConnect diagnostic report\nRedacted: yes\n\nFirmware: 0.1.0\n\nRecent log:\n";
  for (int i = 0; i < lines; ++i) text += "[    " + std::to_string(i) + ".000] line " + std::to_string(i) + "\n";
  return text;
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

void setUp() {}
void tearDown() {}

void test_nothing_is_pending_at_first() {
  MemoryStore store;
  PendingReport out;
  TEST_ASSERT_FALSE(loadPending(store, out));
}

void test_a_saved_report_survives_with_its_description() {
  MemoryStore store;
  TEST_ASSERT_TRUE(savePending(store, "The wand jumps at start", reportWithLog(3)));

  MemoryStore copy = store;  // as after a restart
  PendingReport out;
  TEST_ASSERT_TRUE(loadPending(copy, out));
  TEST_ASSERT_EQUAL_STRING("The wand jumps at start", out.description.c_str());
  TEST_ASSERT_EQUAL_STRING(reportWithLog(3).c_str(), out.report.c_str());
}

void test_a_long_report_keeps_the_summary_and_the_most_recent_log_lines() {
  MemoryStore store;
  TEST_ASSERT_TRUE(savePending(store, "Problem", reportWithLog(500)));

  PendingReport out;
  TEST_ASSERT_TRUE(loadPending(store, out));
  TEST_ASSERT_TRUE(out.description.size() + out.report.size() <= kPendingBytes);
  TEST_ASSERT_TRUE(contains(out.report, "Firmware: 0.1.0"));
  TEST_ASSERT_TRUE(contains(out.report, "line 499\n"));  // newest kept
  TEST_ASSERT_FALSE(contains(out.report, "] line 0\n"));  // oldest dropped
  TEST_ASSERT_TRUE(contains(out.report, "older log lines left out"));
}

void test_a_long_description_is_cut() {
  MemoryStore store;
  TEST_ASSERT_TRUE(savePending(store, std::string(5000, 'x'), reportWithLog(1)));
  PendingReport out;
  TEST_ASSERT_TRUE(loadPending(store, out));
  TEST_ASSERT_EQUAL(kMaxDescription, out.description.size());
}

void test_discarding_leaves_nothing_pending() {
  MemoryStore store;
  savePending(store, "Problem", reportWithLog(2));
  clearPending(store);
  PendingReport out;
  TEST_ASSERT_FALSE(loadPending(store, out));
}

void test_a_new_report_replaces_the_previous_one() {
  MemoryStore store;
  savePending(store, "First", reportWithLog(2));
  savePending(store, "Second", reportWithLog(1));
  PendingReport out;
  TEST_ASSERT_TRUE(loadPending(store, out));
  TEST_ASSERT_EQUAL_STRING("Second", out.description.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nothing_is_pending_at_first);
  RUN_TEST(test_a_saved_report_survives_with_its_description);
  RUN_TEST(test_a_long_report_keeps_the_summary_and_the_most_recent_log_lines);
  RUN_TEST(test_a_long_description_is_cut);
  RUN_TEST(test_discarding_leaves_nothing_pending);
  RUN_TEST(test_a_new_report_replaces_the_previous_one);
  return UNITY_END();
}
