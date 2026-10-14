<h1 align="center">
  <img src="media/banner/banner-robot-cat-wand.jpg" alt="CatWandConnect: a robot holding a toy wand with a fish, and a kitten jumping for it" width="100%">
</h1>

<p align="right"><b>English</b> · <a href="README.fr.md">Français</a></p>

**An open-hardware robot that plays with your cat.** An ESP32 swings a toy wand on two servos. You drive it from your phone through a web page served by the robot itself, with no app, no cloud and no account. Home Assistant is optional.

Everything is free and open: the printable parts, the firmware and the docs.

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="media/buttons/install.svg" alt="Install in your browser" width="340"></a>
  <br>
  <a href="https://github.com/vistorx/CatWandConnect/releases"><img src="https://img.shields.io/github/v/release/vistorx/CatWandConnect?include_prereleases&label=latest" alt="Latest version"></a>
  <a href="https://github.com/vistorx/CatWandConnect/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/vistorx/CatWandConnect/ci.yml?branch=main&label=tests" alt="Tests"></a>
  <a href="docs/install.md"><img src="https://img.shields.io/badge/chips-ESP32%20%C2%B7%20S3%20%C2%B7%20C3-2b6cb0" alt="ESP32, ESP32-S3, ESP32-C3"></a>
  <a href="https://discord.gg/Rbqge3v9dk"><img src="https://img.shields.io/badge/chat-Discord-5865F2" alt="Discord"></a>
</p>

