# Install the software

## Install the firmware

No tools to install: the firmware goes on the board straight from your browser. You can install it on a bare board before [building the robot](assembly.md).

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="../media/buttons/install.svg" alt="Install in your browser" width="340"></a>
</p>

1. **Open** the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/) in **Chrome or Edge** on a computer.
2. **Plug** the ESP32 board in over USB, with a cable that carries data.
3. **Install:** press **Connect and install** and pick the board's port. The right firmware for your chip is chosen for you.
4. **Connect:** enter your Wi-Fi when asked, then choose **Visit device**.

> [!NOTE]
> The web flasher installs the **Stable** version by default. The **Beta** channel gets new changes first and may still have bugs. See [release channels](releasing.md#channels).

## Updating, building from source

- **Update a robot that is already installed:** when a new version is out, the robot's main screen shows *Version X available* with **What's new** and **Update**. Tap **Update**, then **Install now**: your phone downloads the version for the robot's chip and sends it to the robot, which restarts on it about a minute later. Keep the page open until it says the robot is back. Settings and Calibration are kept. The same is in **Settings → Robot → Firmware update**, where *Couldn't check for updates* means your phone has no Internet access (tap **Retry**). Finish or stop a Play session or Calibration first.
- **Stable or Beta:** **Settings → Robot → Firmware update → Update channel** chooses which versions the robot is offered ([release channels](releasing.md#channels)). The robot keeps it across restarts and updates. Until you choose, it follows the version installed: a beta offers betas.
- **No Internet on your phone** (for example on the robot's hotspot): download the `-ota.bin` file for your chip from the [releases](https://github.com/vistorx/CatWandConnect/releases) beforehand, then upload it in **Settings → Robot → Firmware update → Install from a file**.
- **Build from source** with [PlatformIO](https://platformio.org/); [Getting started](getting-started.md) covers the tools:

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # or esp32s3 / esp32c3
  ```

- **The board doesn't show up?** Try another cable, then install the driver for its USB chip: see [Installing with the web flasher](troubleshooting.md#installing-with-the-web-flasher) for the download links.

## First start

1. If you entered your Wi-Fi in the web flasher, skip to step 3. Otherwise power the robot: with no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot.
2. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password, or tap **Continue without Wi-Fi** to use the robot from the hotspot for now.
3. Rejoin your home Wi-Fi and open **`http://catwandconnect.local/`** (or the IP shown on the setup page). Opened by IP, the page first offers **Use the robot's permanent address**: tap **Go** to switch to the `.local` address, which keeps working when the IP changes. It only offers it when your phone or computer can open it; otherwise see [`catwandconnect.local` doesn't open](troubleshooting.md#wi-fi).
4. **Calibrate.** The main screen invites you to. The wizard has you nudge each axis to the ends of its safe travel and pick the Stow position, then sweeps the new envelope slowly so you can check nothing touches. Once the limits are saved, it sweeps each axis while you set its minimum and maximum speed, then makes short moves while you set its acceleration (Skip keeps a value as it is). Until then, movement stays within a conservative 60°–120° range on each axis.
5. **Play.** Drag the joystick to move the wand and let go to stop. Press **Stow** to put it away.
6. **Make it yours.** Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`. On a phone, the first visit shows how to add the robot to your home screen for an app-like icon (**Got it** hides the tip; Settings → Robot or Help → **Add to your home screen** shows it again). Settings → Network shows the robot's IP address, and **Change Wi-Fi network** moves it to another network later (new router, new home).

Something wrong? See [Troubleshooting](troubleshooting.md), then [Getting help](../SUPPORT.md).

Next, optional: [Set up Home Assistant](home-assistant.md).
