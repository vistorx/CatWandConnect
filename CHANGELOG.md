# Changelog

What changed in each CatWandConnect firmware version, for owners. Update from your robot: **Settings → Robot → Firmware update** with the `catwandconnect-<chip>-ota.bin` file of the version; settings and Calibration are kept. New robot: the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/).

Beta versions get new features first, to be tested on real robots; stable versions are for everyone.

## Unreleased

- **Screw lengths.** The parts list and the assembly guide now give the length and head of every M3 screw.
- **Mk3 CAD sources.** The STEP files of every Mk3 part, and of the whole Mechanism, are in `cads/mk3/`: open them in any CAD tool to adapt a part.
- **A new look.** New banner and icon on the README, the website (tab icon, header) and the web flasher.
- **Find the docs from anywhere.** A Documentation button next to Support the project at the top of the README, and the web flasher now has the website's menu (Home, Install, Build, Home Assistant, Use, Community) and a language switch.
- **Clearer docs.** The README walks you through the build in five steps — print, electronics, software, assembly, Home Assistant — each guide starts with what you need, and a full parts list gathers everything. The Mk3 assembly guide is written step by step.
- **Mk2 files removed.** The Mk3 is now the only Mechanism; the Mk2's printable files left the repository (they stay in the Git history).
- **Mk3 parts list.** The Mk3 rolls on 27–28 balls of 6 mm between the top plate and the rotor plate; airsoft BBs do the job very well. The 608 skateboard bearing stays.
- **Mk3: everything inside.** The new reference Mechanism houses the board and the power boards inside the robot, behind a printed door, with a single USB-C connector. Files in `stls/mk3/`.
- **Print without supports.** The print guide now says which parts need supports: none, except under the clamp's horizontal surfaces (5° overhang threshold in PrusaSlicer), along with tested print settings.

## 0.2.1 - 2026-10-16

**Several robots on one network, and a step-by-step build guide.** A new robot gets its own `.local` address, and the wiring guide now walks you through a USB-C powered build.

### Highlights

- **Several robots on one network.** A new robot that keeps the default name answers at its own address, like `catwandconnect-3f9a.local` (shown at the end of setup and in Settings), so two robots no longer clash. Robots already set up keep `catwandconnect.local`, and Home Assistant entities don't change.
- **Step-by-step electronics build.** The wiring guide now walks through the reference build: solder the power boards, check the voltages, then wire both servos and the board on the table with two lever connectors (Wago 221 style) and cut servo extension cables, ready for a first test. The parts list gains those two items.
- **Power it from a USB-C charger.** The wiring guide now shows the USB-C power chain (a 30–45 W charger, a PD trigger board set to 20 V, a buck converter set to 5 V), how to set it up safely, and example parts.
- **Stronger servo option.** The wiring guide now links example MG996R servos and the sturdier DS3218 upgrade (180° version, pulse range and power notes).

### Update

Tap **Update** on the robot's page, or install `catwandconnect-<chip>-v0.2.1-ota.bin` from this release in Settings → Firmware update. Settings and Calibration are kept, and a robot already set up keeps its address.

### Known issues

- The ESP32-S3 and ESP32-C3 builds are tested in CI, not yet on real boards.

## 0.2.0 - 2026-10-16

**The robot plays on its own, and it lives in Home Assistant.** Start a Play session and the wand hunts like prey, then puts itself away. Connect it to Home Assistant without MQTT, set it up in one guided Calibration, and update it in one tap.

### Highlights

