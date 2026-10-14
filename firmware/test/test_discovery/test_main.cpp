#include <discovery/discovery.h>
#include <unity.h>

#include <cstring>

using namespace discovery;

void setUp() {}
void tearDown() {}

void test_api_version_starts_at_1() { TEST_ASSERT_EQUAL(1, kApiVersion); }

void test_service_is_catwandconnect_over_tcp_within_the_dns_sd_limit() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect", kServiceName);
  TEST_ASSERT_EQUAL_STRING("tcp", kServiceProtocol);
  // DNS-SD service names are at most 15 characters (RFC 6335).
  TEST_ASSERT_TRUE(std::strlen(kServiceName) <= 15);
}

void test_txt_records_carry_id_api_version_and_name() {
  std::vector<TxtRecord> txt = txtRecords("1a2b3c4d", "0.2.0-beta.1", "CatWandConnect");
  TEST_ASSERT_EQUAL(4, txt.size());
  TEST_ASSERT_EQUAL_STRING("id", txt[0].key.c_str());
  TEST_ASSERT_EQUAL_STRING("1a2b3c4d", txt[0].value.c_str());
  TEST_ASSERT_EQUAL_STRING("api", txt[1].key.c_str());
  TEST_ASSERT_EQUAL_STRING("1", txt[1].value.c_str());
  TEST_ASSERT_EQUAL_STRING("version", txt[2].key.c_str());
  TEST_ASSERT_EQUAL_STRING("0.2.0-beta.1", txt[2].value.c_str());
  TEST_ASSERT_EQUAL_STRING("name", txt[3].key.c_str());
  TEST_ASSERT_EQUAL_STRING("CatWandConnect", txt[3].value.c_str());
}

void test_txt_name_keeps_the_owners_utf8_name() {
  std::vector<TxtRecord> txt = txtRecords("1a2b3c4d", "0.2.0", "Salon d'été 🐱");
  TEST_ASSERT_EQUAL_STRING("Salon d'été 🐱", txt[3].value.c_str());
}

void test_every_txt_entry_fits_a_dns_txt_string() {
  // key=value must fit the 255-byte limit of one TXT string, even with the
  // longest name the robot accepts (32 bytes).
  for (const TxtRecord& r : txtRecords("ffffffff", "10.20.30-beta.40", std::string(32, 'a'))) {
    TEST_ASSERT_TRUE(r.key.size() + 1 + r.value.size() <= 255);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_api_version_starts_at_1);
  RUN_TEST(test_service_is_catwandconnect_over_tcp_within_the_dns_sd_limit);
  RUN_TEST(test_txt_records_carry_id_api_version_and_name);
  RUN_TEST(test_txt_name_keeps_the_owners_utf8_name);
  RUN_TEST(test_every_txt_entry_fits_a_dns_txt_string);
  return UNITY_END();
}
