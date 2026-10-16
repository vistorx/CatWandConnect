#include <identity/identity.h>
#include <unity.h>

#include "../fakes.h"

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

// --- Hostname with the Device ID (#195) --------------------------------------

constexpr const char* kId = "5894c0de";

void test_default_name_gets_a_suffix_from_the_device_id() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect-5894", hostnameFor(kDefaultName, kId, false).c_str());
}

void test_a_name_giving_the_default_hostname_also_gets_the_suffix() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect-5894", hostnameFor("catwandconnect", kId, false).c_str());
  TEST_ASSERT_EQUAL_STRING("catwandconnect-5894", hostnameFor(" CATWANDCONNECT ", kId, false).c_str());
}

void test_a_chosen_name_keeps_its_own_hostname() {
  TEST_ASSERT_EQUAL_STRING("salon-d-ete", hostnameFor("Salon d'été", kId, false).c_str());
  TEST_ASSERT_EQUAL_STRING("salon-d-ete", hostnameFor("Salon d'été", kId, true).c_str());
  TEST_ASSERT_EQUAL_STRING("catwandconnect-salon", hostnameFor("CatWandConnect Salon", kId, false).c_str());
}

void test_a_legacy_robot_with_the_default_name_keeps_catwandconnect() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect", hostnameFor(kDefaultName, kId, true).c_str());
}

void test_suffix_is_lowercase_hex() {
  TEST_ASSERT_EQUAL_STRING("catwandconnect-a1b2", hostnameFor(kDefaultName, "A1B2C3D4", false).c_str());
}

// --- Which robots keep catwandconnect.local -----------------------------------

void test_a_fresh_install_is_not_legacy_and_remembers_it() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_FALSE(legacyHostname(storage, false, kDefaultName));
  // Wi-Fi set up afterwards: still the suffixed hostname.
  TEST_ASSERT_FALSE(legacyHostname(storage, true, kDefaultName));
}

void test_a_robot_already_on_wifi_with_the_default_name_is_legacy() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_TRUE(legacyHostname(storage, true, kDefaultName));
  TEST_ASSERT_EQUAL(1, storage.writes);
  TEST_ASSERT_TRUE(legacyHostname(storage, true, kDefaultName));
  TEST_ASSERT_EQUAL(1, storage.writes);  // decided once
}

void test_a_robot_already_on_wifi_with_its_own_name_is_not_legacy() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_FALSE(legacyHostname(storage, true, "Minou"));
  // Renamed to the default later: it never answered at catwandconnect.local.
  TEST_ASSERT_FALSE(legacyHostname(storage, true, kDefaultName));
}

void test_a_legacy_robot_renamed_and_back_gets_catwandconnect_again() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_TRUE(legacyHostname(storage, true, kDefaultName));
  bool legacy = legacyHostname(storage, true, "Minou");
  TEST_ASSERT_EQUAL_STRING("minou", hostnameFor("Minou", kId, legacy).c_str());
  legacy = legacyHostname(storage, true, kDefaultName);
  TEST_ASSERT_EQUAL_STRING("catwandconnect", hostnameFor(kDefaultName, kId, legacy).c_str());
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
  RUN_TEST(test_default_name_gets_a_suffix_from_the_device_id);
  RUN_TEST(test_a_name_giving_the_default_hostname_also_gets_the_suffix);
  RUN_TEST(test_a_chosen_name_keeps_its_own_hostname);
  RUN_TEST(test_a_legacy_robot_with_the_default_name_keeps_catwandconnect);
  RUN_TEST(test_suffix_is_lowercase_hex);
  RUN_TEST(test_a_fresh_install_is_not_legacy_and_remembers_it);
  RUN_TEST(test_a_robot_already_on_wifi_with_the_default_name_is_legacy);
  RUN_TEST(test_a_robot_already_on_wifi_with_its_own_name_is_not_legacy);
  RUN_TEST(test_a_legacy_robot_renamed_and_back_gets_catwandconnect_again);
  return UNITY_END();
}
