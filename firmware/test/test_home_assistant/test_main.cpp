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

  TEST_ASSERT_EQUAL(12, messages.size());
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/button/catwandconnect_246f28abcdef/stow/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/horizontal_position/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/vertical_position/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/horizontal_max_speed/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/vertical_max_speed/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/activity/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/switch/catwandconnect_246f28abcdef/play_session/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/number/catwandconnect_246f28abcdef/session_duration/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/update/catwandconnect_246f28abcdef/firmware/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/play_session_end/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/event/catwandconnect_246f28abcdef/play_session/config"));
  TEST_ASSERT_NOT_NULL(find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/sessions_today/config"));
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

// The bridge's MQTT buffer is 1024 bytes for topic, payload and header.
void test_every_discovery_message_fits_the_mqtt_buffer() {
  for (const auto& message : discovery(device())) {
    TEST_ASSERT_LESS_THAN(1000, message.topic.size() + message.payload.size());
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

// A default-named robot's hostname has a Device ID suffix (#195): its topics
// use it, so two robots never share commands, while its entity ids keep the
// documented catwandconnect_* (as with the Native integration).
void test_a_suffixed_hostname_keeps_topics_unique_and_entity_ids_readable() {
  Device d{"CatWandConnect", "catwandconnect-5894", "24:6F:28:AB:CD:EF", "0.1.0", "http://192.168.1.20/"};
  d.entityPrefix = "catwandconnect";
  Topics topics(d);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-5894/status", topics.availability().c_str());

  auto messages = discovery(d);
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/button/catwandconnect_246f28abcdef/stow/config")->payload);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_stow", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_stow", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-5894/stow/set", doc["command_topic"]);
}

// The robot can't download anything (ADR 0012): the Firmware entity only
// shows the versions, with no install command.
void test_firmware_update_entity_reads_its_own_topic_and_has_no_install() {
  Topics topics(device());
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/update", topics.update().c_str());

  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/update/catwandconnect_246f28abcdef/firmware/config")->payload);
  TEST_ASSERT_EQUAL_STRING("Firmware", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("firmware", doc["device_class"]);
  TEST_ASSERT_EQUAL_STRING("config", doc["entity_category"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/update", doc["state_topic"]);
  TEST_ASSERT_TRUE(doc["command_topic"].isNull());
}

void test_update_payload_offers_a_newer_version_with_its_notes() {
  JsonDocument doc;
  deserializeJson(doc, updatePayload({"0.2.0-beta.3", "0.2.0-beta.4", "https://example.org/notes"}));
  TEST_ASSERT_EQUAL_STRING("0.2.0-beta.3", doc["installed_version"]);
  TEST_ASSERT_EQUAL_STRING("0.2.0-beta.4", doc["latest_version"]);
  TEST_ASSERT_EQUAL_STRING("https://example.org/notes", doc["release_url"]);
  TEST_ASSERT_EQUAL_STRING("CatWandConnect firmware", doc["title"]);
}

// Nothing known, or nothing newer: Home Assistant shows the robot up to date.
void test_update_payload_without_a_newer_version_reads_up_to_date() {
  for (const char* latest : {"", "0.1.0", "0.2.0-beta.3", "garbage"}) {
    JsonDocument doc;
    deserializeJson(doc, updatePayload({"0.2.0-beta.3", latest, "https://example.org/notes"}));
    TEST_ASSERT_EQUAL_STRING("0.2.0-beta.3", doc["latest_version"]);
    TEST_ASSERT_TRUE(doc["release_url"].isNull());
  }
}

// Without notes from the Device interface, the release page of that version.
void test_update_payload_defaults_the_release_url_to_the_github_release() {
  JsonDocument doc;
  deserializeJson(doc, updatePayload({"0.2.0-beta.3", "0.2.0", ""}));
  TEST_ASSERT_EQUAL_STRING("https://github.com/vistorx/CatWandConnect/releases/tag/v0.2.0", doc["release_url"]);
}

// The robot has no wall clock: it sends the time left, and Home Assistant
// turns it into the end time with its own clock (#236).
void test_play_session_end_sensor_turns_the_time_left_into_a_timestamp() {
  Topics topics(device());
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/play_session/remaining", topics.playRemaining().c_str());

  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/play_session_end/config")->payload);
  TEST_ASSERT_EQUAL_STRING("Play session end", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_play_session_end", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_salon_play_session_end", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("timestamp", doc["device_class"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/play_session/remaining", doc["state_topic"]);
  // A rendered "None" is Home Assistant's MQTT sensor's way to say unknown.
  TEST_ASSERT_EQUAL_STRING(
      "{{ (now() + timedelta(seconds=value|int(0))).isoformat() if value|int(0) > 0 else None }}",
      doc["value_template"]);
}

void test_play_remaining_payload_is_whole_seconds_and_0_without_a_session() {
  TEST_ASSERT_EQUAL_STRING("587", playRemainingPayload(587).c_str());
  TEST_ASSERT_EQUAL_STRING("0", playRemainingPayload(0).c_str());
}

void test_play_remaining_is_published_once_at_first() {
  PlayRemainingReporter reporter;
  TEST_ASSERT_TRUE(reporter.due(0, 1000));
  TEST_ASSERT_FALSE(reporter.due(0, 1500));
  TEST_ASSERT_FALSE(reporter.due(0, 120000));
}

void test_play_remaining_is_published_when_a_session_starts_and_ends() {
  PlayRemainingReporter reporter;
  reporter.due(0, 1000);
  TEST_ASSERT_TRUE(reporter.due(600, 2000));
  TEST_ASSERT_FALSE(reporter.due(600, 2000));
  TEST_ASSERT_TRUE(reporter.due(0, 5000));  // stopped early
  TEST_ASSERT_FALSE(reporter.due(0, 6000));
}

// Counting down is not news: the end time stays put, so nothing is sent
// every second (Home Assistant shows "in 7 minutes" by itself).
void test_play_remaining_is_not_published_while_it_just_counts_down() {
  PlayRemainingReporter reporter;
  reporter.due(600, 0);
  for (uint32_t s = 1; s < 600; ++s) {
    // Rounded up to whole seconds, read a little after each tick: jitter.
    TEST_ASSERT_FALSE(reporter.due(600 - s, s * 1000 + 999));
  }
}

// A new session started during one moves the end: publish it.
void test_play_remaining_is_published_when_the_end_moves() {
  PlayRemainingReporter reporter;
  reporter.due(600, 0);
  TEST_ASSERT_FALSE(reporter.due(598, 2000));
  TEST_ASSERT_TRUE(reporter.due(900, 300000));
  TEST_ASSERT_FALSE(reporter.due(899, 301000));
}

// After a reconnection or a Home Assistant restart, send it again.
void test_play_remaining_is_published_again_when_forced() {
  PlayRemainingReporter reporter;
  reporter.due(600, 0);
  reporter.force();
  TEST_ASSERT_TRUE(reporter.due(590, 10000));
  TEST_ASSERT_FALSE(reporter.due(589, 11000));
}

void test_recognises_home_assistant_coming_online() {
  TEST_ASSERT_EQUAL_STRING("homeassistant/status", kHomeAssistantStatus);
  TEST_ASSERT_TRUE(homeAssistantOnline("homeassistant/status", "online"));
  TEST_ASSERT_FALSE(homeAssistantOnline("homeassistant/status", "offline"));
  TEST_ASSERT_FALSE(homeAssistantOnline("catwandconnect/catwandconnect-salon/status", "online"));
}

// The Play session journal: one event entity fed by non-retained messages on
// its own topic, so Home Assistant never replays an old event.
void test_play_session_event_entity() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/event/catwandconnect_246f28abcdef/play_session/config")->payload);

  TEST_ASSERT_EQUAL_STRING("Play session", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_play_session_event", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_salon_play_session", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/play_session/event", doc["state_topic"]);
  TEST_ASSERT_EQUAL(4, doc["event_types"].size());
  TEST_ASSERT_EQUAL_STRING("started", doc["event_types"][0]);
  TEST_ASSERT_EQUAL_STRING("ended", doc["event_types"][1]);
  TEST_ASSERT_EQUAL_STRING("stopped", doc["event_types"][2]);
  TEST_ASSERT_EQUAL_STRING("refused", doc["event_types"][3]);
  TEST_ASSERT_TRUE(doc["command_topic"].isNull());
  Topics topics(device());
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/play_session/event", topics.playEvent().c_str());
}

void test_play_session_event_payload_carries_source_and_reason() {
  JsonDocument refused;
  deserializeJson(refused, eventPayload("refused", "home_assistant_mqtt", "busy"));
  TEST_ASSERT_EQUAL_STRING("refused", refused["event_type"]);
  TEST_ASSERT_EQUAL_STRING("home_assistant_mqtt", refused["source"]);
  TEST_ASSERT_EQUAL_STRING("busy", refused["reason"]);

  JsonDocument started;
  deserializeJson(started, eventPayload("started", "device_interface", nullptr));
  TEST_ASSERT_EQUAL_STRING("started", started["event_type"]);
  TEST_ASSERT_TRUE(started["reason"].is<std::nullptr_t>());  // always there, so automations can test it
}

void test_sessions_today_sensor_is_unknown_without_a_clock() {
  auto messages = discovery(device());
  JsonDocument doc;
  deserializeJson(doc, find(messages, "homeassistant/sensor/catwandconnect_246f28abcdef/sessions_today/config")->payload);
  TEST_ASSERT_EQUAL_STRING("Sessions today", doc["name"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_246f28abcdef_sessions_today", doc["unique_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect_salon_sessions_today", doc["object_id"]);
  TEST_ASSERT_EQUAL_STRING("catwandconnect/catwandconnect-salon/state", doc["state_topic"]);
  TEST_ASSERT_EQUAL_STRING("total_increasing", doc["state_class"]);
  // Home Assistant's MQTT sensor reads the text "None" as unknown.
  TEST_ASSERT_EQUAL_STRING("{{ value_json.today if value_json.today is not none else 'None' }}", doc["value_template"]);

  JsonDocument known;
  deserializeJson(known, statePayload(State{{90, 90}, {90, 90}, "idle", 10, 3}));
  TEST_ASSERT_EQUAL(3, known["today"].as<int>());
  JsonDocument unknown;
  deserializeJson(unknown, statePayload(State{{90, 90}, {90, 90}, "idle", 10, -1}));
  TEST_ASSERT_TRUE(unknown["today"].is<std::nullptr_t>());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_topics_live_under_the_hostname);
  RUN_TEST(test_discovery_announces_the_required_entities);
  RUN_TEST(test_every_entity_shares_device_and_availability);
  RUN_TEST(test_every_discovery_message_fits_the_mqtt_buffer);
  RUN_TEST(test_position_entity_reads_state_and_takes_commands);
  RUN_TEST(test_state_payload);
  RUN_TEST(test_play_session_switch_follows_the_activity_and_takes_on_off);
  RUN_TEST(test_session_duration_number_in_minutes);
  RUN_TEST(test_state_payload_reports_the_play_session);
  RUN_TEST(test_parses_play_session_commands);
  RUN_TEST(test_entity_ids_follow_the_hostname_and_unique_ids_the_mac);
  RUN_TEST(test_a_suffixed_hostname_keeps_topics_unique_and_entity_ids_readable);
  RUN_TEST(test_parses_commands);
  RUN_TEST(test_ignores_unknown_topics_and_bad_values);
  RUN_TEST(test_firmware_update_entity_reads_its_own_topic_and_has_no_install);
  RUN_TEST(test_update_payload_offers_a_newer_version_with_its_notes);
  RUN_TEST(test_update_payload_without_a_newer_version_reads_up_to_date);
  RUN_TEST(test_update_payload_defaults_the_release_url_to_the_github_release);
  RUN_TEST(test_play_session_end_sensor_turns_the_time_left_into_a_timestamp);
  RUN_TEST(test_play_remaining_payload_is_whole_seconds_and_0_without_a_session);
  RUN_TEST(test_play_remaining_is_published_once_at_first);
  RUN_TEST(test_play_remaining_is_published_when_a_session_starts_and_ends);
  RUN_TEST(test_play_remaining_is_not_published_while_it_just_counts_down);
  RUN_TEST(test_play_remaining_is_published_when_the_end_moves);
  RUN_TEST(test_play_remaining_is_published_again_when_forced);
  RUN_TEST(test_recognises_home_assistant_coming_online);
  RUN_TEST(test_play_session_event_entity);
  RUN_TEST(test_play_session_event_payload_carries_source_and_reason);
  RUN_TEST(test_sessions_today_sensor_is_unknown_without_a_clock);
  return UNITY_END();
}
