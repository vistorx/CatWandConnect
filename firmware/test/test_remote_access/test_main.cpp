#include <remote_access/remote_access.h>
#include <unity.h>

#include <string>
#include <vector>

#include "../fakes.h"

using namespace remote_access;

namespace {

// A provider client that records what it is asked and reports what the test sets.
class FakeProvider : public Provider {
 public:
  const char* id() const override { return "tailscale"; }
  bool available() const override { return isAvailable; }
  bool routes() const override { return true; }
  bool start(const std::string& key, const std::string& deviceName) override {
    calls.push_back("start " + key + " as " + deviceName);
    running = startWorks;
    return startWorks;
  }
  void stop() override {
    calls.push_back("stop");
    running = false;
  }
  void forget() override { calls.push_back("forget"); }
  void networkChanged() override { calls.push_back("networkChanged"); }
  ProviderStatus status() const override { return reported; }

  bool isAvailable = true;
  bool startWorks = true;
  bool running = false;
  ProviderStatus reported;
  std::vector<std::string> calls;
};

const std::string kKey = "tskey-auth-kAbCdEf1CNTRL-0123456789abcdefghijklmnopqrstuv";

struct Fixture {
  FakeProvider provider;
  cwc::test::FakeStorage storage;
  cwc::test::FakeClock clock;
  RemoteAccess remote{provider, storage, clock};
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_remote_access_is_off_by_default_and_never_connects() {
  Fixture f;
  f.remote.begin("catwandconnect-1a2b");
  f.remote.tick(true);
  f.clock.advance(60000);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(State::Off, f.remote.status().state);
  TEST_ASSERT_FALSE(f.remote.status().enabled);
  TEST_ASSERT_TRUE(f.provider.calls.empty());
}

void test_turned_on_it_joins_once_the_wifi_is_up() {
  Fixture f;
  f.remote.begin("catwandconnect-1a2b");
  TEST_ASSERT_EQUAL(Result::Ok, f.remote.turnOn(kKey));
  f.remote.tick(false);
  TEST_ASSERT_TRUE(f.provider.calls.empty());
  TEST_ASSERT_EQUAL(State::Connecting, f.remote.status().state);
  TEST_ASSERT_TRUE(f.remote.status().enabled);

  f.remote.tick(true);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(1, f.provider.calls.size());
  TEST_ASSERT_EQUAL_STRING(("start " + kKey + " as catwandconnect-1a2b").c_str(), f.provider.calls[0].c_str());
  f.provider.reported.link = Link::Connecting;
  TEST_ASSERT_EQUAL(State::Connecting, f.remote.status().state);
}

void test_connected_it_says_whether_the_path_is_direct_and_gives_its_address() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  f.provider.reported = {Link::Connected, false, (100u << 24) | (101u << 16) | (2u << 8) | 3u, 1767225600};
  Status s = f.remote.status();
  TEST_ASSERT_EQUAL(State::ConnectedRelay, s.state);
  TEST_ASSERT_EQUAL_STRING("100.101.2.3", s.address.c_str());
  TEST_ASSERT_EQUAL_STRING("minou", s.deviceName.c_str());
  TEST_ASSERT_EQUAL_STRING("tailscale", s.provider);
  TEST_ASSERT_TRUE(s.routes);
  TEST_ASSERT_EQUAL_INT64(1767225600, s.keyExpiry);

  f.provider.reported.direct = true;
  TEST_ASSERT_EQUAL(State::ConnectedDirect, f.remote.status().state);
}

void test_the_setting_and_its_key_survive_a_restart() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);

  FakeProvider provider;
  RemoteAccess restarted(provider, f.storage, f.clock);
  restarted.begin("minou");
  TEST_ASSERT_TRUE(restarted.status().enabled);
  restarted.tick(true);
  TEST_ASSERT_EQUAL(1, provider.calls.size());
  TEST_ASSERT_EQUAL_STRING(("start " + kKey + " as minou").c_str(), provider.calls[0].c_str());
}

