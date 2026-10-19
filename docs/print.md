# Print the parts

This guide gets you the seven printed parts of the Mk3 Mechanism, ready to assemble.

## What you will make

The **Mk3** is standalone: the board and the power boards fit inside the robot, in the clamp's technical cavity closed by a printed door, and a single USB-C connector powers it. All files are in [`stls/mk3/`](../stls/mk3/), and [`catwandconnect-mk3.3mf`](../stls/mk3/catwandconnect-mk3.3mf) holds the whole plate, already oriented.

| Part | File |
|------|------|
| Arm | [`catwandconnect-mk3-arm.stl`](../stls/mk3/catwandconnect-mk3-arm.stl) |
| Bearing holder | [`catwandconnect-mk3-bearing_holder.stl`](../stls/mk3/catwandconnect-mk3-bearing_holder.stl) |
| Clamp | [`catwandconnect-mk3-clamp.stl`](../stls/mk3/catwandconnect-mk3-clamp.stl) |
| Door | [`catwandconnect-mk3-door.stl`](../stls/mk3/catwandconnect-mk3-door.stl) |
| Rotor plate | [`catwandconnect-mk3-rotor_plate.stl`](../stls/mk3/catwandconnect-mk3-rotor_plate.stl) |
| Screw | [`catwandconnect-mk3-screw.stl`](../stls/mk3/catwandconnect-mk3-screw.stl) |
| Top plate | [`catwandconnect-mk3-top_plate.stl`](../stls/mk3/catwandconnect-mk3-top_plate.stl) |

![The seven Mk3 parts sliced on one plate in PrusaSlicer, in their print orientation, with the clamp's supports in green.](../media/screenshots/prusa-slicer-plate.jpg)

## What you need

| What | Notes |
|------|-------|
| A 3D printer | A 300 × 300 mm plate fits every part at once; a smaller printer works over several plates. No printer? Any print service or maker space can print them. |
| Filament | PETG or ABS (or equivalent) recommended; PLA can do the job. |
| A slicer | PrusaSlicer or any other. |

**[Full parts list →](bom.md)**

## Steps

<!-- video: printing and cleaning the parts -->
Photos and video coming soon.

### 1. Choose the material

PETG or ABS (or an equivalent) is recommended: tougher when the cat attacks the arm. PLA can do the job too.

<!-- photo: step 1 -->

### 2. Set up the slicer

- **Infill:** gyroid, 25 %.
- **Walls:** 3.
- **Top and bottom layers:** 6 each.
- **Layer height:** 0.2 mm, with a 0.4 mm nozzle.

<!-- photo: step 2 -->

### 3. Split the parts over several plates

Print the parts over several plates, even if your printer can take them all at once (300 × 300 mm): a failed print then costs only one plate.

<!-- photo: step 3 -->

### 4. Orient the parts

Print the top plate and the rotor plate with their ball groove facing the build plate. Orient the other parts as in the [`.3mf`](../stls/mk3/catwandconnect-mk3.3mf).

<!-- photo: step 4 -->

### 5. Add supports to the clamp only

No part needs supports except the clamp; the door prints without them. On the clamp, put supports under its horizontal surfaces only: in PrusaSlicer, turn supports on with an **overhang threshold of 5°** so they don't fill its slopes and holes.

<!-- photo: step 5 -->

### 6. Clean the parts

Remove the clamp's supports, and clean any strings or elephant's foot in the ball groove so the balls roll freely. If a part doesn't fit, sand it rather than force it.

<!-- photo: step 6 -->

Settings that work well on your printer? Share them on [Discord](https://discord.gg/Rbqge3v9dk). A printed part breaks? Reprint it: that is the point of an open design.

Next: [Wire the electronics](wiring.md).
