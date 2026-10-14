// Wi-Fi setup page, served on the CatWandConnect-XXXX hotspot (docs/api.md),
// and opened from Settings → Network to change the robot's network.
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
  // Preselect the strongest network when nothing is chosen yet; when changing
  // network, the strongest one other than the current network.
  const first = items.map(item => item.firstChild.dataset.ssid).find(ssid => ssid !== previous);
  if (!$('ssid').value && first) pick(first, false);
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
// A change from the home network takes up to two attempts (the new network,
// then back to the previous one), hence the longer deadline.
const WATCH_LIMIT_MS = 60000;
const CHANGE_LIMIT_MS = 75000;
let watchUntil = 0;
let host = location.host;  // replaced by the robot's .local name once known
// Opened from the robot's current Wi-Fi rather than its hotspot (#174).
let changing = false;
let previous = '';  // the network the robot was on before a change
let target = '';    // the network asked for

function failed() {
  $('setup-help').hidden = false;
  $('connect').disabled = false;
}

async function watch() {
  let status = null;
  try {
    status = await (await fetch('/api/wifi/status', { cache: 'no-store' })).json();
  } catch {
    // The robot is busy trying the network, or the phone left the hotspot.
  }
  if (status && status.host) host = status.host;
  if (changing && status && status.mode === 'connected' && status.failedSsid) {
    say('setup.change.failed', { ssid: status.failedSsid, previous: status.ssid });
    failed();
  } else if (changing && status && status.mode === 'connected' && status.ssid === target) {
    say('setup.change.done', { ssid: status.ssid, host: status.host, ip: status.ip });
    $('back').hidden = false;
  } else if (changing && status && status.failedSsid) {
    say('setup.change.back', { ssid: status.failedSsid, previous });
    setTimeout(watch, 2000);
  } else if (!changing && status && status.mode === 'connected') {
    say('setup.connected', { ssid: status.ssid, host: status.host, ip: status.ip });
  } else if (!changing && status && status.failed) {
    say('setup.failed');
    failed();
  } else if (Date.now() > watchUntil) {
    say(changing ? 'setup.change.lost' : 'setup.lost', { host, ssid: target, previous });
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
  $('setup-help').hidden = true;
  target = $('ssid').value;
  say(changing ? 'setup.change.connecting' : 'setup.connecting', { ssid: target });
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
  watchUntil = Date.now() + (changing ? CHANGE_LIMIT_MS : WATCH_LIMIT_MS);
  setTimeout(watch, 2000);
});

function showMode(status) {
  host = status.host || host;
  changing = status.mode === 'connected' && status.hotspot === false;
  if (!changing) return;
  previous = status.ssid;
  $('intro').textContent = I18n.t('setup.change.intro');
  $('change-intro').textContent = I18n.t('setup.change.hint', { ssid: status.ssid });
  $('change-intro').hidden = false;
  $('skip-block').hidden = true;
  $('back').hidden = false;
}

Promise.all([I18n.load(), fetch('/api/wifi/status').then(r => r.json()).catch(() => ({}))])
  .then(([, status]) => { showMode(status); scan(); });