bool storageHolds(const cwc::test::FakeStorage& storage, const std::string& text) {
  for (const auto& [key, bytes] : storage.blobs) {
    if (std::string(bytes.begin(), bytes.end()).find(text) != std::string::npos) return true;
  }
  return false;
}

void test_turning_it_off_disconnects_and_forgets_the_key() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  TEST_ASSERT_TRUE(storageHolds(f.storage, kKey));

  TEST_ASSERT_EQUAL(Result::Ok, f.remote.turnOff());
  TEST_ASSERT_FALSE(storageHolds(f.storage, kKey));
  TEST_ASSERT_EQUAL(State::Off, f.remote.status().state);
  f.remote.tick(true);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(3, f.provider.calls.size());
  TEST_ASSERT_EQUAL_STRING("stop", f.provider.calls[1].c_str());
  TEST_ASSERT_EQUAL_STRING("forget", f.provider.calls[2].c_str());

  FakeProvider provider;
  RemoteAccess restarted(provider, f.storage, f.clock);
  restarted.begin("minou");
  restarted.tick(true);
  TEST_ASSERT_FALSE(restarted.status().enabled);
  TEST_ASSERT_TRUE(provider.calls.empty());
}

void test_turning_it_off_before_it_joined_still_forgets_the_device() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.turnOff();
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(2, f.provider.calls.size());
  TEST_ASSERT_EQUAL_STRING("stop", f.provider.calls[0].c_str());
  TEST_ASSERT_EQUAL_STRING("forget", f.provider.calls[1].c_str());
}

void test_a_key_that_is_not_an_auth_key_is_refused_and_nothing_is_saved() {
  Fixture f;
  f.remote.begin("minou");
  for (const std::string& key : {std::string(""), std::string("hello"), std::string("tskey-"),
                                 std::string("tskey-auth-abc def"), std::string("tskey-auth-") + std::string(120, 'a'),
                                 std::string("tskey-auth-abc\"}")}) {
    TEST_ASSERT_EQUAL(Result::InvalidKey, f.remote.turnOn(key));
  }
  TEST_ASSERT_TRUE(f.storage.blobs.empty());
  TEST_ASSERT_FALSE(f.remote.status().enabled);
  TEST_ASSERT_TRUE(validKey(kKey));
  TEST_ASSERT_TRUE(validKey("tskey-auth-k123-ABC"));
}

void test_a_chip_without_a_client_reports_unsupported_and_refuses_to_turn_on() {
  Fixture f;
  f.provider.isAvailable = false;
  f.remote.begin("minou");
  TEST_ASSERT_EQUAL(State::Unsupported, f.remote.status().state);
  TEST_ASSERT_EQUAL(Result::Unsupported, f.remote.turnOn(kKey));
  f.remote.tick(true);
  TEST_ASSERT_TRUE(f.provider.calls.empty());
  TEST_ASSERT_EQUAL_STRING("unsupported", errorCode(Result::Unsupported));
  TEST_ASSERT_EQUAL_STRING("invalid_key", errorCode(Result::InvalidKey));
}

void test_a_rejected_key_says_key_expired() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  f.provider.reported.link = Link::KeyRejected;
  TEST_ASSERT_EQUAL(State::KeyExpired, f.remote.status().state);
  TEST_ASSERT_EQUAL_STRING("key_expired", stateName(State::KeyExpired));
}

void test_a_client_that_cannot_start_is_an_error_retried_later() {
  Fixture f;
  f.provider.startWorks = false;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(State::Error, f.remote.status().state);
  f.clock.advance(RemoteAccess::kRetryMs - 1);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(1, f.provider.calls.size());

  f.provider.startWorks = true;
  f.clock.advance(1);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(2, f.provider.calls.size());
  TEST_ASSERT_EQUAL(State::Connecting, f.remote.status().state);
  f.provider.reported.link = Link::Failed;
  TEST_ASSERT_EQUAL(State::Error, f.remote.status().state);
}

