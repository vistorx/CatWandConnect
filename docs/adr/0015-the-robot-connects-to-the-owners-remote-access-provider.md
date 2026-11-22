# The robot connects to the owner's remote access provider, only when he turns it on

Owners want to open the robot's page away from home (#251). The robot only answers on the home network, and it makes no outbound connection ([ADR 0009](0009-support-flows-end-in-github-issues.md), [ADR 0012](0012-the-browser-fetches-updates-for-the-robot.md), [ADR 0013](0013-the-robot-clock-comes-from-the-browser-and-home-assistant.md)). Opening a port on the router would put the robot's unauthenticated page on the internet. A cloud relay run by the project would be a service between owners and their robot, with accounts and secrets to keep.

We decided the robot may make **outbound connections to the owner's own Remote access provider**, and only once he has turned Remote access on (Settings → Network, behind the settings PIN) with a key from his account. The first provider is Tailscale: the robot runs a Tailscale client ([microlink](https://github.com/CamM2325/microlink), MIT) and joins the owner's tailnet as its own device, reachable at its `100.x` address by his devices and no one else. Remote access is off by default. Turning it off disconnects the robot, erases its device keys and forgets the auth key. Everything else stays as ADRs 0009, 0012 and 0013 say: no telemetry, no update check by the robot, no NTP.

The Remote access module has a provider interface (start, stop, forget, status with address and key expiry, routes supported), so other providers can be added later, each with its own ticket. Its logic (setting, states, retries) is a pure library tested on the host; the provider client is ESP32 glue.

## Consequences

- When it is on, the robot connects to Tailscale's coordination server (`controlplane.tailscale.com`) and DERP relays over TLS, and to the owner's devices over WireGuard (UDP). That is the only outbound traffic, and only the owner's choice starts it.
- The auth key is stored in the robot's flash (NVS, like the Wi-Fi password), never returned by the API, never logged and never in the Diagnostic report. The tailnet address and `*.ts.net` names are redacted in the redacted report.
- A firmware update started from a page opened over remote access needs a second confirmation: nobody may be near the robot to power it back if it goes wrong. The robot refuses the upload (`428 remote_confirm_required`) without it.
- microlink is built against the Arduino core's precompiled ESP-IDF, as a PlatformIO library with its build options in a header (`firmware/lib/microlink/`), not with an ESP-IDF build. It adds about 270 KB of flash on every chip; the classic ESP32 app uses 79 % of its 1.9 MB slot (the ESP32-C3, 84 %). When off it takes no heap; when on, its map and HTTP/2 buffers take 64 KB each, sized for a small tailnet on boards without PSRAM.
- The camera through remote access (a subnet route, #271), the setup wizard (#270) and other providers (NetBird, #266) come later.
