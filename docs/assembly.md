# Assemble the robot

This guide puts the printed parts, the servos and the electronics together into a working robot.

> [!IMPORTANT]
> **Install the software first** ([step 3](install.md)). The robot must boot to put the servos in position while you assemble it.

## What you need

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 608 skateboard bearing | |
| 27–28 | 6 mm balls | Between the top plate and the rotor plate. Airsoft BBs work very well. |
| 11 | M3×8 button head screws | 8 for the two servos (4 each), often supplied with them; 3 for the door. |
| 5 | M3×20 countersunk screws | 4 for the top plate; 1 for the Vertical servo's axis. |
| 1 | M3×14 countersunk screw | For the Horizontal servo's axis. |
| 2 | M3×30 countersunk screws | For the bearing holder. |
| 2 | Servo horns | Supplied with the servos. |
| | Double-sided tape | For the USB-C trigger board. |
| | Grease | For the ball groove, for example lithium or PTFE grease. |
| 1 | Toy wand | Any cat wand with a stick that fits the arm. |
| | The printed parts, the wired electronics | From [step 1](print.md) and [step 2](wiring.md), with the firmware installed ([step 3](install.md)). |

Tools: a screwdriver for M3 screws.

**[Full parts list →](bom.md)**

## Steps

<!-- video: assembling the robot -->
Photos and video coming soon.

### 1. Remove the supports

Remove all the print supports.

<!-- photo: step 1 -->

### 2. Fit the Horizontal servo

Insert the Horizontal servo into the clamp and fit its 4 M3×8 button head screws.

<!-- photo: step 2 -->

### 3. Stick the trigger board

Stick the USB-C trigger board (the board that brings 20 V) into the top plate with double-sided tape.

<!-- photo: step 3 -->

### 4. Fit the top plate

Position the top plate, passing the two power wires through the hole into the technical cavity, with the buck converter in the recess made for it. Fit the top plate's 4 M3×20 countersunk screws.

<!-- photo: step 4 -->

### 5. Connect the electronics

In the technical cavity, fit the lever connectors (Wago) and connect the electronics: the board's power and the Horizontal servo. Then plug in the USB-C charger: the robot starts and the servo goes to its position (neutral).

<!-- photo: step 5 -->

### 6. Fit a servo horn into the rotor plate

Fit a servo horn into the rotor plate.

<!-- photo: step 6 -->

### 7. Grease and lay the balls

Grease the top plate's ball groove and lay the 27–28 balls in it.

<!-- photo: step 7 -->

### 8. Fit the rotor plate

Fit the rotor plate, with its servo horn, onto the Horizontal servo, and screw the M3×14 countersunk screw through the rotor plate and the horn into the servo's axis.

<!-- photo: step 8 -->

### 9. Press in the bearing

Press the 608 bearing into the bearing holder.

<!-- photo: step 9 -->

### 10. Fit the other servo horn into the arm

Fit the other servo horn into the arm.

<!-- photo: step 10 -->

### 11. Fit the Vertical servo

Install the Vertical servo on the rotor plate and fit its 4 M3×8 button head screws.

<!-- photo: step 11 -->

### 12. Connect the Vertical servo and fit the arm

Connect the Vertical servo so it goes to its position. Recommended: on the robot's page, move the Vertical axis to its lowest position, then fit the arm, with its servo horn, onto the Vertical servo and screw it into the servo's axis with the M3×20 countersunk screw.

<!-- photo: step 12 -->

### 13. Fit the bearing holder

Place the bearing and its holder on the arm and fit the 2 M3×30 countersunk screws that hold it.

<!-- photo: step 13 -->

### 14. Close the door

Tuck the electronics into the technical cavity and close it with the door and its 3 M3×8 button head screws.

<!-- photo: step 14 -->

Then run [Calibration](install.md#calibrate) from the robot's main screen, so the robot learns the Travel limits of your build.

Next, optional: [Set up Home Assistant](home-assistant.md). Or just play!
