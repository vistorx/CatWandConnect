# Wire the electronics

How to wire the two servos and the power chain to the board. The firmware drives the servos with 50 Hz PWM pulses; the board only sends the signals and never powers the servos.

## Parts

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 dev board | Any board with USB, from about €3. USB-C is handy. |
| 2 | MG996R servos (sometimes sold as SG996R) | Standard-size metal-gear servos. Upgrade: [DS3218](#stronger-servos-ds3218). |
| 1 | USB-C PD charger, 30 W minimum, 45 W recommended | Must offer 20 V (USB Power Delivery). See [Choosing the charger](#choosing-the-charger). |
| 1 | USB-C PD trigger board, set to 20 V | Asks the charger for 20 V and passes it on. |
| 1 | Buck (step-down) converter, 5 V output, 5 A or more | Turns the 20 V into the 5 V that powers the servos and the board. 8 A gives headroom. |
| 1 | Capacitor, 470–1000 µF, 10 V or more | Optional, against resets and stutter. |
| 1 | Terminal block, Wago-style connectors or perfboard | The +5 V and GND rails. |
| | Wires, Dupont connectors | Thick enough for the servos' 5 V and GND. |

A plain 5 V power supply of 3 A or more also works instead of the charger, trigger board and buck converter: connect it straight to the +5 V and GND rails.

### Example parts

These are the parts the reference build uses. Listings change over time and any equivalent part works. The links are examples, not sponsored or affiliate links.

| Part | Example listings (AliExpress) | What to look for |
|------|-------------------------------|------------------|
| MG996R servo (sometimes sold as SG996R) | [Example](https://fr.aliexpress.com/item/1005012099566939.html) | Standard size, metal gears, 180°. |
| DS3218 servo (upgrade) | [Example](https://fr.aliexpress.com/item/1005010498819386.html) | The **180°** version, metal gears, 20 kg·cm class. See [Stronger servos](#stronger-servos-ds3218). |
| USB-C PD trigger board | [Example 1](https://www.aliexpress.com/item/1005010610660644.html), [example 2](https://www.aliexpress.com/item/1005005461879717.html) | USB-C input, PD3.0/PD3.1 (QC3.0 is a bonus), selectable output from 5 to 48 V, 5 A. |
| Buck converter | [8 A](https://www.aliexpress.com/item/1005009516551580.html), [5 A](https://www.aliexpress.com/item/1005009289437405.html) | Input up to 25 V, output selectable or adjustable (3.3/5/9/12 V), set to 5 V. |

### Stronger servos: DS3218

The DS3218 costs more than the MG996R but is much sturdier and has much more torque (20 kg·cm class, metal gears). It has the same standard servo size, so it should fit the same mounts (not checked on the Mk2 yet).

- Pick the **180°** version. The DS3218 also comes as 270°, which would map the angles wrong.
- Its pulse range is about 500–2500 µs: set it in **Settings → Hardware → Wiring**, then run Calibration again. See [Servo pulse range](#servo-pulse-range).
- It draws more current when it stalls, up to about 2.5–3 A each. With two DS3218, use the 8 A buck converter and a 45 W charger.

## Supported boards

Any ESP32, ESP32-S3 or ESP32-C3 development board. ESP8266 boards are not supported ([ADR 0008](adr/0008-no-esp8266-support.md)).

## Default pins

| Chip | Horizontal servo | Vertical servo |
|------|------------------|----------------|
| ESP32 | GPIO 18 | GPIO 19 |
| ESP32-S3 | GPIO 4 | GPIO 5 |
| ESP32-C3 | GPIO 4 | GPIO 5 |

On the S3 and C3, GPIO 18 and 19 carry USB, so the defaults differ. To wire differently, choose the pins in **Settings → Hardware → Wiring**; the robot restarts to apply them. The list only offers pins that are safe on your chip.

## Power

The robot runs from a USB-C charger, through two small boards:

1. **USB-C charger** → **PD trigger board**, over a USB-C cable. The trigger board asks the charger for 20 V.
2. **PD trigger board** → **buck converter**: the 20 V goes into the buck converter's input.
3. **Buck converter** → **+5 V and GND rails**: its 5 V output feeds the two servos and the board.

Set both boards **before** you connect the servos or the board:

1. Set the trigger board to **20 V** (with its solder pad, jumper or button, as its listing explains), plug it into the charger and check its output with a multimeter: about 20 V.
2. Connect the buck converter, set its output to **5 V**, and check it with the multimeter: about 5 V.
3. Only then connect the buck converter's output to the +5 V and GND rails.

**Never select a trigger voltage above the buck converter's maximum input.** The example buck converters accept up to 25 V: 20 V is the right pick, and 28 V or more would destroy them.

### Choosing the charger

Take a good USB-C PD charger that offers 20 V: **30 W minimum, 45 W much better**. Two MG996R can each draw over 1 A when they stall, and the two boards lose a little power converting it. 30 W covers that; 45 W leaves headroom so the charger doesn't cut out when both servos start together.

## Wiring

The buck converter's 5 V output feeds the two servos and the board.

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

![Wiring: a USB-C charger (30–45 W) feeds a PD trigger board set to 20 V, which feeds a buck converter set to 5 V; the buck converter feeds a +5 V rail and a GND rail; both servos' red and brown wires and the board's 5V and GND pins connect to the rails; GPIO 18 goes to the horizontal servo's orange wire and GPIO 19 to the vertical servo's; a capacitor sits across the rails near the servos.](images/wiring.svg)

- The buck converter's 5 V output powers everything; the board takes its power from its **5V/VIN** pin.
- Plug the board's USB into a computer only to flash or read logs. Most dev boards have a diode that keeps the external 5 V from flowing back into the computer, but not all: if you're not sure about yours, unplug the USB-C charger first.

## Rules

- **Common ground.** The board's GND must be connected to the buck converter's output −, or the servos will jitter or ignore the signal.
- **Never power the servos from the board.** Not from its 3.3 V pin, and not from its 5 V pin when it runs on USB. An MG996R can draw over 1 A when it stalls.
- **Size the power.** Allow 5 A or more at 5 V for two MG996R: a buck converter of 5 A or more, a charger of 30 W or more (or a plain 5 V supply of 3 A or more). Weak power resets the board when both servos start together, often seen as the robot rebooting when the wand moves.
- **Thick power wires.** Keep the 5 V and ground wires to the servos short and reasonably thick.
- **Capacitor.** If the board resets or motion stutters, add a capacitor (470–1000 µF, 10 V or more) across the +5 V and GND rails, close to the servos.

## Servo pulse range

The defaults come from the proof of concept: 400–2200 µs for the Horizontal servo and 600–2400 µs for the Vertical one, mapped to 0°–180°. If your servos don't reach their full travel, or buzz at an end, adjust the pulse range in **Settings → Hardware → Wiring**. The same section reverses an axis whose direction is mirrored. Either change resets Calibration, so run it again afterwards.

Next: [Install the software](install.md).