void test_when_the_wifi_comes_back_the_client_reconnects_once() {
  Fixture f;
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  f.remote.tick(false);
  f.remote.tick(false);
  f.remote.tick(true);
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(2, f.provider.calls.size());
  TEST_ASSERT_EQUAL_STRING("networkChanged", f.provider.calls[1].c_str());
}

void test_a_new_key_while_on_joins_again_with_it() {
  Fixture f;
  const std::string newKey = "tskey-auth-kNew2CNTRL-abcdef";
  f.remote.begin("minou");
  f.remote.turnOn(kKey);
  f.remote.tick(true);
  f.provider.reported.link = Link::KeyRejected;

  TEST_ASSERT_EQUAL(Result::Ok, f.remote.turnOn(newKey));
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(4, f.provider.calls.size());
  TEST_ASSERT_EQUAL_STRING("stop", f.provider.calls[1].c_str());
  TEST_ASSERT_EQUAL_STRING("forget", f.provider.calls[2].c_str());
  TEST_ASSERT_EQUAL_STRING(("start " + newKey + " as minou").c_str(), f.provider.calls[3].c_str());
  TEST_ASSERT_FALSE(storageHolds(f.storage, kKey));
}

void test_an_unreadable_record_leaves_remote_access_off() {
  Fixture f;
  f.storage.blobs["remote"] = std::vector<uint8_t>(123, 0x7f);
  f.remote.begin("minou");
  f.remote.tick(true);
  TEST_ASSERT_EQUAL(State::Off, f.remote.status().state);
  TEST_ASSERT_TRUE(f.provider.calls.empty());
}

void test_tailnet_addresses_are_100_64_0_0_slash_10() {
  auto ip = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d) { return a << 24 | b << 16 | c << 8 | d; };
  TEST_ASSERT_TRUE(isTailnetAddress(ip(100, 64, 0, 1)));
  TEST_ASSERT_TRUE(isTailnetAddress(ip(100, 101, 59, 73)));
  TEST_ASSERT_TRUE(isTailnetAddress(ip(100, 127, 255, 254)));
  TEST_ASSERT_FALSE(isTailnetAddress(ip(100, 63, 255, 255)));
  TEST_ASSERT_FALSE(isTailnetAddress(ip(100, 128, 0, 1)));
  TEST_ASSERT_FALSE(isTailnetAddress(ip(192, 168, 1, 62)));
  TEST_ASSERT_FALSE(isTailnetAddress(0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_remote_access_is_off_by_default_and_never_connects);
  RUN_TEST(test_turned_on_it_joins_once_the_wifi_is_up);
  RUN_TEST(test_connected_it_says_whether_the_path_is_direct_and_gives_its_address);
  RUN_TEST(test_the_setting_and_its_key_survive_a_restart);
  RUN_TEST(test_turning_it_off_disconnects_and_forgets_the_key);
  RUN_TEST(test_turning_it_off_before_it_joined_still_forgets_the_device);
  RUN_TEST(test_a_key_that_is_not_an_auth_key_is_refused_and_nothing_is_saved);
  RUN_TEST(test_a_chip_without_a_client_reports_unsupported_and_refuses_to_turn_on);
  RUN_TEST(test_a_rejected_key_says_key_expired);
  RUN_TEST(test_a_client_that_cannot_start_is_an_error_retried_later);
  RUN_TEST(test_when_the_wifi_comes_back_the_client_reconnects_once);
  RUN_TEST(test_a_new_key_while_on_joins_again_with_it);
  RUN_TEST(test_an_unreadable_record_leaves_remote_access_off);
  RUN_TEST(test_tailnet_addresses_are_100_64_0_0_slash_10);
  return UNITY_END();
}
