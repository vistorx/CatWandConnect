#include "home_assistant/home_assistant.h"

#include <ArduinoJson.h>

#include <cstdlib>

namespace home_assistant {

namespace {

constexpr const char* kAxisNames[] = {"horizontal", "vertical"};
constexpr const char* kAxisTitles[] = {"Horizontal", "Vertical"};
constexpr const char* kStateKeys[] = {"h", "v"};
constexpr const char* kSpeedKeys[] = {"hs", "vs"};

std::string nodeId(const Device& device) {
  std::string id = "catwandconnect_";
  for (char c : device.mac) {
    if (c == ':') continue;
    id += (c >= 'A' && c <= 'F') ? static_cast<char>(c - 'A' + 'a') : c;
  }
  return id;
}

// Readable entity ids follow the hostname: "catwandconnect-salon" gives
// button.catwandconnect_salon_stow. The unique id stays tied to the MAC.
std::string objectPrefix(const Device& device) {
  std::string prefix = device.hostname;
  for (char& c : prefix) {
    if (c == '-') c = '_';
  }
  return prefix;
}

// Fields every entity shares: availability and the device it belongs to.
// `suffix` names the entity, e.g. "stow".
void common(JsonObject entity, const Device& device, const Topics& topics, const std::string& suffix) {
  entity["unique_id"] = nodeId(device) + "_" + suffix;
  entity["object_id"] = objectPrefix(device) + "_" + suffix;
  entity["availability_topic"] = topics.availability();
  JsonObject dev = entity["device"].to<JsonObject>();
  dev["identifiers"].to<JsonArray>().add(nodeId(device));
  dev["connections"].to<JsonArray>().add<JsonArray>().add("mac");
  dev["connections"][0].add(device.mac);
  dev["name"] = device.name;
  dev["manufacturer"] = "CatWandConnect";
  dev["model"] = "CatWandConnect";
  dev["sw_version"] = device.version;
  if (!device.url.empty()) dev["configuration_url"] = device.url;
}

Message config(const std::string& prefix, const char* component, const Device& device, const std::string& object,
               const JsonDocument& doc) {
  Message message;
  message.topic = prefix + "/" + component + "/" + nodeId(device) + "/" + object + "/config";
  serializeJson(doc, message.payload);
  return message;
}

bool parseNumber(const std::string& text, float& value) {
  if (text.empty()) return false;
  char* end = nullptr;
  value = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

}  // namespace

Topics::Topics(const Device& device) : base("catwandconnect/" + device.hostname) {}

std::vector<Message> discovery(const Device& device, const std::string& prefix) {
  Topics topics(device);
  std::vector<Message> messages;

  {
    JsonDocument doc;
    common(doc.to<JsonObject>(), device, topics, "stow");
    doc["name"] = "Stow";
    doc["icon"] = "mdi:home-import-outline";
    doc["command_topic"] = topics.base + "/stow/set";
    doc["payload_press"] = "PRESS";
    messages.push_back(config(prefix, "button", device, "stow", doc));
  }

  for (size_t i = 0; i < cwc::kAxisCount; ++i) {
    const std::string axis = kAxisNames[i];
    {
      JsonDocument doc;
      common(doc.to<JsonObject>(), device, topics, axis + "_position");
      doc["name"] = std::string(kAxisTitles[i]) + " position";
      doc["icon"] = "mdi:axis-arrow";
      doc["state_topic"] = topics.state();
      doc["value_template"] = std::string("{{ value_json.") + kStateKeys[i] + " }}";
      doc["command_topic"] = topics.base + "/" + axis + "/position/set";
      doc["min"] = 0;
      doc["max"] = 180;
      doc["step"] = 1;
      doc["mode"] = "slider";
      doc["unit_of_measurement"] = "°";
      messages.push_back(config(prefix, "number", device, axis + "_position", doc));
    }
    {
      JsonDocument doc;
      common(doc.to<JsonObject>(), device, topics, axis + "_max_speed");
      doc["name"] = std::string(kAxisTitles[i]) + " maximum speed";
      doc["icon"] = "mdi:speedometer";
      doc["entity_category"] = "config";
      doc["state_topic"] = topics.state();
      doc["value_template"] = std::string("{{ value_json.") + kSpeedKeys[i] + " }}";
      doc["command_topic"] = topics.base + "/" + axis + "/max_speed/set";
      doc["min"] = 10;
      doc["max"] = 360;
      doc["step"] = 5;
      doc["mode"] = "box";
      doc["unit_of_measurement"] = "°/s";
      messages.push_back(config(prefix, "number", device, axis + "_max_speed", doc));
    }
  }

  {
    JsonDocument doc;
    common(doc.to<JsonObject>(), device, topics, "activity");
    doc["name"] = "Activity";
    doc["icon"] = "mdi:cat";
    doc["state_topic"] = topics.state();
    doc["value_template"] = "{{ value_json.activity }}";
    messages.push_back(config(prefix, "sensor", device, "activity", doc));
  }
  return messages;
}

std::string statePayload(const State& state) {
  JsonDocument doc;
  for (size_t i = 0; i < cwc::kAxisCount; ++i) {
    doc[kStateKeys[i]] = static_cast<int>(state.position[i] + (state.position[i] < 0 ? -0.5f : 0.5f));
    doc[kSpeedKeys[i]] = state.maxSpeed[i];
  }
  doc["activity"] = state.activity;
  std::string payload;
  serializeJson(doc, payload);
  return payload;
}

Command parseCommand(const Topics& topics, const std::string& topic, const std::string& payload) {
  Command command;
  if (topic == topics.base + "/stow/set") {
    if (payload == "PRESS") command.kind = CommandKind::Stow;
    return command;
  }
  for (size_t i = 0; i < cwc::kAxisCount; ++i) {
    const std::string axis = topics.base + "/" + kAxisNames[i];
    CommandKind kind = topic == axis + "/position/set"    ? CommandKind::Position
                       : topic == axis + "/max_speed/set" ? CommandKind::MaxSpeed
                                                          : CommandKind::None;
    if (kind == CommandKind::None) continue;
    if (parseNumber(payload, command.value)) {
      command.kind = kind;
      command.axis = cwc::kAxes[i];
    }
    return command;
  }
  return command;
}

}  // namespace home_assistant
