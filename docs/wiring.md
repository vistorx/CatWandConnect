# Wire the electronics

This guide gets you the board, the two servos and the USB-C power chain wired and tested on the table, ready for the software.

The firmware drives the servos with 50 Hz PWM pulses; the board only sends the signals and never powers the servos.

<!-- github-only -->
[![CatWandConnect tutorial - Wiring](https://i.ytimg.com/vi/qAPqGfsNPbk/hqdefault.jpg)](https://youtu.be/qAPqGfsNPbk)
<!-- /github-only -->
<iframe style="width:100%;aspect-ratio:16/9;border:0" src="https://www.youtube-nocookie.com/embed/qAPqGfsNPbk" title="CatWandConnect tutorial - Wiring" allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

The video shows every step of [Build it step by step](#build-it-step-by-step) ([watch on YouTube](https://youtu.be/qAPqGfsNPbk)).

## What you need

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32, ESP32-S3 or ESP32-C3 dev board | Any board with USB, from about €3. USB-C is handy. |
| 2 | MG996R servos (sometimes sold as SG996R) | Standard-size metal-gear servos. Upgrade: [DS3218](#stronger-servos-ds3218). |
| 1 | USB-C PD charger, 30 W minimum, 45 W recommended | Must offer 20 V (USB Power Delivery). See [Choosing the charger](#choosing-the-charger). |
| 1 | USB-C PD trigger board, set to 20 V | Asks the charger for 20 V and passes it on. |
| 1 | Buck (step-down) converter, 5 V output, 5 A or more | Turns the 20 V into the 5 V that powers the servos and the board. 8 A gives headroom. |
| 1 | Capacitor, 470–1000 µF, 10 V or more | Optional, against resets and stutter. |
| 2 | Lever connectors (Wago 221 style), 2 ports (e.g. 221-412) | The +5 V and GND rails: one port for the buck converter's output, the other for the servos' and board's wires twisted together. Bigger connectors, a terminal block or perfboard also work. |
| 3 | Servo extension cables, about 30 cm (JR/Futaba 3-pin, the standard kind) | Two take the servos, one feeds the board. See [Build it step by step](#build-it-step-by-step). |
| 40 cm | Wire, about 0.5 mm² (AWG 20), and female Dupont connectors | The wire for the power: 2 × 5 cm (trigger board to buck converter) and 2 × 15 cm (buck converter to the lever connectors in the technical cavity), one + and one − each. The Dupont ends for the board's pins. |

Tools: a soldering iron and a multimeter.

A plain 5 V power supply of 3 A or more also works instead of the charger, trigger board and buck converter: connect it straight to the +5 V and GND rails.

**[Full parts list →](bom.md)**

### Example parts

These are the parts the reference build uses. Listings change over time and any equivalent part works. The links are examples, not sponsored or affiliate links.

| Part | Example listings (AliExpress) | What to look for |
|------|-------------------------------|------------------|
| MG996R servo (sometimes sold as SG996R) | [Example](https://fr.aliexpress.com/item/1005012099566939.html) | Standard size, metal gears, 180°. |
| DS3218 servo (upgrade) | [Example](https://fr.aliexpress.com/item/1005010498819386.html) | The **180°** version, metal gears, 20 kg·cm class. See [Stronger servos](#stronger-servos-ds3218). |
| USB-C PD trigger board | [Example 1](https://www.aliexpress.com/item/1005010610660644.html), [example 2](https://www.aliexpress.com/item/1005005461879717.html) | USB-C input, PD3.0/PD3.1 (QC3.0 is a bonus), selectable output from 5 to 48 V, 5 A. |
| Buck converter | [8 A](https://www.aliexpress.com/item/1005009516551580.html), [5 A](https://www.aliexpress.com/item/1005009289437405.html) | Input up to 25 V, output selectable or adjustable (3.3/5/9/12 V), set to 5 V. |

### Stronger servos: DS3218

The DS3218 costs more than the MG996R but is much sturdier and has much more torque (20 kg·cm class, metal gears). It has the same standard servo size, so it should fit the same mounts of the Mk3 (not checked yet).

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

## Build it step by step

The [video at the top of this guide](#wire-the-electronics) shows every step below. This is how the reference build is wired, with lever connectors (Wago 221 style) as the rails and servo extension cables. It needs a little soldering. Any other build can follow the [Wiring](#wiring) table and diagram above instead: they apply to every build. Wire it on the table and test it first; it goes into the robot in [Assemble the robot](assembly.md).

Keep the charger unplugged unless a step says otherwise.

### 0. Lay out the parts

Put everything on the bench: the three servo extension cables, the two lever connectors, the 5 cm and 15 cm wires, the board, the PD trigger board, the buck converter, solder and the soldering iron.

![Every part on the bench: three servo extension cables, two lever connectors, four black wires, the ESP32 board, the buck converter, the PD trigger board, a reel of solder and the soldering iron.](../media/photos/wiring-step-00-parts.jpg)

### 1. Prepare the servo extension cables

Take the three servo extension cables. On these, the signal wire is white, + is red and − is black (on others: orange, red and brown).

Each cable has two ends: a small plug that pushes onto the board's pins, and a socket that a servo plugs into.

<p align="center"><img src="../media/photos/wiring-step-01-extensions.jpg" alt="Three servo extension cables side by side, each with a small plug at one end and a socket at the other." width="60%"></p>

### 2. The board's power cable

The first extension cable feeds the board. Cut it and keep the half with the small plug. Pull out its signal wire completely, then strip the + and the − (right of the photo). The other half goes in the bin.

![Left, the half with the socket, which is not used; right, the half with the small plug, its red and black wires stripped and its white signal wire pulled out.](../media/photos/wiring-step-02-board-extension.jpg)

### 3. The two servo cables

On the other two cables, keep the signal wire whole, from end to end. Cut the + and the − near the small plug and keep them on the socket's side, then strip their ends (left of the photo). The short pieces of + and − left on the small plug go in the bin.

![Two servo extension cables: the white signal wire runs from the socket to the small plug; the red and black wires leave the socket and stop, stripped; the cut-off red and black pieces lie beside them.](../media/photos/wiring-step-03-servo-extensions.jpg)

### 4–5. Gather the + and the −

Twist the three + wires (one from each cable) together, then the three − wires together. Each bundle becomes one end, ready for a lever connector.

<p>
  <img src="../media/photos/wiring-step-04-gather.jpg" alt="The three prepared cables: the board's cable with only red and black, and the two servo cables with their white wire whole and their red and black wires stripped." width="49%">
  <img src="../media/photos/wiring-step-05-gathered.jpg" alt="The three cables held together: the three red wires twisted into one bundle and the three black wires into another, the two white signal wires apart." width="49%">
</p>

### 6. Trigger board output

Solder the two 5 cm wires to the PD trigger board's output, + and −. Both boards sit on the top plate, so short wires are enough.

![The PD trigger board with a wire soldered to each of its OUT pads, + and −.](../media/photos/wiring-step-06-trigger-board-output.jpg)

### 7. Buck converter input

Solder the other ends of those two wires to the buck converter's IN pins: + to +, − to −.

![The trigger board's output wired to the buck converter's IN+ and IN− pins.](../media/photos/wiring-step-07-buck-input.jpg)

### 8. Buck converter output

Solder the two 15 cm wires to the buck converter's OUT pins, + and −. They go through the top plate's hole into the technical cavity, so they must be long enough to reach the lever connectors there.

![Two longer wires soldered to the buck converter's OUT+ and OUT− pins.](../media/photos/wiring-step-08-buck-output.jpg)

**Set and check the voltages now**, before anything else is connected: the trigger board on 20 V and the buck converter's output on 5 V, both checked with a multimeter, as in [Power](#power). Then unplug the charger.

### 9. Lever connectors

Put a lever connector on the end of each 15 cm wire: one on the +, one on the −.

![A lever connector on the end of each of the buck converter's two output wires.](../media/photos/wiring-step-09-lever-connectors.jpg)

### 10. The rails

Put the bundle of + wires in the second port of the + wire's lever connector, and the bundle of − wires in the second port of the − wire's one. These are the +5 V and GND rails. Not the neatest, but it holds.

![The red bundle in the + lever connector and the black bundle in the − one; the two servo sockets and the two white signal wires come out of the rails.](../media/photos/wiring-step-10-rails.jpg)

### 11–12. Power the board

Push the board's power cable (its small plug, with only + and −) onto the board's **VCC** (5V/VIN) and **GND** pins: + on VCC, − on GND. On the reference board (Mini D1 ESP32), VCC and GND sit next to each other, as framed in green.

**Check your board's pinout first.** If 5V/VIN and GND aren't next to each other, use two separate female Dupont wires instead. Never put the + on the 3.3 V pin.

<p>
  <img src="../media/photos/wiring-step-11-board-power.jpg" alt="The small plug with the red and black wires pushed onto two pins of the ESP32 board." width="49%">
  <img src="../media/photos/wiring-step-12-board-power-pins.jpg" alt="The back of the Mini D1 ESP32 board, with the VCC and GND pins framed in green." width="49%">
</p>

### 13–14. Signal wires

Push the small plug of each servo cable onto the board's **GPIO 18** and **GPIO 19** pins (S3/C3: **GPIO 4** and **GPIO 5**). Which one goes where doesn't matter: the servo you plug into each cable when you [assemble the robot](assembly.md) decides which axis it drives, and you can swap the pins later in **Settings → Hardware → Wiring**.

<p>
  <img src="../media/photos/wiring-step-13-signal-wires.jpg" alt="The two white signal wires pushed onto the board, next to the power plug." width="49%">
  <img src="../media/photos/wiring-step-14-signal-pins.jpg" alt="The back of the board, with the IO18 and IO19 pins framed in green." width="49%">
</p>

### 15. Test the power

Plug the trigger board into the charger: the board's LED should light up. If it doesn't, unplug at once and check the voltages and the + and − of each connection.

![The whole chain on the table: the USB-C cable into the trigger board, the buck converter, the lever connectors with the two servo sockets, and the ESP32 board with its LED lit.](../media/photos/wiring-step-15-power-test.jpg)

The electronics are ready. Next: [Install the software](install.md), then [Assemble the robot](assembly.md).
