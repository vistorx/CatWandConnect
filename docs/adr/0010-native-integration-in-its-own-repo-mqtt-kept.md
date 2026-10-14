# Native Home Assistant integration in its own repository, MQTT discovery kept

V2 adds the dedicated integration that ADR 0005 deferred: a Home Assistant custom integration, installed through HACS, that finds the robot by zeroconf and talks to it over the documented REST/WebSocket API, so users need no MQTT broker. HACS manages one integration per repository, expects `custom_components/<domain>/` and `hacs.json` at the repository root, and treats the repository's releases as integration versions; firmware releases here would show up as integration updates. The integration therefore lives in its own public repository, `vistorx/ha-catwandconnect`, released on its own version line. Firmware-side work (the mDNS service, a versioned API contract the integration checks) stays in this repository.

MQTT discovery is not deprecated: both are supported for good, and the Home Assistant docs present them as two ways to connect, each with its own ready-made dashboard. Users who already run MQTT keep it; others get the shorter setup.

Support stays in this repository (ADR 0009). The integration's repository has issues turned off; its README, `SUPPORT.md` and manifest `issue_tracker` point at this repository's issues and Discord, and integration bugs carry an `ha-integration` label here.
