#include <identity/identity.h>
#include <unity.h>

using namespace identity;

void setUp() {}
void tearDown() {}

void test_default_name_gives_the_documented_hostname() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect", hostnameFor(kDefaultName).c_str());
}

void test_hostname_lowercases_and_hyphenates() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect-salon", hostnameFor("CatWandConnect Salon").c_str());
  TEST_ASSERT_EQUAL_STRING("robot-2", hostnameFor("  Robot -- #2!  ").c_str());
}

void test_hostname_transliterates_french_accents() {
  TEST_ASSERT_EQUAL_STRING("salon-d-ete", hostnameFor("Salon d'été").c_str());
  TEST_ASSERT_EQUAL_STRING("chat-ca", hostnameFor("Chat ça").c_str());
}

void test_hostname_is_capped_and_never_ends_with_a_hyphen() {
  std::string host = hostnameFor("abcdefghijklmnopqrstuvwxyz abcdefghij");
  TEST_ASSERT_TRUE(host.size() <= kMaxHostnameLength);
  TEST_ASSERT_NOT_EQUAL('-', host.back());
}

void test_unusable_names_fall_back_and_are_invalid() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect", hostnameFor("🐱🐱").c_str());
  TEST_ASSERT_FALSE(validName("🐱🐱"));
  TEST_ASSERT_FALSE(validName(""));
  TEST_ASSERT_FALSE(validName("   "));
  TEST_ASSERT_FALSE(validName("bad\nname"));
  TEST_ASSERT_FALSE(validName(std::string(33, 'a')));
}

void test_ordinary_names_are_valid() {
  TEST_ASSERT_TRUE(validName("CatWandConnect"));
  TEST_ASSERT_TRUE(validName("Salon d'été 🐱"));
}

void test_hotspot_name_appends_the_suffix_within_32_bytes() {
  TEST_ASSERT_EQUAL_STRING("CatWandConnect-A1B2", hotspotName(kDefaultName, "A1B2").c_str());
  std::string longName = hotspotName("Chambre des enfants à l'étage du fond", "A1B2");
  TEST_ASSERT_TRUE(longName.size() <= 32);
  TEST_ASSERT_EQUAL_STRING("-A1B2", longName.substr(longName.size() - 5).c_str());
}

void test_hotspot_name_never_cuts_a_utf8_character() {
  // 26 bytes of room: "éééééééééééééé" is 28 bytes, so it must stop after 13 é.
  std::string name = hotspotName("ééééééééééééééé", "A1B2");
  TEST_ASSERT_EQUAL(13 * 2 + 5, name.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_default_name_gives_the_documented_hostname);
  RUN_TEST(test_hostname_lowercases_and_hyphenates);
  RUN_TEST(test_hostname_transliterates_french_accents);
  RUN_TEST(test_hostname_is_capped_and_never_ends_with_a_hyphen);
  RUN_TEST(test_unusable_names_fall_back_and_are_invalid);
  RUN_TEST(test_ordinary_names_are_valid);
  RUN_TEST(test_hotspot_name_appends_the_suffix_within_32_bytes);
  RUN_TEST(test_hotspot_name_never_cuts_a_utf8_character);
  return UNITY_END();
}
