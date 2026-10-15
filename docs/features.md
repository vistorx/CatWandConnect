# Features

Everything the CatWandConnect firmware does, grouped by what you use it for. All of it is in V1 unless marked otherwise.

## Play

- **Phone joystick.** Open `http://catwandconnect.local/` in any browser on your Wi-Fi and drag the joystick: the wand follows in real time. A small drag moves it slowly; a full drag moves it at full speed. Let go and it stops. Works with the phone upright or sideways.
- **Speed slider.** Set how lively the wand is, from each axis's minimum speed to its maximum speed, right on the main screen. Every open phone stays in sync.
- **Play session.** The robot plays with your cat on its own for a set time (1–60 min, 10 by default, remembered), then Stows. Each axis moves to random spots within your Travel limits at random speeds within its Speed range, and pauses for a random time within its Pause range (0–60 s, set per axis, 0–2 s by default), never in step with the other axis, so the wand moves like prey. Touching the joystick takes over at once; Stop or Stow end it. It keeps going when you close the page, and never restarts by itself after a power cut. On the main screen, the Play session block sets the duration with − and + (or by typing it) and starts it; while it plays, the button becomes Stop and the block counts the time left down. Every open phone shows the session, whoever started it. Available once calibrated: otherwise the block says so, with a link to Calibration. [Home Assistant](home-assistant.md) and scripts through the [Device API](api.md#play-session) can drive it too.
- **Live position.** The main screen shows where each axis is.
- **Several phones at once.** Anyone on your Wi-Fi can open the interface and watch or play.

## Safety

- **Travel limits.** Each axis has a minimum and a maximum that the robot never exceeds, whatever sends the command (phone, API or Home Assistant). The wand brakes smoothly before reaching them.
- **Safe until calibrated.** Out of the box, movement stays within a conservative 60°–120° range on each axis.
- **Stow.** A big button, always visible, puts the wand back to its rest position and stops everything else. Nothing can interrupt it. At rest the servos are released, so they don't buzz or heat up.
- **Dead-man rule.** If your phone disconnects or stops sending for half a second, the wand stops.
- **Smooth motion.** Minimum speed, maximum speed and acceleration per axis, so starts and stops never jerk the Mechanism and the slowest setting never stutters. Settings saved by 0.1.0 are kept on update, with a minimum speed of a tenth of the maximum.
- **Speed tuning.** The last steps of the Calibration wizard, and on its own in Settings → Motion → Tune speeds and acceleration: the robot sweeps one axis back and forth between its limits while you tap Faster or Slower, then keep the speed that looks right as that axis's maximum (just before it shakes) or minimum (just above where it turns jerky). Available once calibrated; Stow, the joystick or Cancel keep the previous value.
- **Acceleration tuning.** Right after each axis's maximum speed in the Calibration wizard, and on its own next to Speed tuning: the robot makes short back-and-forth moves at that axis's maximum speed, each from a stop, while you tap Snappier or Softer, so you feel what the acceleration does; keep it just below where the wand jerks, overshoots or the servo strains.
- **Gentle power-up.** The robot remembers where the wand last rested, starts there so the servos don't jump, then crawls back to its Stow position.
- **Optional settings PIN.** Guests can play and Stow, but changing limits, Calibration, wiring, name, Wi-Fi, Home Assistant or firmware asks for the PIN. Repeated wrong PINs lock it for a growing delay.

## Setup

- **Web flasher.** Install the firmware from Chrome or Edge on a computer in one click, with no tools to install. The right file for your chip is picked automatically. Choose the Stable or Beta channel. Reinstalling over a robot keeps its Wi-Fi and settings unless you tick "Erase device". It is at <https://vistorx.github.io/CatWandConnect/flasher/>, one click from the [project website](https://vistorx.github.io/CatWandConnect/), which presents the README and these docs with search.
- **Wi-Fi in two ways.** Enter your Wi-Fi in the web flasher right after installing, or join the robot's own `CatWandConnect-XXXX` hotspot, whose setup page opens by itself and lists nearby networks, strongest first. Or tap "Continue without Wi-Fi" and use the robot from its hotspot; connect it later from Settings → Network → Wi-Fi.
- **Change Wi-Fi network from Settings.** Settings → Network shows the current network, the robot's IP address and `.local` address, and a **Change Wi-Fi network** button that explains what happens, then opens the setup page to pick the new network. A wrong password or an unreachable network sends the robot back to the network it was on, so you never lose it; only if that fails too does its hotspot open.
- **Self-healing Wi-Fi.** If your router disappears, the hotspot comes back; the robot keeps retrying your network and leaves the hotspot as soon as it returns.
- **Calibration wizard.** A step-by-step guide to set the Travel limits and Stow position for your build: each step brings the wand to the current value and says which joystick position it matches (minimum = fully left or down, maximum = fully right or up), nudge it to the right end; if the axis turns the wrong way, tick "Reverse this axis" right there (the box shows the current wiring) without restarting; pick the rest position (the wizard recalls what you have marked and the allowed range), then watch a slow test sweep before saving. Once the limits are saved, the wizard goes on with Speed tuning: Horizontal minimum speed, maximum speed and acceleration, then the same for Vertical, each one shown by the robot while you tap Faster or Slower (Snappier or Softer for the acceleration), or skipped to keep the current value; leaving then keeps the saved limits and the values already kept. Cancel at any time to keep the previous values; when a Calibration is already saved, the wizard offers "Recalibrate", and "Cancel the new Calibration" takes you back to the main screen.
- **Your wiring.** Choose which GPIO drives each servo (only the pins that are safe on your chip are offered), reverse an axis and, under Servo travel (advanced), set each servo's pulse range: how far it turns, with a reset to the defaults.

## Everyday use

- **Settings in tabs.** Motion (limits, Calibration, speed, Pause range), Hardware (wiring), Network (Wi-Fi, Home Assistant) and Robot (name, language, PIN, firmware update, Info). Links can open a tab directly, e.g. `settings.html#network`.
- **Name your robot.** "CatWandConnect salon" becomes `http://catwandconnect-salon.local/` and names its hotspot and its Home Assistant device.
- **English and French.** The interface follows your phone's language, or the one you choose in Settings.
- **Home-screen icon.** Add the interface to your phone's home screen; it opens full screen like an app. On the first visit from an iPhone, iPad or Android phone, a short tip shows how, with the steps for that phone (Safari's Share button, or Chrome's ⋮ menu) and a link to the other one. **Got it** hides it for good; **Later** brings it back on a visit the next day at the earliest. It is not shown on a computer, on the setup hotspot, when the page is already opened from the home screen, or while an update is offered. If the page was opened by IP address, it suggests the robot's `.local` address first. Help → **Add to your home screen** shows it again.
- **Info page.** Firmware version, chip, Wi-Fi signal, IP address, uptime, temperature and memory, with a Copy button for bug reports.
- **Diagnostic report.** On the Info page, a ready-to-attach report for bug reports: firmware, chip, a random Device ID, why it last restarted, memory, Wi-Fi and Home Assistant state, settings, wiring and the robot's recent log. Your Wi-Fi name, addresses and broker details are hidden. Copy it or download it as a text file; a full version, for your own use only, keeps them.
- **Help page.** From Settings → Robot or the Info page: ask a question on Discord, or report a problem. You describe it, check the exact text that will be public, and the robot opens a GitHub bug report prefilled with your description and its redacted Diagnostic report (the most recent log lines that fit), and downloads the full report to attach. With no GitHub account, **Send to support** posts it for you through the Support relay (optional contact, the page shows the created issue's link); if the service is unreachable, the prefilled issue stays available. Tick **Follow it on Discord** to also get a `#support` post linked to the issue, where you are told when the fix ships.
- **Report later from the hotspot.** On the setup hotspot the phone has no internet: the setup page offers the Diagnostic report download and where to ask, and Help can save your description on the robot. Once the robot is on your Wi-Fi, the main screen offers to send it or discard it. It is never sent by itself.

## Help

- **Where to get help.** A [Discord server](https://discord.gg/Rbqge3v9dk) for questions, build help and sharing builds, in several languages and with no GitHub account needed; GitHub issue forms for bugs and installation problems; private reporting for security problems. See [Getting help](../SUPPORT.md).

## Updates

- **One-tap updates.** When a new version is out for the robot's Update channel, its main screen and Settings say so, with what's new and an **Update** button: your phone downloads the right file for the chip and installs it, then waits for the robot to come back. The robot itself never connects to the Internet. Without Internet on the phone, upload the file by hand in Settings. Settings and Calibration are kept, and if the upload fails or is interrupted the robot keeps its current firmware.
- **Releases.** Each version is published with ready-made files for every supported chip.
- **Stable and Beta channels.** Stable versions are for everyone. Beta versions get new changes first so they can be tested on real robots; the web flasher lets you pick either one, and each robot has its own Update channel in Settings, kept across restarts and updates.

## Integrations

- **Home Assistant (optional), two ways to connect.** Either way the robot appears in Home Assistant with a Stow button, each axis's position and maximum speed, its activity, and a Play session switch with its Session duration; commands follow the same safety rules as the phone, and each connection has a ready-made dashboard to copy and paste. Use only one per robot. See [Home Assistant](home-assistant.md).
  - **Native integration.** Installed through HACS from [`vistorx/ha-catwandconnect`](https://github.com/vistorx/ha-catwandconnect): Home Assistant finds the robot under *Discovered*, you click Add (and type the Settings PIN if one is set). No broker, live updates within a second, entity names in Home Assistant's language, a new PIN asked again when you change it, and a redacted diagnostics download for bug reports. Needs firmware 0.2.0-beta.2 or later and Home Assistant 2026.10 or later.
  - **MQTT connection.** Enter your MQTT broker in Settings and the robot announces itself through MQTT discovery. Works with any Home Assistant, for owners who already run a broker. Its Firmware entity shows when a new version is available (install it from the robot's page).
- **Open API.** Everything the interface does goes through a documented HTTP, WebSocket and MQTT API, for your own scripts and integrations. The API is versioned, and the robot announces itself on the network as `_catwandconnect._tcp` with its Device ID and API version, so the Native integration for Home Assistant finds it without MQTT. See [Device API](api.md).

## Hardware

- **Boards:** ESP32, ESP32-S3 and ESP32-C3. ESP8266 boards are not supported ([ADR 0008](adr/0008-no-esp8266-support.md)).
- **Mechanism:** the Mk2, with two MG996R servos.

## Planned

- **Automatic play sessions** (V1.1): the robot plays on its own, within its Travel limits.
- **Bluetooth control.**
