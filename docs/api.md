# Device API

The robot's HTTP and WebSocket API, served on port 80 at `http://<device-name>.local/`. The Device interface uses only this API, and it is open to third parties: scripts, dashboards and a future Home Assistant integration.

Every command goes through the Robot core, so Travel limits, speed limits and priorities apply whatever the source.

Angles are in degrees, as sent to the servos (0–180).

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
{"t":"state","h":92.5,"v":88.0,"activity":"manual","held":true,"playSpeed":0.6}
```

| Field | Meaning |
|-------|---------|
| `h`, `v` | Current position of each Axis. |
| `activity` | `idle`, `manual`, `stowing` or `calibrating`. |
| `held` | Whether the servos are being driven. They are released only once the wand rests at the Stow position. |
| `playSpeed` | The speed slider's value, so every open interface stays in sync. |
| `calibrated` | Whether Calibration has been completed. |
| `calibration` | The current Calibration step, `none` when not calibrating (see below). |
| `name` | The robot's name. |

## HTTP

### Stow

`POST /api/stow` (no body) does the same as the `stow` WebSocket message and returns `202`.

### Priorities

At power-up the robot commands the position where it last rested (clamped into the Travel limits) and then Stows at the slowest speed, 10 °/s; `activity` is `stowing` meanwhile.

Stow beats every other command and cannot be interrupted. Manual control, from any source, cancels any other activity. Nothing resumes on its own afterwards.

### Calibration

`GET /api/calibration` returns the current step and the values recorded so far:

```json
{"step":"horizontal_max","draft":{"horizontal":{"min":32,"max":120,"stow":90},"vertical":{"min":60,"max":120,"stow":90}}}
```

`POST /api/calibration` with `{"action": …}` drives it and returns the same object:

| Action | Effect |
|--------|--------|
| `{"action":"start"}` | Starts Calibration from the current Travel limits. |
| `{"action":"nudge","axis":"horizontal","degrees":-5}` | Moves the step's axis (both axes in the `stow` step) by that many degrees, at 20 °/s, anywhere in 0–180°. |
| `{"action":"mark"}` | Records the current position for the step and moves to the next. Returns `400` with `travel_order` or `stow_out_of_travel` (and `axis`) when the position is not acceptable. |
| `{"action":"confirm"}` | In the `confirm` step, saves the new limits and Stow position, marks the robot calibrated and Stows. |
| `{"action":"cancel"}` | Abandons Calibration, keeping the previous settings, and Stows. |

The steps are, in order: `horizontal_min`, `horizontal_max`, `vertical_min`, `vertical_max`, `stow`, then `sweep` (the robot visits the four corners of the new envelope and returns to Stow by itself), then `confirm`. Stow and Manual control abandon Calibration too.

### Identity and info

`GET /api/info` returns what the Info page shows:

```json
{"name":"CatWandConnect","hostname":"catwandconnect.local","version":"0.1.0","chip":"ESP32-D0WD-V3",
 "chipRevision":3,"cores":2,"cpuMhz":240,"flashMb":4,"freeHeap":182000,"mac":"24:6F:28:00:00:01",
 "ssid":"Home","ip":"192.168.1.20","rssi":-61,"uptime":3600,"temperature":47.5}
```

`uptime` is in seconds, `rssi` in dBm and `temperature` in °C. The internal sensor is rough, so treat it as a trend.

`PUT /api/device` with `{"name":"CatWandConnect salon"}` renames the robot and returns `{"name":…,"hostname":"catwandconnect-salon.local","restart":true}`. The robot restarts about a second later.

- **Valid names:** 1 to 32 bytes, no control characters, at least one letter or digit. Otherwise the request returns `400` with `invalid_name`.
- **Derived names:** the hostname is the name in lowercase ASCII, with accents removed and other characters turned into hyphens. The hotspot is called `<name>-XXXX`.

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

`PUT /api/hardware` with any subset of `axes` changes it.

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
    "horizontal": {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "maxAccel": 360},
    "vertical":   {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "maxAccel": 360}
  }
}
```

| Field | Meaning | Valid range |
|-------|---------|-------------|
| `playSpeed` | Fraction of `maxSpeed` used by Manual control: the main screen's speed slider. | 0.1–1 |
| `calibrated` | Whether Calibration has been completed. Until then, movement stays within 60°–120° whatever `min` and `max` say. Read-only: only Calibration sets it. | |
| `min`, `max` | Travel limits, in degrees. | 0–180, `min` < `max` |
| `stow` | Stow position, in degrees. | within `min`–`max` |
| `maxSpeed` | Maximum speed, in °/s. | 10–360 |
| `maxAccel` | Acceleration, in °/s². | 20–2000 |

`PUT /api/settings` with a JSON body containing any subset of these fields changes them; the fields left out keep their values. The settings are validated as a whole:

- **Valid:** `200` with the full new settings. They are saved and survive reboots and firmware updates.
- **Invalid:** `400` with `{"error":"<code>","axis":"horizontal"|"vertical"}`, and nothing changes. The codes are `travel_order`, `travel_out_of_range`, `stow_out_of_travel`, `speed_out_of_range`, `accel_out_of_range`, `play_speed_out_of_range` (no `axis`) and `invalid_json`.

Each successful `PUT` writes to flash, so send it when a value is settled, not on every slider step.

### Wi-Fi

| Request | Response |
|---------|----------|
| `GET /api/wifi/status` | `{"mode":"hotspot"\|"connecting"\|"connected","failed":bool,"ssid":"…","ip":"…","host":"catwandconnect.local"}`. `failed` is true when the last attempt failed. |
| `GET /api/wifi/networks` | `{"scanning":true}` while a scan runs (poll again), then `{"scanning":false,"networks":[{"ssid":"…","rssi":-60,"secure":true}]}`. |
| `POST /api/wifi` | Form fields `ssid` and `password`. Returns 202 and connects in the background; follow progress with `/api/wifi/status`. Only accepted while the hotspot is up (409 otherwise). |

### Interface files

`/` serves the Wi-Fi setup page while the robot is not on a Wi-Fi network, and the main screen once it is. The interface's files (`/app.js`, `/lang/en.json`, `/manifest.webmanifest`, …) are served gzip-compressed.
