// The Play session journal and the Robot clock (lib/play_journal).

#include <ArduinoJson.h>
#include <play_journal/play_journal.h>
#include <unity.h>

#include "../fakes.h"

using namespace play_journal;
using cwc::test::FakeClock;
using cwc::test::FakeStorage;

namespace {

// 2026-10-23 16:00:00 UTC, a Friday.
constexpr int64_t kFridayFourPmMs = 1792771200000LL;
constexpr uint32_t kFridayFourPmS = 1792771200u;

struct Rig {
  FakeClock uptime;
  FakeStorage storage;
  RobotClock clock{uptime};
  Journal journal{storage, clock};

  Rig() { journal.begin(); }

  void advanceS(uint32_t seconds) { uptime.advance(seconds * 1000); }
};

JsonDocument parse(const std::string& text) {
  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, text));
  return doc;
}

}  // namespace

void setUp() {}
void tearDown() {}

// --- Robot clock --------------------------------------------------------------

void test_the_clock_is_unset_after_boot() {
  Rig rig;
  TEST_ASSERT_FALSE(rig.clock.isSet());
  TEST_ASSERT_EQUAL_INT64(0, rig.clock.nowMs());
}

void test_the_clock_runs_from_the_time_it_was_given() {
  Rig rig;
  rig.advanceS(30);
  TEST_ASSERT_TRUE(rig.clock.set(kFridayFourPmMs, 120, "Europe/Paris"));
  rig.advanceS(90);
  TEST_ASSERT_TRUE(rig.clock.isSet());
  TEST_ASSERT_EQUAL_INT64(kFridayFourPmMs + 90000, rig.clock.nowMs());
  TEST_ASSERT_EQUAL(120, rig.clock.offsetMinutes());
  TEST_ASSERT_EQUAL_STRING("Europe/Paris", rig.clock.zone().c_str());
}

void test_the_clock_refuses_an_implausible_time_offset_or_zone() {
  Rig rig;
  TEST_ASSERT_FALSE(rig.clock.set(0, 0, "UTC"));                                  // 1970
  TEST_ASSERT_FALSE(rig.clock.set(4200000000000LL, 0, "UTC"));                    // 2103
  TEST_ASSERT_FALSE(rig.clock.set(kFridayFourPmMs, 900, "UTC"));                  // UTC+15:00
  TEST_ASSERT_FALSE(rig.clock.set(kFridayFourPmMs, -800, "UTC"));                 // UTC-13:20
  TEST_ASSERT_FALSE(rig.clock.set(kFridayFourPmMs, 0, "Europe/Paris\"<b>"));      // not a zone name
  TEST_ASSERT_FALSE(rig.clock.set(kFridayFourPmMs, 0, std::string(65, 'A')));     // too long
  TEST_ASSERT_FALSE(rig.clock.isSet());
  TEST_ASSERT_TRUE(rig.clock.set(kFridayFourPmMs, -300, "America/Argentina/Buenos_Aires"));
  TEST_ASSERT_TRUE(rig.clock.set(kFridayFourPmMs, 0, ""));  // a browser may not name its zone
}

void test_uptime_keeps_counting_past_the_millisecond_counter_wrap() {
  Rig rig;
  rig.uptime.now = 0xFFFFF000u;
  TEST_ASSERT_EQUAL_UINT64(0xFFFFF000ull, rig.clock.uptimeMs());
  rig.uptime.now = 0x1000u;  // wrapped after 49.7 days
  TEST_ASSERT_EQUAL_UINT64(0x100001000ull, rig.clock.uptimeMs());
}

// --- Recording ----------------------------------------------------------------

void test_records_events_newest_first_with_increasing_ids() {
  Rig rig;
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.journal.record(What::Stopped, Source::HomeAssistantMqtt, Reason::Stop);
  TEST_ASSERT_EQUAL(2, rig.journal.size());
  TEST_ASSERT_EQUAL_UINT32(2, rig.journal.newest(0).id);
  TEST_ASSERT_TRUE(rig.journal.newest(0).what == What::Stopped);
  TEST_ASSERT_TRUE(rig.journal.newest(0).source == Source::HomeAssistantMqtt);
  TEST_ASSERT_TRUE(rig.journal.newest(0).reason == Reason::Stop);
  TEST_ASSERT_EQUAL_UINT32(1, rig.journal.newest(1).id);
  TEST_ASSERT_TRUE(rig.journal.newest(1).reason == Reason::None);
  TEST_ASSERT_EQUAL_UINT32(2, rig.journal.lastId());
}

