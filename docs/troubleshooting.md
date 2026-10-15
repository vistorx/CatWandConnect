# Troubleshooting

When something goes wrong, open the serial monitor first. Plug the board in over USB and run `pio device monitor` from `firmware/` (115200 baud). The robot logs what it is doing, for example `[wifi] connected, open http://192.168.1.20/`.

## Installing with the web flasher

**The board doesn't show up in the port list.**
Work through these in order:

1. **Try another USB cable.** Many cables only carry power; you need one that carries data. Plug straight into the computer, not through a hub.
2. **Find the USB chip.** It is the small chip next to the board's USB connector; its marking says which driver you need. Boards whose USB goes straight to an ESP32-S3 or ESP32-C3 (no chip next to the connector) need no driver.
3. **Install its driver**, then unplug and replug the board and reopen the port list:

   | Chip marking | Windows | macOS |
   |--------------|---------|-------|
   | **CH340**, CH341 | [CH341SER.EXE](https://www.wch-ic.com/downloads/CH341SER_EXE.html) | [CH34XSER_MAC.ZIP](https://www.wch-ic.com/downloads/CH34XSER_MAC_ZIP.html) |
   | **CH9102**, CH343 | [CH343SER.EXE](https://www.wch-ic.com/downloads/CH343SER_EXE.html) | [CH34XSER_MAC.ZIP](https://www.wch-ic.com/downloads/CH34XSER_MAC_ZIP.html) |
   | **CP2102**, CP2104, CP210x | [CP210x VCP drivers](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers) | same page, macOS package |

   Linux and ChromeOS include these drivers. On macOS, allow the driver in **System Settings → Privacy & Security** after installing it.
4. **Still nothing?** Hold the board's **BOOT** button while plugging it in, then try again. Espressif's guide to [checking the serial connection](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html) helps find the port by hand.

**"Failed to initialize" or the install stops halfway.**
Close any other program using the port (serial monitor, Arduino IDE, another browser tab), hold **BOOT**, and press Connect and install again.

## Wi-Fi

**I can't find the `CatWandConnect-XXXX` hotspot.**
The hotspot only appears when the robot can't reach a known Wi-Fi. If it joined your network, open `http://catwandconnect.local/` instead. After a failed attempt, it can take up to 20 s for the hotspot to appear.

**The setup page doesn't open when I join the hotspot.**
Browse to `http://192.168.4.1/`. Some phones also ask whether to "stay connected" to a network without internet; answer yes.

**"Could not connect" on the setup page.**
Check the network name and password. The ESP32 only supports **2.4 GHz** Wi-Fi; if your router has separate 2.4 GHz and 5 GHz networks, pick the 2.4 GHz one.

**How do I move the robot to another Wi-Fi network?**
Open **Settings → Network**: the Wi-Fi block shows the current network, the robot's IP address and its `.local` address. Tap **Change Wi-Fi network**, read the warning, then pick the new network and enter its password. Once it joins, put your phone on that network and open the address shown. If it can't join (wrong password, 5 GHz network, out of range), it goes back to the current network within about a minute and nothing changes; if that fails too, its `CatWandConnect-XXXX` hotspot opens, as at first setup. With a settings PIN, the change asks for it.

**`catwandconnect.local` doesn't open.**
If you renamed the robot, its address changed too: it is shown in Settings and on the Info page, for example `catwandconnect-salon.local`.
Some Android phones and older Windows PCs don't resolve `.local` names. Use the IP address shown at the end of setup or in Settings → Network, or find it in your router's device list or the serial monitor. Also check that your phone is on the same Wi-Fi as the robot, not on mobile data or a guest network.
Opened by IP, the robot's page checks whether this phone or computer can open the `.local` address: if it can, it offers to go there; if not, it says so and suggests the next step. Then give the robot a fixed address in your router (a *DHCP reservation*, usually under the connected devices or LAN settings) so the IP address, and the home-screen icon that uses it, keep working.

**The robot keeps falling back to its hotspot.**
The signal is too weak or the router changed. Move the robot closer, or enter the new password on the hotspot's setup page. While nobody is connected to the hotspot, the robot retries your network every 2 minutes.

## Motion

**The servos don't move at all.**
- Check the common ground between the board and the servo supply.
- Check that the signal wires use the pins shown in Settings → Hardware → Wiring (the defaults are in [Wire the electronics](wiring.md#default-pins)).
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
Easiest: start Calibration and tick **Reverse this axis** on that axis's minimum step; the arm stays put and nothing restarts. Or tick **Reverse direction** in Settings → Hardware → Wiring, then run Calibration again.

**I chose the wrong pins and the servos don't move.**
Rewire to the pins shown in Settings → Hardware → Wiring, or pick the pins you wired. If you can no longer open the interface, erase and reflash the board (`pio run -e <chip> -t erase`, then upload) to return to the defaults.

## Firmware updates

**On the robot's hotspot, I can't pick the file.**
Phones open hotspot pages in a Wi-Fi sign-in window that has no file picker. Stay on the hotspot, open `http://192.168.4.1/settings.html#robot` in your regular browser (Chrome, Safari…), and upload from there. Over your home Wi-Fi this doesn't happen.

**"This firmware is for another chip".**
Each chip has its own file. The Info page shows your chip (ESP32, ESP32-S3 or ESP32-C3); download the `-ota.bin` file with that name.

**The update failed or the page lost the connection.**
The robot keeps its current firmware: the new one is only used once it has been received completely and verified. Reload the page and try again, closer to the router if Wi-Fi is weak.

**The robot doesn't come back after an update.**
Reflash it over USB with the web flasher (or PlatformIO). Settings are kept unless you choose to erase.

## Settings PIN

**I forgot the PIN.**
It can't be recovered over the network. Erase and reflash the robot (`pio run -e <chip> -t erase`, then upload). Everything goes back to factory settings, including Wi-Fi and Calibration.

**"Too many wrong PINs".**
After 5 wrong PINs the robot refuses new tries for 30 s, then longer after each further failure, up to 15 minutes. Wait and try again. Playing and Stow still work meanwhile.

## The robot stopped responding

**The page says "Offline" or "Need help?" and nothing opens.**
Your phone can no longer reach the robot, so its own Help page can't open either.

- Check the robot is powered: a USB charger or computer port that gives enough current, and a cable that carries power.
- Check your phone is on the same Wi-Fi as the robot, not on mobile data or a guest network.
- If the robot lost your Wi-Fi, it starts its `CatWandConnect-XXXX` hotspot: join it and open `http://192.168.4.1/` (see [Wi-Fi](#wi-fi)).
- Unplug the robot for a few seconds and plug it back in. The page reconnects by itself.
- Still nothing: connect it to a computer and open "Logs & Console" on the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/) to read what it says, then ask on [Discord](https://discord.gg/Rbqge3v9dk) or [open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) with that log.

## Still stuck?

See [Getting help](../SUPPORT.md): ask on the [Discord server](https://discord.gg/Rbqge3v9dk), no GitHub account needed, or [open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose). Include the **Diagnostic report** from the Info page (Settings → Robot → Open Info → Copy the report): it already holds the robot's recent log, with your Wi-Fi name and addresses hidden. Security problems go [privately](../SECURITY.md), never in public.
