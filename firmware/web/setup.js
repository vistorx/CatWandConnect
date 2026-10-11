// Wi-Fi setup page, served on the CatWandConnect-XXXX hotspot (docs/api.md).
const $ = id => document.getElementById(id);
const say = (key, vars) => { $('status').textContent = I18n.t(key, vars); };

async function scan() {
  const result = await (await fetch('/api/wifi/networks')).json();
  if (result.scanning) return setTimeout(scan, 1500);
  const seen = new Set();
  for (const network of result.networks.sort((a, b) => b.rssi - a.rssi)) {
    if (!network.ssid || seen.has(network.ssid)) continue;
    seen.add(network.ssid);
    $('networks').append(new Option(network.ssid));
  }
}

async function watch() {
  const status = await (await fetch('/api/wifi/status')).json();
  if (status.mode === 'connected') {
    say('setup.connected', { ssid: status.ssid, host: status.host, ip: status.ip });
  } else if (status.failed) {
    say('setup.failed');
    $('connect').disabled = false;
  } else {
    setTimeout(watch, 2000);
  }
}

$('form').addEventListener('submit', async event => {
  event.preventDefault();
  $('connect').disabled = true;
  say('setup.connecting');
  // The Wi-Fi form is form-encoded, so it handles the PIN itself.
  const post = () => fetch('/api/wifi', {
    method: 'POST',
    headers: Api.token() ? { Authorization: `Bearer ${Api.token()}` } : {},
    body: new URLSearchParams(new FormData($('form'))),
  });
  let response = await post();
  if (response.status === 401) {
    if (!(await Api.askPin())) {
      $('connect').disabled = false;
      $('status').textContent = '';
      return;
    }
    response = await post();
  }
  if (!response.ok) {
    $('status').textContent = await response.text();
    $('connect').disabled = false;
    return;
  }
  setTimeout(watch, 2000);
});

I18n.load();
scan();