void test_keeps_the_last_50_events() {
  Rig rig;
  for (int i = 0; i < 60; ++i) rig.journal.record(What::Started, Source::DeviceInterface);
  TEST_ASSERT_EQUAL(50, rig.journal.size());
  TEST_ASSERT_EQUAL_UINT32(60, rig.journal.newest(0).id);
  TEST_ASSERT_EQUAL_UINT32(11, rig.journal.newest(49).id);
}

void test_an_event_before_the_clock_is_set_has_its_age_but_no_time() {
  Rig rig;
  rig.advanceS(100);
  rig.journal.record(What::Refused, Source::HomeAssistantMqtt, Reason::Busy);
  rig.advanceS(720);
  const Event& event = rig.journal.newest(0);
  TEST_ASSERT_EQUAL_UINT32(0, event.epochS);
  TEST_ASSERT_EQUAL_INT64(720, rig.journal.ageS(event));
}

void test_an_event_after_the_clock_is_set_has_its_time() {
  Rig rig;
  rig.clock.set(kFridayFourPmMs, 120, "Europe/Paris");
  rig.advanceS(60);
  rig.journal.record(What::Started, Source::HomeAssistantNative);
  rig.advanceS(5);
  TEST_ASSERT_EQUAL_UINT32(kFridayFourPmS + 60, rig.journal.newest(0).epochS);
  TEST_ASSERT_EQUAL_INT64(5, rig.journal.ageS(rig.journal.newest(0)));
}

void test_setting_the_clock_dates_the_events_of_this_boot() {
  Rig rig;
  rig.advanceS(100);
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.advanceS(600);
  rig.clock.set(kFridayFourPmMs, 0, "UTC");
  rig.journal.clockSet();
  TEST_ASSERT_EQUAL_UINT32(kFridayFourPmS - 600, rig.journal.newest(0).epochS);

  Journal reloaded(rig.storage, rig.clock);  // and the date is saved
  reloaded.begin();
  TEST_ASSERT_EQUAL_UINT32(kFridayFourPmS - 600, reloaded.newest(0).epochS);
}

// --- Kept across restarts -----------------------------------------------------

void test_events_are_kept_across_a_restart() {
  Rig rig;
  rig.clock.set(kFridayFourPmMs, 0, "UTC");
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.journal.record(What::Ended, Source::DeviceInterface);

  FakeClock uptime;  // the next boot
  RobotClock clock{uptime};
  Journal next{rig.storage, clock};
  next.begin();
  TEST_ASSERT_EQUAL(2, next.size());
  TEST_ASSERT_TRUE(next.newest(0).what == What::Ended);
  TEST_ASSERT_EQUAL_UINT32(kFridayFourPmS, next.newest(0).epochS);
  TEST_ASSERT_FALSE(next.newest(0).thisBoot);
  next.record(What::Started, Source::HomeAssistantMqtt);
  TEST_ASSERT_EQUAL_UINT32(3, next.newest(0).id);  // ids carry on
  TEST_ASSERT_TRUE(next.newest(0).thisBoot);
}

void test_an_undated_event_from_before_a_restart_has_no_age_and_stays_undated() {
  Rig rig;
  rig.journal.record(What::Started, Source::DeviceInterface);

  FakeClock uptime;
  RobotClock clock{uptime};
  Journal next{rig.storage, clock};
  next.begin();
  uptime.advance(5000);
  TEST_ASSERT_EQUAL_INT64(-1, next.ageS(next.newest(0)));
  clock.set(kFridayFourPmMs, 0, "UTC");
  next.clockSet();
  TEST_ASSERT_EQUAL_UINT32(0, next.newest(0).epochS);  // its uptime belongs to another boot
  TEST_ASSERT_EQUAL_INT64(-1, next.ageS(next.newest(0)));
}

void test_a_missing_or_foreign_record_starts_an_empty_journal() {
  FakeClock uptime;
  RobotClock clock{uptime};
  FakeStorage storage;
  std::vector<uint8_t> foreign(808, 0);
  foreign[0] = 99;  // a record version this firmware doesn't know
  storage.blobs["journal"] = foreign;
  Journal journal{storage, clock};
  journal.begin();
  TEST_ASSERT_EQUAL(0, journal.size());
  TEST_ASSERT_EQUAL_UINT32(0, journal.lastId());
}

// --- Sessions today -----------------------------------------------------------

void test_sessions_today_is_unknown_without_a_clock() {
  Rig rig;
  rig.journal.record(What::Started, Source::DeviceInterface);
  TEST_ASSERT_EQUAL(-1, rig.journal.sessionsToday());
}

