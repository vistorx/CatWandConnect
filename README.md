# CatWandConnect

**An open-hardware robot that plays with your cat.** An ESP32 swings a toy wand on two servos. You drive it from your phone through a web page served by the robot itself, with no app, no cloud and no account. Home Assistant is optional.

Everything is free and open: the printable parts, the firmware and the docs.

> **Status: in development, no release yet.** The firmware already does Wi-Fi setup, Manual control from a phone, speed settings, Stow, a gentle power-up, Calibration, wiring settings, naming, a French translation, an optional settings PIN and firmware updates from the browser. It has not been validated on the real robot yet. Follow progress in [issue #1](https://github.com/vistorx/CatWandConnect/issues/1).

## What it does

| | Feature | Status |
|---|---|---|
| 📱 | **Phone joystick.** Move the wand in real time from any browser on your Wi-Fi, at `http://catwandconnect.local/`. | ✅ |
| 🛑 | **Stow button.** One tap puts the wand back to rest and stops everything; nothing can interrupt it. | ✅ |
| 🐢 | **Smooth and safe motion.** Configurable speed and acceleration. The wand brakes before its Travel limits and stops if your phone disconnects. | ✅ |
| 📶 | **Easy Wi-Fi setup.** Enter your Wi-Fi in the browser right after flashing, or through the robot's own `CatWandConnect-XXXX` hotspot. | ✅ |
| 🔌 | **Gentle power-up.** The robot remembers where it rested and crawls back to its Stow position, so the servos don't jump. | ✅ |
| 🎯 | **Calibration wizard.** Set the Travel limits and Stow position for your build, step by step, then watch a slow test sweep before saving. | ✅ |
| 🔧 | **Your wiring.** Choose pins (only the safe ones for your chip), reverse axes and set servo pulse ranges from the interface. | ✅ |
| 🇫🇷 | **English and French interface.** Name your robot (it becomes `http://<name>.local/`), check the Info page, and add the interface to your phone's home screen. | ✅ |
| 🔒 | **Optional settings PIN.** Guests can play and Stow, but not change the setup. | ✅ |
| ⬆️ | **Firmware updates from the browser.** Upload the new file in Settings; settings and Calibration are kept, and a failed upload leaves the old firmware running. | ✅ |
| ⚡ | **Web flasher.** Install the robot from Chrome or Edge in one click, no tools needed. | 🚧 |
| 🏠 | **Home Assistant** through MQTT discovery. | 🚧 |

Automatic play sessions are planned for V1.1.

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
| 1 | 5 V power supply, 3 A or more | For the servos. Never power them from the board. |
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
- [Electronics](docs/electronics.md): wiring the servos and the power supply, with the default pins for each chip.

### Install the firmware

The one-click web flasher will come with the first release. Until then, build and flash from source with [PlatformIO](https://platformio.org/). Plug the board in over USB, then:

```sh
cd firmware
pio run -e esp32 -t upload   # or esp32s3 / esp32c3
```

[Getting started](docs/getting-started.md) covers the tools in detail.

### First start

1. Power the robot. With no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot.
2. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password.
3. Rejoin your home Wi-Fi and open **`http://catwandconnect.local/`** (or the IP shown on the setup page).
4. **Calibrate.** The main screen invites you to. The wizard has you nudge each axis to the ends of its safe travel and pick the Stow position, then sweeps the new envelope slowly so you can check nothing touches. Until then, movement stays within a conservative 60°–120° range on each axis.
5. **Play.** Drag the joystick to move the wand and let go to stop. Press **Stow** to put it away.
6. **Make it yours.** Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`. Use your browser's "Add to Home Screen" for an app-like icon.

Something wrong? See [Troubleshooting](docs/troubleshooting.md).

## Documentation

| Doc | For |
|-----|-----|
| [Getting started](docs/getting-started.md) | Setting up a machine, building, testing, the simulator and the code layout |
| [Device API](docs/api.md) | The robot's HTTP and WebSocket API, for scripts and integrations |
| [Acceptance checklist](docs/acceptance-checklist.md) | What to check on the real robot before each release |
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
