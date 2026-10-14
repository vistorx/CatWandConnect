# Changelog

What changed in each CatWandConnect firmware version, for owners. Update from your robot: **Settings → Robot → Firmware update** with the `catwandconnect-<chip>-ota.bin` file of the version; settings and Calibration are kept. New robot: the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/).

Beta versions get new features first, to be tested on real robots; stable versions are for everyone.

## Unreleased

- **Joystick in the Native integration's ready-made dashboard.** Move the wand with your finger right from Home Assistant (integration 0.1.1 or later).

- **Bigger Play session control in the ready-made dashboards.** The Play session is now a large button: tap to start, tap again to stop.

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
