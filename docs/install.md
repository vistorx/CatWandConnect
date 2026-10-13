# Install and first start

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
> Until the first stable release, the web flasher installs the **beta**, which gets new changes first and may still have bugs. See [release channels](releasing.md#channels).

## Updating, building from source

- **Update a robot that is already installed:** download the `-ota.bin` file for your chip from the [latest release](https://github.com/vistorx/CatWandConnect/releases), then upload it in the robot's **Settings → Robot → Firmware update**. Settings and Calibration are kept.
- **Build from source** with [PlatformIO](https://platformio.org/); [Getting started](getting-started.md) covers the tools:

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # or esp32s3 / esp32c3
  ```

- **The board doesn't show up?** Try another cable, then install the driver for its USB chip: see [Installing with the web flasher](troubleshooting.md#installing-with-the-web-flasher) for the download links.

## First start

1. If you entered your Wi-Fi in the web flasher, skip to step 3. Otherwise power the robot: with no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot.
2. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password, or tap **Continue without Wi-Fi** to use the robot from the hotspot for now.
3. Rejoin your home Wi-Fi and open **`http://catwandconnect.local/`** (or the IP shown on the setup page).
4. **Calibrate.** The main screen invites you to. The wizard has you nudge each axis to the ends of its safe travel and pick the Stow position, then sweeps the new envelope slowly so you can check nothing touches. Until then, movement stays within a conservative 60°–120° range on each axis.
5. **Play.** Drag the joystick to move the wand and let go to stop. Press **Stow** to put it away.
6. **Make it yours.** Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`. Use your browser's "Add to Home Screen" for an app-like icon.

Something wrong? See [Troubleshooting](troubleshooting.md), then [Getting help](../SUPPORT.md).