void test_sessions_today_counts_the_starts_of_the_local_day() {
  Rig rig;
  // 23:30 local the day before (UTC+2): 21:30 UTC on Thursday.
  rig.clock.set(kFridayFourPmMs - (18 * 3600 + 30 * 60) * 1000LL, 120, "Europe/Paris");
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.advanceS(45 * 60);  // 00:15 local, Friday
  rig.journal.record(What::Refused, Source::HomeAssistantMqtt, Reason::Busy);
  rig.journal.record(What::Started, Source::HomeAssistantMqtt);
  rig.journal.record(What::Ended, Source::HomeAssistantMqtt);
  rig.journal.record(What::Started, Source::DeviceInterface);
  TEST_ASSERT_EQUAL(2, rig.journal.sessionsToday());

  rig.advanceS(24 * 3600);  // the next local midnight passed
  TEST_ASSERT_EQUAL(0, rig.journal.sessionsToday());
}

void test_sessions_today_counts_starts_dated_once_the_clock_is_set() {
  Rig rig;
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.advanceS(60);
  rig.clock.set(kFridayFourPmMs, 0, "UTC");
  rig.journal.clockSet();
  TEST_ASSERT_EQUAL(1, rig.journal.sessionsToday());
}

// --- JSON and report ----------------------------------------------------------

void test_journal_json_lists_events_newest_first() {
  Rig rig;
  rig.advanceS(10);
  rig.journal.record(What::Refused, Source::HomeAssistantMqtt, Reason::NotCalibrated);
  rig.advanceS(50);
  rig.clock.set(kFridayFourPmMs, 120, "Europe/Paris");
  rig.journal.record(What::Started, Source::DeviceInterface);
  rig.advanceS(30);

  JsonDocument doc = parse(rig.journal.json());
  TEST_ASSERT_TRUE(doc["clock"]["set"]);
  TEST_ASSERT_EQUAL(120, doc["clock"]["offset"]);
  TEST_ASSERT_EQUAL_STRING("Europe/Paris", doc["clock"]["zone"]);
  TEST_ASSERT_EQUAL(1, doc["sessionsToday"]);
  TEST_ASSERT_EQUAL(2, doc["events"].size());
  JsonObject newest = doc["events"][0];
  TEST_ASSERT_EQUAL(2, newest["id"]);
  TEST_ASSERT_EQUAL_STRING("started", newest["what"]);
  TEST_ASSERT_EQUAL_STRING("device_interface", newest["source"]);
  TEST_ASSERT_TRUE(newest["reason"].isNull());
  TEST_ASSERT_EQUAL_UINT32(kFridayFourPmS, newest["at"].as<uint32_t>());
  TEST_ASSERT_EQUAL(30, newest["ago"]);
  JsonObject oldest = doc["events"][1];
  TEST_ASSERT_EQUAL_STRING("refused", oldest["what"]);
  TEST_ASSERT_EQUAL_STRING("home_assistant_mqtt", oldest["source"]);
  TEST_ASSERT_EQUAL_STRING("not_calibrated", oldest["reason"]);
  TEST_ASSERT_TRUE(oldest["at"].isNull());  // recorded before the clock was set, not dated yet
  TEST_ASSERT_EQUAL(80, oldest["ago"]);
}

void test_journal_json_without_a_clock() {
  Rig rig;
  JsonDocument doc = parse(rig.journal.json());
  TEST_ASSERT_FALSE(doc["clock"]["set"]);
  TEST_ASSERT_TRUE(doc["clock"]["offset"].isNull());
  TEST_ASSERT_TRUE(doc["sessionsToday"].isNull());
  TEST_ASSERT_EQUAL(0, doc["events"].size());
}

void test_event_json_for_the_websocket() {
  Rig rig;
  rig.journal.record(What::Stopped, Source::HomeAssistantNative, Reason::ManualControl);
  JsonDocument doc = parse(rig.journal.eventJson(rig.journal.newest(0)));
  TEST_ASSERT_EQUAL_STRING("journal", doc["t"]);
  TEST_ASSERT_EQUAL(1, doc["event"]["id"]);
  TEST_ASSERT_EQUAL_STRING("stopped", doc["event"]["what"]);
  TEST_ASSERT_EQUAL_STRING("home_assistant_native", doc["event"]["source"]);
  TEST_ASSERT_EQUAL_STRING("manual_control", doc["event"]["reason"]);
  TEST_ASSERT_EQUAL(0, doc["event"]["ago"]);
}

