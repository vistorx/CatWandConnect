# Wire the electronics

How to wire the two servos and the power supply to the board. The firmware drives the servos with 50 Hz PWM pulses; the board only sends the signals and never powers the servos.

## Parts

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 dev board | Any board with USB, from about €3. USB-C is handy. |
| 2 | MG996R servos | Standard-size metal-gear servos. |
| 1 | 5 V power supply, 3 A or more | Powers the servos and the board. |
| 1 | Capacitor, 470–1000 µF, 10 V or more | Optional, against resets and stutter. |
| 1 | Terminal block, Wago-style connectors or perfboard | The +5 V and GND rails. |
| | Wires, Dupont connectors | Thick enough for the servos' 5 V and GND. |

## Supported boards

Any ESP32, ESP32-S3 or ESP32-C3 development board. ESP8266 boards are not supported ([ADR 0008](adr/0008-no-esp8266-support.md)).

## Default pins

| Chip | Horizontal servo | Vertical servo |
|------|------------------|----------------|
| ESP32 | GPIO 18 | GPIO 19 |
| ESP32-S3 | GPIO 4 | GPIO 5 |
| ESP32-C3 | GPIO 4 | GPIO 5 |

On the S3 and C3, GPIO 18 and 19 carry USB, so the defaults differ. To wire differently, choose the pins in **Settings → Hardware → Wiring**; the robot restarts to apply them. The list only offers pins that are safe on your chip.

## Wiring

The robot runs from a single 5 V supply that feeds the two servos and the board.

Each MG996R has three wires: **orange** (signal), **red** (+5 V) and **brown** (ground).

| Wire | Connect to |
|------|------------|
| Servo red | +5 V |
| Servo brown | GND |
| Horizontal servo orange | GPIO 18 (S3/C3: GPIO 4) |
| Vertical servo orange | GPIO 19 (S3/C3: GPIO 5) |
| Board 5V / VIN pin | +5 V |
| Board GND pin | GND |

Use a small terminal block, a Wago-style connector or a perfboard as the +5 V and GND "rails" that everything plugs into.

![Wiring: a 5 V power supply feeds a +5 V rail and a GND rail; both servos' red and brown wires and the board's 5V and GND pins connect to the rails; GPIO 18 goes to the horizontal servo's orange wire and GPIO 19 to the vertical servo's; a capacitor sits across the rails near the servos.](images/wiring.svg)

- One 5 V supply of **3 A or more** powers everything; the board takes its power from its **5V/VIN** pin.
- Plug the board's USB into a computer only to flash or read logs. Most dev boards have a diode that keeps the external 5 V from flowing back into the computer, but not all: if you're not sure about yours, unplug the 5 V supply first.

## Rules

- **Common ground.** The board's GND must be connected to the supply's −, or the servos will jitter or ignore the signal.
- **Never power the servos from the board.** Not from its 3.3 V pin, and not from its 5 V pin when it runs on USB. An MG996R can draw over 1 A when it stalls.
- **Size the supply.** Allow 3 A or more for two MG996R. A weak supply resets the board when both servos start together, often seen as the robot rebooting when the wand moves.
- **Thick power wires.** Keep the 5 V and ground wires to the servos short and reasonably thick.
- **Capacitor.** If the board resets or motion stutters, add a capacitor (470–1000 µF, 10 V or more) across the servo supply, close to the servos.

## Servo pulse range

The defaults come from the proof of concept: 400–2200 µs for the Horizontal servo and 600–2400 µs for the Vertical one, mapped to 0°–180°. If your servos don't reach their full travel, or buzz at an end, adjust the pulse range in **Settings → Hardware → Wiring**. The same section reverses an axis whose direction is mirrored. Either change resets Calibration, so run it again afterwards.

Next: [Install the software](install.md).