> **Status: V1 stable.** [v0.1.0](https://github.com/vistorx/CatWandConnect/releases/tag/v0.1.0), the first stable release, passed the full [acceptance checklist](docs/acceptance-checklist.md) on a classic ESP32. The ESP32-S3 and ESP32-C3 builds are tested automatically, not yet on real hardware.

## What it is

- **Play from your phone.** A joystick and a speed slider in any browser on your Wi-Fi, with no app to install.
- **Play sessions.** Pick a duration and tap Start: the wand plays with your cat on its own, within your limits, then Stows.
- **Safe by design.** Travel limits it never exceeds, a Stow button nothing can interrupt, smooth motion, and a stop as soon as your phone disconnects.
- **Easy to set up.** Install from the browser, enter your Wi-Fi (or skip it and use the robot's hotspot), and let the Calibration wizard fit the robot to your build.
- **Yours to adapt.** Your pins and servos, your robot's name, English or French, an optional PIN, and updates over Wi-Fi.
- **Home Assistant if you want it**, through MQTT, and an open API for everything else.

**Specs:** [every feature in detail](docs/features.md) · [V1 spec](https://github.com/vistorx/CatWandConnect/issues/1) · [Support spec](https://github.com/vistorx/CatWandConnect/issues/47) · [planned for later](https://github.com/vistorx/CatWandConnect/issues?q=is%3Aopen+label%3Afuture) · [design decisions](docs/adr/) · [glossary](CONTEXT.md)

## See it

<p align="center">
  <a href="media/video/mk2-presentation.mp4"><img src="media/video/mk2-presentation-poster.jpg" alt="The Mk2 Mechanism clamped to a table edge, holding the wand: click to watch the 50-second presentation" width="375"></a><br>
  <em>▶ <a href="media/video/mk2-presentation.mp4">Watch the 50-second presentation</a>: print it, assemble it, add your cat's favourite toy, play.</em>
</p>

*Photos of the real robot are coming.*

## Build your own

Three parts: install the firmware, wire the electronics, print and assemble the Mechanism. Any order works; the firmware can go on a bare board first.

### 1. Firmware

The firmware installs from your browser, with nothing to download: open the web flasher in Chrome or Edge, plug the board in over USB and press **Connect and install**. The right build for your chip is picked for you. Later updates go over Wi-Fi, from the robot's own Settings page.

**[Open the web flasher →](https://vistorx.github.io/CatWandConnect/flasher/)** · [Install and first start](docs/install.md) · [Release channels](docs/releasing.md#channels)

### 2. Electronics

A single 5 V supply feeds both servos and the board; the board only sends the servo signals. No soldering is required.

| Qty | Part |
|-----|------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 dev board (not ESP8266, see [ADR 0008](docs/adr/0008-no-esp8266-support.md)) |
| 2 | MG996R servos |
| 1 | 5 V power supply, 3 A or more |
| | Terminal block or Wago-style connectors, wires, an optional 470–1000 µF capacitor |

<p align="center"><img src="docs/images/wiring-compact.svg" alt="Compact wiring: the 5 V supply feeds a +5 V rail and a GND rail; both servos and the ESP32 take power from the rails; the ESP32 sends a signal to each servo." width="560"></p>

**[Electronics guide →](docs/electronics.md)**: full diagram, pins per chip, power rules.

### 3. Hardware

The Mechanism is 3D printed: six parts, all in [`stls/mk2/`](stls/mk2/) or as one [`.3mf`](stls/mk2/catwandconnect-mk2.3mf) plate. No printer? Any print service or maker space can print them. Add a skateboard bearing, steel balls, a few M3 screws and a toy wand.

| Qty | Part |
|-----|------|
| 6 | Printed parts (arm, bearing, clamp, screw, rotor plate, top plate) |
| 1 | 608 skateboard bearing |
| 16–24 | 4 mm steel balls |
| 15 | M3 screws, 8 to 30 mm |
| 1 | Toy wand |

**[Parts and print settings →](docs/hardware.md)** · **[Assembly guide →](docs/assembly.md)** (coming soon)

## Documentation

**Build and use**

| Doc | For |
|-----|-----|
| [Install and first start](docs/install.md) | Flashing from the browser, Wi-Fi, Calibration, updates |
| [Electronics](docs/electronics.md) | Electronic parts, wiring diagram, power, default pins per chip |
| [Hardware](docs/hardware.md) | Printed parts, print settings, mechanical parts of the Mk2 |
| [Assembly guide](docs/assembly.md) | Putting the Mk2 together (coming soon) |
| [Features](docs/features.md) | Everything the robot does, in detail |
| [Home Assistant](docs/home-assistant.md) | Connecting the robot over MQTT |
| [Troubleshooting](docs/troubleshooting.md) | Installing, Wi-Fi, motion, updates, PIN |

**Develop and contribute**

| Doc | For |
|-----|-----|
| [Getting started](docs/getting-started.md) | Tools, building, tests, the simulator and the code layout |
| [Device API](docs/api.md) | HTTP, WebSocket and MQTT API, for scripts and integrations |
| [Acceptance checklist](docs/acceptance-checklist.md) | What to check on the real robot before each release |
| [Releasing](docs/releasing.md) | Release channels, publishing a version and the web flasher |
| [Glossary](CONTEXT.md) · [Decisions](docs/adr/) | The project's words, and why things are the way they are |
| [Media](media/README.md) | Logo, banners, photos and screenshots, and how to add them |
| [Discord server](docs/discord.md) | How the community server is organised, for maintainers |
| [AGENTS.md](AGENTS.md) | Instructions for AI coding agents |

## Get help

- **Questions, build help, show your robot:** the [CatWandConnect Discord](https://discord.gg/Rbqge3v9dk). No GitHub account needed; English, French and other languages welcome.
- **Found a bug?** [Open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose), or post it in `#support` on Discord.
- **Security problem?** Report it [privately](SECURITY.md), never in public.

CatWandConnect is maintained in spare time, with no guaranteed response time. See [Getting help](SUPPORT.md) for what to include.

## Contributing and support

Issues and pull requests are welcome: start with [Getting started](docs/getting-started.md). The interface can be worked on without hardware, thanks to the simulator in `firmware/tools/dev_server.mjs`.

CatWandConnect is free. If your cat enjoys it, you can support it through [GitHub Sponsors](https://github.com/sponsors/vistorx). Ready-to-assemble kits may follow later.

## Licence

| | |
|---|---|
| Firmware | [GPL-3.0-or-later](LICENSES/GPL-3.0-or-later.txt) |
| Hardware designs | [CERN-OHL-S-2.0](LICENSES/CERN-OHL-S-2.0.txt) |
| Documentation | [CC BY-SA 4.0](LICENSES/CC-BY-SA-4.0.txt) |

You may build, modify and sell your own version. See [LICENSE.md](LICENSE.md). The name "CatWandConnect" is reserved for the official project and kits; see the [trademark policy](TRADEMARK.md).
