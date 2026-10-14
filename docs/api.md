# Device API

The robot's HTTP and WebSocket API, served on port 80 at `http://<device-name>.local/`. The Device interface uses only this API, and it is open to third parties: scripts, dashboards and the Native integration for Home Assistant.

Every command goes through the Robot core, so Travel limits, speed limits and priorities apply whatever the source.

Angles are in degrees, as sent to the servos (0–180).

## API version

The current API version is **1**. `GET /api/info` returns it as `api`, and the robot announces it on the network (see [Discovery](#discovery-mdns)), so an integration can check it before using the robot.

The API version goes up only when a change would break an existing integration: a route or field removed or renamed, a meaning or a unit changed. Additive changes (a new route, a new field, a new value) keep it; clients ignore fields they don't know.

| API version | First firmware | What it has |
|-------------|----------------|-------------|
| 1 | the release after 0.2.0-beta.1 | Everything on this page, including `api` and `id` in `GET /api/info`, `POST /api/position` and the `_catwandconnect._tcp` service. A robot whose `/api/info` has no `api` predates it. |

## Discovery (mDNS)

Once on Wi-Fi, the robot answers at `<hostname>.local` and advertises two services on port 80:

- `_http._tcp`, for browsers and network tools;
- `_catwandconnect._tcp`, which the Native integration looks for (Home Assistant's zeroconf), with these TXT records:

| Key | Value | Example |
|-----|-------|---------|
| `id` | Device ID: 8 hex digits, drawn once and kept in flash. Stable across renames, IP changes and updates; use it to tell robots apart. | `3f9a12c4` |
| `api` | API version. | `1` |
| `version` | Firmware version. | `0.2.0` |
| `name` | The robot's name, as the owner typed it (UTF-8). | `CatWandConnect` |

The records are set at boot; renaming the robot restarts it, so `name` follows.

## Settings PIN

The owner can set an optional PIN (off by default). While it is on, the requests marked 🔒 below answer `401 {"error":"pin_required"}` unless they carry a session token in an `Authorization: Bearer <token>` header. Manual control (including `/api/position`), Stow, the speed slider (`playSpeed`), the Play session (`/api/play` and `sessionMinutes`) and every `GET` never need it.

| Request | Response |
|---------|----------|
| `GET /api/auth` | `{"enabled":true,"authorized":false,"lockedFor":0}`. `authorized` says whether the token sent, if any, is valid. |
| `POST /api/auth/unlock` with `{"pin":"2468"}` | `200 {"token":"…"}`. A wrong PIN returns `401 {"error":"wrong_pin"}`. After 5 wrong PINs, unlocking is refused for 30 s, doubling with each further failure up to 15 min: `429 {"error":"locked_out","retryAfter":30}`. |
| 🔒 `PUT /api/auth/pin` with `{"pin":"2468"}` | Sets or changes the PIN (4–8 digits) and returns `{"enabled":true,"token":"…"}`, a fresh token for the caller. `{"pin":""}` removes it. An invalid PIN returns `400 invalid_pin`. |

Tokens live in the robot's memory. They end after 12 h without use, when the PIN changes, or when the robot restarts. A forgotten PIN can only be cleared by erasing and reflashing the robot. Over plain HTTP the PIN and tokens are not encrypted, so the PIN keeps guests from changing the setup but is no defence against someone sniffing your Wi-Fi.

## WebSocket: `/ws`

JSON text messages, one per frame, each with a type in `t`.

### Client → robot

| Message | Effect |
|---------|--------|
| `{"t":"move","h":0.5,"v":-1}` | Manual control. `h` and `v` are joystick deflections in [-1, 1] for the Horizontal and Vertical Axis, as a fraction of the maximum speed. Values outside the range are capped. |
| `{"t":"stop"}` | Ends Manual control. The wand decelerates and stops. |
| `{"t":"stow"}` | Stow: stops everything else, moves the wand to the Stow position and releases the servos there. `move` is ignored until Stow completes. |

**Dead-man rule:** while moving, resend `move` at least every 500 ms; the interface sends it every 100 ms. If no `move` arrives for 500 ms, or if any client disconnects, the robot stops.

### Robot → client

`state` is sent when a client connects, whenever something changes (at most every 100 ms), and every 2 s otherwise:

```json
{"t":"state","h":92.5,"v":88.0,"activity":"playing","held":true,"playSpeed":0.6,"sessionMinutes":10,"play":{"remaining":587}}
```

| Field | Meaning |
|-------|---------|
| `h`, `v` | Current position of each Axis. |
| `activity` | `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` or `playing`. |
| `held` | Whether the servos are being driven. They are released only once the wand rests at the Stow position. |
| `playSpeed` | The speed slider's value, so every open interface stays in sync. |
| `calibrated` | Whether Calibration has been completed. |
| `calibration` | The current Calibration step, `none` when not calibrating (see below). |
| `tuning` | Only while `activity` is `tuning_speed`: the Speed tuning object (see below). |
| `sessionMinutes` | The Session duration, in minutes, so every open interface stays in sync. |
| `play` | Only while `activity` is `playing`: `{"remaining": <seconds>}`, the time left in the Play session, rounded up. |
| `name` | The robot's name. |

## HTTP

### Stow

`POST /api/stow` (no body) does the same as the `stow` WebSocket message and returns `202`.

### Position

`POST /api/position` with `{"axis":"horizontal","degrees":90}` moves one Axis to a position and returns `202`: Manual control by position, the same as setting a position from Home Assistant over MQTT. The axis goes there at the Manual control speed and stops; the target is clamped into the Travel limits (before Calibration, into the default 60–120°). Like any Manual control, it ends a Play session without Stow, and it is ignored while the robot stows. `axis` is `horizontal` or `vertical`; any other axis, or a `degrees` that is not a number, returns `400 {"error":"invalid_request"}`. No PIN.

### Priorities

At power-up the robot commands the position where it last rested (clamped into the Travel limits) and then Stows at the slowest speed, 10 °/s; `activity` is `stowing` meanwhile.

Stow beats every other command and cannot be interrupted. Manual control, from any source, cancels any other activity, a Play session included. Nothing resumes on its own afterwards.

### Play session

The wand plays on its own for the Session duration, then Stows. Each Axis loops on its own, never in step with the other: it moves to a random position anywhere within its Travel limits, at a random speed within its Speed range (`minSpeed`–`maxSpeed`), then rests for a random time within its Pause range (`pauseMin`–`pauseMax`), and starts again.

`GET /api/play` returns:

```json
{"active":true,"remaining":587,"minutes":10}
```

`active` says whether a session runs, `remaining` is the time left in seconds (0 when none runs) and `minutes` the Session duration.

`POST /api/play` (no PIN) with `{"action": …}` drives it and returns the same object:

| Action | Effect |
|--------|--------|
| `{"action":"start"}` | Starts a session of the Session duration, from zero even if one is running. |
| `{"action":"start","sessionMinutes":20}` | Same with that duration (1–60), which becomes the Session duration. |
| `{"action":"stop"}` | Ends the session with Stow, without finishing the move in progress. Does nothing when no session runs. |

Errors: `409 {"error":"not_calibrated"}` before Calibration, `409 {"error":"busy"}` while stowing, calibrating or tuning a speed, `400 session_duration_out_of_range`, `400 invalid_request` for an unknown action.

A session ends with Stow when its time is up, on `stop` and on Stow from any source. Manual control (the joystick, or a position from Home Assistant) ends it at once, without Stow, leaving the wand to the person. Closing the page or losing the connection does not stop it. It never resumes, and it does not survive a restart.

To change the Session duration without starting a session, send `PUT /api/settings` with only `{"sessionMinutes": 15}` (no PIN, like `playSpeed`). It is written to flash once the robot is idle, and only when it changed.

### Calibration

`GET /api/calibration` returns the current step and the values recorded so far:

```json
{"step":"horizontal_max","draft":{"horizontal":{"min":32,"max":120,"stow":90},"vertical":{"min":60,"max":120,"stow":90}}}
```

🔒 `POST /api/calibration` with `{"action": …}` drives it and returns the same object:

| Action | Effect |
|--------|--------|
| `{"action":"start"}` | Starts Calibration from the current Travel limits. |
| `{"action":"nudge","axis":"horizontal","degrees":-5}` | Moves the step's axis (both axes in the `stow` step) by that many degrees, at 20 °/s, anywhere in 0–180°. |
| `{"action":"mark"}` | Records the current position for the step and moves to the next. Returns `400` with `travel_order` or `stow_out_of_travel` (and `axis`) when the position is not acceptable. |
| `{"action":"confirm"}` | In the `confirm` step, saves the new limits and Stow position, marks the robot calibrated and Stows. |
| `{"action":"cancel"}` | Abandons Calibration, keeping the previous settings, and Stows. |
| `{"action":"reverse","axis":"horizontal"}` | Only on that axis's minimum or maximum step. Reverses the axis's direction without moving the wand or restarting: every angle of the axis (draft, saved limits and Stow position) is mirrored (`a` → `180 − a`), the wiring is saved with the new direction, and the axis starts over at its minimum step. `400 invalid_request` on any other step. |

The steps are, in order: `horizontal_min`, `horizontal_max`, `vertical_min`, `vertical_max`, `stow`, then `sweep` (the robot visits the four corners of the new envelope and returns to Stow by itself), then `confirm`. Stow and Manual control abandon Calibration too.

### Speed tuning

`GET /api/speed-tuning` returns `{"active":false}`, or while tuning:

```json
{"active":true,"axis":"horizontal","bound":"max","speed":110}
```

🔒 `POST /api/speed-tuning` with `{"action": …}` drives it and returns the same object:

| Action | Effect |
|--------|--------|
| `{"action":"start","axis":"horizontal","bound":"max"}` | Starts sweeping the axis between its Travel limits at its current maximum (`max`) or minimum (`min`) speed; the other axis holds still. `400 invalid_request` before Calibration, while stowing or calibrating. |
| `{"action":"faster"}` / `{"action":"slower"}` | One step: 2 °/s below 30 °/s, 5 °/s up to 90 °/s, 10 °/s above. A maximum stays within 10–360 °/s, a minimum within 1–360 °/s. |
| `{"action":"keep"}` | Saves the trial speed and Stows. Keeping a maximum at or below the minimum resets the minimum to a tenth of the new maximum. A minimum not below the maximum returns `400 min_speed_out_of_range` and the tuning goes on. |
| `{"action":"cancel"}` | Keeps the previous value and Stows. |

Stow and Manual control end Speed tuning too, without saving.

### Identity and info

`GET /api/info` returns what the Info page shows:

```json
{"name":"CatWandConnect","hostname":"catwandconnect.local","version":"0.2.0","api":1,"id":"3f9a12c4","chip":"ESP32-D0WD-V3",
 "chipRevision":3,"cores":2,"cpuMhz":240,"flashMb":4,"freeHeap":182000,"mac":"24:6F:28:00:00:01",
 "ssid":"Home","ip":"192.168.1.20","rssi":-61,"uptime":3600,"temperature":47.5}
```

`api` is the [API version](#api-version) and `id` the Device ID (the same as the `id` TXT record). `uptime` is in seconds, `rssi` in dBm and `temperature` in °C. The internal sensor is rough, so treat it as a trend.

`GET /api/diagnostics` returns the Diagnostic report as plain text (`text/plain`): firmware version, chip, a random Device ID (drawn once, kept in flash, not personal, so support can group the reports of one robot without its MAC address), last reset reason, uptime, memory, settings PIN state, Wi-Fi and Home Assistant (MQTT) state, settings, wiring and the last 200 log lines (at most 12 KB). It is redacted by default: the robot's name and hostname (unless they are the default), the Wi-Fi network, IP and MAC addresses, the broker address and the MQTT user are replaced by placeholders such as `<wifi-network>`, in the log lines too. `GET /api/diagnostics?full=1` keeps them, for local use only. The MQTT password is never included.

`/api/diagnostics/pending` holds the Pending report, a problem described on the hotspot and sent later:

| Method | Body | Answer |
|---|---|---|
| `GET` | | `{"pending": true, "description": "...", "report": "..."}` or `{"pending": false}` |
| `PUT` | `{"description": "..."}` | Saves the description (600 characters at most) with the redacted report the robot builds itself, replacing any previous one. The report keeps its summary and its most recent log lines within 3 KB. `400` without a description. |
| `DELETE` | | Discards it. |

The robot never sends it: the Device interface offers it on the main screen once the robot is on the owner's network.

```text
CatWandConnect diagnostic report
Redacted: yes

Firmware: 0.1.0-beta.2
Last reset: power-on
Wi-Fi network: <wifi-network>
...
Recent log:
[    1.204] [wifi] connecting to "<wifi-network>"
```

🔒 `PUT /api/device` with `{"name":"CatWandConnect salon"}` renames the robot and returns `{"name":…,"hostname":"catwandconnect-salon.local","restart":true}`. The robot restarts about a second later.

- **Valid names:** 1 to 32 bytes, no control characters, at least one letter or digit. Otherwise the request returns `400` with `invalid_name`.
- **Derived names:** the hostname is the name in lowercase ASCII, with accents removed and other characters turned into hyphens. The hotspot is called `<name>-XXXX`.

### Firmware update

🔒 `POST /api/update` with a `multipart/form-data` body holding the app image (`firmware.bin`, the `-ota.bin` file of a release).

- **Before writing**, the robot checks that the file is an ESP image built for its chip. It refuses `not_firmware` and `wrong_chip` with `400`.
- **While writing**, it stops Manual control and writes the image to the inactive OTA slot. Only a complete, verified image is marked bootable.
- **Success:** `200 {"ok":true,"restart":true}`, and the robot restarts on the new firmware about a second later.
- **Failure:** `400 {"error":"update_failed","detail":"…"}`. If the upload is interrupted, the robot keeps running its current firmware.

Settings, Calibration, Wi-Fi, name and PIN live in NVS and are kept. A Play session in progress ends when the update starts. Example: `curl -F firmware=@catwandconnect-esp32-ota.bin http://catwandconnect.local/api/update`.

### Home Assistant (MQTT) settings

`GET /api/mqtt` returns `{"enabled":false,"host":"","port":1883,"user":"","passwordSet":false,"connected":false,"error":"","baseTopic":"catwandconnect/catwandconnect"}`. The password is never returned.

🔒 `PUT /api/mqtt` with any of `enabled`, `host`, `port`, `user` and `password` saves them and reconnects; a missing `password` keeps the current one. Returns the same object as `GET`.

- **Invalid:** `400` with `mqtt_host_required` (when enabling without a host) or `mqtt_port_invalid`.
- **`error`** reports the last connection failure: `unreachable`, `bad_credentials` or `refused`.

### Hardware

`GET /api/hardware` returns the wiring and the pins this chip allows:

```json
{
  "chip": "ESP32",
  "allowedPins": [4, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33],
  "axes": {
    "horizontal": {"pin": 18, "inverted": false, "minPulse": 400, "maxPulse": 2200},
    "vertical":   {"pin": 19, "inverted": false, "minPulse": 600, "maxPulse": 2400}
  }
}
```

🔒 `PUT /api/hardware` with any subset of `axes` changes it.

- **Allowed pins** are those the chip can safely use. Flash, PSRAM, USB, serial-console, strapping and input-only pins are excluded, and the two servos can't share a pin.
- **Pulses** (`minPulse` at 0°, `maxPulse` at 180°) must be between 300 and 2700 µs, at least 500 µs apart.
- **Valid:** `200` with the new wiring plus `"restart": true`, and the robot restarts about a second later to apply it. `"calibrationReset": true` means the change in direction or pulse range made the old Travel limits meaningless: Calibration is forgotten and movement is back to 60°–120°.
- **Invalid:** `400` with `pin_not_allowed`, `pin_duplicate` or `pulse_out_of_range`, plus `axis`.

### Settings

`GET /api/settings` returns the Robot core's settings:

```json
{
  "playSpeed": 0.6,
  "calibrated": false,
  "axes": {
    "horizontal": {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "minSpeed": 9, "maxAccel": 360, "pauseMin": 0.5, "pauseMax": 3},
    "vertical":   {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "minSpeed": 9, "maxAccel": 360, "pauseMin": 1, "pauseMax": 5}
  },
  "sessionMinutes": 10
}
```

| Field | Meaning | Valid range |
|-------|---------|-------------|
| `playSpeed` | The main screen's speed slider: Manual control runs at `minSpeed` at 0.1 and at `maxSpeed` at 1, linearly in between. | 0.1–1 |
| `calibrated` | Whether Calibration has been completed. Until then, movement stays within 60°–120° whatever `min` and `max` say. Read-only: only Calibration sets it. | |
| `min`, `max` | Travel limits, in degrees. | 0–180, `min` < `max` |
| `stow` | Stow position, in degrees. | within `min`–`max` |
| `maxSpeed` | Maximum speed, in °/s. | 10–360 |
| `minSpeed` | Minimum speed, in °/s: Manual control's speed at the bottom of the speed slider. Defaults to a tenth of `maxSpeed`. | 1 to below `maxSpeed` |
| `maxAccel` | Acceleration, in °/s². | 20–2000 |
| `pauseMin`, `pauseMax` | Pause range, in seconds: how long the axis rests between two moves of a Play session. `0` means non-stop. | 0–60, `pauseMin` ≤ `pauseMax` |
| `sessionMinutes` | Session duration: how long a Play session lasts, in minutes. Whole numbers. | 1–60 |

🔒 `PUT /api/settings` (except a body containing only `playSpeed` and/or `sessionMinutes`) with a JSON body containing any subset of these fields changes them; the fields left out keep their values. The settings are validated as a whole:

- **Valid:** `200` with the full new settings. They are saved and survive reboots and firmware updates.
- **Invalid:** `400` with `{"error":"<code>","axis":"horizontal"|"vertical"}`, and nothing changes. The codes are `travel_order`, `travel_out_of_range`, `stow_out_of_travel`, `speed_out_of_range`, `min_speed_out_of_range`, `accel_out_of_range`, `pause_out_of_range`, `pause_order`, `play_speed_out_of_range` and `session_duration_out_of_range` (these two without `axis`), and `invalid_json`.

Each successful `PUT` writes to flash, so send it when a value is settled, not on every slider step.

### Wi-Fi

| Request | Response |
|---------|----------|
| `GET /api/wifi/status` | `{"mode":"hotspot"\|"connecting"\|"connected","failed":bool,"ssid":"…","ip":"…","host":"catwandconnect.local"}`. `failed` is true when the last attempt failed. |
| `GET /api/wifi/networks` | `{"scanning":true}` while a scan runs (poll again), then `{"scanning":false,"networks":[{"ssid":"…","rssi":-60,"secure":true}]}`. |
| 🔒 `POST /api/wifi` | Form fields `ssid` and `password`. Returns 202 and connects in the background; follow progress with `/api/wifi/status`. Only accepted while the hotspot is up (409 otherwise). |

### Interface files

`/` serves the Wi-Fi setup page while the robot is not on a Wi-Fi network, and the main screen once it is. The interface's files (`/app.js`, `/lang/en.json`, `/manifest.webmanifest`, …) are served gzip-compressed.

## Home Assistant (MQTT)

When enabled in Settings, the robot connects to an MQTT broker and announces itself with Home Assistant MQTT discovery. Setup is described in [Home Assistant](home-assistant.md). Topics use the robot's hostname, for example `catwandconnect/catwandconnect-salon`, written `<base>` below.

| Topic | Direction | Payload |
|-------|-----------|---------|
| `<base>/status` | robot → | `online` / `offline` (retained; `offline` is the last will) |
| `<base>/state` | robot → | `{"h":92,"v":88,"hs":90,"vs":90,"activity":"idle","play":"OFF","sm":10}` (retained): positions in degrees, maximum speeds in °/s; `activity` as in `/ws` (`playing` during a Play session); `play` is `ON` while a Play session runs; `sm` is the Session duration in minutes |
| `<base>/stow/set` | → robot | `PRESS` |
| `<base>/horizontal/position/set`, `<base>/vertical/position/set` | → robot | A number of degrees. The axis moves there at the Manual control speed, clamped into the Travel limits. |
| `<base>/horizontal/max_speed/set`, `<base>/vertical/max_speed/set` | → robot | A number of °/s. Validated like the Settings value. |
| `<base>/play_session/set` | → robot | `ON` starts a Play session for the Session duration, `OFF` stops it (Stow). A refused start (not calibrated, busy) is logged and the state is republished with `play` still `OFF`. |
| `<base>/session_duration/set` | → robot | Whole minutes, 1–60. Saved once the robot is idle; other values are ignored. |
| `homeassistant/<component>/catwandconnect_<mac>/<object>/config` | robot → | Discovery messages (retained) for the Stow button, the five numbers, the activity sensor and the Play session switch. |

Commands go through the Robot core like Manual control: Travel limits, speed limits and priorities apply, and Stow beats everything.
