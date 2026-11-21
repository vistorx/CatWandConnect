# Remote access prototype: Tailscale on a classic ESP32

> **Throwaway.** This folder answers the questions of
> [#267](https://github.com/vistorx/CatWandConnect/issues/267) and is deleted
> once the results are recorded on
> [#251](https://github.com/vistorx/CatWandConnect/issues/251). It is not the
> robot firmware and never will be.

A small ESP-IDF 5 firmware that puts a classic ESP32 (no PSRAM) on your
tailnet with [microlink](https://github.com/CamM2325/microlink) (MIT, an
unofficial Tailscale-compatible client, version 2.3.0), and shows how much
memory is left while a page and a `/ws`-like WebSocket are open over the
tunnel.

What it runs:

- **Serial setup** on the USB console: Wi-Fi network, Wi-Fi password,
  Tailscale auth key, device name and an optional subnet route. Saved in NVS
  on the board. The password and the key are never shown back (only their
  length) and never logged. Nothing secret is in this repository or in the
  binary.
- **Status page** on port 80: `http://<tailnet IP>/` (free heap, minimum free
  heap, largest free block, uptime, tailnet IP and name, Tailscale state,
  reconnect count, Wi-Fi drops and RSSI, each peer with its path: direct or
  DERP relay). The same figures as JSON at `/api/status`.
- **WebSocket** at `/ws`: about 10 small JSON messages per second, like the
  robot's own `/ws`. The page's *WebSocket load test* button opens it and
  shows the message rate and the round-trip time through the tunnel (the page
  sends a ping every second and the board echoes it).
- **Subnet route** (optional): microlink can act as a subnet router. Type a
  `/32` route (for example the camera's address, #252) in the setup and the
  board advertises it.
- A **status line on the serial log every 10 s** with the same figures.

## Build size (classic `esp32`)

Built with PlatformIO `espressif32@6.12.0` (ESP-IDF 5.5.0), microlink 2.3.0.

| | Bytes |
|---|---:|
| App image (`firmware.bin`) | 912,496 (46.4 % of the slot) |
| App slot (`min_spiffs`, same as the robot) | 1,966,080 |
| microlink's own code and constants (archive, before linking) | ~138,600 |
| Static RAM (`.dram0.data` + `.dram0.bss`) | 41,640 |
| microlink's static RAM (archive `.bss`) | ~18,500 |
| IRAM code | 94,131 |

The free heap is only known at run time: the boot banner and every
`STATUS` line print it.

## 1. Create a Tailscale auth key

1. Open the [Tailscale admin console](https://login.tailscale.com/admin/settings/keys)
   → **Settings** → **Keys** → **Generate auth key**.
2. **Reusable**: off. **Ephemeral**: off (an ephemeral node disappears when it
   goes offline, which would spoil the "stays up for hours" test).
   **Pre-approved**: on if your tailnet requires device approval.
   Expiration: the default is fine; the key is only used for the first
   registration.
3. Copy the key (`tskey-auth-…`). Keep it for step 3; don't paste it anywhere
   else.

After the board has joined, you can disable key expiry for it in the admin
console (**Machines** → the board → **Disable key expiry**) so the test is not
cut short.

## 2. Flash the board

> **This erases the robot firmware and its settings on that board.** To go
> back afterwards, reinstall the robot firmware from the
> [web flasher](https://vistorx.github.io/CatWandConnect/flasher/).

1. Download
   [`bin/remote-access-proto-esp32-merged.bin`](bin/remote-access-proto-esp32-merged.bin)
   (on GitHub: open the file, then **Download raw file**).
2. Open <https://espressif.github.io/esptool-js/> in **Chrome or Edge** on a
   computer, plug the board in by USB.
3. **Program** section: Baudrate 921600 (or 115200 if it fails), click
   **Connect** and pick the board's serial port (CH9102F / USB serial).
4. Flash Address **`0x0`**, choose the downloaded `.bin`, click **Program**.
   Wait for the end (about a minute).
5. Click **Disconnect** (the serial port can only be open in one tab).

The image covers the whole start of the flash (bootloader, partition table,
app), and the gaps are filled with `0xFF`, so the robot's old settings are
wiped as well.

## 3. Serial setup

The esptool-js page's Console only shows the log (it can't send what you
type), so the setup uses Google Chrome Labs' web serial terminal.

1. Open <https://googlechromelabs.github.io/serial-terminal/> in Chrome or
   Edge. Baud rate **115200**. Leave **Local echo unticked** (otherwise the
   page itself would show the password and the key as you type them).
   Click **Connect** and pick the board's port.
2. Press the board's **EN/RST** button. With no settings saved, the board
   repeats `Press Enter to start the setup` every 5 s: press Enter, then
   answer each question and press Enter:
   - Wi-Fi network name
   - Wi-Fi password (shown as `*`)
   - Tailscale auth key (shown as `*`; paste it with Ctrl+V / Cmd+V)
   - Device name on the tailnet (Enter = `catwand-proto`)
   - Subnet route, e.g. `192.168.1.50/32` (Enter = none)
3. The board joins Wi-Fi, then the tailnet. Wait for
   `Tailscale state: CONNECTED`; the `STATUS` lines then show `ip=100.x.y.z`.

Later you only need the log: the esptool-js page's **Console** (115200) or
this terminal both work.

To change the settings later: reset the board and **press any key within
3 s** of the boot message, then `1` (set up again) or `2` (erase everything).
While it runs, type `s` (status now), `r` (restart), `e` (erase all settings
and the tailnet identity, asks for `Y`) or `h` (help).

## 4. What to test and what to report

Report on #251 (copy the list, fill it in). Copy the figures from the page or
from the `STATUS` lines; a few raw `STATUS` lines help.

- [ ] **Joins the tailnet**: the board appears in the admin console
      (as *Linux*), `Tailscale state: CONNECTED`, tailnet IP: ______
- [ ] **Page over Tailscale away from home**: phone on 4G (Wi-Fi off),
      Tailscale app on, open `http://<tailnet IP>/`. Opens? ______
- [ ] **Path**: in the page's Peers table, your phone shows `direct` or
      `relay`? On 4G: ______ On home Wi-Fi: ______
      (`tailscale status` on a computer says it too.)
- [ ] **Latency**: `tailscale ping <device name>` or `ping <tailnet IP>` from a
      computer (and the page's round-trip time from the phone): ______ ms on
      4G, ______ ms at home
- [ ] **Heap with page + WebSocket open** (from the phone over the tailnet,
      press *Start* in WebSocket load test and leave it a few minutes):
      free heap ______, minimum free heap ______, largest free block ______,
      messages/s ______
- [ ] **Heap idle** (no page open, after an hour): free ______, minimum ______,
      largest block ______
- [ ] **Stays up for hours**: after ______ h, still `CONNECTED`? Reconnects
      ______, uptime ______ (an uptime reset means it rebooted: copy the serial
      log if you can)
- [ ] **Wi-Fi drop**: turn the router's Wi-Fi off for a minute, or reboot the
      router. Does the page come back over Tailscale by itself? After how
      long? ______ (Wi-Fi drops and rebinds counters go up by one)
- [ ] **Subnet route** (only if you typed one): approve it in the admin
      console (**Machines** → the board → **Edit route settings**), turn on
      *Use Tailscale subnets* on the phone, then open the camera's address on
      4G: works? ______ Same test with the phone on a Wi-Fi network that also
      uses 192.168.1.x: ______
- [ ] Anything odd in the serial log (errors, `AUTH_FAILED`, out of memory,
      reboots).

## Building it yourself

Needs [PlatformIO](https://platformio.org/). From this folder:

```sh
pio run -e esp32                       # app: .pio/build/esp32/firmware.bin
pio run -e esp32 -t upload -t monitor  # flash by USB and open the console
```

The merged image at offset 0x0 is made with esptool (shipped with
PlatformIO):

```sh
esptool.py --chip esp32 merge_bin -o bin/remote-access-proto-esp32-merged.bin \
  --flash_mode keep --flash_freq keep --flash_size keep \
  0x1000 .pio/build/esp32/bootloader.bin \
  0x8000 .pio/build/esp32/partitions.bin \
  0x10000 .pio/build/esp32/firmware.bin
```

Settings that matter for a board without PSRAM are in `sdkconfig.defaults`:
map-response buffers of 64 KB (`ML_H2_BUFFER_SIZE_KB`/`ML_JSON_BUFFER_SIZE_KB`,
enough for a tailnet of roughly 20 devices), 8 peers, DERP home region 18
(Paris), and a trimmed mbedTLS like microlink's size profile.

## Known limits (from microlink's README)

- IPv4 inside the tunnel only; the device does not use other nodes' subnet
  routes or exit nodes (it can only offer its own).
- No MagicDNS server or HTTPS certificates on the device: open the page by
  its tailnet IP. Your phone may resolve `<device name>` through MagicDNS.
- The DERP home region is fixed at build time (no latency-based choice).
- Tailscale's ACLs are enforced on the device; the default policy allows
  everything.
