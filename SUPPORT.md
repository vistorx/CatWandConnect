# Getting help

CatWandConnect is a free project maintained in spare time. Help is public and best effort: there is **no guaranteed response time**, but every report is read. English and French are both welcome; we reply in your language when we can.

## Where to go

| You have… | Go to |
|---|---|
| A question, a build problem, or you want to show your robot | The [Discord server](https://discord.gg/Rbqge3v9dk): post in `#support` (one post per problem) or `📸・showroom`. No GitHub account needed. |
| A bug, with a GitHub account | [Open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) and pick the form that matches. |
| A bug, without a GitHub account | On your robot: Settings → Robot → Help → Report a problem → **Send to support**. The robot's redacted Diagnostic report becomes a public GitHub issue for you. Or post it in `#support` on [Discord](https://discord.gg/Rbqge3v9dk): confirmed bugs are turned into GitHub issues for you. |
| A security vulnerability | **Never in public.** Report it privately: see [SECURITY.md](SECURITY.md). |

Before asking, the [Troubleshooting guide](docs/troubleshooting.md) answers the common problems: the board doesn't show up when installing, Wi-Fi setup, Motion, updates and the settings PIN.

## What to include

The more of this you include, the faster we can help:

- **The Diagnostic report.** In the Device interface, open Settings → Info, then **Copy the report** or **Download the report**, and paste or attach it. It holds the firmware version, chip, why the robot last restarted, its settings and its recent log, with your Wi-Fi name and addresses hidden. Firmware older than 0.1.0-beta.2 has no report: press **Copy** under the Info list instead.
- **What you did, what you expected, and what happened instead.**
- **Your build:** board and chip (ESP32, ESP32-S3 or ESP32-C3), Mechanism (for example Mk2), how you installed the firmware (web flasher, update, built from source).
- **The serial log, if you can.** Open the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/), connect the board, then **Logs & Console**. Copy the lines around the problem.

Check that what you paste does not contain your Wi-Fi password or MQTT password: issues and Discord posts are public.

## What we don't support

- **ESP8266 boards.** They are not supported, and almost certainly never will be: see [ADR 0008](docs/adr/0008-no-esp8266-support.md).
- **Modified firmware.** We help with the published releases. If you changed the code, say so and include your changes.
