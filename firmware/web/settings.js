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

async function start() {
  axisFields($('motion-axes'), MOTION);
  await I18n.load();
  const settings = await (await fetch('/api/settings')).json();
  fill(settings, MOTION);

  $('motion').addEventListener('submit', event => {
    event.preventDefault();
    save($('motion'), collect(MOTION), MOTION);
  });
}

start();
