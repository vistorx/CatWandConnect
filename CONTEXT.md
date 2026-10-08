# CatWandConnect

An open-source robot that plays with a cat by moving a toy wand. It runs on its own, with a dedicated interface served by the robot itself, and can optionally be connected to Home Assistant.

## Language

**CatWandConnect**:
The robot as a whole: the printed mechanism, its two servos and the ESP32 that drives them.
_Avoid_: Cat Playing Platform, Platform, Cat-Bot

**Standalone mode**:
The robot operating with no Home Assistant instance: all play logic and its user interface live on the robot.
_Avoid_: Offline mode

**Device interface**:
The user interface served by the robot itself, usable by people without Home Assistant.
_Avoid_: Web UI, app

**Manual control**:
A person moving the wand directly, in real time, from the Device interface.
_Avoid_: Shake, jog

**Travel limits**:
The user-set minimum and maximum position of each axis, which the robot never exceeds whatever the source of the command, so the mechanism never hits anything.
_Avoid_: Endstops, butées

**Stow**:
Moving the wand to its rest position and stopping everything else. Takes priority over any other command.
_Avoid_: Park, home

**Calibration**:
The guided process in which the user sets each axis's Travel limits and the Stow position, then confirms the robot can sweep that envelope without hitting anything.
_Avoid_: Setup, homing

## Hardware

**Mechanism**:
A printed mechanical design of the robot, identified by a Mark number (Mk1, Mk2…). Independent from firmware version numbers.
_Avoid_: Version, V1, V2 (reserved for firmware releases)

**Mk1**:
The first Mechanism, retired because its vertical servo overheated under load.

**Mk2**:
The current reference Mechanism, reinforced with a 608 skateboard bearing and a 4 mm steel-ball track. The only one the firmware targets.
