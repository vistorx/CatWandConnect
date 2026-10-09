# Physical acceptance checklist

Checks that need the real robot, run before each release on a Mk2 Mechanism. The automated tests cover the Robot core; everything here is the hardware and network around it. Note the board, chip and firmware version you tested with.

Keep a serial monitor open (`pio device monitor` in `firmware/`) to read the `[wifi]` log lines.

## Wi-Fi provisioning

Start each section from a board with no saved Wi-Fi: erase it with `pio run -e <chip> -t erase`, then flash it.

### Improv over USB

- [ ] Flash the board, then open an Improv client on the USB port (the web flasher once #15 lands; until then <https://www.improv-wifi.com/> → "Connect device to Wi-Fi"). It shows the device as CatWandConnect with the right chip and version.
- [ ] The client lists nearby networks.
- [ ] Entering a wrong password reports "unable to connect", and the robot opens its hotspot.
- [ ] Entering the right credentials connects, and the client shows a link to the robot's address. The link opens the robot's page.
- [ ] Rebooting the board (unplug and replug) reconnects without asking again.

### Hotspot and setup page

- [ ] With no saved Wi-Fi, a `CatWandConnect-XXXX` network appears within a few seconds of power-up.
- [ ] Joining it from a phone opens the setup page automatically, on both iOS and Android. If not, `http://192.168.4.1/` opens it.
- [ ] The network field suggests nearby networks.
- [ ] A wrong password shows "Could not connect" and the hotspot stays up.
- [ ] The right password shows "Connected" with the robot's address. The hotspot disappears about 30 s later.
- [ ] Rebooting reconnects to that network without the hotspot.

### Name and recovery

- [ ] From a computer or phone on the home network, `http://catwandconnect.local/` opens the robot's page.
- [ ] Switching the router off: within about a minute the hotspot reappears.
- [ ] Switching the router back on: within about 2 minutes, with no phone on the hotspot, the robot rejoins the home network and the hotspot disappears.
- [ ] Changing the router password: the hotspot appears, and the new password can be entered on the setup page.

## Manual control

Wire the servos to the default pins: GPIO 18 (Horizontal) and 19 (Vertical) on ESP32, GPIO 4 and 5 on ESP32-S3 and ESP32-C3. Power the servos from their own 5 V supply, sharing ground with the board.

- [ ] At power-up, both servos move to the middle position (90°) and hold it.
- [ ] `http://catwandconnect.local/` opens the main screen on a phone, showing "Connected" and both positions.
- [ ] Dragging the joystick moves the wand smoothly in the dragged direction. A small drag moves it slowly; a full drag moves it at full speed.
- [ ] The positions on screen follow the wand.
- [ ] Letting go stops the wand within a fraction of a second.
- [ ] Holding the joystick at full deflection in each direction: the wand stops at the default Travel limits (60°–120°) and never hits the Mechanism.
- [ ] While moving, switching off the phone's Wi-Fi or closing the tab stops the wand within about a second.
- [ ] A second phone on the main screen sees the positions move too.
- [ ] The page is usable on a phone in portrait orientation, without zooming or scrolling the joystick off screen.

## Speed and settings

- [ ] The speed slider changes how fast the joystick moves the wand. At 10 % the wand crawls; at 100 % it reaches the maximum speed.
- [ ] Moving the slider on one phone updates it on a second phone.
- [ ] In Settings, lowering the Horizontal maximum speed slows only the horizontal motion.
- [ ] Lowering the acceleration makes starts and stops visibly softer; raising it makes them snappier, without the Mechanism jerking or skipping.
- [ ] Entering 5 °/s as the maximum speed shows an error and keeps the previous value.
- [ ] After a power cycle, the slider and Settings values are kept.

## Stow, priority, hold and release

- [ ] The Stow button stays visible at the bottom of the main screen, including on a small phone.
- [ ] Pressing Stow while the wand is away moves it back to the Stow position. The button shows "Stowing…" until it arrives.
- [ ] At the Stow position the servos go limp: the arm can be moved by hand with little resistance. Note whether the Mk2 stays in place without torque (see the spec's note on hold and release).
- [ ] Pushing the joystick while stowing does nothing; the wand still reaches the Stow position.
- [ ] After Stow, the joystick works again and the servos hold firmly while moving and after letting go away from Stow.
- [ ] `curl -X POST http://catwandconnect.local/api/stow` stows the wand.

## Power-up

- [ ] Move the wand away from Stow with the joystick, let go and wait 2 s, then unplug the power. When you plug it back in, the servos don't jump: the wand starts where it was and crawls back to the Stow position, then the servos go limp.
- [ ] Repeat, but cut the power while the wand is moving. At power-up, it starts from the last position where it rested, so expect a small jump at most, then crawls to Stow.
- [ ] On a freshly erased board (`pio run -e <chip> -t erase`), power-up goes straight to the Stow position.
- [ ] While the robot crawls to Stow after power-up, the main screen shows "Stowing…" and the joystick does nothing.
