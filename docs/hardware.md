# Hardware: Mk2 Mechanism

What to print and what to buy for the Mk2, the current Mechanism. The electronic parts (board, servos, supply) are in [Electronics](electronics.md); putting it together is in the [assembly guide](assembly.md).

## Printed parts

The **Mk2** is the current Mechanism. It is reinforced with a 608 skateboard bearing and a track of 4 mm steel balls. All files are in [`stls/mk2/`](../stls/mk2/), and [`catwandconnect-mk2.3mf`](../stls/mk2/catwandconnect-mk2.3mf) contains the whole plate.

| Part | File |
|------|------|
| Arm | [`catwandconnect-mk2-arm.stl`](../stls/mk2/catwandconnect-mk2-arm.stl) |
| Bearing | [`catwandconnect-mk2-bearing.stl`](../stls/mk2/catwandconnect-mk2-bearing.stl) |
| Clamp | [`catwandconnect-mk2-clamp.stl`](../stls/mk2/catwandconnect-mk2-clamp.stl) |
| Screw | [`catwandconnect-mk2-screw.stl`](../stls/mk2/catwandconnect-mk2-screw.stl) |
| Rotor plate | [`catwandconnect-mk2-rotor_plate.stl`](../stls/mk2/catwandconnect-mk2-rotor_plate.stl) |
| Top plate | [`catwandconnect-mk2-top_plate.stl`](../stls/mk2/catwandconnect-mk2-top_plate.stl) |

## Print recommendations

> Starting points, not yet checked against the reference build. Share the settings that work for you on [Discord](https://discord.gg/Rbqge3v9dk) and they will land here.

- **Material:** PLA works for a robot that stays indoors; PETG is tougher if the cat attacks the arm.
- **Strength:** 3 walls or more and 20–30 % infill. The arm, the clamp and the parts that hold the servos take the load.
- **Ball track:** print it flat with a fine first layer, and clean any strings or elephant's foot so the balls roll freely.
- **Fit:** if the bearing or a servo doesn't go in, sand the part rather than force it.
- The [`.3mf`](../stls/mk2/catwandconnect-mk2.3mf) holds the whole plate in one file.

## Mechanical parts

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

A printed part breaks? Reprint it: that is the point of an open design.
