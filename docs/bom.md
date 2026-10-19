# Full parts list

Everything you need to build a CatWandConnect, in one place. Each guide lists only what its own step needs.

## Printed parts

The seven parts of the Mk3 Mechanism, printed in [step 1](print.md). The files are in [`stls/mk3/`](../stls/mk3/), and [`catwandconnect-mk3.3mf`](../stls/mk3/catwandconnect-mk3.3mf) holds the whole plate.

| Qty | Part | File |
|-----|------|------|
| 1 | Arm | [`catwandconnect-mk3-arm.stl`](../stls/mk3/catwandconnect-mk3-arm.stl) |
| 1 | Bearing holder | [`catwandconnect-mk3-bearing_holder.stl`](../stls/mk3/catwandconnect-mk3-bearing_holder.stl) |
| 1 | Clamp | [`catwandconnect-mk3-clamp.stl`](../stls/mk3/catwandconnect-mk3-clamp.stl) |
| 1 | Door | [`catwandconnect-mk3-door.stl`](../stls/mk3/catwandconnect-mk3-door.stl) |
| 1 | Rotor plate | [`catwandconnect-mk3-rotor_plate.stl`](../stls/mk3/catwandconnect-mk3-rotor_plate.stl) |
| 1 | Screw | [`catwandconnect-mk3-screw.stl`](../stls/mk3/catwandconnect-mk3-screw.stl) |
| 1 | Top plate | [`catwandconnect-mk3-top_plate.stl`](../stls/mk3/catwandconnect-mk3-top_plate.stl) |

## Electronics

Wired in [step 2](wiring.md).

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 dev board | Any board with USB, from about €3. Not ESP8266 ([ADR 0008](adr/0008-no-esp8266-support.md)). |
| 2 | MG996R servos (sometimes sold as SG996R) | Standard-size metal-gear servos, 180°. Each comes with its servo horns. |
| 1 | USB-C PD charger, 30 W minimum, 45 W recommended | Must offer 20 V (USB Power Delivery). |
| 1 | USB-C PD trigger board, set to 20 V | Asks the charger for 20 V. |
| 1 | Buck (step-down) converter, 5 V output, 5 A or more | 8 A gives headroom. |
| 1 | Capacitor, 470–1000 µF, 10 V or more | Optional, against resets and stutter. |
| 2 | Lever connectors (Wago 221 style), 2 ports | The +5 V and GND rails. |
| 2 | Servo extension cables, about 30 cm | Cut in half for the servos and the board. |
| | Wire, about 0.5 mm² (AWG 20), and female Dupont connectors | |

Example listings for the servos, the trigger board and the buck converter are in the wiring guide's [Example parts](wiring.md#example-parts). Nothing more is needed to [install the software](install.md) (step 3), except a USB data cable and a computer with Chrome or Edge.

## Assembly hardware

Used in [step 4](assembly.md).

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 608 skateboard bearing | |
| 27–28 | 6 mm balls | Between the top plate and the rotor plate. Airsoft BBs work very well. |
| 19 | M3 screws | Lengths coming soon. 8 for the two servos, 1 for each servo axis, 4 for the top plate, 2 for the bearing holder, 3 for the door. |
| 2 | Servo horns | Supplied with the servos. |
| | Double-sided tape | Holds the USB-C trigger board in the top plate. |
| | Grease | For the ball groove, for example lithium or PTFE grease. |
| 1 | Toy wand | Any cat wand with a stick that fits the arm. |

## Tools

- A 3D printer, or a print service or maker space ([step 1](print.md)).
- A soldering iron and a multimeter ([step 2](wiring.md)).
- A computer with Chrome or Edge, and a USB data cable ([step 3](install.md)).
- A screwdriver for M3 screws ([step 4](assembly.md)).

## Optional upgrades

| Part | Why |
|------|-----|
| 2 × DS3218 servos, 180° version | Much sturdier and more torque than the MG996R, same standard size. Use the 8 A buck converter and a 45 W charger with them. See [Stronger servos: DS3218](wiring.md#stronger-servos-ds3218). |
