// Settings page: reads and writes /api/settings (docs/api.md).
const $ = id => document.getElementById(id);
const AXES = ['horizontal', 'vertical'];

// One fieldset per Axis, built from a list of fields.
function axisFields(container, fields) {
  for (const axis of AXES) {
    const set = document.createElement('fieldset');
    set.innerHTML = `<legend data-i18n="axis.${axis}"></legend>` + fields.map(f => `
      <div class="field">
        <label for="${axis}-${f.key}"><span data-i18n="settings.${f.key}"></span>
          <span class="unit">${f.unit}</span></label>
        <input id="${axis}-${f.key}" type="number" inputmode="decimal" step="${f.step}" min="${f.min}" max="${f.max}" required>
      </div>`).join('');
    container.append(set);
  }
}

function fill(settings, fields) {
  for (const axis of AXES) {
    for (const f of fields) $(`${axis}-${f.key}`).value = settings.axes[axis][f.key];
  }
}

function collect(fields) {
  const axes = {};
  for (const axis of AXES) {
    axes[axis] = {};
    for (const f of fields) axes[axis][f.key] = Number($(`${axis}-${f.key}`).value);
  }
  return { axes };
}

function report(form, kind, text) {
  const message = form.querySelector('.message');
  message.dataset.kind = kind;
  message.textContent = text;
}

async function save(form, body, fields) {
  const { ok, data: result } = await Api.call('PUT', '/api/settings', body);
  if (result.error === 'pin_required') return;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    report(form, 'error', I18n.t(`error.${result.error}`, { axis }));
    return;
  }
  fill(result, fields);
  report(form, 'ok', I18n.t('settings.saved'));
}

const MOTION = [
  { key: 'maxSpeed', unit: '°/s', step: 1, min: 10, max: 360 },
  { key: 'minSpeed', unit: '°/s', step: 1, min: 1, max: 359 },
  { key: 'maxAccel', unit: '°/s²', step: 10, min: 20, max: 2000 },
];

const LIMITS = [
  { key: 'min', unit: '°', step: 0.5, min: 0, max: 180 },
  { key: 'max', unit: '°', step: 0.5, min: 0, max: 180 },
  { key: 'stow', unit: '°', step: 0.5, min: 0, max: 180 },
];

// --- Wiring: GPIO, direction and pulse range (/api/hardware) -------------------

// Factory pulse ranges, as in defaultHardware() (lib/robot_core/.../hardware.cpp).
const DEFAULT_PULSES = { horizontal: { min: 400, max: 2200 }, vertical: { min: 600, max: 2400 } };

function hardwareFields(hardware) {
  const options = hardware.allowedPins.map(pin => `<option value="${pin}">GPIO ${pin}</option>`).join('');
  $('hardware-axes').innerHTML = AXES.map(axis => `
    <fieldset>
      <legend>${I18n.t(`axis.${axis}`)}</legend>
      <div class="field">
        <label for="${axis}-pin">${I18n.t('settings.pin')}</label>
        <select id="${axis}-pin">${options}</select>
      </div>
      <div class="field">
        <label for="${axis}-inverted">${I18n.t('settings.inverted')}</label>
        <input id="${axis}-inverted" type="checkbox">
      </div>
      <details class="advanced">
        <summary>${I18n.t('settings.pulse.title')}</summary>
        <p class="hint">${I18n.t('settings.pulse.hint')}</p>
        <div class="field">
          <label for="${axis}-minPulse">${I18n.t('settings.minPulse')} <span class="unit">µs</span></label>
          <input id="${axis}-minPulse" type="number" step="10" min="300" max="2700" required>
        </div>
        <div class="field">
          <label for="${axis}-maxPulse">${I18n.t('settings.maxPulse')} <span class="unit">µs</span></label>
          <input id="${axis}-maxPulse" type="number" step="10" min="300" max="2700" required>
        </div>
        <p class="hint">
          ${I18n.t('settings.pulse.default', DEFAULT_PULSES[axis])}
          <button type="button" class="text-button" data-reset-pulse="${axis}">${I18n.t('settings.pulse.reset')}</button>
        </p>
      </details>
    </fieldset>`).join('');
  // A wrong value in a collapsed section must be visible to be fixed.
  $('hardware-axes').querySelectorAll('details input').forEach(input => {
    input.addEventListener('invalid', () => { input.closest('details').open = true; });
  });
  $('hardware-axes').querySelectorAll('[data-reset-pulse]').forEach(button => {
    button.addEventListener('click', () => {
      const axis = button.dataset.resetPulse;
      $(`${axis}-minPulse`).value = DEFAULT_PULSES[axis].min;
      $(`${axis}-maxPulse`).value = DEFAULT_PULSES[axis].max;
    });
  });
  for (const axis of AXES) {
    const a = hardware.axes[axis];
    $(`${axis}-pin`).value = a.pin;
    $(`${axis}-inverted`).checked = a.inverted;
    $(`${axis}-minPulse`).value = a.minPulse;
    $(`${axis}-maxPulse`).value = a.maxPulse;
  }
}

