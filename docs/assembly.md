# Assemble the Mechanism

> **Coming soon.** A step-by-step guide with photos will be written from the reference build. Until then, the draft below gives the order, reconstructed from the parts and screw list and not yet checked against a build. Corrections and photos are welcome.

You need the [printed parts](print.md) and the parts below. The servos are wired in [Wire the electronics](wiring.md).

## Parts to buy

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 608 skateboard bearing | |
| 16–24 | 4 mm steel balls | Adjust the count to your print's finish. |
| 4 | M3×8 button-head screws | Horizontal servo mounting |
| 2 | M3×25 button-head screws | Lower plate of the horizontal axis to the ball track |
| 1 | M3×8 countersunk screw | Horizontal servo shaft plate |
| 1 | M3×20 countersunk screw | Vertical servo shaft |
| 4 | M3×8 button-head screws | Vertical servo mounting |
| 3 | M3×30 countersunk screws | Bearing to the vertical shaft |
| 1 | A toy wand | Any cat wand with a stick that fits the arm |


<details>
<summary><b>Draft assembly order</b></summary>

### Before you start

- [Print](print.md) every part in [`stls/mk2/`](../stls/mk2/).
- Clean the ball track and the bearing seat of strings and elephant's foot, so the balls roll freely.
- Plug each servo into a powered, flashed board and let the robot Stow once, so both servos sit at their Stow position before you fit the arm. This keeps the arm centred within its range.

### Steps

1. **Bearing.** Press the 608 bearing into its seat. It should sit flush and turn without rubbing.
2. **Ball track.** Lay the 4 mm steel balls in the track, starting with 16 and adding more until the plate turns smoothly without play. Usually 16–24 are needed.
3. **Horizontal servo.** Mount it with four M3×8 button-head screws, then fix the horizontal servo shaft plate with the M3×8 countersunk screw.
4. **Horizontal axis.** Fix the lower plate of the horizontal axis to the ball track with the two M3×25 button-head screws.
5. **Vertical servo.** Mount it with four M3×8 button-head screws, and fix its shaft with the M3×20 countersunk screw.
6. **Bearing to the vertical shaft.** Fix it with the three M3×30 countersunk screws.
7. **Arm and wand.** Fit the arm and attach the wand.
8. **Clamp.** Fix the robot in place with the clamp and its screw.
9. **Electronics in place.** From the reference build's notes, while you follow [Build it step by step](wiring.md#build-it-step-by-step):
   - Install the trigger board and the buck converter on top of the robot.
   - Pass the buck converter's two output wires through the hole that leads into the technical cavity inside the clamp.
   - The two lever connectors (the +5 V and GND rails) sit in the technical cavity, next to the Horizontal servo.
   - Pass the Vertical servo's extension half through the hole to the outside of the cavity.

### Checks

- Unpowered, both axes turn by hand without binding, and the bearing doesn't rub anything.
- No screw crushes a printed part. Tighten until snug, not more.
- Route the servo cables so they can't snag as the axes move.
- Power up and stay close: the wand should crawl to its Stow position. Then try the joystick gently. See the [acceptance checklist](acceptance-checklist.md).

</details>

Next: [Wire the electronics](wiring.md).
