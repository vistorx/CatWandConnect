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
  const response = await fetch('/api/settings', {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
  const result = await response.json();
  if (!response.ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    report(form, 'error', I18n.t(`error.${result.error}`, { axis }));
    return;
  }
  fill(result, fields);
  report(form, 'ok', I18n.t('settings.saved'));
}

const MOTION = [
  { key: 'maxSpeed', unit: '°/s', step: 1, min: 10, max: 360 },
  { key: 'maxAccel', unit: '°/s²', step: 10, min: 20, max: 2000 },
];

const LIMITS = [
  { key: 'min', unit: '°', step: 0.5, min: 0, max: 180 },
  { key: 'max', unit: '°', step: 0.5, min: 0, max: 180 },
  { key: 'stow', unit: '°', step: 0.5, min: 0, max: 180 },
];

// --- Wiring: GPIO, direction and pulse range (/api/hardware) -------------------

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
      <div class="field">
        <label for="${axis}-minPulse">${I18n.t('settings.minPulse')} <span class="unit">µs</span></label>
        <input id="${axis}-minPulse" type="number" step="10" min="300" max="2700" required>
      </div>
      <div class="field">
        <label for="${axis}-maxPulse">${I18n.t('settings.maxPulse')} <span class="unit">µs</span></label>
        <input id="${axis}-maxPulse" type="number" step="10" min="300" max="2700" required>
      </div>
    </fieldset>`).join('');
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
  const response = await fetch('/api/hardware', {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ axes }),
  });
  const result = await response.json();
  if (!response.ok) {
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
    const response = await fetch('/api/device', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name: $('name').value.trim() }),
    });
    const result = await response.json();
    if (!response.ok) return report($('device'), 'error', I18n.t(`error.${result.error}`));
    report($('device'), 'ok', I18n.t('settings.name_saved', { host: result.hostname }));
    setTimeout(() => { location.href = `http://${result.hostname}/settings.html`; }, 8000);
  });
}

async function start() {
  axisFields($('limits-axes'), LIMITS);
  axisFields($('motion-axes'), MOTION);
  await I18n.load();
  deviceSection(await (await fetch('/api/info')).json());
  const settings = await (await fetch('/api/settings')).json();
  fill(settings, LIMITS);
  fill(settings, MOTION);

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
