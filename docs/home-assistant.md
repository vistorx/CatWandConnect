# Home Assistant

CatWandConnect works without Home Assistant. If you have it, the robot can appear in it by itself through **MQTT discovery**: you get a device with a Stow button, the position and maximum speed of each axis, its activity, and a Play session switch with its Session duration, ready for dashboards and automations.

## What you need

- Home Assistant with an **MQTT broker**. The easiest is the official **Mosquitto broker** add-on.
- The **MQTT integration** in Home Assistant, connected to that broker.
- The robot on the same network.

## 1. Set up the broker

If you already use MQTT, skip to step 2.

1. In Home Assistant, go to **Settings → Add-ons → Add-on store**, install **Mosquitto broker** and start it.
2. Create a user for the robot: **Settings → People → Users → Add user**, for example `catwandconnect`, with a password. Enable advanced mode in your profile if the Users tab is missing. Mosquitto accepts Home Assistant users.
3. Home Assistant usually discovers the broker: accept the **MQTT** integration it proposes under **Settings → Devices & services**.

## 2. Connect the robot

1. Open the robot's interface, then **Settings → Network → Home Assistant**.
2. Tick **Enabled**, and enter:
   - **Broker**: your Home Assistant's address, for example `homeassistant.local` or its IP;
   - **Port**: `1883`;
   - **User** and **Password**: the user from step 1.
3. Save. Within a few seconds, the status should say **Connected**.

If a settings PIN is set, saving asks for it.

## 3. Find it in Home Assistant

Go to **Settings → Devices & services → MQTT → Devices**. The robot appears under its name, with:

| Entity | Type | What it does |
|--------|------|--------------|
| Stow | Button | Puts the wand away and releases the servos. Beats every other command. |
| Horizontal position, Vertical position | Number (0–180°) | Shows where each axis is. Setting a value moves the axis there at the Manual control speed, within the Travel limits. |
| Horizontal maximum speed, Vertical maximum speed | Number (10–360 °/s) | The same setting as in the robot's Settings. A maximum at or below the minimum speed resets the minimum to a tenth of the new maximum. |
| Activity | Sensor | `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` or `playing`. |
| Play session | Switch | On starts a Play session for the Session duration; off stops it and Stows. It shows on while a session runs, whether it was started from Home Assistant or from the robot's interface, and turns off by itself when the session ends. Refused (the switch flips back off) until the robot is calibrated, and while it is stowing, calibrating or tuning a speed. |
| Session duration | Number (1–60 min) | How long the next Play session lasts, the same value as on the robot's main screen. Remembered once the robot is idle. |

When the robot is switched off or loses Wi-Fi, its entities show as unavailable.

Commands from Home Assistant go through the same safety rules as the phone: Travel limits, speed limits and priorities. Nothing from Home Assistant can push the wand beyond its limits, and Stow can't be interrupted.

## Example automation

Stow the wand every evening at 22:00:

```yaml
alias: Put the cat wand away at night
triggers:
  - trigger: time
    at: "22:00:00"
actions:
  - action: button.press
    target:
      entity_id: button.catwandconnect_stow
```

Start a 15-minute Play session when you leave for work on weekdays:

```yaml
alias: Play with the cat after we leave
triggers:
  - trigger: time
    at: "08:30:00"
conditions:
  - condition: time
    weekday: [mon, tue, wed, thu, fri]
actions:
  - action: number.set_value
    target:
      entity_id: number.catwandconnect_session_duration
    data:
      value: 15
  - action: switch.turn_on
    target:
      entity_id: switch.catwandconnect_play_session
```

Entity ids follow the robot's name: `button.catwandconnect_stow` by default, `button.catwandconnect_salon_stow` for a robot named "CatWandConnect salon".

## Troubleshooting

| Status in the robot's Settings | What to check |
|--------------------------------|---------------|
| Can't reach the broker | The broker address and port, and that the Mosquitto add-on is running. Try the IP instead of `homeassistant.local`. |
| The broker refused the user or password | The user and password; the user must exist in Home Assistant (or in Mosquitto's own logins). |
| Connected, but no device in Home Assistant | The MQTT integration is set up and uses the default discovery prefix `homeassistant`. |

The robot's MQTT topics are documented in the [Device API](api.md#home-assistant-mqtt). Still stuck? See [Getting help](../SUPPORT.md), or open **Help** in the robot's Settings → Robot: Home Assistant's device page has a **Visit** link to the robot's interface (MQTT discovery only allows one link per device, so it can't point to the Help page directly).

A dedicated Home Assistant integration may come later ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md)).
