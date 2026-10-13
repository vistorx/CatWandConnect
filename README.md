<h1 align="center">
  <img src="media/banner/banner-robot-cat-wand.jpg" alt="CatWandConnect: a robot holding a toy wand with a fish, and a kitten jumping for it" width="100%">
</h1>

**An open-hardware robot that plays with your cat.** An ESP32 swings a toy wand on two servos. You drive it from your phone through a web page served by the robot itself, with no app, no cloud and no account. Home Assistant is optional.

Everything is free and open: the printable parts, the firmware and the docs.

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/"><img src="media/buttons/install.svg" alt="Install in your browser" width="340"></a>
  <br>
  <a href="https://github.com/vistorx/CatWandConnect/releases"><img src="https://img.shields.io/github/v/release/vistorx/CatWandConnect?include_prereleases&label=latest" alt="Latest version"></a>
  <a href="https://github.com/vistorx/CatWandConnect/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/vistorx/CatWandConnect/ci.yml?branch=main&label=tests" alt="Tests"></a>
  <a href="#install-the-firmware"><img src="https://img.shields.io/badge/chips-ESP32%20%C2%B7%20S3%20%C2%B7%20C3-2b6cb0" alt="ESP32, ESP32-S3, ESP32-C3"></a>
</p>

> **Status: V1 in beta.** Every V1 feature is implemented and tested automatically. The firmware is now being validated on the real robot as beta releases, against the [acceptance checklist](docs/acceptance-checklist.md). The first stable release follows once it passes.

## What it does

