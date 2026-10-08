# Cat Playing Platform

This project provides an automatic toy platform for cats, designed to be controlled by an ESP32 through ESPHome. It is available in two mechanical versions: a simple V1 and a more evolved V2 with a skateboard bearing and 4 mm steel balls.

## BOM by version

### Version V1

Deprecated: this first mechanical design was too demanding for the servo axis. After a few hours of play, the vertical servo could overheat and fail because of the mechanical stress and repeated load on the shaft. I corrected the design several times, and the V2 was reinforced to make the mechanism more resilient and durable.

Files for this version are available in the folder [cat_playing_platform_V1](cat_playing_platform_V1).

#### Printed parts

* [CLAMP - Clamp.stl](cat_playing_platform_V1/CLAMP%20-%20Clamp.stl.nextcloud)
* [CLAMP - Pad.stl](cat_playing_platform_V1/CLAMP%20-%20Pad.stl.nextcloud)
* [CLAMP - Screw.stl](cat_playing_platform_V1/CLAMP%20-%20Screw.stl.nextcloud)
* [CLAMP - Screw_pad.stl](cat_playing_platform_V1/CLAMP%20-%20Screw_pad.stl.nextcloud)
* [Part Studio 1 - Part 2.stl](cat_playing_platform_V1/Part%20Studio%201%20-%20Part%202.stl.nextcloud)
* [Part Studio 1 - Part 3.stl](cat_playing_platform_V1/Part%20Studio%201%20-%20Part%203.stl.nextcloud)
* [Servo support.stl](cat_playing_platform_V1/Servo%20support.stl.nextcloud)
* [Stick support.stl](cat_playing_platform_V1/Stick%20support.stl.nextcloud)

#### External parts

* 1 ESP32 (with USB-C connector recommanded)
* 2 micro-servos MG996R
* 1 stable power supply

### Version V2

This version is paid and is not included in the public Git repository. If you want access to it, use the purchase link below once it is available:

* Purchase link: [Coming soon]()

#### Printed parts

* cat_playing_platform_v2 - arm.stl
* cat_playing_platform_v2 - bearing.stl
* cat_playing_platform_v2 - clamp.stl
* cat_playing_platform_v2 - rotor_plate.stl
* cat_playing_platform_v2 - screw.stl
* cat_playing_platform_v2 - top_plate.stl

#### External parts

* 1 ESP32 (with USB-C connector recommanded)
* 2 micro-servos MG996R
* 1 stable power supply
* 1 skateboard bearing (608 type recommended)
* 4 mm steel balls: 16 to 24 pieces recommended, to be adjusted according to the mechanical finish

#### Required screws (M3 only)

* 4x M3x8 mm button-head screws: horizontal servo mounting
* 2x M3x25 mm button-head screws: fixing the lower plate of the horizontal axis to the ball track
* 1x M3x8 mm countersunk screw: fixing the horizontal servo shaft plate
* 1x M3x20 mm countersunk screw: fixing the vertical servo shaft
* 4x M3x8 mm button-head screws: vertical servo mounting
* 3x M3x30 mm countersunk screws: fixing the skateboard bearing to the vertical shaft

## Repository structure

* [cat-playing-platform.yaml](cat-playing-platform.yaml) : main ESPHome configuration
* [board.yaml](board.yaml) : pin mapping and board-specific settings
* [docs/electronics.md](docs/electronics.md) : electronic implementation documentation
* [docs/assembly.md](docs/assembly.md) : mechanical assembly guide
* [docs/troubleshooting.md](docs/troubleshooting.md) : troubleshooting guide

## ESPHome implementation

### 1. Prerequisites

* Install ESPHome on your machine or via Home Assistant
* Have an ESP32 board available
* Have Wi-Fi access for the initial flashing step

### 2. Configure Wi-Fi in ESPHome

Copy `sources/secrets.yaml.example` to `sources/secrets.yaml` and fill in your Wi-Fi credentials, API encryption key, OTA password and fallback hotspot password. `secrets.yaml` is git-ignored and must never be committed.

### 3. Upload the configuration

From a terminal, run:

```bash
esphome run cat-playing-platform.yaml
```

The first build flashes the firmware onto the board. If the board is not connected yet, plug it in via USB and rerun the command.

### 4. Use the board after flashing

Once the firmware is loaded:



1. The board connects to the Wi-Fi network configured in ESPHome.
2. It then appears in ESPHome or Home Assistant as a device named `Cat Playing Platform`.
3. The play sequence switch can be used to trigger the platform motion.

## Electronic documentation

The detailed documentation for servo and power connections is available in [docs/electronics.md](docs/electronics.md).

## Additional documentation topics to consider

Other useful sections can be added later:

* step-by-step mechanical assembly procedure
* servo calibration settings
* safety and component protection guidelines
* Wi-Fi and OTA troubleshooting procedure
* final assembly photo and version overview


