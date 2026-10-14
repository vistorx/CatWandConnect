#include <ArduinoJson.h>
#include <home_assistant/home_assistant.h>
#include <unity.h>

using namespace home_assistant;

namespace {

Device device() { return {"CatWandConnect salon", "catwandconnect-salon", "24:6F:28:AB:CD:EF", "0.1.0", "http://192.168.1.20/"}; }

const Message* find(const std::vector<Message>& messages, const std::string& topic) {
  for (const auto& m : messages) {
    if (m.topic == topic) return &m;
  }
  return nullptr;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_topics_live_under_the_hostname() {
  Topics topics(device());
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/status", topics.availability().c_str());
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/state", topics.state().c_str());
}

void test_discovery_announces_the_required_entities() {
  auto messages = discovery(device());

  TEST_ASSERT_EQUAL(8, messages.size());
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/button/catwandconnect_246f28abcdef/stow/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/horizontal_position/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/vertical_position/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/horizontal_max_speed/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/vertical_max_speed/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/activity/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/switch/catwandconnect_246f28abcdef/play_session/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/session_duration/config"));
}

void test_every_entity_shares_device_and_availability() {
  for (const auto& message : discovery(device())) {
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, message.payload));
    TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/status", doc["availability_topic"]);
    TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef", doc["device"]["identifiers"][0]);
    TEST_ASSERT_EQUAL_STRING("CatWandConnect salon", doc["device"]["name"]);
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.20/", doc["device"]["configuration_url"]);
    TEST_ASSERT_TRUE(doc["unique_id"].is<const char*>());
  }
}

void test_position_entity_reads_state_and_takes_commands() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/number/catwandconnect_246f28abcdef/vertical_position/config")->payload);

  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/state", doc["state_topic"]);
  TEST_ASSERT_EQUAL_STRING("{{ value_json.v }}", doc["value_template"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/vertical/position/set", doc["command_topic"]);
}

void test_play_session_switch_follows_the_activity_and_takes_on_off() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/switch/catwandconnect_246f28abcdef/play_session/config")->payload);

  TEST_ASSERT_EQUAL_STRING("Play session", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_play_session", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_salon_play_session", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/state", doc["state_topic"]);
  TEST_ASSERT_EQUAL_STRING("{{ value_json.play }}", doc["value_template"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/play_session/set", doc["command_topic"]);
  TEST_ASSERT_EQUAL_STRING("ON", doc["payload_on"]);
  TEST_ASSERT_EQUAL_STRING("OFF", doc["payload_off"]);
  TEST_ASSERT_EQUAL_STRING("ON", doc["state_on"]);
  TEST_ASSERT_EQUAL_STRING("OFF", doc["state_off"]);
}

void test_session_duration_number_in_minutes() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/number/catwandconnect_246f28abcdef/session_duration/config")->payload);

  TEST_ASSERT_EQUAL_STRING("Session duration", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_session_duration", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/state", doc["state_topic"]);
  TEST_ASSERT_EQUAL_STRING("{{ value_json.sm }}", doc["value_template"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/session_duration/set", doc["command_topic"]);
  TEST_ASSERT_EQUAL(1, doc["min"].as<int>());
  TEST_ASSERT_EQUAL(60, doc["max"].as<int>());
  TEST_ASSERT_EQUAL(1, doc["step"].as<int>());
  TEST_ASSERT_EQUAL_STRING("box", doc["mode"]);
  TEST_ASSERT_EQUAL_STRING("min", doc["unit_of_measurement"]);
}

void test_state_payload_reports_the_play_session() {
  JsonDocument playing;
  deserializeJson(playing, statePayload(State{{90, 90}, {90, 90}, "playing", 15}));
  TEST_ASSERT_EQUAL_STRING("ON", playing["play"]);
  TEST_ASSERT_EQUAL(15, playing["sm"].as<int>());

  JsonDocument idle;
  deserializeJson(idle, statePayload(State{{90, 90}, {90, 90}, "idle", 10}));
  TEST_ASSERT_EQUAL_STRING("OFF", idle["play"]);
  TEST_ASSERT_EQUAL(10, idle["sm"].as<int>());
}

void test_parses_play_session_commands() {
  Topics topics(device());

  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/catwandconnect-salon/play_session/set", "ON").kind ==
                   CommandKind::StartPlay);
  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/catwandconnect-salon/play_session/set", "OFF").kind ==
                   CommandKind::StopPlay);
  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/catwandconnect-salon/play_session/set", "on").kind ==
                   CommandKind::None);

  Command duration = parseCommand(topics, "catwandconnect/catwandconnect-salon/session_duration/set", "15");
  TEST_ASSERT_TRUE(duration.kind == CommandKind::SessionDuration);
  TEST_ASSERT_EQUAL_FLOAT(15, duration.value);
  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/catwandconnect-salon/session_duration/set", "abc").kind ==
                   CommandKind::None);
}

