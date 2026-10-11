# Troubleshooting

When something goes wrong, open the serial monitor first. Plug the board in over USB and run `pio device monitor` from `firmware/` (115200 baud). The robot logs what it is doing, for example `[wifi] connected, open http://192.168.1.20/`.

## Wi-Fi

**I can't find the `CatWandConnect-XXXX` hotspot.**
The hotspot only appears when the robot can't reach a known Wi-Fi. If it joined your network, open `http://catwandconnect.local/` instead. After a failed attempt, it can take up to 20 s for the hotspot to appear.

**The setup page doesn't open when I join the hotspot.**
Browse to `http://192.168.4.1/`. Some phones also ask whether to "stay connected" to a network without internet; answer yes.

**"Could not connect" on the setup page.**
Check the network name and password. The ESP32 only supports **2.4 GHz** Wi-Fi; if your router has separate 2.4 GHz and 5 GHz networks, pick the 2.4 GHz one.

**`catwandconnect.local` doesn't open.**
If you renamed the robot, its address changed too: it is shown in Settings and on the Info page, for example `catwandconnect-salon.local`.
Some Android phones and older Windows PCs don't resolve `.local` names. Use the IP address shown at the end of setup, or find it in your router's device list or the serial monitor. Also check that your phone is on the same Wi-Fi as the robot, not on mobile data or a guest network.

**The robot keeps falling back to its hotspot.**
The signal is too weak or the router changed. Move the robot closer, or enter the new password on the hotspot's setup page. While nobody is connected to the hotspot, the robot retries your network every 2 minutes.

## Motion

**The servos don't move at all.**
- Check the common ground between the board and the servo supply.
- Check that the signal wires use the pins shown in Settings → Wiring (the defaults are in [Electronics](electronics.md#default-pins)).
- At power-up the robot crawls very slowly (10 °/s) to its Stow position, then releases the servos. That is expected.

**The servos jitter or the board resets when they move.**
The supply is too weak or the wires too thin. Use a 5 V supply of 3 A or more, and add a 470–1000 µF capacitor across the servo supply.

**The joystick does nothing.**
- Check that the top right corner says "Connected".
- If the Stow button says "Stowing…", the robot is stowing, and nothing interrupts Stow. Wait for it to finish.

**The wand stops at a narrow range.**
Until Calibration has been completed, movement is limited to 60°–120° on each axis for safety. Run it from the banner on the main screen or from Settings.

**Calibration won't accept a position.**
The maximum of an axis must be above its minimum, and the Stow position must be inside the limits you just set. Nudge further and mark again.

**The wand moves the wrong way.**
Tick **Reverse direction** for that axis in Settings → Wiring, then run Calibration again.

**I chose the wrong pins and the servos don't move.**
Rewire to the pins shown in Settings → Wiring, or pick the pins you wired. If you can no longer open the interface, erase and reflash the board (`pio run -e <chip> -t erase`, then upload) to return to the defaults.

## Still stuck?

Open an [issue](https://github.com/vistorx/CatWandConnect/issues). Include the details from the **Info** page (Settings → Info → Copy) and, if you can, the serial log.
