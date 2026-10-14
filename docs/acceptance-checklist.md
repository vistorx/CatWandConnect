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
- [ ] On the setup page, "Continue without Wi-Fi" opens the main screen through the hotspot; the joystick and Settings work, and Settings → Network → Wi-Fi → "Connect to Wi-Fi" brings the setup page back.
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

- [ ] The speed slider changes how fast the joystick moves the wand. At 10 % the wand moves at the minimum speed; at 100 % it reaches the maximum speed.
- [ ] Moving the slider on one phone updates it on a second phone.
- [ ] In Settings, lowering the Horizontal maximum speed slows only the horizontal motion.
- [ ] Lowering the acceleration makes starts and stops visibly softer; raising it makes them snappier, without the Mechanism jerking or skipping.
- [ ] Entering 5 °/s as the maximum speed shows an error and keeps the previous value.
- [ ] Setting the Horizontal minimum speed to 20 °/s makes the slider's 10 % move the wand noticeably faster than before; a minimum equal to the maximum shows an error.
- [ ] After updating from 0.1.0 by OTA, settings and Calibration are kept, and each minimum speed is a tenth of its maximum.
- [ ] Settings → Motion → Tune speeds, Horizontal maximum: the wand sweeps smoothly between the Travel limits and the vertical axis stays still. Faster/Slower change the sweep speed within a second; the number shown matches what you see.
- [ ] Keep this speed saves it (Settings → Motion shows it) and Stows; Cancel, Stow from another phone or Home Assistant, and touching the joystick all end the tuning and keep the previous value.
- [ ] Tuning the minimum speed of an SG90 axis: at 2–4 °/s the motion visibly jerks; the kept value makes the speed slider's 10 % smooth.
- [ ] After a power cycle, the slider and Settings values are kept.

## Play session

Use the Play session block under the speed slider on the main screen.

- [ ] Before Calibration, Start shows "Run Calibration first" with a Calibrate link, and nothing moves.
- [ ] − and + change the duration by one minute, within 1–60. After a page reload, and on a second phone, the last duration chosen is shown.
- [ ] Once calibrated, Start moves both axes on their own rhythm: they rarely start or stop together, small hops happen, and the wand never passes the Travel limits.
- [ ] While it plays, the button reads Stop and the time left counts down smoothly, second by second; a second phone shows the same session and time left.
- [ ] In Settings → Motion, set both Pause ranges to 0–0 s and save: the wand never stops moving during a session. A minimum above the maximum shows an error and keeps the previous values.
- [ ] A 1-minute session Stows by itself after a minute; Stop and the Stow button end it with Stow without finishing the current move, and the block goes back to Start.
- [ ] Touching the joystick during a session gives control at once, without Stow, and the block goes back to Start.
- [ ] Closing the page does not stop the session; reopening it shows Stop and the time left. A power cut during a session ends it, and the robot Stows gently at power-up instead of playing.
- [ ] After updating from a build without Play session by OTA, settings and Calibration are kept, and Settings → Motion shows the default Pause ranges (Horizontal 0.5–3 s, Vertical 1–5 s).

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
- [ ] The wizard walks through Horizontal minimum, Horizontal maximum, Vertical minimum, Vertical maximum, Stow, sweep, confirm, then Horizontal minimum speed, Horizontal maximum speed, Vertical minimum speed, Vertical maximum speed and "Your robot is ready", showing "Step n of 10".
- [ ] From the second step on, "Marked so far" shows the values already marked; the Stow step shows each axis's allowed range; a maximum below the minimum names the minimum in the error.
- [ ] The ±1° and ±5° buttons move only the step's axis, slowly, and the position on screen follows.
- [ ] The nudges can go beyond 60°–120°.
- [ ] Each minimum/maximum step says which joystick position it matches (Horizontal minimum = fully left, maximum = fully right; Vertical minimum = fully down, maximum = fully up).
- [ ] On the Horizontal minimum step, the "Reverse this axis" checkbox shows the current wiring (ticked when Settings → Hardware → Wiring says reversed). If − moves the arm right, ticking it keeps the arm where it is, restarts the axis at its minimum, and − now moves it left. The robot does not restart; Settings → Hardware → Wiring shows the axis as reversed, still after a power cycle, and the joystick moves that axis the right way.
- [ ] Marking a maximum at or below its minimum shows an error and stays on the step.
- [ ] Marking a Stow position outside the new limits shows an error and stays on the step.
- [ ] The sweep visits the four corners of the new envelope slowly and ends at the Stow position, without touching anything.
- [ ] "Nothing touched: save" saves; the banner disappears and the joystick now reaches the new limits but not beyond.
- [ ] After saving, the robot Stows, then the Horizontal minimum speed step sweeps that axis by itself between the new limits; Faster/Slower change the speed and "Keep this speed" saves it and moves on to the next speed.
- [ ] On a speed step, "Skip (keep N °/s)" keeps that speed unchanged (Settings → Motion shows the same value) and moves on.
- [ ] The maximum speed steps start the sweep from the current maximum speed.
- [ ] Stow from the main screen during a speed step stops the sweep without saving; "Sweep again" restarts it.
- [ ] Leaving the wizard with Back during a speed step stops the sweep; the new limits stay saved, and the speeds already kept stay too.
- [ ] "Your robot is ready" lists each axis's Speed range; Done goes back to the main screen.
- [ ] Settings shows the new limits and Stow position. Changing them there is validated the same way.
- [ ] Pressing Cancel at any step, or Stow on the main screen, keeps the previous limits and Stows.
- [ ] After a power cycle the robot is still calibrated, with the same limits.
- [ ] Running Calibration again starts from the current limits.
- [ ] Once a Calibration is saved (including when reopening the wizard right after one), the wizard's button reads "Recalibrate"; during a new Calibration the cancel button reads "Cancel the new Calibration", keeps the saved limits, Stows and goes back to the main screen.

