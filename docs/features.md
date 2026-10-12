# Features

Everything the CatWandConnect firmware does, grouped by what you use it for. All of it is in V1 unless marked otherwise.

## Play

- **Phone joystick.** Open `http://catwandconnect.local/` in any browser on your Wi-Fi and drag the joystick: the wand follows in real time. A small drag moves it slowly; a full drag moves it at full speed. Let go and it stops.
- **Speed slider.** Set how lively the wand is, from a slow crawl to full speed, right on the main screen. Every open phone stays in sync.
- **Live position.** The main screen shows where each axis is.
- **Several phones at once.** Anyone on your Wi-Fi can open the interface and watch or play.

## Safety

- **Travel limits.** Each axis has a minimum and a maximum that the robot never exceeds, whatever sends the command (phone, API or Home Assistant). The wand brakes smoothly before reaching them.
- **Safe until calibrated.** Out of the box, movement stays within a conservative 60°–120° range on each axis.
- **Stow.** A big button, always visible, puts the wand back to its rest position and stops everything else. Nothing can interrupt it. At rest the servos are released, so they don't buzz or heat up.
- **Dead-man rule.** If your phone disconnects or stops sending for half a second, the wand stops.
- **Smooth motion.** Maximum speed and acceleration per axis, so starts and stops never jerk the Mechanism.
- **Gentle power-up.** The robot remembers where the wand last rested, starts there so the servos don't jump, then crawls back to its Stow position.
- **Optional settings PIN.** Guests can play and Stow, but changing limits, Calibration, wiring, name, Wi-Fi, Home Assistant or firmware asks for the PIN. Repeated wrong PINs lock it for a growing delay.

## Setup

- **Web flasher.** Install the firmware from Chrome or Edge on a computer in one click, with no tools to install. The right file for your chip is picked automatically. Choose the Stable or Beta channel. Reinstalling over a robot keeps its Wi-Fi and settings unless you tick "Erase device".
- **Wi-Fi in two ways.** Enter your Wi-Fi in the web flasher right after installing, or join the robot's own `CatWandConnect-XXXX` hotspot, whose setup page opens by itself and lists nearby networks, strongest first.
- **Self-healing Wi-Fi.** If your router disappears, the hotspot comes back; the robot keeps retrying your network and leaves the hotspot as soon as it returns.
- **Calibration wizard.** A step-by-step guide to set the Travel limits and Stow position for your build: each step brings the wand to the current value, nudge it to the right end, pick the rest position, then watch a slow test sweep before saving. Cancel at any time to keep the previous values.
- **Your wiring.** Choose which GPIO drives each servo (only the pins that are safe on your chip are offered), reverse an axis, and set each servo's pulse range.

## Everyday use

- **Name your robot.** "CatWandConnect salon" becomes `http://catwandconnect-salon.local/` and names its hotspot and its Home Assistant device.
- **English and French.** The interface follows your phone's language, or the one you choose in Settings.
- **Home-screen icon.** Add the interface to your phone's home screen; it opens full screen like an app.
- **Info page.** Firmware version, chip, Wi-Fi signal, IP address, uptime, temperature and memory, with a Copy button for bug reports.
- **Diagnostic report.** On the Info page, a ready-to-attach report for bug reports: firmware, chip, why it last restarted, memory, Wi-Fi and Home Assistant state, settings, wiring and the robot's recent log. Your Wi-Fi name, addresses and broker details are hidden. Copy it or download it as a text file; a full version, for your own use only, keeps them.

## Help

- **Where to get help.** A [Discord server](https://discord.gg/Rbqge3v9dk) for questions, build help and sharing builds, in several languages and with no GitHub account needed; GitHub issue forms for bugs and installation problems; private reporting for security problems. See [Getting help](../SUPPORT.md).

## Updates

- **Firmware updates from the browser.** Upload the new file in Settings. Settings and Calibration are kept, and if the upload fails or is interrupted the robot keeps its current firmware.
- **Releases.** Each version is published with ready-made files for every supported chip.
- **Stable and Beta channels.** Stable versions are for everyone. Beta versions get new changes first so they can be tested on real robots; the web flasher lets you pick either one.

## Integrations

- **Home Assistant (optional).** Enter your MQTT broker in Settings and the robot appears in Home Assistant by itself, with a Stow button, each axis's position and maximum speed, and its activity. Commands follow the same safety rules as the phone. See [Home Assistant](home-assistant.md).
- **Open API.** Everything the interface does goes through a documented HTTP, WebSocket and MQTT API, for your own scripts and integrations. See [Device API](api.md).

## Hardware

- **Boards:** ESP32, ESP32-S3 and ESP32-C3. ESP8266 boards are not supported ([ADR 0008](adr/0008-no-esp8266-support.md)).
- **Mechanism:** the Mk2, with two MG996R servos.

## Planned

- **Automatic play sessions** (V1.1): the robot plays on its own, within its Travel limits.
- **A dedicated Home Assistant integration**, built on the same API ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md)).
- **Bluetooth control.**