async function saveHardware(form) {
  const axes = {};
  for (const axis of AXES) {
    axes[axis] = {
      pin: Number($(`${axis}-pin`).value),
      inverted: $(`${axis}-inverted`).checked,
      minPulse: Number($(`${axis}-minPulse`).value),
      maxPulse: Number($(`${axis}-maxPulse`).value),
    };
  }
  const { ok, data: result } = await Api.call('PUT', '/api/hardware', { axes });
  if (result.error === 'pin_required') return;
  if (!ok) {
    report(form, 'error', I18n.t(`error.${result.error}`, { axis: result.axis ? I18n.t(`axis.${result.axis}`) : '' }));
    return;
  }
  report(form, 'ok', I18n.t(result.calibrationReset ? 'settings.restarting_recalibrate' : 'settings.restarting'));
  setTimeout(() => location.reload(), 8000);
}

// --- Robot name and interface language ----------------------------------------

// Mirrors the firmware's hostname rule (lib/identity) for the live preview.
function hostnameFor(name) {
  const host = name.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase()
    .replace(/[^a-z0-9]+/g, '-').replace(/^-+/, '').slice(0, 32).replace(/-+$/, '');
  return host || 'catwandconnect';
}

function deviceSection(info) {
  $('name').value = info.name;
  const preview = () => { $('address').textContent = `${hostnameFor($('name').value)}.local`; };
  $('name').addEventListener('input', preview);
  preview();

  const select = $('language');
  select.append(new Option(I18n.t('settings.language.auto'), 'auto'));
  for (const [code, label] of Object.entries(I18n.LANGUAGES)) select.append(new Option(label, code));
  select.value = I18n.preference();
  // The language is a per-browser choice: apply it at once, no restart.
  select.addEventListener('change', () => {
    I18n.setPreference(select.value);
    location.reload();
  });

  $('device').addEventListener('submit', async event => {
    event.preventDefault();
    const { ok, data: result } = await Api.call('PUT', '/api/device', { name: $('name').value.trim() });
    if (result.error === 'pin_required') return;
    if (!ok) return report($('device'), 'error', I18n.t(`error.${result.error}`));
    report($('device'), 'ok', I18n.t('settings.name_saved', { host: result.hostname }));
    setTimeout(() => { location.href = `http://${result.hostname}/settings.html`; }, 8000);
  });
}

// --- Home Assistant over MQTT (/api/mqtt) ------------------------------------------

function showMqtt(mqtt) {
  $('mqtt-enabled').checked = mqtt.enabled;
  $('mqtt-host').value = mqtt.host;
  $('mqtt-port').value = mqtt.port;
  $('mqtt-user').value = mqtt.user;
  $('mqtt-password').value = '';
  $('mqtt-password').placeholder = mqtt.passwordSet ? I18n.t('mqtt.password_kept') : '';
  $('mqtt-status').textContent = mqttStatus(mqtt);
}

function mqttStatus(mqtt) {
  if (!mqtt.enabled) return I18n.t('mqtt.off');
  if (mqtt.connected) return I18n.t('mqtt.connected');
  return mqtt.error ? I18n.t(`mqtt.error.${mqtt.error}`) : I18n.t('mqtt.connecting');
}