## Hardware configuration

- [ ] Settings opens on the Motion tab and has four tabs (Motion, Hardware, Network, Robot) that fit a phone screen; each section is in one tab, and reloading keeps the tab.
- [ ] Settings → Hardware → Wiring lists only pins allowed on the board's chip, and shows the current wiring.
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
- [ ] Settings → Robot and the Info page link to Help. "Open Discord" opens the server invite; "Common problems" opens the troubleshooting doc.
- [ ] On Help, "Prepare the report" with an empty description asks for one; with a description, it shows the exact public text (description + redacted report) and a warning that it will be public.
- [ ] With the robot unplugged for 10 s, the main screen shows "Need help?" under the header, opening the "The robot stopped responding" troubleshooting section on GitHub in a new tab. A failed firmware upload and a failed Wi-Fi attempt show "Need help?" leading to the robot's Help page.
- [ ] "Open the issue on GitHub" downloads the report and opens GitHub's bug form with the description, the report, the firmware version and the chip already filled in.
- [ ] With internet on the phone, after "Prepare the report", Help offers "Send to support" with an optional Contact field. Sending shows "Sent: issue #N was created" and a link to that issue on GitHub, labelled `from-device`, containing the description, the Device ID and the full redacted report. Sending a saved hotspot report the same way removes it.
- [ ] With "Follow it on Discord" ticked and a Discord username in Contact, sending also opens a `#support` post tagged bug that mentions that user and links to the issue; the page shows "Open the Discord thread", and the issue links back to the post.
- [ ] In a `#support` post, a moderator's `/issue` creates a `needs-triage` issue with the first message and attached report, and answers with its link; running it again answers with the same issue; a member without Manage Threads is refused.
- [ ] After a release that closes an issue linked to a `#support` post, the post receives a message with the version and how to update, once.
- [ ] With the phone offline or the support service disabled, "Send to support" is not offered, or shows that the service is unavailable, and "Open the issue on GitHub" stays available.
- [ ] On the setup hotspot, "A problem?" downloads the redacted report; Help shows "Save for later" and saves a description.
- [ ] After a restart and once on Wi-Fi, the main screen shows "A problem report saved on the hotspot is waiting"; Help → "Review and send" prepares the same issue, "Discard" removes it and the banner goes away.
- [ ] A phone set to French shows the interface in French. Choosing English in Settings switches it at once and the choice is remembered after a reload. "Automatic" goes back to the phone's language.
- [ ] iPhone (Safari) → Share → Add to Home Screen: the icon appears, and opening it shows the interface full screen without the Safari bars.
- [ ] Android (Chrome) → menu → Add to Home screen: the icon appears and opens the interface.

## Settings PIN

