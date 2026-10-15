# CatWandConnect

An open-source robot that plays with a cat by moving a toy wand. It runs on its own, with a dedicated interface served by the robot itself, and can optionally be connected to Home Assistant.

## Language

**CatWandConnect**:
The robot as a whole: the printed mechanism, its two servos and the ESP32 that drives them.
_Avoid_: Cat Playing Platform, Platform, Cat-Bot

**Standalone mode**:
The robot operating with no Home Assistant instance: all play logic and its user interface live on the robot.
_Avoid_: Offline mode

**Device interface**:
The user interface served by the robot itself, usable by people without Home Assistant.
_Avoid_: Web UI, app

**Axis**:
One of the wand's two motions, each driven by its own servo: **Horizontal** (left–right) and **Vertical** (up–down).
_Avoid_: Pan, tilt, X, Y

**Manual control**:
A person moving the wand directly, in real time, from the Device interface.
_Avoid_: Shake, jog

**Travel limits**:
The user-set minimum and maximum position of each axis, which the robot never exceeds whatever the source of the command, so the mechanism never hits anything.
_Avoid_: Endstops, butées

**Stow**:
Moving the wand to its rest position and stopping everything else. Takes priority over any other command.
_Avoid_: Park, home

**Minimum speed**:
The slowest speed of an axis under Manual control, at the bottom of the speed slider: the slowest it still moves smoothly. Set per axis, below the maximum speed.
_Avoid_: Crawl speed, low speed

**Speed range**:
An axis's speeds from its Minimum speed to its maximum speed. The main screen's speed slider moves within it.
_Avoid_: Speed limits (Travel limits are positions, not speeds)

**Speed tuning**:
The guided process in which the robot sweeps one axis between its Travel limits while the owner makes it faster or slower, then keeps that speed as the axis's Minimum speed or maximum speed. Distinct from Calibration, which sets positions: the Calibration wizard goes on with Speed tuning steps once the Travel limits are saved, but skipping or leaving them does not undo the Calibration.
_Avoid_: Speed calibration

**Acceleration tuning**:
The Speed tuning variant for an axis's maximum acceleration: the robot makes short back-and-forth moves at the axis's maximum speed, each from a stop, while the owner makes them snappier or softer, then keeps the value. The Calibration wizard runs it right after each axis's maximum speed.
_Avoid_: Accel calibration, ramp tuning

**Play session**:
The robot playing on its own for the Session duration: each Axis loops, independently of the other, through a move to a random position within its Travel limits at a random speed within its Speed range, then a pause within its Pause range. Ends with Stow when time is up, on Stop or on Stow; Manual control ends it at once, without Stow. Never resumes and does not survive a restart.
_Avoid_: Auto mode, automatic play, program, routine

**Pause range**:
How long an axis rests between two moves of a Play session: a minimum and a maximum in seconds (0–60), set per axis. 0 means non-stop.
_Avoid_: Delay, wait time, idle time

**Session duration**:
How long a Play session lasts, in minutes (1–60). Remembered for the next session.
_Avoid_: Timer, play time

**Calibration**:
The guided process in which the user sets each axis's Travel limits and the Stow position, then confirms the robot can sweep that envelope without hitting anything.
_Avoid_: Setup, homing

**MQTT connection**:
Connecting the robot to Home Assistant through an MQTT broker and MQTT discovery: the robot publishes its entities, Home Assistant's MQTT integration picks them up. Available since V1, kept alongside the Native integration.
_Avoid_: MQTT mode, legacy integration

**Native integration**:
The CatWandConnect Home Assistant integration (V2, installed through HACS from `vistorx/ha-catwandconnect`), which finds the robot on the network and talks to it directly, with no broker. Support for it happens in this repository.
_Avoid_: HACS integration, custom component, add-on, app (an add-on/app is something else in Home Assistant)

**API version**:
The version number of the documented Device API contract (`docs/api.md`), starting at 1, returned by `GET /api/info` and announced in the robot's `_catwandconnect._tcp` mDNS service. Goes up only on a change that would break an existing integration; additive changes keep it. Distinct from the firmware version.
_Avoid_: Protocol version, firmware version (that is `version`)

## Hardware

**Mechanism**:
A printed mechanical design of the robot, identified by a Mark number (Mk1, Mk2…). Independent from firmware version numbers.
_Avoid_: Version, V1, V2 (reserved for firmware releases)

**Mk1**:
The first Mechanism, retired because its vertical servo overheated under load. Its files were removed from the repo; they remain in the Git history.

**Mk2**:
The current reference Mechanism, reinforced with a 608 skateboard bearing and a 4 mm steel-ball track. The only one the firmware targets.

## Releases

**Release channel**:
Which firmware releases a user is offered: **Stable** (`vX.Y.Z`, the default) or **Beta** (`vX.Y.Z-beta.N` pre-releases, tested on real robots before going stable).
_Avoid_: Nightly, dev build, preview (a dev build is an untagged local or CI build, not a channel)

**Update channel**:
The Release channel a given robot follows: a robot setting (Settings → Robot → Firmware update), stored on the robot and kept across restarts and updates, so every phone and Home Assistant offer it the same versions. Never set, it is the channel of the firmware it runs (a `-beta` version is on Beta).
_Avoid_: Update track, branch

**Update feed**:
The file listing the newest version of each Release channel, its release notes and the OTA image per chip family: the web flasher's `channels.json` on GitHub Pages. The Device interface and the Native integration read it to offer a one-tap update; the robot never reads it itself (ADR 0012).
_Avoid_: Update server, manifest (the manifest is the web flasher's per-channel install file)

## Support

**Diagnostic report**:
The text the robot generates about itself to diagnose a problem: firmware version, chip, last reset reason, settings, Calibration, Wi-Fi and MQTT state, and recent log lines. Redacted (Wi-Fi name, IP and MAC addresses, broker address and credentials) whenever it is sent to support.
_Avoid_: Crash report, dump, telemetry (the robot never sends anything by itself)

**Pending report**:
A problem described while the phone had no internet (on the hotspot), kept in the robot's flash with its redacted Diagnostic report until the owner sends or discards it once the robot is on their network. One at a time; never sent by itself.
_Avoid_: Queued report, outbox, offline report

**Support relay**:
A small online service that receives a Diagnostic report from the user's browser and creates the GitHub issue, so people without a GitHub account can report bugs. It holds the secrets the open-source firmware cannot.
_Avoid_: Backend, cloud, server (the robot has none)

**Device ID**:
A random 8-hex-digit identifier the robot draws once and keeps in flash, shown in the Diagnostic report so support can group the reports of one robot. Not personal data, unlike the MAC address, so it stays in the redacted report.
_Avoid_: Serial number, MAC, UUID
