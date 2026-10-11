# Electronics

How to wire the two servos and the power supply to the board. The firmware drives the servos with 50 Hz PWM pulses; the board only sends the signals and never powers the servos.

## Supported boards

Any ESP32, ESP32-S3 or ESP32-C3 development board. ESP8266 boards are not supported ([ADR 0008](adr/0008-no-esp8266-support.md)).

## Default pins

| Chip | Horizontal servo | Vertical servo |
|------|------------------|----------------|
| ESP32 | GPIO 18 | GPIO 19 |
| ESP32-S3 | GPIO 4 | GPIO 5 |
| ESP32-C3 | GPIO 4 | GPIO 5 |

On the S3 and C3, GPIO 18 and 19 carry USB, so the defaults differ. Choosing pins from the interface is coming ([#10](https://github.com/vistorx/CatWandConnect/issues/10)); until then, change `firmware/src/main.cpp` if your wiring differs.

## Wiring

Each MG996R has three wires:

| Servo wire | Connect to |
|------------|------------|
| Brown or black (ground) | Power supply − **and** a GND pin of the board |
| Red (power) | Power supply + (5 V) |
| Orange or yellow (signal) | The servo's GPIO on the board |

```
            5 V supply (3 A or more)
             +                −
             │                │
   ┌─────────┼────────┬───────┼────────┐
   │ red     │        │ red   │        │
 Horizontal servo   Vertical servo     │
   │ brown ──┴────────┼─ brown┘        │
   │ signal           │ signal         │
   │                  │                │
   └──► GPIO 18       └──► GPIO 19     └──► GND   (board)
```

The board is powered over USB, or from the same 5 V supply on its 5 V/VIN pin if your board accepts it.

## Rules

- **Common ground.** The board's GND must be connected to the supply's −, or the servos will jitter or ignore the signal.
- **Never power the servos from the board.** Not from its 3.3 V pin, and not from its 5 V pin when it runs on USB. An MG996R can draw over 1 A when it stalls.
- **Size the supply.** Allow 3 A or more for two MG996R. A weak supply resets the board when both servos start together.
- **Thick power wires.** Keep the 5 V and ground wires to the servos short and reasonably thick.
- **Capacitor.** If the board resets or motion stutters, add a capacitor (470–1000 µF, 10 V or more) across the servo supply, close to the servos.

## Servo pulse range

The defaults come from the proof of concept: 400–2200 µs for the Horizontal servo and 600–2400 µs for the Vertical one, mapped to 0°–180°. Setting the pulse range from the interface is coming with [#10](https://github.com/vistorx/CatWandConnect/issues/10).
