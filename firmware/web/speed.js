// Speed tuning: sweeps one axis while the owner adjusts its minimum or maximum
// speed. Drives /api/speed-tuning and follows the robot over /ws (docs/api.md).
const $ = id => document.getElementById(id);
const AXES = ['horizontal', 'vertical'];
const BOUNDS = ['max', 'min'];

let settings = null;  // from /api/settings, for the current values
let tuning = { active: false };

function say(kind, text) {
  $('message').dataset.kind = kind;
  $('message').textContent = text;
}

async function call(action, extra = {}) {
  const { ok, data: result } = await Api.call('POST', '/api/speed-tuning', { action, ...extra });
  if (result.error === 'pin_required') return false;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    say('error', I18n.t(`error.${result.error}`, { axis }));
    return false;
  }
  say('', '');
  render(result);
  return true;
}

async function loadSettings() {
  try {
    settings = await (await fetch('/api/settings')).json();
  } catch { /* keep the last known values */ }
}

function current(axis, bound) {
  if (!settings) return '';
  const a = settings.axes[axis];
  return Math.round(bound === 'max' ? a.maxSpeed : a.minSpeed);
}

function renderChoices() {
  const calibrated = settings?.calibrated === true;
  $('not-calibrated').hidden = calibrated;
  $('choices').innerHTML = AXES.map(axis => `
    <fieldset>
      <legend>${I18n.t(`axis.${axis}`)}</legend>
      ${BOUNDS.map(bound => `
        <div class="field">
          <span>${I18n.t(`tuning.${bound}`)} <span class="unit">${current(axis, bound)} °/s</span></span>
          <button type="button" class="secondary" data-axis="${axis}" data-bound="${bound}"${calibrated ? '' : ' disabled'}>
            ${I18n.t('tuning.start')}</button>
        </div>`).join('')}
    </fieldset>`).join('');
}

function render(next) {
  const wasActive = tuning.active;
  tuning = next;
  $('choose').hidden = tuning.active;
  $('tune').hidden = !tuning.active;
  $('tune-actions').hidden = !tuning.active;
  if (tuning.active) {
    const axis = I18n.t(`axis.${tuning.axis}`);
    $('tune-title').textContent = I18n.t(`tuning.${tuning.bound}.title`, { axis });
    $('tune-text').textContent = I18n.t(`tuning.${tuning.bound}.text`);
    $('speed').textContent = Math.round(tuning.speed);
  } else if (wasActive) {
    loadSettings().then(renderChoices);
  }
}

$('choices').addEventListener('click', event => {
  const button = event.target.closest('button[data-axis]');
  if (button) call('start', { axis: button.dataset.axis, bound: button.dataset.bound });
});
$('faster').addEventListener('click', () => call('faster'));
$('slower').addEventListener('click', () => call('slower'));
$('keep').addEventListener('click', async () => {
  const speed = Math.round(tuning.speed);
  if (await call('keep')) say('ok', I18n.t('tuning.kept', { speed }));
});
$('cancel').addEventListener('click', () => call('cancel'));

// Stow or the joystick, from any phone or Home Assistant, end the tuning too.
function connect() {
  const socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onmessage = event => {
    const state = JSON.parse(event.data);
    if (state.t !== 'state') return;
    const next = state.activity === 'tuning_speed' && state.tuning ? state.tuning : { active: false };
    if (next.active) {
      const position = next.axis === 'horizontal' ? state.h : state.v;
      $('position').textContent = I18n.t('tuning.position', { degrees: Math.round(position) });
    }
    if (next.active !== tuning.active || next.speed !== tuning.speed) render(next);
  };
  socket.onclose = () => setTimeout(connect, 2000);
}

I18n.load().then(async () => {
  await loadSettings();
  renderChoices();
  try {
    render(await (await fetch('/api/speed-tuning')).json());
  } catch { /* the state over /ws will tell */ }
  connect();
});
