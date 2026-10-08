# Electronic implementation documentation

This page describes the minimal connection between an ESP32 and one or two servos for controlling the cat toy platform.

## General principle

The ESPHome firmware drives the servos through two PWM outputs. The servos are powered from 5 V, while the ESP32 only provides the control signals.

## Recommended connections

### Servo 1: main motion
- Brown/black wire: ground (GND)
- Red wire: 5 V power
- Orange/yellow wire: PWM signal to GPIO18

### Servo 2: secondary motion
- Brown/black wire: ground (GND)
- Red wire: 5 V power
- Orange/yellow wire: PWM signal to GPIO19

### Power supply
- Connect the ESP32 ground to the 5 V supply ground
- Connect the ESP32 5 V pin or VIN pin to the power supply if the voltage is compatible
- Never power a servo from the ESP32 3.3 V pin

## Wiring tips

- Use a thicker power cable if several servos are connected in parallel
- Add a decoupling capacitor close to the servos if motion becomes unstable
- Verify that the ground is common between the ESP32, power supply, and servos
- If only one servo is used, leave the second output unused or connect it to a future motor

## Safety note

Servos can draw significant current for short bursts. If the motion is jerky, prefer a dedicated 5 V supply or a more powerful power block.

## Pin reference used

The default ESPHome configuration expects:
- Servo 1 on GPIO18
- Servo 2 on GPIO19

If your wiring differs, update the values in [board.yaml](../board.yaml).