- [ ] On a fresh robot, Settings → Robot → Settings PIN says "Off", and every setting can be changed without a PIN.
- [ ] Set a PIN of 4 digits. You stay signed in on that phone.
- [ ] On a second phone (or a private window), the joystick, the speed slider and Stow work without a PIN.
- [ ] On that second phone, saving any setting (limits, speed and acceleration, wiring, name), starting Calibration or changing Wi-Fi from the hotspot asks for the PIN. With the right PIN the change goes through.
- [ ] Five wrong PINs in a row show "Too many wrong PINs" with a countdown; the right PIN is refused until it ends.
- [ ] After a restart, the PIN is asked again.
- [ ] Changing the PIN signs out other phones. Removing it makes everything open again.
- [ ] A 3-digit or non-numeric PIN is refused.

## Firmware update (OTA)

Build two versions that differ visibly (for example change `CWC_VERSION` in `platformio.ini`) and use the `firmware.bin` of the second.

- [ ] Settings → Robot → Firmware update shows the installed version.
- [ ] Uploading the new file shows progress, then "Installed". The robot restarts, the page reloads and the Info page shows the new version.
- [ ] Wi-Fi, name, Calibration, speed, wiring and PIN are unchanged after the update.
- [ ] Uploading a file built for another chip is refused with "for another chip", and nothing changes.
- [ ] Uploading a non-firmware file (a photo) is refused with "not an ESP32 firmware".
- [ ] From the hotspot (Continue without Wi-Fi), Settings → Robot → Firmware update explains to open `http://192.168.4.1/settings.html#robot` in the regular browser; from there, the update works.
- [ ] Unplugging the robot's Wi-Fi (or closing the tab) halfway through: the robot keeps running its old firmware, and a new upload then works.
- [ ] With a PIN set, uploading from a phone that hasn't entered it asks for it.

## Home Assistant (MQTT)

Use a Home Assistant with the Mosquitto add-on and the MQTT integration ([setup](home-assistant.md)).

- [ ] On a fresh robot, Settings → Network → Home Assistant is off and the robot makes no MQTT connection.
- [ ] Enabling it with the broker address, user and password shows "Connected" within a few seconds.
- [ ] A wrong password shows "refused the user or password"; a wrong address shows "Can't reach the broker". The joystick stays smooth while the robot keeps retrying.
- [ ] The device appears in Home Assistant under the robot's name, with Stow, Horizontal/Vertical position, Horizontal/Vertical maximum speed and Activity.
- [ ] Moving the wand with the joystick updates the positions and Activity in Home Assistant.
- [ ] Setting a position in Home Assistant moves the axis there smoothly. A value beyond the Travel limits stops at the limit.
- [ ] Pressing Stow in Home Assistant stows, even while someone is using the joystick.
- [ ] Changing a maximum speed in Home Assistant shows in the robot's Settings, and a value of 5 °/s is ignored.
- [ ] Unplugging the robot makes its entities unavailable in Home Assistant within about a minute. Plugging it back makes them available again.
- [ ] Renaming the robot: after its restart, Home Assistant shows the new name.
- [ ] The device also shows a Play session switch and a Session duration number (1–60 min) matching the main screen's duration.
- [ ] On a calibrated robot, turning Play session on starts a session for the Session duration; Activity shows `playing`. Turning it off Stows and the switch stays off.
- [ ] A session started from the main screen turns the switch on in Home Assistant; when its time is up (try 1 min), the switch turns off by itself. Touching the joystick or pressing Stow also turns it off.
- [ ] Before Calibration, turning Play session on does nothing: the switch flips back off and the robot's log shows "Play session refused: not calibrated".
- [ ] Changing Session duration in Home Assistant updates the main screen's duration, and it is still there after a restart. A value of 0 or 61 is refused.

## Native integration (discovery and API)

