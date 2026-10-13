# Physical acceptance checklist

Checks that need the real robot, run before each release on a Mk2 Mechanism. The automated tests cover the Robot core; everything here is the hardware and network around it. Note the board, chip and firmware version you tested with. Each ticket that adds behaviour tests can't cover adds its section here.

Keep a serial monitor open (`pio device monitor` in `firmware/`) to read the `[wifi]` log lines.

## Wi-Fi provisioning

Start each section from a board with no saved Wi-Fi: erase it with `pio run -e <chip> -t erase`, then flash it.

### Improv over USB

- [ ] Flash the board with the web flasher (or with PlatformIO, then open the web flasher and connect to the board). It offers "Connect to Wi-Fi" and shows the device as CatWandConnect with the right version.
- [ ] The client lists nearby networks.
- [ ] Entering a wrong password reports "unable to connect", and the robot opens its hotspot.
- [ ] Entering the right credentials connects, and the client shows a link to the robot's address. The link opens the robot's page.
- [ ] Rebooting the board (unplug and replug) reconnects without asking again.

### Hotspot and setup page

- [ ] With no saved Wi-Fi, a `CatWandConnect-XXXX` network appears within a few seconds of power-up.
- [ ] Joining it from a phone opens the setup page automatically, on both iOS and Android. If not, `http://192.168.4.1/` opens it.
- [ ] The setup page lists nearby networks, strongest first and preselected, with a lock on secured ones. Tapping one fills the network name; "Rescan" searches again; a hidden network can still be typed.
- [ ] On the setup page, "Continue without Wi-Fi" opens the main screen through the hotspot; the joystick and Settings work, and Settings → Wi-Fi → "Connect to Wi-Fi" brings the setup page back.
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
- [ ] In landscape, the joystick, positions, speed slider and Stow fit on screen without scrolling; dragging the joystick never scrolls the page.

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

## Calibration

- [ ] On a robot that was never calibrated, the main screen shows the "Not calibrated yet" banner, and the joystick is limited to 60°–120°.
- [ ] The wizard walks through Horizontal minimum, Horizontal maximum, Vertical minimum, Vertical maximum, Stow, sweep, confirm, showing "Step n of 6".
- [ ] From the second step on, "Marked so far" shows the values already marked; the Stow step shows each axis's allowed range; a maximum below the minimum names the minimum in the error.
- [ ] The ±1° and ±5° buttons move only the step's axis, slowly, and the position on screen follows.
- [ ] The nudges can go beyond 60°–120°.
- [ ] Marking a maximum at or below its minimum shows an error and stays on the step.
- [ ] Marking a Stow position outside the new limits shows an error and stays on the step.
- [ ] The sweep visits the four corners of the new envelope slowly and ends at the Stow position, without touching anything.
- [ ] "Nothing touched: save" saves; the banner disappears and the joystick now reaches the new limits but not beyond.
- [ ] Settings shows the new limits and Stow position. Changing them there is validated the same way.
- [ ] Pressing Cancel at any step, or Stow on the main screen, keeps the previous limits and Stows.
- [ ] After a power cycle the robot is still calibrated, with the same limits.
- [ ] Running Calibration again starts from the current limits.

## Hardware configuration

- [ ] Settings → Wiring lists only pins allowed on the board's chip, and shows the current wiring.
- [ ] Choosing the same pin for both servos shows an error and saves nothing.
- [ ] Rewire the Horizontal servo to another allowed pin, select it and save: the robot restarts, and after boot the servo works on the new pin.
- [ ] Ticking "Reverse direction" on an axis and saving: the robot restarts, the joystick moves that axis the other way, and the "Not calibrated yet" banner is back.
- [ ] The pulse range sits under "Servo travel (advanced)", collapsed, with an explanation, the defaults and "Reset to default".
- [ ] Narrowing a pulse range (for example 600–2000 µs) and saving reduces that servo's travel, and resets Calibration.
- [ ] A pulse range under 500 µs wide shows an error and saves nothing.
- [ ] The wiring survives a power cycle.

## Identity, Info, language and home screen

