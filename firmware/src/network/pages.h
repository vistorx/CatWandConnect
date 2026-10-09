#pragma once

// Pages served before the Device interface exists. The setup page is shown in
// the hotspot; the home page is a placeholder until the Device interface (#5).

constexpr const char kHomePage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CatWandConnect</title>
<style>body{font-family:system-ui,sans-serif;margin:2rem auto;max-width:28rem;padding:0 1rem;color:#222;background:#fff}</style>
</head><body><h1>CatWandConnect</h1><p>Connected. The controls are coming soon.</p></body></html>
)html";

constexpr const char kSetupPage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CatWandConnect setup</title>
<style>
:root{--fg:#222;--bg:#fff;--muted:#666;--accent:#2b6cb0;--line:#ccc}
@media (prefers-color-scheme:dark){:root{--fg:#eee;--bg:#161616;--muted:#aaa;--accent:#63a4ff;--line:#444}}
body{font-family:system-ui,sans-serif;margin:2rem auto;max-width:28rem;padding:0 1rem;color:var(--fg);background:var(--bg)}
label{display:block;margin:1rem 0 .3rem}
input,button{width:100%;box-sizing:border-box;font-size:1rem;padding:.7rem;border-radius:.4rem;border:1px solid var(--line);background:var(--bg);color:var(--fg)}
button{margin-top:1.5rem;background:var(--accent);border:0;color:#fff;font-weight:600}
button:disabled{opacity:.5}
#status{margin-top:1.5rem;color:var(--muted)}
</style></head><body>
<h1>CatWandConnect</h1>
<p>Connect the robot to your Wi-Fi.</p>
<form id="form">
<label for="ssid">Network</label>
<input id="ssid" name="ssid" list="networks" autocomplete="off" required>
<datalist id="networks"></datalist>
<label for="password">Password</label>
<input id="password" name="password" type="password" autocomplete="off">
<button id="connect">Connect</button>
</form>
<p id="status"></p>
<script>
const $ = id => document.getElementById(id);
const say = text => { $('status').textContent = text; };

async function scan() {
  const r = await (await fetch('/api/wifi/networks')).json();
  if (r.scanning) return setTimeout(scan, 1500);
  const seen = new Set();
  for (const n of r.networks.sort((a, b) => b.rssi - a.rssi)) {
    if (!n.ssid || seen.has(n.ssid)) continue;
    seen.add(n.ssid);
    $('networks').append(new Option(n.ssid));
  }
}

async function watch() {
  const s = await (await fetch('/api/wifi/status')).json();
  if (s.mode === 'connected') {
    say(`Connected to ${s.ssid}. Rejoin your home Wi-Fi, then open http://${s.host}/ or http://${s.ip}/`);
  } else if (s.failed) {
    say('Could not connect. Check the network name and password.');
    $('connect').disabled = false;
  } else {
    setTimeout(watch, 2000);
  }
}

$('form').addEventListener('submit', async e => {
  e.preventDefault();
  $('connect').disabled = true;
  say('Connecting…');
  const r = await fetch('/api/wifi', { method: 'POST', body: new URLSearchParams(new FormData($('form'))) });
  if (!r.ok) { say(await r.text()); $('connect').disabled = false; return; }
  setTimeout(watch, 2000);
});

scan();
</script></body></html>
)html";
