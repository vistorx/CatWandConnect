# Install the software

This guide puts the firmware on the board from your browser and gets the robot on your Wi-Fi.

## What you need

| What | Notes |
|------|-------|
| The wired board and servos | From [step 2](wiring.md). A bare board works too: the firmware can go on it before anything is wired. |
| A USB cable that carries data | Some cables only charge: if the board doesn't show up, try another one. |
| A computer with Chrome or Edge | The web flasher needs Web Serial, which other browsers lack. |

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="../media/buttons/install.svg" alt="Install in your browser" width="260"></a>
</p>

## Steps

<!-- video: installing the firmware with the web flasher -->
Photos and video coming soon.

### 1. Open the web flasher

Open the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/) in **Chrome or Edge** on a computer. No tools to install.

> [!NOTE]
> The web flasher installs the **Stable** version by default. The **Beta** channel gets new changes first and may still have bugs. See [release channels](releasing.md#channels).

<!-- photo: step 1 -->

### 2. Plug in the board

Plug the ESP32 board in over USB, with a cable that carries data. Keep the USB-C charger unplugged while the board is on the computer, unless you know your board blocks the external 5 V from flowing back (see [Wiring](wiring.md#wiring)).

The board doesn't show up? Try another cable, then install the driver for its USB chip: see [Installing with the web flasher](troubleshooting.md#installing-with-the-web-flasher) for the download links.

<!-- photo: step 2 -->

### 3. Install the firmware

Press **Connect and install** and pick the board's port. The right firmware for your chip is chosen for you.

<!-- photo: step 3 -->

### 4. Join your Wi-Fi

Enter your Wi-Fi when the web flasher asks, then choose **Visit device**.

Skipped it? Power the robot: with no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password, or tap **Continue without Wi-Fi** to use the robot from the hotspot for now.

<!-- photo: step 4 -->

### 5. Open the robot's page

Rejoin your home Wi-Fi and open the address shown at the end of setup, for example **`http://catwandconnect-3f9a.local/`** (or the IP shown there). With the default name, the robot's `.local` address ends with 4 characters of its own, so two robots never clash; robots set up before version 0.2.1 keep `catwandconnect.local`. Opened by IP, the page first offers **Use the robot's permanent address**: tap **Go** to switch to the `.local` address, which keeps working when the IP changes. It only offers it when your phone or computer can open it; otherwise see [the `.local` address doesn't open](troubleshooting.md#wi-fi).

<!-- photo: step 5 -->

### 6. Make it yours

Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`. On a phone, the first visit shows how to add the robot to your home screen for an app-like icon (**Got it** hides the tip; Settings → Robot or Help → **Add to your home screen** shows it again). Settings → Network shows the robot's IP address, and **Change Wi-Fi network** moves it to another network later (new router, new home).

<!-- photo: step 6 -->

## Calibrate

Do this once the robot is [assembled](assembly.md). The main screen invites you to. The wizard has you nudge each axis to the ends of its safe travel and pick the Stow position, then sweeps the new envelope slowly so you can check nothing touches. Once the limits are saved, it sweeps each axis while you set its minimum and maximum speed, then makes short moves while you set its acceleration (Skip keeps a value as it is). Until then, movement stays within a conservative 60°–120° range on each axis.

Then **play**: drag the joystick to move the wand and let go to stop. Press **Stow** to put it away.

## Updating, building from source

- **Update a robot that is already installed:** when a new version is out, the robot's main screen shows *Version X available* with **What's new** and **Update**. Tap **Update**, then **Install now**: your phone downloads the version for the robot's chip and sends it to the robot, which restarts on it about a minute later. Keep the page open until it says the robot is back. Settings and Calibration are kept. The same is in **Settings → Robot → Firmware update**, where *Couldn't check for updates* means your phone has no Internet access (tap **Retry**). Finish or stop a Play session or Calibration first.
- **Stable or Beta:** **Settings → Robot → Firmware update → Update channel** chooses which versions the robot is offered ([release channels](releasing.md#channels)). The robot keeps it across restarts and updates. Until you choose, it follows the version installed: a beta offers betas.
- **No Internet on your phone** (for example on the robot's hotspot): download the `-ota.bin` file for your chip from the [releases](https://github.com/vistorx/CatWandConnect/releases) beforehand, then upload it in **Settings → Robot → Firmware update → Install from a file**.
- **Build from source** with [PlatformIO](https://platformio.org/); [Getting started](getting-started.md) covers the tools:

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # or esp32s3 / esp32c3
  ```

Something wrong? See [Troubleshooting](troubleshooting.md), then [Getting help](../SUPPORT.md).

Next: [Assemble the robot](assembly.md).