- **Play from your phone.** A joystick and a speed slider in any browser on your Wi-Fi, with no app to install.
- **Safe by design.** Travel limits it never exceeds, a Stow button nothing can interrupt, smooth motion, and a stop as soon as your phone disconnects.
- **Easy to set up.** [Install from the browser](#install-the-firmware), enter your Wi-Fi, and let the Calibration wizard fit the robot to your build.
- **Yours to adapt.** Your pins and servos, your robot's name, English or French, an optional PIN, and updates over Wi-Fi.
- **Home Assistant if you want it**, through MQTT, and an open API for everything else.

**[See every feature in detail →](docs/features.md)**

Automatic play sessions are planned for V1.1.

## Install the firmware

No tools to install: the firmware goes on the board straight from your browser. You can install it on a bare board before [building the robot](#build-one).

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/"><img src="media/buttons/install.svg" alt="Install in your browser" width="340"></a>
</p>

1. **Open** the [web flasher](https://vistorx.github.io/CatWandConnect/) in **Chrome or Edge** on a computer.
2. **Plug** the ESP32 board in over USB, with a cable that carries data.
3. **Install:** press **Connect and install** and pick the board's port. The right firmware for your chip is chosen for you.
4. **Connect:** enter your Wi-Fi when asked, then choose **Visit device**.

> [!NOTE]
> Until the first stable release, the web flasher installs the **beta**, which gets new changes first and may still have bugs. See [release channels](docs/releasing.md#channels).

<details>
<summary><b>Updating, building from source, troubleshooting</b></summary>

- **Update a robot that is already installed:** download the `-ota.bin` file for your chip from the [latest release](https://github.com/vistorx/CatWandConnect/releases), then upload it in the robot's **Settings → Robot → Firmware update**. Settings and Calibration are kept.
- **Build from source** with [PlatformIO](https://platformio.org/); [Getting started](docs/getting-started.md) covers the tools:

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # or esp32s3 / esp32c3
  ```

- **The board doesn't show up?** Try another cable, then install the driver for its USB chip: see [Installing with the web flasher](docs/troubleshooting.md#installing-with-the-web-flasher) for the download links.

</details>

## Build one

### Printed parts: Mk2 Mechanism

The **Mk2** is the current Mechanism. It is reinforced with a 608 skateboard bearing and a track of 4 mm steel balls. All files are in [`stls/mk2/`](stls/mk2/), and [`catwandconnect-mk2.3mf`](stls/mk2/catwandconnect-mk2.3mf) contains the whole plate.

| Part | File |
|------|------|
| Arm | [`catwandconnect-mk2-arm.stl`](stls/mk2/catwandconnect-mk2-arm.stl) |
| Bearing | [`catwandconnect-mk2-bearing.stl`](stls/mk2/catwandconnect-mk2-bearing.stl) |
| Clamp | [`catwandconnect-mk2-clamp.stl`](stls/mk2/catwandconnect-mk2-clamp.stl) |
| Screw | [`catwandconnect-mk2-screw.stl`](stls/mk2/catwandconnect-mk2-screw.stl) |
| Rotor plate | [`catwandconnect-mk2-rotor_plate.stl`](stls/mk2/catwandconnect-mk2-rotor_plate.stl) |
| Top plate | [`catwandconnect-mk2-top_plate.stl`](stls/mk2/catwandconnect-mk2-top_plate.stl) |

The retired Mk1 is kept in [`archive/`](archive/) for reference only. Its vertical servo overheated under load, and the firmware does not target it.

### Other parts

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 board | Any dev board with USB works, from about €3. USB-C is handy. |
| 2 | MG996R servos | Standard-size metal-gear servos. |
| 1 | 5 V power supply, 3 A or more | Powers the servos and the board. See [Electronics](docs/electronics.md). |
| 1 | 608 skateboard bearing | |
| 16–24 | 4 mm steel balls | Adjust the count to your print's finish. |
| 4 | M3×8 button-head screws | Horizontal servo mounting |
| 2 | M3×25 button-head screws | Lower plate of the horizontal axis to the ball track |
| 1 | M3×8 countersunk screw | Horizontal servo shaft plate |
| 1 | M3×20 countersunk screw | Vertical servo shaft |
| 4 | M3×8 button-head screws | Vertical servo mounting |
| 3 | M3×30 countersunk screws | Bearing to the vertical shaft |
| | A toy wand, wires, Dupont connectors | |

**ESP8266 boards are not supported, and almost certainly never will be.** They lack the memory and hardware the robot needs; see [ADR 0008](docs/adr/0008-no-esp8266-support.md).

### Assemble and wire

- [Assembly](docs/assembly.md): mechanical assembly order and checks.
- [Electronics](docs/electronics.md): the wiring diagram, with the default pins for each chip.

## First start

1. If you entered your Wi-Fi in the web flasher, skip to step 3. Otherwise power the robot: with no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot.
2. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password.
3. Rejoin your home Wi-Fi and open **`http://catwandconnect.local/`** (or the IP shown on the setup page).
4. **Calibrate.** The main screen invites you to. The wizard has you nudge each axis to the ends of its safe travel and pick the Stow position, then sweeps the new envelope slowly so you can check nothing touches. Until then, movement stays within a conservative 60°–120° range on each axis.
5. **Play.** Drag the joystick to move the wand and let go to stop. Press **Stow** to put it away.
6. **Make it yours.** Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`. Use your browser's "Add to Home Screen" for an app-like icon.

Something wrong? See [Troubleshooting](docs/troubleshooting.md), then [Getting help](SUPPORT.md).

## Get help

- **Questions, build help, show your robot:** the [CatWandConnect Discord](https://discord.gg/Rbqge3v9dk). No GitHub account needed; English, French and other languages welcome.
- **Found a bug?** [Open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose), or post it in `#support` on Discord if you don't have a GitHub account.
- **Security problem?** Report it [privately](SECURITY.md), never in public.

There is no guaranteed response time: CatWandConnect is maintained in spare time. See [SUPPORT.md](SUPPORT.md) for what to include.

## Documentation

| Doc | For |
|-----|-----|
| [Getting started](docs/getting-started.md) | Setting up a machine, building, testing, the simulator and the code layout |
| [Features](docs/features.md) | Everything the robot does, in detail |
| [Home Assistant](docs/home-assistant.md) | Connecting the robot to Home Assistant over MQTT |
| [Device API](docs/api.md) | The robot's HTTP, WebSocket and MQTT API, for scripts and integrations |
| [Acceptance checklist](docs/acceptance-checklist.md) | What to check on the real robot before each release |
| [Releasing](docs/releasing.md) | Publishing a version and the web flasher |
| [Media](media/README.md) | Logo, banners, photos and screenshots, and how to add them |
| [Getting help](SUPPORT.md) · [Security](SECURITY.md) | Where to ask, how to report a bug or a vulnerability |
| [Discord server](docs/discord.md) | How the community server is organised, for maintainers |
| [Glossary](CONTEXT.md) | The words the project uses: Stow, Travel limits, Calibration… |
| [Decisions](docs/adr/) | Why things are the way they are |

## Contributing

Issues and pull requests are welcome. Start with [Getting started](docs/getting-started.md). You can work on the interface without hardware thanks to the simulator in `firmware/tools/dev_server.mjs`. AI coding agents of any brand get their instructions from [AGENTS.md](AGENTS.md).

## Support the project

CatWandConnect is free. If your cat enjoys it, you can support its development through [GitHub Sponsors](https://github.com/sponsors/vistorx). Ready-to-assemble kits may follow later.

## Licence

| | |
|---|---|
| Firmware | [GPL-3.0-or-later](LICENSES/GPL-3.0-or-later.txt) |
| Hardware designs | [CERN-OHL-S-2.0](LICENSES/CERN-OHL-S-2.0.txt) |
| Documentation | [CC BY-SA 4.0](LICENSES/CC-BY-SA-4.0.txt) |

You may build, modify and sell your own version. See [LICENSE.md](LICENSE.md). The name "CatWandConnect" is reserved for the official project and kits; see the [trademark policy](TRADEMARK.md).