void test_state_payload() {
  State state{{92.4f, 87.6f}, {90, 45}, "manual", 10};
  JsonDocument doc;
  deserializeJson(doc, statePayload(state));

  TEST_ASSERT_EQUAL(92, doc["h"].as<int>());
  TEST_ASSERT_EQUAL(88, doc["v"].as<int>());
  TEST_ASSERT_EQUAL_FLOAT(45, doc["vs"].as<float>());
  TEST_ASSERT_EQUAL_STRING("manual", doc["activity"]);
}

void test_parses_commands() {
  Topics topics(device());

  Command stow = parseCommand(topics, "catwandconnect/catwandconnect-salon/stow/set", "PRESS");
  TEST_ASSERT_TRUE(stow.kind == CommandKind::Stow);

  Command position = parseCommand(topics, "catwandconnect/catwandconnect-salon/horizontal/position/set", "105");
  TEST_ASSERT_TRUE(position.kind == CommandKind::Position);
  TEST_ASSERT_TRUE(position.axis == cwc::Axis::Horizontal);
  TEST_ASSERT_EQUAL_FLOAT(105, position.value);

  Command speed = parseCommand(topics, "catwandconnect/catwandconnect-salon/vertical/max_speed/set", "45.5");
  TEST_ASSERT_TRUE(speed.kind == CommandKind::MaxSpeed);
  TEST_ASSERT_TRUE(speed.axis == cwc::Axis::Vertical);
  TEST_ASSERT_EQUAL_FLOAT(45.5f, speed.value);
}

void test_ignores_unknown_topics_and_bad_values() {
  Topics topics(device());
  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/other/stow/set", "PRESS").kind == CommandKind::None);
  TEST_ASSERT_TRUE(parseCommand(topics, "catwandconnect/catwandconnect-salon/stow/set", "go").kind == CommandKind::None);
  TEST_ASSERT_TRUE(
      parseCommand(topics, "catwandconnect/catwandconnect-salon/horizontal/position/set", "abc").kind ==
      CommandKind::None);
  TEST_ASSERT_TRUE(
      parseCommand(topics, "catwandconnect/catwandconnect-salon/horizontal/position/set", "").kind ==
      CommandKind::None);
}

void test_entity_ids_follow_the_hostname_and_unique_ids_the_mac() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/button/catwandconnect_246f28abcdef/stow/config")->payload);

  TEST_ASSERT_EQUAL_STRING("catwandconnect_salon_stow", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_stow", doc["unique_id"]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_topics_live_under_the_hostname);
  RUN_TEST(test_discovery_announces_the_required_entities);
  RUN_TEST(test_every_entity_shares_device_and_availability);
  RUN_TEST(test_position_entity_reads_state_and_takes_commands);
  RUN_TEST(test_state_payload);
  RUN_TEST(test_play_session_switch_follows_the_activity_and_takes_on_off);
  RUN_TEST(test_session_duration_number_in_minutes);
  RUN_TEST(test_state_payload_reports_the_play_session);
  RUN_TEST(test_parses_play_session_commands);
  RUN_TEST(test_entity_ids_follow_the_hostname_and_unique_ids_the_mac);
  RUN_TEST(test_parses_commands);
  RUN_TEST(test_ignores_unknown_topics_and_bad_values);
  return UNITY_END();
}
