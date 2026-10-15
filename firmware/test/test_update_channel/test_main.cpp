#include <unity.h>
#include <update_channel/update_channel.h>

#include "../fakes.h"

using namespace update_channel;

void setUp() {}
void tearDown() {}

void test_pre_releases_come_before_their_release() {
  TEST_ASSERT_LESS_THAN(0, compareVersions("0.2.0-beta.3", "0.2.0"));
  TEST_ASSERT_GREATER_THAN(0, compareVersions("0.2.0", "0.2.0-beta.3"));
}

void test_pre_release_numbers_compare_as_numbers() {
  TEST_ASSERT_LESS_THAN(0, compareVersions("0.2.0-beta.3", "0.2.0-beta.10"));
  TEST_ASSERT_LESS_THAN(0, compareVersions("0.2.0-beta.10", "0.2.0"));
}

void test_core_numbers_compare_as_numbers() {
  TEST_ASSERT_LESS_THAN(0, compareVersions("0.9.0", "0.10.0"));
  TEST_ASSERT_LESS_THAN(0, compareVersions("0.2.9", "1.0.0-beta.1"));
  TEST_ASSERT_GREATER_THAN(0, compareVersions("0.2.1-beta.1", "0.2.0"));
}

void test_leading_v_and_build_metadata_are_ignored() {
  TEST_ASSERT_EQUAL(0, compareVersions("v0.2.0", "0.2.0"));
  TEST_ASSERT_EQUAL(0, compareVersions("0.2.0-beta.4+abc1234", "0.2.0-beta.4"));
  TEST_ASSERT_EQUAL(0, compareVersions("0.0.0-dev (simulator)", "0.0.0-dev"));
}

void test_a_dev_build_is_older_than_any_release() {
  TEST_ASSERT_TRUE(isNewer("0.1.0-beta.1", "0.0.0-dev+abc1234"));
}

void test_numeric_identifiers_come_before_alphanumeric_ones() {
  TEST_ASSERT_LESS_THAN(0, compareVersions("1.0.0-1", "1.0.0-beta"));
  TEST_ASSERT_LESS_THAN(0, compareVersions("1.0.0-beta", "1.0.0-beta.1"));
  TEST_ASSERT_LESS_THAN(0, compareVersions("1.0.0-alpha", "1.0.0-beta"));
}

void test_only_a_strictly_newer_version_is_an_update() {
  TEST_ASSERT_TRUE(isNewer("0.2.0-beta.4", "0.2.0-beta.3"));
  TEST_ASSERT_FALSE(isNewer("0.2.0-beta.3", "0.2.0-beta.3"));
  // A beta robot on the Stable channel is not offered an older stable.
  TEST_ASSERT_FALSE(isNewer("0.1.0", "0.2.0-beta.3"));
}

void test_unreadable_versions_are_never_an_update() {
  TEST_ASSERT_FALSE(isNewer("", "0.2.0"));
  TEST_ASSERT_FALSE(isNewer("latest", "0.2.0"));
  TEST_ASSERT_FALSE(isNewer("1.x", "0.2.0"));
}

void test_the_running_firmware_gives_the_default_channel() {
  TEST_ASSERT_EQUAL(Channel::Beta, channelOf("0.2.0-beta.3"));
  TEST_ASSERT_EQUAL(Channel::Stable, channelOf("0.2.0"));
  TEST_ASSERT_EQUAL(Channel::Stable, channelOf("0.0.0-dev+abc1234"));
}

void test_channel_names() {
  Channel channel = Channel::Stable;
  TEST_ASSERT_TRUE(parseChannel("beta", channel));
  TEST_ASSERT_EQUAL(Channel::Beta, channel);
  TEST_ASSERT_TRUE(parseChannel("stable", channel));
  TEST_ASSERT_EQUAL(Channel::Stable, channel);
  TEST_ASSERT_FALSE(parseChannel("nightly", channel));
  TEST_ASSERT_EQUAL_STRING("beta", channelName(Channel::Beta));
  TEST_ASSERT_EQUAL_STRING("stable", channelName(Channel::Stable));
}

void test_never_set_follows_the_running_firmware() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_EQUAL(Channel::Beta, loadChannel(storage, "0.2.0-beta.3"));
  TEST_ASSERT_EQUAL(Channel::Stable, loadChannel(storage, "0.2.0"));
}

// Kept across restarts and updates: a beta robot whose owner chose Stable
// stays on Stable after installing the next beta, and the other way round.
void test_a_chosen_channel_is_kept_whatever_the_firmware() {
  cwc::test::FakeStorage storage;
  TEST_ASSERT_TRUE(saveChannel(storage, Channel::Stable));
  TEST_ASSERT_EQUAL(Channel::Stable, loadChannel(storage, "0.2.0-beta.4"));
  TEST_ASSERT_TRUE(saveChannel(storage, Channel::Beta));
  TEST_ASSERT_EQUAL(Channel::Beta, loadChannel(storage, "0.2.0"));
}

void test_an_unreadable_record_falls_back_to_the_running_firmware() {
  cwc::test::FakeStorage storage;
  storage.blobs["channel"] = {0x7f};
  TEST_ASSERT_EQUAL(Channel::Beta, loadChannel(storage, "0.2.0-beta.3"));
  storage.blobs["channel"] = {1, 2, 3};
  TEST_ASSERT_EQUAL(Channel::Stable, loadChannel(storage, "0.2.0"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pre_releases_come_before_their_release);
  RUN_TEST(test_pre_release_numbers_compare_as_numbers);
  RUN_TEST(test_core_numbers_compare_as_numbers);
  RUN_TEST(test_leading_v_and_build_metadata_are_ignored);
  RUN_TEST(test_a_dev_build_is_older_than_any_release);
  RUN_TEST(test_numeric_identifiers_come_before_alphanumeric_ones);
  RUN_TEST(test_only_a_strictly_newer_version_is_an_update);
  RUN_TEST(test_unreadable_versions_are_never_an_update);
  RUN_TEST(test_the_running_firmware_gives_the_default_channel);
  RUN_TEST(test_channel_names);
  RUN_TEST(test_never_set_follows_the_running_firmware);
  RUN_TEST(test_a_chosen_channel_is_kept_whatever_the_firmware);
  RUN_TEST(test_an_unreadable_record_falls_back_to_the_running_firmware);
  return UNITY_END();
}
