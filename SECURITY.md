# Security policy

## Reporting a vulnerability

Please **do not** report security problems in public issues, discussions or on Discord.

Report them privately through GitHub: open the repository's [Security tab](https://github.com/vistorx/CatWandConnect/security) and choose **Report a vulnerability**. Only the maintainer sees the report. You need a GitHub account.

Include what you found, how to reproduce it, which firmware version and chip you used, and what an attacker could do with it. We will acknowledge the report, keep you informed, and credit you in the release notes unless you prefer otherwise. CatWandConnect is maintained in spare time, so there is no guaranteed response time, but security reports are handled first.

## Supported versions

Only the latest stable release and the latest beta receive fixes. Update through the web flasher or Settings → Firmware update before reporting.

## Scope

In scope: the CatWandConnect firmware, its Device interface and Device API, the Home Assistant MQTT integration, the web flasher site and the release workflow.

The robot is designed for a trusted home network. These are known limits, not vulnerabilities:

- The Device interface is served over plain HTTP on the local network, as ESP32 devices on `.local` cannot get a trusted certificate (see [ADR 0007](docs/adr/0007-plain-web-interface-no-pwa.md)).
- Anyone on your network can play with the robot and Stow it. The optional settings PIN protects the settings, Calibration, Wi-Fi, Home Assistant and firmware updates; it does not lock play.
- The setup hotspot has no password. It is only active during first setup or while the robot cannot reach your Wi-Fi, and a nearby person can join it during that time.

Reports that show a way around the settings PIN, a way to change settings or firmware without it, a way to make the robot exceed its Travel limits, or a leak of the Wi-Fi or MQTT password are very welcome.
