#include <diagnostics/log_buffer.h>
#include <diagnostics/redactor.h>
#include <diagnostics/report.h>
#include <unity.h>

using namespace diagnostics;

void setUp() {}
void tearDown() {}

void test_log_lines_are_kept_in_order_with_their_uptime() {
  LogBuffer log(10, 1000);
  log.add(1500, "[wifi] connecting");
  log.add(62250, "[wifi] connected");
  TEST_ASSERT_EQUAL_STRING("[    1.500] [wifi] connecting\n[   62.250] [wifi] connected\n", log.text().c_str());
}

void test_oldest_lines_are_dropped_beyond_the_line_limit() {
  LogBuffer log(2, 1000);
  log.add(1000, "one");
  log.add(2000, "two");
  log.add(3000, "three");
  TEST_ASSERT_EQUAL_STRING("[    2.000] two\n[    3.000] three\n", log.text().c_str());
}

void test_oldest_lines_are_dropped_beyond_the_byte_limit() {
  LogBuffer log(100, 40);  // each stamped line below is 18 bytes with its newline
  log.add(1000, "aaaaa");
  log.add(2000, "bbbbb");
  log.add(3000, "ccccc");
  TEST_ASSERT_EQUAL_STRING("[    2.000] bbbbb\n[    3.000] ccccc\n", log.text().c_str());
}

void test_known_values_are_replaced_everywhere() {
  Redactor redactor;
  redactor.add("Maison Dupont", "wifi-network");
  redactor.add("mqtt.dupont.lan", "broker");
  std::string text = "Wi-Fi network: Maison Dupont\n[wifi] connecting to \"Maison Dupont\"\n[mqtt] connected to mqtt.dupont.lan:1883\n";
  TEST_ASSERT_EQUAL_STRING(
      "Wi-Fi network: <wifi-network>\n[wifi] connecting to \"<wifi-network>\"\n[mqtt] connected to <broker>:1883\n",
      redactor.apply(text).c_str());
}

void test_ip_and_mac_addresses_are_replaced_without_being_told() {
  Redactor redactor;
  std::string text = "[wifi] connected, open http://192.168.1.20/ (MAC 24:6F:28:AB:cd:01), gateway 10.0.0.1.";
  TEST_ASSERT_EQUAL_STRING("[wifi] connected, open http://<ip>/ (MAC <mac>), gateway <ip>.", redactor.apply(text).c_str());
}

void test_versions_and_numbers_that_are_not_addresses_are_kept() {
  Redactor redactor;
  std::string text = "Firmware: 0.1.0-beta.2, uptime [ 62.250], 1.2.3.4.5, 300.1.1.1, 12:34:56";
  TEST_ASSERT_EQUAL_STRING(text.c_str(), redactor.apply(text).c_str());
}

void test_empty_values_are_ignored() {
  Redactor redactor;
  redactor.add("", "broker");
  TEST_ASSERT_EQUAL_STRING("MQTT: disabled", redactor.apply("MQTT: disabled").c_str());
}

void test_a_value_inside_a_longer_value_does_not_leave_part_of_the_longer_one() {
  Redactor redactor;
  redactor.add("home", "mqtt-user");
  redactor.add("home-wifi", "wifi-network");
  TEST_ASSERT_EQUAL_STRING("<wifi-network> / <mqtt-user>", redactor.apply("home-wifi / home").c_str());
}

ReportData sample() {
  ReportData d;
  d.name = "Robot Dupont";
  d.hostname = "robot-dupont";
  d.version = "0.1.0-beta.2";
  d.chip = "ESP32 rev 3, 2 x 240 MHz, 4 MB flash";
  d.resetReason = "power-on";
  d.uptimeS = 3725;
  d.freeHeap = 184320;
  d.minFreeHeap = 153600;
  d.temperature = 45.1f;
  d.pinEnabled = true;
  d.wifiConnected = true;
  d.wifiNetwork = "Maison Dupont";
  d.ip = "192.168.1.20";
  d.mac = "24:6F:28:AB:CD:01";
  d.rssi = -61;
  d.mqttEnabled = true;
  d.mqttConnected = false;
  d.mqttHost = "mqtt.dupont.lan";
  d.mqttPort = 1883;
  d.mqttUser = "dupont";
  d.mqttError = "connection refused";
  d.settings = "{\"playSpeed\":0.5,\"calibrated\":true}";
  d.hardware = "{\"chip\":\"esp32\"}";
  d.log = "[    1.200] [wifi] connecting to \"Maison Dupont\"\n"
          "[    3.400] [wifi] connected, open http://192.168.1.20/ or http://robot-dupont.local/\n"
          "[   13.000] [mqtt] connection to mqtt.dupont.lan:1883 failed (state -2)\n";
  return d;
}

bool contains(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }

void test_report_holds_what_is_needed_to_diagnose() {
  std::string report = reportText(sample(), true);
  TEST_ASSERT_TRUE(contains(report, "Firmware: 0.1.0-beta.2\n"));
  TEST_ASSERT_TRUE(contains(report, "Chip: ESP32 rev 3, 2 x 240 MHz, 4 MB flash\n"));
  TEST_ASSERT_TRUE(contains(report, "Last reset: power-on\n"));
  TEST_ASSERT_TRUE(contains(report, "Running for: 0 d 1 h 2 min (3725 s)\n"));
  TEST_ASSERT_TRUE(contains(report, "Free memory: 180 KB (lowest 150 KB)\n"));
  TEST_ASSERT_TRUE(contains(report, "Chip temperature: 45.1 C\n"));
  TEST_ASSERT_TRUE(contains(report, "Settings PIN: on\n"));
  TEST_ASSERT_TRUE(contains(report, "Wi-Fi: connected, signal -61 dBm\n"));
  TEST_ASSERT_TRUE(contains(report, "Home Assistant (MQTT): enabled, not connected, last error: connection refused\n"));
  TEST_ASSERT_TRUE(contains(report, "{\"playSpeed\":0.5,\"calibrated\":true}"));
  TEST_ASSERT_TRUE(contains(report, "{\"chip\":\"esp32\"}"));
  TEST_ASSERT_TRUE(contains(report, "[mqtt] connection to <broker>:1883 failed (state -2)"));
}

