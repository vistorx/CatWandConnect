# The Robot clock comes from the browser and Home Assistant, never NTP

The Play session journal (#239) needs the time of each event and the start of the local day for Sessions today, and the schedule (#176) will need the time and the time zone. The ESP32 has no battery-backed clock, and the robot makes no outbound connection (ADR 0012): asking an NTP server would be the first one, and a broken or blocked one would be a silent failure on networks where the robot otherwise works.

We decided the robot gets its clock from those already talking to it. The Device interface sends the browser's time, current UTC offset and IANA time zone name on every page load (`POST /api/time`, no PIN); the Native integration sends Home Assistant's when it connects. The robot keeps it in memory and runs it on from its uptime; after a restart it is unset until the next page load or connection. It uses the offset for "today"; the zone name is stored for the schedule, which will need daylight saving changes.

## Consequences

- No NTP, no outbound connection, nothing to configure. The time is as right as the phone's or Home Assistant's.
- An owner using only the MQTT connection gets the time when they open the Device interface. Until then, events carry their age since boot ("12 min ago, before the clock was set"), dated once the clock is set; Sessions today is unknown.
- Events from before a restart that were never dated stay undated: the uptime they kept belongs to another boot.
- Anyone on the Wi-Fi can set the clock, like they can start a Play session. A wrong clock only misdates the journal and Sessions today; anything that will act on the time (the schedule) has to keep that in mind.
- The offset is a snapshot: after a daylight saving change, "today" is off by an hour until the next page load or Home Assistant connection.