void test_names_cover_every_value() {
  TEST_ASSERT_EQUAL_STRING("ended", whatName(What::Ended));
  TEST_ASSERT_EQUAL_STRING("paw_tap", sourceName(Source::PawTap));
  TEST_ASSERT_EQUAL_STRING("schedule", sourceName(Source::Schedule));
  TEST_ASSERT_NULL(reasonName(Reason::None));
  TEST_ASSERT_EQUAL_STRING("rest_time", reasonName(Reason::RestTime));
  TEST_ASSERT_EQUAL_STRING("daily_limit", reasonName(Reason::DailyLimit));
  TEST_ASSERT_EQUAL_STRING("quiet_hours", reasonName(Reason::QuietHours));
  TEST_ASSERT_EQUAL_STRING("automatic_off", reasonName(Reason::AutomaticOff));
  TEST_ASSERT_EQUAL_STRING("stow", reasonName(Reason::Stow));
  TEST_ASSERT_EQUAL_STRING("speed_tuning", reasonName(Reason::SpeedTuning));
}

void test_maps_the_robot_core_reports() {
  TEST_ASSERT_TRUE(whatOf(cwc::PlayEvent::Refused) == What::Refused);
  TEST_ASSERT_TRUE(reasonOf(cwc::PlayCause::None) == Reason::None);
  TEST_ASSERT_TRUE(reasonOf(cwc::PlayCause::Busy) == Reason::Busy);
  TEST_ASSERT_TRUE(reasonOf(cwc::PlayCause::NotCalibrated) == Reason::NotCalibrated);
  TEST_ASSERT_TRUE(reasonOf(cwc::PlayCause::ManualControl) == Reason::ManualControl);
  TEST_ASSERT_TRUE(reasonOf(cwc::PlayCause::Calibration) == Reason::Calibration);
}

void test_report_text_dates_what_it_can() {
  Rig rig;
  rig.journal.record(What::Started, Source::DeviceInterface);  // from a previous boot, undated
  FakeClock uptime;
  RobotClock clock{uptime};
  Journal journal{rig.storage, clock};
  journal.begin();
  uptime.advance(60000);
  journal.record(What::Refused, Source::HomeAssistantMqtt, Reason::Busy);
  uptime.advance(12 * 60000);
  std::string unset = journal.reportText();
  TEST_ASSERT_NOT_NULL(strstr(unset.c_str(), "Robot clock: not set"));
  TEST_ASSERT_NOT_NULL(strstr(unset.c_str(), "Sessions today: unknown"));
  TEST_ASSERT_NOT_NULL(strstr(unset.c_str(), "12 min ago, before the clock was set: refused, home_assistant_mqtt, busy"));
  TEST_ASSERT_NOT_NULL(strstr(unset.c_str(), "before a restart, time unknown: started, device_interface"));

  clock.set(kFridayFourPmMs, 120, "Europe/Paris");
  journal.clockSet();
  std::string set = journal.reportText();
  TEST_ASSERT_NOT_NULL(strstr(set.c_str(), "Robot clock: set, UTC+02:00"));
  TEST_ASSERT_NOT_NULL(strstr(set.c_str(), "2026-10-23 15:48:00 UTC: refused, home_assistant_mqtt, busy"));
  TEST_ASSERT_NULL(strstr(set.c_str(), "Europe/Paris"));  // the zone hints at where the owner lives
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_the_clock_is_unset_after_boot);
  RUN_TEST(test_the_clock_runs_from_the_time_it_was_given);
  RUN_TEST(test_the_clock_refuses_an_implausible_time_offset_or_zone);
  RUN_TEST(test_uptime_keeps_counting_past_the_millisecond_counter_wrap);
  RUN_TEST(test_records_events_newest_first_with_increasing_ids);
  RUN_TEST(test_keeps_the_last_50_events);
  RUN_TEST(test_an_event_before_the_clock_is_set_has_its_age_but_no_time);
  RUN_TEST(test_an_event_after_the_clock_is_set_has_its_time);
  RUN_TEST(test_setting_the_clock_dates_the_events_of_this_boot);
  RUN_TEST(test_events_are_kept_across_a_restart);
  RUN_TEST(test_an_undated_event_from_before_a_restart_has_no_age_and_stays_undated);
  RUN_TEST(test_a_missing_or_foreign_record_starts_an_empty_journal);
  RUN_TEST(test_sessions_today_is_unknown_without_a_clock);
  RUN_TEST(test_sessions_today_counts_the_starts_of_the_local_day);
  RUN_TEST(test_sessions_today_counts_starts_dated_once_the_clock_is_set);
  RUN_TEST(test_journal_json_lists_events_newest_first);
  RUN_TEST(test_journal_json_without_a_clock);
  RUN_TEST(test_event_json_for_the_websocket);
  RUN_TEST(test_names_cover_every_value);
  RUN_TEST(test_maps_the_robot_core_reports);
  RUN_TEST(test_report_text_dates_what_it_can);
  return UNITY_END();
}
