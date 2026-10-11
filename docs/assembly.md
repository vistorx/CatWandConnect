# Assembly: Mk2 Mechanism

The recommended assembly order for the Mk2. The parts and hardware are listed in the [README](../README.md#build-one).

> This order was reconstructed from the parts and screw list and hasn't been checked step by step against a build yet. Corrections and photos are welcome.

## Before you start

- Print every part in [`stls/mk2/`](../stls/mk2/).
- Clean the ball track and the bearing seat of strings and elephant's foot, so the balls roll freely.
- Plug each servo into a powered, flashed board and let the robot Stow once, so both servos sit at their Stow position before you fit the arm. This keeps the arm centred within its range.

## Steps

1. **Bearing.** Press the 608 bearing into its seat. It should sit flush and turn without rubbing.
2. **Ball track.** Lay the 4 mm steel balls in the track, starting with 16 and adding more until the plate turns smoothly without play. Usually 16–24 are needed.
3. **Horizontal servo.** Mount it with four M3×8 button-head screws, then fix the horizontal servo shaft plate with the M3×8 countersunk screw.
4. **Horizontal axis.** Fix the lower plate of the horizontal axis to the ball track with the two M3×25 button-head screws.
5. **Vertical servo.** Mount it with four M3×8 button-head screws, and fix its shaft with the M3×20 countersunk screw.
6. **Bearing to the vertical shaft.** Fix it with the three M3×30 countersunk screws.
7. **Arm and wand.** Fit the arm and attach the wand.
8. **Clamp.** Fix the robot in place with the clamp and its screw.

## Checks

- Unpowered, both axes turn by hand without binding, and the bearing doesn't rub anything.
- No screw crushes a printed part. Tighten until snug, not more.
- Route the servo cables so they can't snag as the axes move.
- Power up and stay close: the wand should crawl to its Stow position. Then try the joystick gently. See the [acceptance checklist](acceptance-checklist.md).
