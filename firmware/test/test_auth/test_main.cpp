#include <auth/pin_guard.h>
#include <unity.h>

using auth::PinGuard;

namespace {

uint32_t counter = 1;
uint32_t fakeRandom() { return counter++ * 2654435761u; }

}  // namespace

void setUp() {}
void tearDown() {}

void test_disabled_by_default_and_everything_is_allowed() {
  PinGuard guard(fakeRandom);
  TEST_ASSERT_FALSE(guard.enabled());
  TEST_ASSERT_TRUE(guard.authorized("", 0));
}

void test_pin_rules() {
  TEST_ASSERT_TRUE(PinGuard::validPin("1234"));
  TEST_ASSERT_TRUE(PinGuard::validPin("12345678"));
  TEST_ASSERT_FALSE(PinGuard::validPin("123"));
  TEST_ASSERT_FALSE(PinGuard::validPin("123456789"));
  TEST_ASSERT_FALSE(PinGuard::validPin("12a4"));

  PinGuard guard(fakeRandom);
  TEST_ASSERT_FALSE(guard.setPin("12"));
  TEST_ASSERT_FALSE(guard.enabled());
}

void test_enabled_pin_requires_a_token() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");

  TEST_ASSERT_FALSE(guard.authorized("", 0));
  TEST_ASSERT_FALSE(guard.authorized("made-up", 0));
}

void test_right_pin_gives_a_working_token() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;

  TEST_ASSERT_TRUE(guard.unlock("2468", 1000, token) == PinGuard::Unlock::Ok);
  TEST_ASSERT_EQUAL(32, token.size());
  TEST_ASSERT_TRUE(guard.authorized(token, 2000));
}

void test_wrong_pin_gives_no_token() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;

  TEST_ASSERT_TRUE(guard.unlock("1357", 1000, token) == PinGuard::Unlock::Wrong);
  TEST_ASSERT_TRUE(token.empty());
}

void test_repeated_wrong_pins_lock_out_with_growing_delays() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;
  for (int i = 0; i < PinGuard::kFreeAttempts; ++i) guard.unlock("0000", 0, token);

  TEST_ASSERT_EQUAL(PinGuard::kFirstLockoutMs, guard.lockedForMs(0));
  TEST_ASSERT_TRUE(guard.unlock("2468", 1000, token) == PinGuard::Unlock::LockedOut);

  uint32_t now = PinGuard::kFirstLockoutMs;
  guard.unlock("0000", now, token);
  TEST_ASSERT_EQUAL(2 * PinGuard::kFirstLockoutMs, guard.lockedForMs(now));

  now += 2 * PinGuard::kFirstLockoutMs;
  TEST_ASSERT_TRUE(guard.unlock("2468", now, token) == PinGuard::Unlock::Ok);
  TEST_ASSERT_EQUAL(0, guard.lockedForMs(now));
}

void test_lockout_is_capped() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;
  uint32_t now = 0;
  for (int i = 0; i < 40; ++i) {
    now += guard.lockedForMs(now);
    guard.unlock("0000", now, token);
  }
  TEST_ASSERT_EQUAL(PinGuard::kMaxLockoutMs, guard.lockedForMs(now));
}

void test_idle_sessions_expire() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;
  guard.unlock("2468", 0, token);

  TEST_ASSERT_TRUE(guard.authorized(token, PinGuard::kSessionIdleMs - 1));  // keeps it alive
  TEST_ASSERT_TRUE(guard.authorized(token, 2 * PinGuard::kSessionIdleMs - 2));
  TEST_ASSERT_FALSE(guard.authorized(token, 3 * PinGuard::kSessionIdleMs));
}

void test_changing_or_removing_the_pin_ends_sessions() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string token;
  guard.unlock("2468", 0, token);

  guard.setPin("1357");
  TEST_ASSERT_FALSE(guard.authorized(token, 10));

  guard.setPin("");
  TEST_ASSERT_TRUE(guard.authorized("", 10));
}

void test_a_fifth_session_replaces_the_oldest() {
  PinGuard guard(fakeRandom);
  guard.setPin("2468");
  std::string tokens[5];
  for (int i = 0; i < 5; ++i) guard.unlock("2468", 1000 * (i + 1), tokens[i]);

  TEST_ASSERT_FALSE(guard.authorized(tokens[0], 6000));
  TEST_ASSERT_TRUE(guard.authorized(tokens[4], 6000));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_disabled_by_default_and_everything_is_allowed);
  RUN_TEST(test_pin_rules);
  RUN_TEST(test_enabled_pin_requires_a_token);
  RUN_TEST(test_right_pin_gives_a_working_token);
  RUN_TEST(test_wrong_pin_gives_no_token);
  RUN_TEST(test_repeated_wrong_pins_lock_out_with_growing_delays);
  RUN_TEST(test_lockout_is_capped);
  RUN_TEST(test_idle_sessions_expire);
  RUN_TEST(test_changing_or_removing_the_pin_ends_sessions);
  RUN_TEST(test_a_fifth_session_replaces_the_oldest);
  return UNITY_END();
}
