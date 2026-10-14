# Home Assistant via MQTT discovery first, dedicated integration later

A dedicated Home Assistant integration (WLED's model) gives the best experience but means maintaining Python code beside the firmware. V1 instead publishes MQTT discovery messages, which costs little firmware code but requires an MQTT broker on the user's side; setup is covered by documentation only. A dedicated integration may follow, built on the same documented REST/WebSocket API the Device interface uses.

Update: the dedicated integration is planned for V2, alongside MQTT discovery, see [ADR 0010](0010-native-integration-in-its-own-repo-mqtt-kept.md).