async function mqttSection() {
  showMqtt((await Api.call('GET', '/api/mqtt')).data);
  // Follow the connection for a while after a change.
  setInterval(async () => {
    if (document.activeElement?.closest('#mqtt')) return;  // don't overwrite what is being typed
    const { data } = await Api.call('GET', '/api/mqtt');
    $('mqtt-status').textContent = mqttStatus(data);
  }, 3000);

  $('mqtt').addEventListener('submit', async event => {
    event.preventDefault();
    const body = {
      enabled: $('mqtt-enabled').checked,
      host: $('mqtt-host').value.trim(),
      port: Number($('mqtt-port').value) || 1883,
      user: $('mqtt-user').value.trim(),
    };
    if ($('mqtt-password').value) body.password = $('mqtt-password').value;
    if (!body.user) body.password = '';
    const { ok, data } = await Api.call('PUT', '/api/mqtt', body);
    if (data.error === 'pin_required') return;
    if (!ok) return report($('mqtt'), 'error', I18n.t(`error.${data.error}`));
    showMqtt(data);
    report($('mqtt'), 'ok', I18n.t('settings.saved'));
  });
}

// --- Settings PIN ---------------------------------------------------------------

async function securitySection() {
  const state = (await Api.call('GET', '/api/auth')).data;
  $('pin-status').textContent = I18n.t(state.enabled ? 'security.on' : 'security.off');
  $('pin-remove').hidden = !state.enabled;
}

async function savePin(pin) {
  const { ok, data } = await Api.call('PUT', '/api/auth/pin', { pin });
  if (data.error === 'pin_required') return;
  if (!ok) return report($('security'), 'error', I18n.t(`error.${data.error}`));
  Api.setToken(data.token || '');
  $('pin').value = '';
  report($('security'), 'ok', I18n.t(data.enabled ? 'security.saved' : 'security.removed'));
  securitySection();
}

// --- Firmware update (/api/update) -------------------------------------------------

// XHR rather than fetch, for upload progress.
function uploadFirmware(file) {
  return new Promise(resolve => {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/update');
    if (Api.token()) xhr.setRequestHeader('Authorization', `Bearer ${Api.token()}`);
    xhr.upload.onprogress = event => {
      if (event.lengthComputable) $('firmware-progress').value = Math.round((event.loaded / event.total) * 100);
    };
    xhr.onload = () => {
      let data = {};
      try { data = JSON.parse(xhr.responseText); } catch { /* not JSON */ }
      resolve({ status: xhr.status, data });
    };
    xhr.onerror = () => resolve({ status: 0, data: { error: 'upload_interrupted' } });
    const form = new FormData();
    form.append('firmware', file, file.name);
    xhr.send(form);
  });
}

async function updateFirmware() {
  const form = $('firmware');
  const file = $('firmware-file').files[0];
  if (!file) return;
  const button = form.querySelector('button');
  button.disabled = true;
  $('firmware-progress').hidden = false;
  $('firmware-progress').value = 0;
  report(form, '', I18n.t('firmware.uploading'));

  let result = await uploadFirmware(file);
  if (result.status === 401 && (await Api.askPin())) result = await uploadFirmware(file);

  button.disabled = false;
  if (result.status === 200) {
    report(form, 'ok', I18n.t('firmware.done'));
    setTimeout(() => location.reload(), 15000);
  } else if (result.data.error !== 'pin_required') {
    $('firmware-progress').hidden = true;
    report(form, 'error', I18n.t(`error.${result.data.error || 'update_failed'}`));
    form.querySelector('.message').insertAdjacentHTML('beforeend', ` <a href="help.html">${I18n.t('help.stuck')}</a>`);
  } else {
    $('firmware-progress').hidden = true;
    report(form, '', '');
  }
}

// --- Wi-Fi: state, and the setup page while the hotspot is up -------------------