- [ ] In Settings, the address preview follows the name as you type ("Salon d'été" → `salon-d-ete.local`).
- [ ] Saving a new name restarts the robot. Afterwards the main screen title shows it, `http://<new-name>.local/` opens, and the old address no longer does.
- [ ] With the robot's Wi-Fi forgotten, the hotspot is called `<name>-XXXX`.
- [ ] An empty name, or a name made only of emoji, is refused with a message.
- [ ] The Info page shows name, address, firmware version, chip, Wi-Fi network, IP, signal, uptime, temperature, free memory and MAC, refreshing every few seconds. "Copy" copies them, or selects them if the browser refuses.
- [ ] The Info page shows a Diagnostic report with the firmware version, "Last reset: power-on" after unplugging and replugging, the settings, the wiring and recent log lines such as `[wifi] connected`.
- [ ] In the report, the Wi-Fi network, the IP and MAC addresses, and the broker and MQTT user (with Home Assistant set up) appear only as `<wifi-network>`, `<ip>`, `<mac>`, `<broker>` and `<mqtt-user>`, including inside the log lines. A robot renamed "Salon" shows `<name>` and `<hostname>`. The MQTT password never appears.
- [ ] "Copy the report" copies it, or selects it if the browser refuses; "Download the report" saves a `.txt` file on the phone and on a computer.
- [ ] "Download the full report" saves a version with the real Wi-Fi name and addresses, and its file name ends in `-full`.
- [ ] After a restart from Settings (renaming the robot), the report says "Last reset: software restart".
- [ ] A phone set to French shows the interface in French. Choosing English in Settings switches it at once and the choice is remembered after a reload. "Automatic" goes back to the phone's language.
- [ ] iPhone (Safari) → Share → Add to Home Screen: the icon appears, and opening it shows the interface full screen without the Safari bars.
- [ ] Android (Chrome) → menu → Add to Home screen: the icon appears and opens the interface.

## Settings PIN

- [ ] On a fresh robot, Settings → Settings PIN says "Off", and every setting can be changed without a PIN.
- [ ] Set a PIN of 4 digits. You stay signed in on that phone.
- [ ] On a second phone (or a private window), the joystick, the speed slider and Stow work without a PIN.
- [ ] On that second phone, saving any setting (limits, speed and acceleration, wiring, name), starting Calibration or changing Wi-Fi from the hotspot asks for the PIN. With the right PIN the change goes through.
- [ ] Five wrong PINs in a row show "Too many wrong PINs" with a countdown; the right PIN is refused until it ends.
- [ ] After a restart, the PIN is asked again.
- [ ] Changing the PIN signs out other phones. Removing it makes everything open again.
- [ ] A 3-digit or non-numeric PIN is refused.

## Firmware update (OTA)

Build two versions that differ visibly (for example change `CWC_VERSION` in `platformio.ini`) and use the `firmware.bin` of the second.

- [ ] Settings → Firmware update shows the installed version.
- [ ] Uploading the new file shows progress, then "Installed". The robot restarts, the page reloads and the Info page shows the new version.
- [ ] Wi-Fi, name, Calibration, speed, wiring and PIN are unchanged after the update.
- [ ] Uploading a file built for another chip is refused with "for another chip", and nothing changes.
- [ ] Uploading a non-firmware file (a photo) is refused with "not an ESP32 firmware".
- [ ] Unplugging the robot's Wi-Fi (or closing the tab) halfway through: the robot keeps running its old firmware, and a new upload then works.
- [ ] With a PIN set, uploading from a phone that hasn't entered it asks for it.

## Home Assistant (MQTT)

Use a Home Assistant with the Mosquitto add-on and the MQTT integration ([setup](home-assistant.md)).

- [ ] On a fresh robot, Settings → Home Assistant is off and the robot makes no MQTT connection.
- [ ] Enabling it with the broker address, user and password shows "Connected" within a few seconds.
- [ ] A wrong password shows "refused the user or password"; a wrong address shows "Can't reach the broker". The joystick stays smooth while the robot keeps retrying.
- [ ] The device appears in Home Assistant under the robot's name, with Stow, Horizontal/Vertical position, Horizontal/Vertical maximum speed and Activity.
- [ ] Moving the wand with the joystick updates the positions and Activity in Home Assistant.
- [ ] Setting a position in Home Assistant moves the axis there smoothly. A value beyond the Travel limits stops at the limit.
- [ ] Pressing Stow in Home Assistant stows, even while someone is using the joystick.
- [ ] Changing a maximum speed in Home Assistant shows in the robot's Settings, and a value of 5 °/s is ignored.
- [ ] Unplugging the robot makes its entities unavailable in Home Assistant within about a minute. Plugging it back makes them available again.
- [ ] Renaming the robot: after its restart, Home Assistant shows the new name.

## Web flasher and releases

- [ ] The release page has a `-factory.bin` and an `-ota.bin` per chip, and the version in the Info page matches the tag.
- [ ] The web flasher opens in Chrome or Edge on a computer and shows the release's version. In Safari or Firefox it explains that it needs Chrome or Edge.
- [ ] With an ESP32, an ESP32-S3 and an ESP32-C3 board in turn: "Connect and install" detects the chip and installs. Erasing on a first install works.
- [ ] After installing, "Connect to Wi-Fi" provisions the robot and "Visit device" opens its interface.
- [ ] Installing over an existing robot without erasing keeps its settings.

