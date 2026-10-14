#include <unity.h>
#include <wifi_fallback/wifi_fallback.h>

using namespace wifi_fallback;

void setUp() {}
void tearDown() {}

// A failed change made while the robot was on its network goes straight back
// to that network, so the owner finds the robot where it was (#174).
void test_failed_change_from_the_home_network_rejoins_the_previous_network() {
  TEST_ASSERT_EQUAL(Next::RejoinSaved, afterFailedAttempt(Source::SetupPage, true, false));
}

void test_failed_improv_attempt_while_on_a_saved_network_rejoins_it() {
  TEST_ASSERT_EQUAL(Next::RejoinSaved, afterFailedAttempt(Source::Improv, true, false));
}

// On the hotspot the owner is already there to try again: keep it, the
// hotspot still retries the saved network in the background.
void test_failed_attempt_made_from_the_hotspot_keeps_the_hotspot() {
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::SetupPage, true, true));
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::Improv, true, true));
}

void test_without_a_saved_network_a_failure_opens_the_hotspot() {
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::SetupPage, false, false));
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::Improv, false, false));
}

// The saved network itself failing never loops on itself: the hotspot opens.
void test_failed_saved_network_opens_the_hotspot() {
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::Saved, true, false));
  TEST_ASSERT_EQUAL(Next::OpenHotspot, afterFailedAttempt(Source::Saved, true, true));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_failed_change_from_the_home_network_rejoins_the_previous_network);
  RUN_TEST(test_failed_improv_attempt_while_on_a_saved_network_rejoins_it);
  RUN_TEST(test_failed_attempt_made_from_the_hotspot_keeps_the_hotspot);
  RUN_TEST(test_without_a_saved_network_a_failure_opens_the_hotspot);
  RUN_TEST(test_failed_saved_network_opens_the_hotspot);
  return UNITY_END();
}
