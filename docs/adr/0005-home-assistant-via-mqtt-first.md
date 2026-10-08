# Home Assistant via MQTT discovery first, dedicated integration later

A dedicated Home Assistant integration (WLED's model) gives the best experience but means maintaining Python code beside the firmware. V1 instead publishes MQTT discovery messages, which costs little firmware code but requires an MQTT broker on the user's side; setup is covered by documentation only. A dedicated integration may follow, built on the same documented REST/WebSocket API the Device interface uses.