- **Play session.** 1 to 60 minutes: each axis moves to random spots within your Travel limits, at random speeds, with short random pauses (0 to 2 s by default), never in step with the other. Touch the joystick to take over at any time.
- **Native Home Assistant integration.** Install [`vistorx/ha-catwandconnect`](https://github.com/vistorx/ha-catwandconnect) from HACS (Home Assistant 2026.10 or later): Play session, duration, Stow, positions, speeds, a joystick card and firmware updates, live within a second, no broker. The MQTT connection stays, now with the Play session too. The [Home Assistant guide](https://vistorx.github.io/CatWandConnect/home-assistant/) presents both, each with a ready-made dashboard.
- **One guided Calibration.** After the limits, the wizard sweeps each axis so you pick its minimum and maximum speed, then lets you feel the acceleration with Snappier or Softer. Also in Settings → Motion → Tune speeds and acceleration.
- **Update in one tap.** When a new version is out, the robot's page says so and installs it, on the Stable or Beta channel you choose. Home Assistant shows it too, and the Native integration installs it. The robot itself still never goes on the Internet.
- **Change Wi-Fi from Settings.** Settings → Network shows the network, IP and `.local` address, with **Change Wi-Fi network**. A wrong password sends the robot back to the network it was on.
- **On your home screen.** On a phone, the robot's page offers its permanent `.local` address, then shows how to add it to your home screen so it opens like an app.
- **Project website in English and French** at <https://vistorx.github.io/CatWandConnect/>: one page per step, from printing the parts to Home Assistant.
- **For scripts and integrations:** API version 1, `_catwandconnect._tcp` announcement, `POST /api/position`. See the [Device API](https://vistorx.github.io/CatWandConnect/api/).

### Update

From 0.1.0: install `catwandconnect-<chip>-v0.2.0-ota.bin` from this release in Settings → Firmware update. From a 0.2.0 beta: tap **Update** on the robot's page. Settings and Calibration are kept. To set the acceleration on a robot calibrated before, open Settings → Motion → Tune speeds and acceleration.

### Known issues

- Home Assistant may not show the robot under *Discovered* ([#175](https://github.com/vistorx/CatWandConnect/issues/175)). Add it by hand: Settings → Devices & services → Add integration → CatWandConnect, and type the robot's address.
- Two robots that both keep the default name share `catwandconnect.local`: give each one its own name in Settings → Robot.
- The ESP32-S3 and ESP32-C3 builds are tested in CI, not yet on real boards.

## 0.2.0-beta.5 - 2026-10-16

**Your robot, one tap away.** Open its page on your phone and it shows you how to put it on your home screen, at an address that doesn't change. And Home Assistant should now find it on its own.

### Highlights

- **Found on its own by Home Assistant.** The robot's Wi-Fi no longer dozes between packets, so it answers Home Assistant's discovery and shows up under Discovered more reliably. Its page and the joystick respond faster too.
- **Your robot on your home screen.** The first time you open the robot's page on an iPhone or Android phone, a short tip shows how to add it to your home screen so it opens like an app. Find it again any time in Help.
- **The robot's permanent address first.** Open the robot's page by its IP address and it offers to switch to its `.local` address, which keeps working when your router changes the IP, before you add it to your home screen. The home-screen tip is now also in Settings → Robot.

### Update

Your robot's page offers this version: tap **Update** on the banner, or Settings → Robot → Firmware update. Your settings and Calibration are kept.

## 0.2.0-beta.4 - 2026-10-15

**Updates in one tap.** No more downloading files from GitHub: when a new version is out, your robot's page tells you and installs it for you. Pick Stable or Beta once, the robot remembers.

### Highlights

- **Update in one tap.** When a new version is out, the robot's main screen says so, with what's new and an **Update** button: your phone downloads the right file and installs it. Installing a file by hand is still there, in Settings → Robot → Firmware update.
- **Stable or Beta, your choice.** Choose the Update channel in Settings → Robot → Firmware update. The robot remembers it across restarts and updates; it starts on the channel of the version you installed.
- **Updates in Home Assistant.** With the Native integration (0.2.0 or later), a **Firmware** entity shows the installed and latest versions, and **Install** updates the robot from Home Assistant. With the MQTT connection, the Firmware entity shows when an update is out. Both ready-made dashboards show a Firmware badge when there is one.
- **Your robot stays private.** The robot still never goes on the Internet: your phone or Home Assistant checks for updates and brings the file.

### Update

This is the last update you install by hand: download `catwandconnect-<chip>-v0.2.0-beta.4-ota.bin` from the release and install it in Settings → Robot → Firmware update. From then on, the robot's page offers the next ones by itself. Settings and Calibration are kept.

## 0.2.0-beta.3 - 2026-10-14

**Set up a new robot in one go, and feel every setting before you keep it.** The Calibration wizard now goes on after the limits: it sweeps each axis so you pick its minimum and maximum speed, then makes short moves while you tap Snappier or Softer to set its acceleration. No more guessing numbers.

### Highlights

- **Calibration sets the speeds.** After saving the limits, pick each axis's minimum and maximum speed with Faster and Slower while the wand sweeps, or skip to keep the current one.
- **Feel the acceleration.** Right after each maximum speed, the wand makes short moves while you tap Snappier or Softer, so you choose how sharply it starts and stops. Also in Settings → Motion → Tune speeds and acceleration.
- **Change the robot's Wi-Fi from Settings.** Settings → Network shows the network, the robot's IP address and its `.local` address, with a **Change Wi-Fi network** button. A wrong password sends the robot back to the network it was on, so it never disappears.
- **Livelier Play sessions out of the box.** Default pauses are now 0 to 2 s on both axes, so the wand rests less. Pauses you already set are kept.
- **Home Assistant dashboards, upgraded.** The ready-made dashboards get a big Play session button (tap to start, tap again to stop), and the Native integration's adds the joystick (integration 0.1.1 or later). Screenshots of both are in the [Home Assistant guide](https://vistorx.github.io/CatWandConnect/home-assistant/).
- **Robot not under *Discovered*?** The guide now covers Home Assistant in a virtual machine (use a bridged network adapter) and how to check whether it hears the robot.

### Update

Update by OTA from 0.2.0-beta.2, 0.2.0-beta.1 or 0.1.0: settings and Calibration are kept. To set the new acceleration on an already calibrated robot, open Settings → Motion → Tune speeds and acceleration, or run the Calibration again.

## 0.2.0-beta.2 - 2026-10-14

**Home Assistant without MQTT.** The new CatWandConnect integration for Home Assistant finds your robot on the network by itself: click **Add**, and the Play session, Stow, positions and speeds are on your dashboard. No broker, no extra user, live updates within a second.

### Highlights

- **Native Home Assistant integration.** Install it from HACS ([`vistorx/ha-catwandconnect`](https://github.com/vistorx/ha-catwandconnect)), and the robot shows up under *Discovered*. Type the Settings PIN only if you set one. Needs Home Assistant 2026.10 or later.
- **The MQTT connection stays.** Already running MQTT? Nothing changes. The [Home Assistant guide](https://vistorx.github.io/CatWandConnect/home-assistant/) now presents both ways side by side, each with a ready-made dashboard to copy and paste.
- **Clearer docs, one topic per page.** Print the parts, assemble, wire, install, then connect to Home Assistant, in order, in English and French.
- **For scripts and integrations:** the API is versioned (version 1), the robot announces itself as `_catwandconnect._tcp`, and `POST /api/position` moves an axis to an angle. See the [Device API](https://vistorx.github.io/CatWandConnect/api/).

### Update

Update by OTA from 0.2.0-beta.1 or 0.1.0. If you use the MQTT connection, keep it or switch to the Native integration, but not both for the same robot.

## 0.2.0-beta.1 - 2026-10-14

**The robot plays on its own.** Start a Play session and the wand hunts like prey for the time you choose, then puts itself away.

### Highlights

- **Play session.** 1 to 60 minutes. Each axis moves to random spots within your Travel limits, at random speeds, with random pauses, never in step with the other. Touch the joystick to take over at any time.
- **Play session in Home Assistant.** A switch to start or stop it, and a number for its duration, through the MQTT connection.
- **Speed tuning.** Settings → Motion → Tune speeds: the robot sweeps an axis while you tap Faster or Slower, so you find the fastest speed before it shakes and the slowest before it stutters.
- **Minimum speed per axis.** The speed slider now runs from each axis's minimum to its maximum, for smooth slow moves.
- **Project website in English and French** at <https://vistorx.github.io/CatWandConnect/>, with search, the Mk2 video and a ready-made Home Assistant dashboard.

## 0.1.0 - 2026-10-13

**First stable release.** An ESP32 drives a wand on two servos, controlled from your phone or Home Assistant.

### Highlights

- **Setup:** install from the web flasher, Wi-Fi over USB or through the robot's own hotspot, or continue without Wi-Fi.
- **Phone interface** in English and French: joystick with speed control, Stow, optional Settings PIN, home-screen icon.
- **Calibration wizard** with Travel limits and "Reverse this axis"; settings in four tabs; firmware updates over the air.
- **Home Assistant over MQTT:** Stow, positions, maximum speeds and Activity.
- **Help built in:** a redacted Diagnostic report, and "Send to support" files an issue without a GitHub account, followed on [Discord](https://discord.gg/Rbqge3v9dk).

Tested on a classic ESP32 (125/125 checklist items). ESP32-S3 and ESP32-C3 are built and tested in CI, not yet on real hardware.

Earlier betas (0.1.0-beta.1 to beta.6): see the [GitHub releases](https://github.com/vistorx/CatWandConnect/releases).
