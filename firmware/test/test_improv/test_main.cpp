#include <improv/improv.h>
#include <unity.h>

#include <string>
#include <vector>

using namespace improv;

namespace {

// Builds an RPC command packet the way the web flasher sends it.
std::vector<uint8_t> rpcPacket(const std::vector<uint8_t>& rpc, int checksumDelta = 0) {
  std::vector<uint8_t> packet = {'I', 'M', 'P', 'R', 'O', 'V', 1, 0x03,
                                 static_cast<uint8_t>(rpc.size())};
  packet.insert(packet.end(), rpc.begin(), rpc.end());
  uint8_t checksum = 0;
  for (uint8_t b : packet) checksum += b;
  packet.push_back(static_cast<uint8_t>(checksum + checksumDelta));
  return packet;
}

std::vector<uint8_t> wifiSettingsRpc(const std::string& ssid, const std::string& password) {
  std::vector<uint8_t> data;
  data.push_back(static_cast<uint8_t>(ssid.size()));
  data.insert(data.end(), ssid.begin(), ssid.end());
  data.push_back(static_cast<uint8_t>(password.size()));
  data.insert(data.end(), password.begin(), password.end());
  std::vector<uint8_t> rpc = {0x01, static_cast<uint8_t>(data.size())};
  rpc.insert(rpc.end(), data.begin(), data.end());
  return rpc;
}

// Feeds bytes and returns the status after the last one.
Parser::Status feedAll(Parser& parser, const std::vector<uint8_t>& bytes) {
  Parser::Status status = Parser::Status::Pending;
  for (uint8_t b : bytes) status = parser.feed(b);
  return status;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_parses_wifi_settings() {
  Parser parser;

  auto status = feedAll(parser, rpcPacket(wifiSettingsRpc("MyHome", "s3cret pass")));

  TEST_ASSERT_TRUE(status == Parser::Status::Request);
  TEST_ASSERT_TRUE(parser.request().command == Command::WifiSettings);
  TEST_ASSERT_EQUAL_STRING("MyHome", parser.request().ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("s3cret pass", parser.request().password.c_str());
}

void test_parses_open_network_with_empty_password() {
  Parser parser;

  auto status = feedAll(parser, rpcPacket(wifiSettingsRpc("Guest", "")));

  TEST_ASSERT_TRUE(status == Parser::Status::Request);
  TEST_ASSERT_EQUAL_STRING("Guest", parser.request().ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("", parser.request().password.c_str());
}

void test_parses_state_info_and_scan_requests() {
  Parser parser;

  TEST_ASSERT_TRUE(feedAll(parser, rpcPacket({0x02, 0})) == Parser::Status::Request);
  TEST_ASSERT_TRUE(parser.request().command == Command::GetCurrentState);
  TEST_ASSERT_TRUE(feedAll(parser, rpcPacket({0x03, 0})) == Parser::Status::Request);
  TEST_ASSERT_TRUE(parser.request().command == Command::GetDeviceInfo);
  TEST_ASSERT_TRUE(feedAll(parser, rpcPacket({0x04, 0})) == Parser::Status::Request);
  TEST_ASSERT_TRUE(parser.request().command == Command::GetWifiNetworks);
}

void test_skips_log_output_around_packets() {
  Parser parser;
  std::string log = "[I] CatWandConnect starting\nIMPRO";
  std::vector<uint8_t> bytes(log.begin(), log.end());
  auto packet = rpcPacket({0x03, 0});
  bytes.insert(bytes.end(), packet.begin(), packet.end());

  TEST_ASSERT_TRUE(feedAll(parser, bytes) == Parser::Status::Request);
  TEST_ASSERT_TRUE(parser.request().command == Command::GetDeviceInfo);
}

void test_rejects_bad_checksum() {
  Parser parser;

  auto status = feedAll(parser, rpcPacket({0x02, 0}, 1));

  TEST_ASSERT_TRUE(status == Parser::Status::Invalid);
  TEST_ASSERT_TRUE(parser.error() == Error::InvalidRpc);
}

void test_rejects_unknown_command() {
  Parser parser;

  auto status = feedAll(parser, rpcPacket({0x7F, 0}));

  TEST_ASSERT_TRUE(status == Parser::Status::Invalid);
  TEST_ASSERT_TRUE(parser.error() == Error::UnknownRpc);
}

void test_rejects_truncated_wifi_settings() {
  Parser parser;
  auto rpc = wifiSettingsRpc("MyHome", "pw");
  rpc[2] = 40;  // SSID length runs past the end of the data

  auto status = feedAll(parser, rpcPacket(rpc));

  TEST_ASSERT_TRUE(status == Parser::Status::Invalid);
  TEST_ASSERT_TRUE(parser.error() == Error::InvalidRpc);
}

void test_recovers_after_an_invalid_packet() {
  Parser parser;
  feedAll(parser, rpcPacket({0x7F, 0}));

  TEST_ASSERT_TRUE(feedAll(parser, rpcPacket({0x02, 0})) == Parser::Status::Request);
}

void test_encodes_state_packet() {
  std::vector<uint8_t> expected = {'I', 'M', 'P', 'R', 'O', 'V', 1, 0x01, 1, 0x04};
  uint8_t checksum = 0;
  for (uint8_t b : expected) checksum += b;
  expected.push_back(checksum);
  expected.push_back('\n');

  auto packet = statePacket(State::Provisioned);

  TEST_ASSERT_EQUAL(expected.size(), packet.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.data(), packet.data(), expected.size());
}

void test_encodes_rpc_result_with_strings() {
  auto packet = rpcResultPacket(Command::WifiSettings, {"http://192.168.1.20/"});

  std::string url = "http://192.168.1.20/";
  TEST_ASSERT_EQUAL_UINT8(0x04, packet[7]);                 // RPC result
  TEST_ASSERT_EQUAL_UINT8(2 + 1 + url.size(), packet[8]);   // packet data length
  TEST_ASSERT_EQUAL_UINT8(0x01, packet[9]);                 // answered command
  TEST_ASSERT_EQUAL_UINT8(1 + url.size(), packet[10]);      // RPC data length
  TEST_ASSERT_EQUAL_UINT8(url.size(), packet[11]);          // string length
  TEST_ASSERT_EQUAL_STRING_LEN(url.c_str(), &packet[12], url.size());
  TEST_ASSERT_EQUAL_UINT8('\n', packet.back());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parses_wifi_settings);
  RUN_TEST(test_parses_open_network_with_empty_password);
  RUN_TEST(test_parses_state_info_and_scan_requests);
  RUN_TEST(test_skips_log_output_around_packets);
  RUN_TEST(test_rejects_bad_checksum);
  RUN_TEST(test_rejects_unknown_command);
  RUN_TEST(test_rejects_truncated_wifi_settings);
  RUN_TEST(test_recovers_after_an_invalid_packet);
  RUN_TEST(test_encodes_state_packet);
  RUN_TEST(test_encodes_rpc_result_with_strings);
  return UNITY_END();
}