async function wifiSection() {
  try {
    const status = await (await fetch('/api/wifi/status')).json();
    const connected = status.mode === 'connected';
    $('wifi-state').textContent = connected
      ? I18n.t('settings.wifi.connected', { ssid: status.ssid })
      : I18n.t('settings.wifi.hotspot');
    $('wifi-setup').hidden = connected;
    // Phones open hotspot pages in a sign-in window that cannot pick files.
    $('firmware-hotspot').hidden = connected;
    $('firmware-address').textContent = `http://${location.host}/settings.html#robot`;
  } catch {
    $('wifi-state').textContent = '';
  }
}

$('firmware-copy').addEventListener('click', async () => {
  const text = $('firmware-address').textContent;
  try {
    await navigator.clipboard.writeText(text);
  } catch {
    // No clipboard in sign-in windows or over http: select the text instead.
    getSelection().selectAllChildren($('firmware-address'));
  }
  $('firmware-copy').textContent = I18n.t('firmware.copied');
});

// --- Tabs: one entry per tab, the open one is in the URL hash -----------------

const TABS = [
  { id: 'motion', label: 'settings.tab.motion' },
  { id: 'hardware', label: 'settings.tab.hardware' },
  { id: 'network', label: 'settings.tab.network' },
  { id: 'robot', label: 'settings.tab.robot' },
];

function showTab(id, focus = false) {
  const tab = TABS.find(t => t.id === id) || TABS[0];
  for (const t of TABS) {
    const button = $(`tab-button-${t.id}`);
    const selected = t === tab;
    button.setAttribute('aria-selected', String(selected));
    button.tabIndex = selected ? 0 : -1;
    $(`tab-${t.id}`).hidden = !selected;
  }
  if (focus) $(`tab-button-${tab.id}`).focus();
  // Some tab ids are also form ids (#motion, #hardware): don't let the hash
  // scroll the page to them, start each tab at the top.
  window.scrollTo(0, 0);
  if (location.hash.slice(1) !== tab.id) history.replaceState(null, '', `#${tab.id}`);
}

function tabs() {
  const bar = $('tabs');
  bar.innerHTML = TABS.map(t => `
    <button type="button" role="tab" id="tab-button-${t.id}" aria-controls="tab-${t.id}">${I18n.t(t.label)}</button>`).join('');
  for (const t of TABS) {
    $(`tab-${t.id}`).setAttribute('aria-labelledby', `tab-button-${t.id}`);
    $(`tab-button-${t.id}`).addEventListener('click', () => showTab(t.id));
  }
  bar.addEventListener('keydown', event => {
    const step = { ArrowRight: 1, ArrowLeft: -1 }[event.key];
    if (!step) return;
    const current = TABS.findIndex(t => $(`tab-button-${t.id}`).getAttribute('aria-selected') === 'true');
    showTab(TABS[(current + step + TABS.length) % TABS.length].id, true);
    event.preventDefault();
  });
  window.addEventListener('hashchange', () => showTab(location.hash.slice(1)));
  showTab(location.hash.slice(1));
  requestAnimationFrame(() => window.scrollTo(0, 0));  // after the browser's own jump to the hash
}

async function start() {
  axisFields($('limits-axes'), LIMITS);
  axisFields($('motion-axes'), MOTION);
  await I18n.load();
  tabs();
  const info = await (await fetch('/api/info')).json();
  deviceSection(info);
  wifiSection();
  $('firmware-version').textContent = info.version;
  $('firmware').addEventListener('submit', event => {
    event.preventDefault();
    updateFirmware();
  });
  const settings = await (await fetch('/api/settings')).json();
  fill(settings, LIMITS);
  fill(settings, MOTION);

  await mqttSection();
  await securitySection();
  $('security').addEventListener('submit', event => {
    event.preventDefault();
    savePin($('pin').value);
  });
  $('pin-remove').addEventListener('click', () => savePin(''));

  hardwareFields(await (await fetch('/api/hardware')).json());
  $('hardware').addEventListener('submit', event => {
    event.preventDefault();
    saveHardware($('hardware'));
  });

  for (const [form, fields] of [[$('limits'), LIMITS], [$('motion'), MOTION]]) {
    form.addEventListener('submit', event => {
      event.preventDefault();
      save(form, collect(fields), fields);
    });
  }
}

start();