void test_redacted_report_holds_no_personal_data() {
  std::string report = reportText(sample(), true);
  for (const char* secret : {"Maison Dupont", "192.168.1.20", "24:6F:28:AB:CD:01", "mqtt.dupont.lan", "Robot Dupont",
                             "robot-dupont", "Dupont", "dupont"}) {
    TEST_ASSERT_FALSE_MESSAGE(contains(report, secret), secret);
  }
  TEST_ASSERT_TRUE(contains(report, "Redacted: yes"));
  TEST_ASSERT_TRUE(contains(report, "Wi-Fi network: <wifi-network>\n"));
  TEST_ASSERT_TRUE(contains(report, "[wifi] connecting to \"<wifi-network>\""));
  TEST_ASSERT_TRUE(contains(report, "open http://<ip>/ or http://<hostname>.local/"));
  TEST_ASSERT_TRUE(contains(report, "MQTT broker: <broker>:1883, user <mqtt-user>\n"));
}

void test_full_report_keeps_personal_data_for_local_use() {
  std::string report = reportText(sample(), false);
  TEST_ASSERT_TRUE(contains(report, "Redacted: no"));
  TEST_ASSERT_TRUE(contains(report, "Name: Robot Dupont\n"));
  TEST_ASSERT_TRUE(contains(report, "Wi-Fi network: Maison Dupont\n"));
  TEST_ASSERT_TRUE(contains(report, "IP address: 192.168.1.20\n"));
  TEST_ASSERT_TRUE(contains(report, "MAC address: 24:6F:28:AB:CD:01\n"));
  TEST_ASSERT_TRUE(contains(report, "MQTT broker: mqtt.dupont.lan:1883, user dupont\n"));
}

void test_default_name_stays_readable_when_redacted() {
  ReportData d = sample();
  d.name = "CatWandConnect";
  d.hostname = "catwandconnect";
  d.log = "[    0.010] CatWandConnect 0.1.0-beta.2 starting\n";
  std::string report = reportText(d, true);
  TEST_ASSERT_TRUE(contains(report, "Name: CatWandConnect\n"));
  TEST_ASSERT_TRUE(contains(report, "CatWandConnect 0.1.0-beta.2 starting"));
}

void test_report_says_when_wifi_or_mqtt_are_not_in_use() {
  ReportData d = sample();
  d.wifiConnected = false;
  d.wifiNetwork = "";
  d.mqttEnabled = false;
  d.mqttHost = "";
  d.mqttUser = "";
  std::string report = reportText(d, true);
  TEST_ASSERT_TRUE(contains(report, "Wi-Fi: not connected (setup hotspot or connecting)\n"));
  TEST_ASSERT_TRUE(contains(report, "Wi-Fi network: none saved\n"));
  TEST_ASSERT_TRUE(contains(report, "Home Assistant (MQTT): disabled\n"));
  TEST_ASSERT_FALSE(contains(report, "MQTT broker:"));
}

void test_personal_values_noted_while_logging_are_redacted() {
  ReportData d = sample();
  d.log += "[   40.000] [wifi] could not connect to \"Voisin Free\"\n";
  d.notedPersonal.push_back({"Voisin Free", "wifi-network"});
  std::string report = reportText(d, true);
  TEST_ASSERT_FALSE(contains(report, "Voisin Free"));
  TEST_ASSERT_TRUE(contains(report, "could not connect to \"<wifi-network>\""));
  TEST_ASSERT_TRUE(contains(reportText(d, false), "Voisin Free"));
}

void test_values_are_only_replaced_as_whole_words() {
  Redactor redactor;
  redactor.add("C", "name");
  redactor.add("chat", "wifi-network");
  TEST_ASSERT_EQUAL_STRING("Chip: ESP32, chatty <wifi-network> \"<name>\"",
                           redactor.apply("Chip: ESP32, chatty chat \"C\"").c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_log_lines_are_kept_in_order_with_their_uptime);
  RUN_TEST(test_oldest_lines_are_dropped_beyond_the_line_limit);
  RUN_TEST(test_oldest_lines_are_dropped_beyond_the_byte_limit);
  RUN_TEST(test_known_values_are_replaced_everywhere);
  RUN_TEST(test_ip_and_mac_addresses_are_replaced_without_being_told);
  RUN_TEST(test_versions_and_numbers_that_are_not_addresses_are_kept);
  RUN_TEST(test_empty_values_are_ignored);
  RUN_TEST(test_a_value_inside_a_longer_value_does_not_leave_part_of_the_longer_one);
  RUN_TEST(test_report_holds_what_is_needed_to_diagnose);
  RUN_TEST(test_redacted_report_holds_no_personal_data);
  RUN_TEST(test_full_report_keeps_personal_data_for_local_use);
  RUN_TEST(test_default_name_stays_readable_when_redacted);
  RUN_TEST(test_report_says_when_wifi_or_mqtt_are_not_in_use);
  RUN_TEST(test_personal_values_noted_while_logging_are_redacted);
  RUN_TEST(test_values_are_only_replaced_as_whole_words);
  return UNITY_END();
}
