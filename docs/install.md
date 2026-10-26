# Install the software

This guide puts the firmware on the board from your browser, gets the robot on your Wi-Fi and checks that the servos move. About 15 minutes, no tools to install.

<!-- github-only -->
[![CatWandConnect tutorial - Install firmware and first tests](https://i.ytimg.com/vi/rjmGGUYjKDc/hqdefault.jpg)](https://youtu.be/rjmGGUYjKDc)
<!-- /github-only -->
<iframe style="width:100%;aspect-ratio:16/9;border:0" src="https://www.youtube-nocookie.com/embed/rjmGGUYjKDc" title="CatWandConnect tutorial - Install firmware and first tests" allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

The video shows every step below ([watch on YouTube](https://youtu.be/rjmGGUYjKDc)).

## What you need

| What | Notes |
|------|-------|
| The wired electronics and the two servos | From [step 2](wiring.md). |
| The USB-C charger | The one from the wiring guide. |
| A USB cable that carries data | Some cables only charge: if the board doesn't show up, try another one. |
| A computer with Chrome or Edge | The web flasher needs Web Serial, which other browsers lack. |

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="../media/buttons/install.svg" alt="Install in your browser" width="260"></a>
</p>

## Steps

### 1. Connect the servos

Plug each servo into the socket of a servo cable from the wiring guide: the servo's brown wire on the cable's − (black) side. Keep the charger unplugged.

![The two servos plugged into the servo cables of the wired electronics, the board's pins framed in green.](../media/photos/install-step-01-servos.jpg)

### 2. Connect the board

Plug the board into the computer over USB. Its LED lights up: the board is powered.

The board doesn't show up later, or its LED stays off? Try another cable, then see [Installing with the web flasher](troubleshooting.md#installing-with-the-web-flasher) for the driver of its USB chip.

![The board's USB port, framed in green, ready for the cable to the computer.](../media/photos/install-step-02-board-usb.jpg)

### 3. Open the web flasher

In **Chrome or Edge** on the computer, open the website, go to **Build → Install the software**, then press **Install in your browser**. Or open the [web flasher](https://vistorx.github.io/CatWandConnect/flasher/) directly.

> [!NOTE]
> The web flasher installs the **Stable** version by default. The **Beta** channel gets new changes first and may still have bugs. See [release channels](releasing.md#channels).

<p>
  <img src="../media/screenshots/install-step-03a-website.png" alt="The website's home page, with the Build tab framed." width="49%">
  <img src="../media/screenshots/install-step-03b-build.png" alt="The Build tab, with Install the software framed in the menu." width="49%">
  <img src="../media/screenshots/install-step-03c-install-page.png" alt="The Install the software page, with the Install in your browser button framed." width="49%">
  <img src="../media/screenshots/install-step-03d-flasher.png" alt="The web flasher, with the Connect and install button framed." width="49%">
</p>

### 4. Pick the board

Press **Connect and install**. The browser lists the serial ports: pick the board's (often *USB Single Serial*, *CP210x* or *CH910x*), then **Connect**. In the small menu, choose **Install CatWandConnect**.

<p>
  <img src="../media/screenshots/install-step-04a-port.png" alt="The browser's port list, with the board's USB serial port framed." width="49%">
  <img src="../media/screenshots/install-step-04b-connect.png" alt="The port selected and the Connect button framed." width="49%">
  <img src="../media/screenshots/install-step-04c-install-menu.png" alt="The flasher's menu, with Install CatWandConnect framed." width="49%">
</p>

### 5. Install the firmware

For a first install, keep **Erase device** ticked and press **Next**, then **Install**. Wait about two minutes and keep the page visible until it finishes.

<p>
  <img src="../media/screenshots/install-step-05a-erase.png" alt="The Erase device dialog, the box ticked and Next framed." width="49%">
  <img src="../media/screenshots/install-step-05b-confirm.png" alt="The Confirm installation dialog, with Install framed." width="49%">
  <img src="../media/screenshots/install-step-05c-installing.png" alt="The Installing dialog with its progress, which takes about two minutes." width="49%">
</p>

### 6. Join your Wi-Fi

When the installation is complete, press **Next**. The flasher looks for networks: pick yours, enter its password and press **Connect**.

<p>
  <img src="../media/screenshots/install-step-06a-complete.png" alt="Installation complete, with Next framed." width="49%">
  <img src="../media/screenshots/install-step-06b-scanning.png" alt="Configure Wi-Fi, scanning for networks." width="49%">
  <img src="../media/screenshots/install-step-06c-network.png" alt="The list of Wi-Fi networks to choose from." width="49%">
  <img src="../media/screenshots/install-step-06d-password.png" alt="The Wi-Fi password entered and Connect framed." width="49%">
</p>

<details markdown="1">
<summary>Skipped it, or it didn't work?</summary>

Power the robot: with no Wi-Fi configured, it opens a **`CatWandConnect-XXXX`** hotspot. Join it from your phone. The setup page opens by itself; if not, browse to `http://192.168.4.1/`. Pick your Wi-Fi and enter its password, or tap **Continue without Wi-Fi** to use the robot from the hotspot for now.

</details>

### 7. Open the robot's page

Press **Visit device**: the robot's page opens. From your phone, open the same address, for example **`http://catwandconnect-3f9a.local/`**.

![Device connected to the network, with Visit device framed.](../media/screenshots/install-step-07-visit-device.png)

<details markdown="1">
<summary>About the address, the name and the home screen</summary>

- With the default name, the robot's `.local` address ends with 4 characters of its own, so two robots never clash; robots set up before version 0.2.1 keep `catwandconnect.local`.
- Opened by IP, the page first offers **Use the robot's permanent address**: tap **Go** to switch to the `.local` address, which keeps working when the IP changes. It only offers it when your phone or computer can open it; otherwise see [the `.local` address doesn't open](troubleshooting.md#wi-fi).
- Name the robot in Settings; its address follows, for example `http://catwandconnect-salon.local/`.
- On a phone, the first visit shows how to add the robot to your home screen for an app-like icon (**Got it** hides the tip; Settings → Robot or Help → **Add to your home screen** shows it again).
- Settings → Network shows the robot's IP address, and **Change Wi-Fi network** moves it to another network later (new router, new home).

</details>

### 8. Put the servos in their neutral position

Unplug the USB cable, then plug the charger in. The robot starts and both servos turn to the middle of their travel, their neutral position: the robot's page shows **Horizontal 90°** and **Vertical 90°**. Don't turn the servos' axes by hand: [assembly](assembly.md) fits the parts in this position.

Push a servo horn onto each servo, in line with it and without screwing it: it shows the neutral position and makes the next test easy to see.

<p>
  <img src="../media/screenshots/install-step-08a-neutral.png" alt="The robot's page, with Horizontal 90° and Vertical 90° framed." width="49%">  <img src="../media/photos/install-step-08b-servo-horn.jpg" alt="A servo horn pushed onto a servo, in line with it." width="49%">
</p>

### 9. Test that the servos move

On the robot's page, drag the joystick gently: one servo turns left and right, the other up and down. Let go to stop. Until Calibration, each axis stays in a safe range around the middle (60°–120°).

Then press **Stow**: both servos go back to the middle. Take the servo horns off: the robot is ready to be assembled.

![The servo horns turning while the joystick is dragged on the robot's page.](../media/photos/install-step-09-test.jpg)

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
