// Wi-Fi setup page, served on the CatWandConnect-XXXX hotspot (docs/api.md).
const $ = id => document.getElementById(id);

// "Continue without Wi-Fi" holds for this tab: while the robot is in setup,
// "/" serves this page, so links back to "/" come here and move on.
const SKIP_KEY = 'cwc.skipSetup';
if (location.pathname === '/' && sessionStorage.getItem(SKIP_KEY)) location.replace('/index.html');
const say = (key, vars) => { $('status').textContent = I18n.t(key, vars); };

// Signal bars from RSSI: -55 dBm and above is excellent, -85 and below weak.
const bars = rssi => (rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -78 ? 2 : 1);

function note(key) {
  const item = document.createElement('li');
  item.className = 'networks-note';
  item.textContent = I18n.t(key);
  $('networks').replaceChildren(item);
}

function pick(ssid, focus = true) {
  $('ssid').value = ssid;
  for (const button of $('networks').querySelectorAll('button')) {
    button.setAttribute('aria-pressed', String(button.dataset.ssid === ssid));
  }
  if (focus) $('password').focus();
}

function show(networks) {
  const seen = new Set();
  const items = [];
  for (const network of networks.sort((a, b) => b.rssi - a.rssi)) {
    if (!network.ssid || seen.has(network.ssid)) continue;
    seen.add(network.ssid);
    const button = document.createElement('button');
    button.type = 'button';
    button.dataset.ssid = network.ssid;
    button.setAttribute('aria-pressed', String(network.ssid === $('ssid').value));
    const name = document.createElement('span');
    name.className = 'network-name';
    name.textContent = network.ssid;
    const signal = document.createElement('span');
    signal.className = 'network-signal';
    signal.textContent = `${network.secure ? '🔒 ' : ''}${'▂▄▆█'.slice(0, bars(network.rssi))}`;
    signal.title = `${network.rssi} dBm`;
    button.append(name, signal);
    button.addEventListener('click', () => pick(network.ssid));
    const item = document.createElement('li');
    item.append(button);
    items.push(item);
  }
  if (items.length === 0) return note('setup.none');
  $('networks').replaceChildren(...items);
  // Preselect the strongest network when nothing is chosen yet.
  if (!$('ssid').value) pick(items[0].firstChild.dataset.ssid, false);
}

async function scan() {
  $('rescan').disabled = true;
  note('setup.scanning');
  try {
    for (;;) {
      const result = await (await fetch('/api/wifi/networks')).json();
      if (!result.scanning) {
        show(result.networks);
        break;
      }
      await new Promise(resolve => setTimeout(resolve, 1500));
    }
  } catch {
    note('setup.scanFailed');
  }
  $('rescan').disabled = false;
}

$('rescan').addEventListener('click', scan);
$('skip').addEventListener('click', () => {
  sessionStorage.setItem(SKIP_KEY, '1');
  location.href = '/index.html';
});
// Coming here on purpose (Settings → Network) brings the setup back to "/".
if (location.pathname !== '/') sessionStorage.removeItem(SKIP_KEY);

// While the robot tries a network its radio leaves the hotspot's channel, so
// some status requests fail: keep asking until an answer or the deadline.
const WATCH_LIMIT_MS = 60000;
let watchUntil = 0;
let host = location.host;  // replaced by the robot's .local name once known

async function watch() {
  let status = null;
  try {
    status = await (await fetch('/api/wifi/status', { cache: 'no-store' })).json();
  } catch {
    // The robot is busy trying the network, or the phone left the hotspot.
  }
  if (status && status.host) host = status.host;
  if (status && status.mode === 'connected') {
    say('setup.connected', { ssid: status.ssid, host: status.host, ip: status.ip });
  } else if (status && status.failed) {
    say('setup.failed');
    $('setup-help').hidden = false;
    $('connect').disabled = false;
  } else if (Date.now() > watchUntil) {
    say('setup.lost', { host });
    // The phone may have left the hotspot: the robot's Help page is out of reach.
    $('setup-help-offline').hidden = false;
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
  watchUntil = Date.now() + WATCH_LIMIT_MS;
  setTimeout(watch, 2000);
});

I18n.load().then(scan);
fetch('/api/wifi/status').then(r => r.json()).then(status => { host = status.host || host; }).catch(() => {});
