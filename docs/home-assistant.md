# Set up Home Assistant

CatWandConnect works without Home Assistant. If you have it, the robot can appear in it by itself: a device with a Play session switch and its duration, a Stow button, its activity, and each axis's position and maximum speed, ready for dashboards and automations.

The robot talks to Home Assistant through **MQTT**, a small messaging service that runs next to Home Assistant (the *broker*). The robot sends its state to the broker and receives commands from it; Home Assistant reads and writes the same messages. Setting it up takes about 10 minutes:

1. [Check the prerequisites](#1-check-the-prerequisites)
2. [Install the MQTT broker](#2-install-the-mqtt-broker)
3. [Create a user for the robot](#3-create-a-user-for-the-robot)
4. [Add the MQTT integration](#4-add-the-mqtt-integration)
5. [Connect the robot](#5-connect-the-robot)
6. [Find it in Home Assistant](#6-find-it-in-home-assistant)

Already use MQTT (Zigbee2MQTT, Tasmota...)? Your broker and integration are ready: create a user (step 3) and go to [step 5](#5-connect-the-robot).

## 1. Check the prerequisites

- **Home Assistant OS** (a Home Assistant Green or Yellow, a Raspberry Pi, a virtual machine). To check: **Settings → About**, the *Installation type* line. On **Home Assistant Container** or **Core** there is no *Apps* menu: install a Mosquitto broker yourself (for example its Docker image) and go to [step 4](#4-add-the-mqtt-integration).
- An **administrator** account in Home Assistant.
- The robot **installed and on your Wi-Fi** ([Install the software](install.md)), on the same network as Home Assistant.
- Home Assistant's **IP address**: **Settings → System → Network**, under *Network adapter*, for example `192.168.1.124`. Write it down for step 5. Give it a fixed address in your router (a *DHCP reservation*) so it never changes.

## 2. Install the MQTT broker

1. In Home Assistant, go to **Settings → Apps → Install app** (called *Add-ons* before Home Assistant 2026.2).
2. Search for **Mosquitto broker** and open it.
3. Click **Install**, wait, then turn on **Start on boot** and click **Start**.
4. Open the **Log** tab: after a few seconds it should end without errors.

Leave its *Configuration* tab as it is: the broker accepts Home Assistant's own users, which is what step 3 uses.

## 3. Create a user for the robot

The broker refuses anonymous connections, so the robot needs its own login.

1. Turn on **Advanced mode**: click your name at the bottom left, then turn on *Advanced mode* (otherwise the *Users* tab is hidden).
2. Go to **Settings → People → Users** and click **Add user**.
3. Fill in:
   - **Display name**: `CatWandConnect`;
   - **Username**: `catwandconnect` (not `homeassistant` or `addons`: the broker reserves them);
   - **Password**: a new password, used only by the robot. Note it for step 5.
4. Leave **Administrator** off, then click **Create**.

This user never logs in to Home Assistant's interface; it only lets the robot talk to the broker.

## 4. Add the MQTT integration

This connects Home Assistant itself to the broker.

1. Go to **Settings → Devices & services**.
2. Usually **MQTT** is waiting under *Discovered*: click **Add**, then **Submit**.
3. If it isn't there: **Add integration** at the bottom right, search for **MQTT**, choose **MQTT**, and confirm the Mosquitto broker.
4. **MQTT** now shows in the list of integrations, with no error.

Leave *Enable discovery* on and the discovery prefix at `homeassistant` (the defaults): the robot announces itself there.

## 5. Connect the robot

1. Open the robot's interface (`http://catwandconnect.local/`, or the name you gave it), then **Settings → Network → Home Assistant**.
2. Tick **Enabled**, and fill in:
   - **Broker**: Home Assistant's IP address from step 1, for example `192.168.1.124`. `homeassistant.local` also works on most networks, but the IP is more reliable;
   - **Port**: `1883`;
   - **User** and **Password**: the user from step 3.
3. Click **Save**. If a settings PIN is set, the robot asks for it.
4. Within a few seconds, **Status** says **Connected**. If it shows an error instead, see [Troubleshooting](#troubleshooting).

The robot reconnects by itself after a power cut or a Home Assistant restart.

## 6. Find it in Home Assistant

Go to **Settings → Devices & services → MQTT**, then **Devices**. The robot appears under its name. Its page lists these entities:

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

## Ready-made dashboard

A control panel for the robot, made only of built-in cards (nothing to install from HACS): the Play session switch and its duration, Stow, the Activity, each axis's position and maximum speed.

1. Go to **Settings → Dashboards → Add dashboard → New dashboard from scratch**, name it, for example "Cat wand", and open it.
2. Click the pencil (**Edit dashboard**), then the three dots → **Raw configuration editor**.
3. Replace everything with the YAML below and **Save**.

<!-- ha-dashboard -->
```yaml
title: Cat wand
views:
  - title: Cat wand
    path: catwandconnect
    type: sections
    max_columns: 3
    sections:
      - type: grid
        cards:
          - type: heading
            heading: Play session
            icon: mdi:cat
            badges:
              - type: entity
                entity: sensor.catwandconnect_activity
                show_state: true
          - type: tile
            entity: switch.catwandconnect_play_session
            name: Play session
            icon: mdi:play-circle-outline
            features:
              - type: toggle
            features_position: inline
          - type: tile
            entity: number.catwandconnect_session_duration
            name: Session duration
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: button.catwandconnect_stow
            name: Stow
            color: red
            tap_action:
              action: perform-action
              perform_action: button.press
              target:
                entity_id: button.catwandconnect_stow
      - type: grid
        cards:
          - type: heading
            heading: Position
            icon: mdi:axis-arrow
          - type: tile
            entity: number.catwandconnect_horizontal_position
            name: Horizontal
            features:
              - type: numeric-input
                style: slider
          - type: tile
            entity: number.catwandconnect_vertical_position
            name: Vertical
            features:
              - type: numeric-input
                style: slider
      - type: grid
        cards:
          - type: heading
            heading: Maximum speed
            icon: mdi:speedometer
          - type: tile
            entity: number.catwandconnect_horizontal_maximum_speed
            name: Horizontal
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: number.catwandconnect_vertical_maximum_speed
            name: Vertical
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
```

On a phone, the sections stack: Play session first, then the positions and speeds. To add the cards to an existing dashboard instead, copy only the three `- type: grid` sections into one of its sections views.

**Renamed robot?** The ids above are those of a robot with the default name. Entity ids follow the robot's name: for a robot named "CatWandConnect salon", replace every `catwandconnect_` with `catwandconnect_salon_` (find and replace in the editor). The exact ids are on the device page under **Settings → Devices & services → MQTT**.

## Automations

In **Settings → Automations & scenes → Create automation → Create new automation**, three dots → **Edit in YAML**, paste one of these, then **Save**.


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
| Can't reach the broker | The broker address and port `1883`, and that the Mosquitto broker app is started (step 2). Use Home Assistant's IP instead of `homeassistant.local`. |
| The broker refused the user or password | The user and password from step 3. Retype the password in the robot (it never shows the saved one). The username can't be `homeassistant` or `addons`. |
| Connected, but no device in Home Assistant | The MQTT integration is set up (step 4) with discovery on and the prefix `homeassistant`. |
| Entities unavailable | The robot is off or off Wi-Fi. They come back within a minute of it reconnecting. |
| Play session switch flips back off | The robot is not calibrated yet, or it is stowing, calibrating or tuning a speed. Calibrate it from its main screen. |

The robot's MQTT topics are documented in the [Device API](api.md#home-assistant-mqtt). Still stuck? See [Getting help](../SUPPORT.md), or open **Help** in the robot's Settings → Robot: Home Assistant's device page has a **Visit** link to the robot's interface (MQTT discovery only allows one link per device, so it can't point to the Help page directly).

A dedicated Home Assistant integration may come later ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md)).
