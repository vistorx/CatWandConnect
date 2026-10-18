# Print the parts

The seven printed parts of the Mk3, the current Mechanism, and how to print them. Next: [assemble them](assembly.md).

https://github.com/user-attachments/assets/914092e7-1f09-4832-85f9-01eeb9a11499

<p align="center"><em>The presentation of the Mk2, the previous Mechanism (50 s, 3D animation).</em></p>

## The parts

The **Mk3** is the current Mechanism. It is standalone: the board and the power boards fit inside the robot, in the clamp's technical cavity closed by a printed door, and a single USB-C connector powers it. All files are in [`stls/mk3/`](../stls/mk3/), and [`catwandconnect-mk3.3mf`](../stls/mk3/catwandconnect-mk3.3mf) contains the whole plate.

| Part | File |
|------|------|
| Arm | [`catwandconnect-mk3-arm.stl`](../stls/mk3/catwandconnect-mk3-arm.stl) |
| Bearing holder | [`catwandconnect-mk3-bearing_holder.stl`](../stls/mk3/catwandconnect-mk3-bearing_holder.stl) |
| Clamp | [`catwandconnect-mk3-clamp.stl`](../stls/mk3/catwandconnect-mk3-clamp.stl) |
| Door | [`catwandconnect-mk3-door.stl`](../stls/mk3/catwandconnect-mk3-door.stl) |
| Screw | [`catwandconnect-mk3-screw.stl`](../stls/mk3/catwandconnect-mk3-screw.stl) |
| Rotor plate | [`catwandconnect-mk3-rotor_plate.stl`](../stls/mk3/catwandconnect-mk3-rotor_plate.stl) |
| Top plate | [`catwandconnect-mk3-top_plate.stl`](../stls/mk3/catwandconnect-mk3-top_plate.stl) |

**What changed from the Mk2:** the electronics move inside the robot, behind the new door, and the robot needs only one USB-C cable.

### Previous Mechanism: Mk2

The Mk2, reinforced with a 608 skateboard bearing and a track of 4 mm steel balls, leaves the electronics outside the robot. Pick it if you'd rather keep the board and the power boards outside. Its six parts are in [`stls/mk2/`](../stls/mk2/), and [`catwandconnect-mk2.3mf`](../stls/mk2/catwandconnect-mk2.3mf) contains its whole plate.

## Print recommendations

> Starting points, not yet checked against the reference build. Share the settings that work for you on [Discord](https://discord.gg/Rbqge3v9dk) and they will land here.

- **Material:** PLA works for a robot that stays indoors; PETG is tougher if the cat attacks the arm.
- **Supports:** none, except on the clamp. Print every other part, door included, without supports. The clamp needs supports under its horizontal surfaces only: in PrusaSlicer, turn on supports with an **overhang threshold of 5°** so they don't fill its slopes and holes.
- **Strength:** 3 walls or more and 20–30 % infill. The arm, the clamp and the parts that hold the servos take the load.
- **Ball track:** print it flat with a fine first layer, and clean any strings or elephant's foot so the balls roll freely.
- **Fit:** if the bearing or a servo doesn't go in, sand the part rather than force it.
- The [`.3mf`](../stls/mk3/catwandconnect-mk3.3mf) holds the whole plate in one file.

A printed part breaks? Reprint it: that is the point of an open design.

Next: [Assemble the Mechanism](assembly.md).