- [ ] The robot is visible as `_catwandconnect._tcp` to Home Assistant's zeroconf: with the Native integration installed it shows under Settings → Devices & services → Discovered; without it, `avahi-browse -rt _catwandconnect._tcp` (Linux) or `dns-sd -B _catwandconnect._tcp` (macOS) on the same network lists it on port 80 with TXT records `id` (the Device ID of the Diagnostic report), `api=1`, `version` (the Info page's) and `name` (the robot's name).
- [ ] `GET /api/info` contains `"api":1` and `"id"` equal to the Device ID of the Diagnostic report.
- [ ] `curl -X POST -H 'Content-Type: application/json' -d '{"axis":"horizontal","degrees":90}' http://<name>.local/api/position` moves the wand there smoothly and answers `202`. A value beyond the Travel limits stops at the limit; during a Play session it ends the session without Stow; `{"axis":"up","degrees":90}` answers `400`.

## Native integration (Home Assistant)

Use a Home Assistant 2026.10 or later with HACS, on the robot's network, and a robot without the MQTT connection ([setup](home-assistant.md#native-integration)). Check the entities on the device page under Settings → Devices & services → CatWandConnect.

- [x] HACS → Custom repositories accepts `https://github.com/vistorx/ha-catwandconnect` as an Integration; CatWandConnect downloads, and after a restart it is in Add integration.
- [ ] The robot shows under Discovered with its name. Without a Settings PIN, Add then Submit creates the device with no PIN asked: name, manufacturer CatWandConnect, model, chip, firmware version, and a Visit link that opens the robot's interface.
- [x] Discovering the robot again (restart Home Assistant or the robot) creates no second device.
- [x] Add integration → CatWandConnect with the robot's IP address, then with its `.local` name, adds it (remove it in between).
- [x] With a Settings PIN set, adding the robot asks for it: a wrong PIN shows "Wrong PIN" and lets you retry; enough wrong PINs show the wait in seconds; the right PIN adds it.
- [x] A robot on a firmware older than 0.2.0-beta.2 is refused with "firmware is too old", the minimum version and a link to the update steps.
- [x] The device has 8 entities with the default ids (`sensor.catwandconnect_activity`, `switch.catwandconnect_play_session`, `number.catwandconnect_session_duration`, `button.catwandconnect_stow`, `number.catwandconnect_horizontal_position`, `number.catwandconnect_vertical_position`, `number.catwandconnect_horizontal_maximum_speed`, `number.catwandconnect_vertical_maximum_speed`), named in French when Home Assistant is in French.
- [x] Activity follows the robot within a second: `manual` with the joystick, `stowing`, `calibrating`, `tuning_speed`, `playing`, then `idle`.
- [x] Moving the wand with the joystick updates both positions within a second. Setting a position in Home Assistant moves the axis there smoothly; the slider only spans the Travel limits.
- [x] Changing a maximum speed in Home Assistant shows in the robot's Settings → Motion; a change in the robot's Settings shows in Home Assistant after a reload of the device page.
- [x] Session duration matches the main screen; changing it in Home Assistant updates the main screen, and 0 or 61 is refused.
- [x] On a calibrated robot, Play session on starts a session for the Session duration; off Stows and the switch stays off. A session started from the main screen turns the switch on, and it turns off by itself when the session ends.
- [x] Before Calibration, Play session on shows an error saying the robot isn't calibrated, and the switch stays off.
- [x] Stow in Home Assistant stows, even while someone is using the joystick.
- [x] Unplugging the robot makes every entity unavailable within a few seconds; plugging it back makes them available again by themselves, with current values.
- [x] Restarting Home Assistant with the robot on brings the entities back without any action.
- [x] Renaming the robot: after its restart, the device name in Home Assistant follows.
- [x] Changing the robot's Settings PIN: the next command that needs it (a maximum speed) fails and Home Assistant asks to Reconfigure; typing the new PIN makes the commands work again.
- [x] Download diagnostics on the device page: the file has the firmware, settings and state, with the Wi-Fi name, IP and MAC addresses, broker address and MQTT credentials redacted, and no PIN.
- [x] With the MQTT connection also on, a second device appears with ids ending in `_2`, as the guide warns; turning MQTT off in the robot and deleting the MQTT device leaves one.
- [x] The ready-made Native integration dashboard from the guide, pasted as is, shows every card with no "Entity not available".
- [x] The joystick card (integration 0.1.1+) is in the card picker; holding and dragging moves the wand, letting go stops it; it greys out while the robot is offline.

## Web flasher and releases

- [ ] The release page has a `-factory.bin` and an `-ota.bin` per chip, and the version in the Info page matches the tag.
- [ ] The web flasher opens in Chrome or Edge on a computer and shows the release's version. In Safari or Firefox it explains that it needs Chrome or Edge.
- [ ] With an ESP32, an ESP32-S3 and an ESP32-C3 board in turn: "Connect and install" detects the chip and installs. Erasing on a first install works.
- [ ] After installing, "Connect to Wi-Fi" provisions the robot and "Visit device" opens its interface.
- [ ] Installing over an existing robot without erasing keeps its settings.

