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

**Dead-man rule:** while moving, resend `move` at least every 500 ms; the interface sends it every 100 ms. If no `move` arrives for 500 ms, or if any client disconnects, the robot stops.

### Robot → client

`state` is sent when a client connects, whenever something changes (at most every 100 ms), and every 2 s otherwise:

```json
{"t":"state","h":92.5,"v":88.0,"activity":"manual"}
```

| Field | Meaning |
|-------|---------|
| `h`, `v` | Current position of each Axis. |
| `activity` | `idle` or `manual`. |

## HTTP

### Wi-Fi

| Request | Response |
|---------|----------|
| `GET /api/wifi/status` | `{"mode":"hotspot"\|"connecting"\|"connected","failed":bool,"ssid":"…","ip":"…","host":"catwandconnect.local"}`. `failed` is true when the last attempt failed. |
| `GET /api/wifi/networks` | `{"scanning":true}` while a scan runs (poll again), then `{"scanning":false,"networks":[{"ssid":"…","rssi":-60,"secure":true}]}`. |
| `POST /api/wifi` | Form fields `ssid` and `password`. Returns 202 and connects in the background; follow progress with `/api/wifi/status`. Only accepted while the hotspot is up (409 otherwise). |

### Interface files

`/` serves the Wi-Fi setup page while the robot is not on a Wi-Fi network, and the main screen once it is. The interface's files (`/app.js`, `/lang/en.json`, …) are served gzip-compressed.
