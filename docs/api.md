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
| `activity` | `idle`, `manual` or `stowing`. |
| `held` | Whether the servos are being driven. They are released only once the wand rests at the Stow position. |
| `playSpeed` | The speed slider's value, so every open interface stays in sync. |

## HTTP

### Stow

`POST /api/stow` (no body) does the same as the `stow` WebSocket message and returns `202`.

### Priorities

Stow beats every other command and cannot be interrupted. Manual control, from any source, cancels any other activity. Nothing resumes on its own afterwards.

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
| `calibrated` | Whether Calibration has been completed. Until then, movement stays within 60°–120° whatever `min` and `max` say. Read-only. | |
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

`/` serves the Wi-Fi setup page while the robot is not on a Wi-Fi network, and the main screen once it is. The interface's files (`/app.js`, `/lang/en.json`, …) are served gzip-compressed.
