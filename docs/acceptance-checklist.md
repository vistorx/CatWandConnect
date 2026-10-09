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
