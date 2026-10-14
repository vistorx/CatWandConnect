# A Wi-Fi change made from the home network falls back to that network

Owners asked to change the robot's Wi-Fi from Settings while it is on their network (#174). Until then, `POST /api/wifi` was refused unless the hotspot was up, and any failed attempt opened the `CatWandConnect-XXXX` hotspot.

We now accept `POST /api/wifi` from the current network too (behind the settings PIN, like before), reusing the setup page. A failed attempt then depends on where it was made (`lib/wifi_fallback`, tested natively):

- **From the home network** (setup page or Improv, a network saved): the robot tries the saved network again at once. The owner finds it where it was, at the same address, without joining a hotspot. If that rejoin fails too, the hotspot opens as before.
- **From the hotspot, with nothing saved, or the saved network itself failing:** the hotspot opens (or stays), as before.

The saved network is replaced only after the new one connects, so a wrong password never loses the working credentials. `GET /api/wifi/status` reports `hotspot` and `failedSsid` so the page can tell "back on the previous network" from "connected to the new one".

We rejected a dedicated "change network" page: the setup page already scans and joins, and has the PIN prompt. We rejected going straight to the hotspot on failure, which forces the owner to find a `CatWandConnect-XXXX` network to recover from a typo.
