# Remote access

Remote access opens the robot's page when you are away from home: check on it, start a Play session or Stow it from work or on holiday. It is optional and off until you turn it on.

The robot does it with [Tailscale](https://tailscale.com/), a free private network for your own devices (a *tailnet*). The robot joins your tailnet as a device of its own, and your phone joins it with the Tailscale app. Only your devices can reach the robot; nothing is opened on your router and no CatWandConnect service sits in the middle.

## What you need

| What | Notes |
|------|-------|
| A Tailscale account | Free for personal use. Sign up at [tailscale.com](https://tailscale.com/) with a Google, Microsoft, Apple or GitHub account. |
| The Tailscale app on your phone | From the App Store or Google Play, signed in to the same account. |
| The robot on your Wi-Fi | A classic ESP32, ESP32-S3 or ESP32-C3 with firmware that has Remote access. |

## Turn it on

1. In your Tailscale account, open the [Keys page](https://login.tailscale.com/admin/settings/keys) and click **Generate auth key**. Turn **Reusable** off, and set the expiry as long as you can (90 days at most): Tailscale asks a key for the robot's first login only, then the robot keeps its own device key.
2. Copy the key. It starts with `tskey-auth-`.
3. On the robot's page, open **Settings → Network → Remote access**, tick **Enabled**, paste the key and tap **Save** (type the Settings PIN if one is set).
4. **Status** goes from *Connecting…* to *Connected*, within a minute. **Remote address** shows the robot's tailnet address, like `http://100.101.102.103/`.
5. In the [Tailscale admin console](https://login.tailscale.com/admin/machines), find the robot (named like its `.local` address, e.g. `catwandconnect-3f9a`), open its **⋯** menu and choose **Disable key expiry**, or the robot drops off the tailnet when its key expires. **Key expires** in Settings shows the date until you do.

Away from home, open the Tailscale app on your phone, check it is connected, and open the robot's **Remote address** in your browser. Add it to your home screen too.

!!! note "Direct or through a relay"
    *Connected (direct)* means your phone reached the robot straight through the internet. *Connected (through a relay)* means it goes through one of Tailscale's relays, which works everywhere but is a little slower. Both are encrypted end to end.

## Turn it off

Untick **Enabled** and tap **Save**. The robot leaves your tailnet at once and forgets its key and its device keys. Remove the robot from the Tailscale admin console too. To use Remote access again, paste a new key.

## Good to know

- **A new key.** If Status says *Key expired: paste a new key*, generate one as above and paste it: the robot leaves your tailnet and joins again with it.
- **Firmware updates from afar ask twice.** If the update goes wrong, nobody may be near the robot to power it back. You can still update; the page asks you to confirm a second time.
- **The key stays on the robot.** It is stored in the robot's flash like your Wi-Fi password, and never shown again, never in the robot's log and never in the Diagnostic report. The report hides the robot's tailnet address and name too.
- **What the robot connects to.** When it is on, and only then, the robot connects to Tailscale's servers to find your devices, and to your devices. When it is off, it connects to nothing outside your home ([ADR 0015](adr/0015-the-robot-connects-to-the-owners-remote-access-provider.md)).
- **Small tailnets.** The robot has little memory: it is made for a personal tailnet with a handful of devices.

## If it does not connect

| Status | What to do |
|------|-------|
| *Connecting…* for more than 2 minutes | Check that the robot's Wi-Fi reaches the internet. Some guest or school networks block Tailscale. |
| *Key expired: paste a new key* | The key was already used, expired or revoked. Generate a new one and paste it. |
| *Error, retrying* | The robot retries every 30 seconds. If it lasts, [open an issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) with the Diagnostic report from the Info page. |
| *Not available on this chip* | This firmware has no Tailscale client for your board. |
