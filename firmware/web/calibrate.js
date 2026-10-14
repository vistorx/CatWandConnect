// Calibration wizard: drives /api/calibration, then Speed tuning and
// Acceleration tuning (/api/speed-tuning) once the limits are saved, and follows the robot over
// /ws (docs/api.md).
const $ = id => document.getElementById(id);

// Which axes each step can nudge, and its number in the progress line.
const STEPS = {
  none: { axes: [], number: 0 },
  horizontal_min: { axes: ['horizontal'], number: 1 },
  horizontal_max: { axes: ['horizontal'], number: 2 },
  vertical_min: { axes: ['vertical'], number: 3 },
  vertical_max: { axes: ['vertical'], number: 4 },
  stow: { axes: ['horizontal', 'vertical'], number: 5 },
  sweep: { axes: [], number: 6 },
  confirm: { axes: [], number: 6 },
};
const NUDGES = [-5, -1, 1, 5];
// Speed tuning steps, after the limits are saved, numbered after the confirm
// step: minimum speed, maximum speed, then acceleration, for each axis.
const SPEED_STEPS = [
  { axis: 'horizontal', bound: 'min' },
  { axis: 'horizontal', bound: 'max' },
  { axis: 'horizontal', target: 'accel' },
  { axis: 'vertical', bound: 'min' },
  { axis: 'vertical', bound: 'max' },
  { axis: 'vertical', target: 'accel' },
];
const isAccel = info => info.target === 'accel';
// The tuning the robot runs is this step's.
const runs = (t, info) => t.active && t.axis === info.axis &&
  (isAccel(info) ? t.target === 'accel' : t.target !== 'accel' && t.bound === info.bound);
const TOTAL = 6 + SPEED_STEPS.length;
// Steps where the axis can be reversed (minimum and maximum of each axis).
const REVERSIBLE = ['horizontal_min', 'horizontal_max', 'vertical_min', 'vertical_max'];

let step = null;
let position = { horizontal: null, vertical: null };
let draft = null;  // values recorded so far, from /api/calibration
let calibrated = false;  // a Calibration is already saved (from the state over /ws)
let inverted = { horizontal: false, vertical: false };  // wiring, from /api/hardware

// Speed tuning part of the wizard. Page-only: the limits are already saved,
// so leaving keeps a valid Calibration and the speeds kept so far.
let speedIndex = null;  // index in SPEED_STEPS, SPEED_STEPS.length = Done, null = limits part
let speedStarted = false;  // the sweep of this step was started once
let activity = 'idle';  // from the state over /ws
let tuning = { active: false };  // from /api/speed-tuning and the state over /ws
let settings = null;  // current speeds, from /api/settings

// Steps already marked in this Calibration, before `step`.
const ORDER = ['horizontal_min', 'horizontal_max', 'vertical_min', 'vertical_max', 'stow'];

async function call(action, extra = {}) {
  const { ok, data: result } = await Api.call('POST', '/api/calibration', { action, ...extra });
  if (result.error === 'pin_required') return false;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    if (result.error === 'travel_order' && draft && result.axis) {
      say('error', I18n.t('error.travel_order_at', { axis, min: Math.round(draft[result.axis].min) }));
    } else {
      say('error', I18n.t(`error.${result.error}`, { axis }));
    }
    return false;
  }
  say('', '');
  if (result.draft) draft = result.draft;
  if (action === 'confirm') {
    await startSpeeds();
    return true;
  }
  show(result.step);
  renderMarked();
  return true;
}

// --- Speed tuning steps -----------------------------------------------------

async function tune(action, extra = {}) {
  const { ok, data: result } = await Api.call('POST', '/api/speed-tuning', { action, ...extra });
  if (result.error === 'pin_required') return false;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    say('error', I18n.t(`error.${result.error}`, { axis }));
    return false;
  }
  tuning = result;
  return true;
}

async function loadSettings() {
  try {
    settings = await (await fetch('/api/settings')).json();
  } catch { /* keep the last known values */ }
}

// The value this step tunes, as saved: a speed, or the acceleration.
function currentSpeed(info) {
  if (!settings) return '';
  const a = settings.axes[info.axis];
  return Math.round(isAccel(info) ? a.maxAccel : info.bound === 'max' ? a.maxSpeed : a.minSpeed);
}

// The limits are saved and the robot Stows: on to the speeds.
async function startSpeeds() {
  step = null;
  activity = 'stowing';
  $('marked').hidden = true;
  say('ok', I18n.t('calibration.done'));
  await goSpeed(0);
}

async function goSpeed(index) {
  speedIndex = index;
  speedStarted = false;
  renderSpeed();
  await loadSettings();
  renderSpeed();
  maybeStartSweep();
}

// Starts this step's sweep once the robot is free (it Stows after each step).
async function maybeStartSweep() {
  const info = SPEED_STEPS[speedIndex];
  if (!info || speedStarted || tuning.active || !['idle', 'manual'].includes(activity)) return;
  speedStarted = true;
  if (await tune('start', info)) renderSpeed();
  else speedStarted = false;
}

// Keep and Skip Stow the robot; the next sweep starts once it rests.
function nextSpeed() {
  activity = 'stowing';
  return goSpeed(speedIndex + 1);
}

function renderSpeed() {
  const info = SPEED_STEPS[speedIndex];
  const primary = $('primary');
  const secondary = $('secondary');
  $('cancel').hidden = true;
  $('nudges').innerHTML = '';
  primary.hidden = false;
  primary.disabled = false;
  secondary.hidden = true;

  if (!info) {  // Done
    $('speed').hidden = true;
    $('progress').textContent = '';
    $('step-title').textContent = I18n.t('calibration.finished.title');
    $('step-text').textContent = I18n.t('calibration.finished.text');
    renderSpeeds();
    primary.textContent = I18n.t('calibration.finished.button');
    primary.onclick = () => { location.href = '/'; };
    return;
  }

  const axis = I18n.t(`axis.${info.axis}`);
  const accel = isAccel(info);
  const kind = accel ? 'accel' : info.bound;
  const part = accel ? 'accel' : 'speed';  // wording of the calibration.* keys
  const active = runs(tuning, info);
  $('progress').textContent = I18n.t('calibration.progress', { n: 7 + speedIndex, total: TOTAL });
  $('step-title').textContent = I18n.t(`tuning.${kind}.title`, { axis });
  $('step-text').textContent = I18n.t(`tuning.${kind}.text`);
  $('speed').hidden = false;
  $('speed-value').textContent = active ? Math.round(accel ? tuning.accel : tuning.speed) : currentSpeed(info);
  $('speed-unit').textContent = accel ? '°/s²' : '°/s';
  $('slower').textContent = I18n.t(accel ? 'tuning.softer' : 'tuning.slower');
  $('faster').textContent = I18n.t(accel ? 'tuning.snappier' : 'tuning.faster');
  $('speed-buttons').hidden = !active;
  if (!active) {
    $('speed-position').textContent = I18n.t(speedStarted ? `calibration.${part}.stopped` : `calibration.${part}.waiting`);
  }

  if (active) {
    primary.textContent = I18n.t(accel ? 'tuning.keep_accel' : 'tuning.keep');
    primary.onclick = async () => {
      const value = Math.round(accel ? tuning.accel : tuning.speed);
      primary.disabled = true;
      if (await tune('keep')) {
        say('ok', I18n.t(accel ? 'tuning.kept_accel' : 'tuning.kept', { speed: value, accel: value }));
        await nextSpeed();
      } else {
        primary.disabled = false;
      }
    };
  } else {
    primary.textContent = I18n.t(`calibration.${part}.start`);
    primary.disabled = !speedStarted || !['idle', 'manual'].includes(activity);
    primary.onclick = () => { speedStarted = false; maybeStartSweep(); };
  }
  secondary.hidden = false;
  secondary.textContent = I18n.t(`calibration.${part}.skip`, { speed: currentSpeed(info), accel: currentSpeed(info) });
  secondary.onclick = async () => {
    if (tuning.active) await tune('cancel');
    say('', '');
    await nextSpeed();
  };
}

// The speeds and acceleration the robot now uses, on the Done step.
function renderSpeeds() {
  if (!settings) return;
  const items = ['horizontal', 'vertical'].map(axis => {
    const a = settings.axes[axis];
    return `<li><strong>${I18n.t(`axis.${axis}`)}</strong> ${
      I18n.t('calibration.speed_range', { min: Math.round(a.minSpeed), max: Math.round(a.maxSpeed) })}, ${
      I18n.t('calibration.accel_value', { accel: Math.round(a.maxAccel) })}</li>`;
  });
  $('marked').innerHTML = `<h3>${I18n.t('calibration.speeds')}</h3><ul>${items.join('')}</ul>`;
  $('marked').hidden = false;
}

$('faster').addEventListener('click', async () => { if (await tune('faster')) renderSpeed(); });
$('slower').addEventListener('click', async () => { if (await tune('slower')) renderSpeed(); });
// Leaving during a sweep stops it: the speed keeps its previous value.
$('back').addEventListener('click', async event => {
  if (speedIndex === null || !tuning.active) return;
  event.preventDefault();
  await tune('cancel');
  location.href = '/';
});

// Recalls the values marked at earlier steps, and the limits on the Stow step.
function renderMarked() {
  const box = $('marked');
  const done = ORDER.slice(0, Math.max(0, ORDER.indexOf(step)));
  if (!draft || !done.length) {
    box.hidden = true;
    return;
  }
  const items = [];
  for (const axis of ['horizontal', 'vertical']) {
    const min = done.includes(`${axis}_min`);
    const max = done.includes(`${axis}_max`);
    if (!min && !max) continue;
    const value = min && max
      ? I18n.t('calibration.range', { min: Math.round(draft[axis].min), max: Math.round(draft[axis].max) })
      : `${I18n.t(min ? 'calibration.min' : 'calibration.max')} ${Math.round(draft[axis][min ? 'min' : 'max'])}°`;
    items.push(`<li><strong>${I18n.t(`axis.${axis}`)}</strong> ${value}</li>`);
  }
  box.innerHTML = `<h3>${I18n.t('calibration.marked')}</h3><ul>${items.join('')}</ul>`;
  box.hidden = false;
}

function say(kind, text) {
  $('message').dataset.kind = kind;
  $('message').textContent = text;
}

function show(next) {
  if (next === step) return;
  step = next;
  const info = STEPS[step] || STEPS.none;
  $('speed').hidden = true;

  $('progress').textContent = info.number ? I18n.t('calibration.progress', { n: info.number, total: TOTAL }) : '';
  $('step-title').textContent = I18n.t(`calibration.${step}.title`);
  $('step-text').textContent = I18n.t(`calibration.${step}.text`);

  $('nudges').innerHTML = info.axes.map(axis => `
    <fieldset>
      <legend>${I18n.t(`axis.${axis}`)} <output id="pos-${axis}"></output></legend>
      ${step === 'stow' && draft ? `<p class="hint">${I18n.t('calibration.limits', { min: Math.round(draft[axis].min), max: Math.round(draft[axis].max) })}</p>` : ''}
      <div class="nudge-row">
        ${NUDGES.map(d => `<button type="button" class="secondary" data-axis="${axis}" data-degrees="${d}">${d > 0 ? '+' : '−'}${Math.abs(d)}°</button>`).join('')}
      </div>
      ${REVERSIBLE.includes(step) ? `<div class="field">
        <label for="reverse-${axis}">${I18n.t('calibration.reverse.button')}</label>
        <input id="reverse-${axis}" type="checkbox" data-reverse="${axis}"${inverted[axis] ? ' checked' : ''}>
      </div>
      <p class="hint">${I18n.t(`calibration.reverse.${axis}`)}</p>` : ''}
    </fieldset>`).join('');
  renderPositions();

  const primary = $('primary');
  const secondary = $('secondary');
  primary.hidden = false;
  primary.disabled = false;
  secondary.hidden = true;
  $('cancel').hidden = step === 'none';
  $('cancel').textContent = I18n.t(calibrated ? 'calibration.cancel_new' : 'calibration.cancel');

  if (step === 'none') {
    primary.textContent = I18n.t(calibrated ? 'calibration.recalibrate' : 'calibration.start');
    primary.onclick = () => call('start');
  } else if (step === 'sweep') {
    primary.textContent = I18n.t('calibration.sweeping');
    primary.disabled = true;
  } else if (step === 'confirm') {
    primary.textContent = I18n.t('calibration.confirm');
    primary.onclick = () => call('confirm');
    secondary.hidden = false;
    secondary.textContent = I18n.t('calibration.restart');
    secondary.onclick = async () => {
      await call('cancel');
      await call('start');
    };
  } else {
    primary.textContent = I18n.t(step === 'stow' ? 'calibration.mark_stow' : 'calibration.mark');
    primary.onclick = () => call('mark');
  }
}

function renderPositions() {
  for (const axis of ['horizontal', 'vertical']) {
    const output = $(`pos-${axis}`);
    if (output && position[axis] !== null) output.textContent = `${Math.round(position[axis])}°`;
  }
}

// Ticking or unticking "Reverse this axis" reverses it in place (the arm stays
// put); the box then shows the wiring the robot reports.
$('nudges').addEventListener('change', async event => {
  const box = event.target.closest('input[data-reverse]');
  if (!box) return;
  const axis = box.dataset.reverse;
  box.disabled = true;
  await call('reverse', { axis });
  await loadWiring();
  const current = $(`reverse-${axis}`);
  if (current) {
    current.checked = inverted[axis];
    current.disabled = false;
  }
  if ($('message').dataset.kind !== 'error') say('ok', I18n.t('calibration.reverse.done'));
});

async function loadWiring() {
  try {
    const hardware = await (await fetch('/api/hardware')).json();
    for (const axis of ['horizontal', 'vertical']) inverted[axis] = hardware.axes[axis].inverted === true;
  } catch { /* keep the last known wiring */ }
}

$('nudges').addEventListener('click', event => {
  const button = event.target.closest('button[data-axis]');
  if (button) call('nudge', { axis: button.dataset.axis, degrees: Number(button.dataset.degrees) });
});
// With a Calibration already saved, cancelling the new one goes back home.
$('cancel').addEventListener('click', async () => {
  const leave = calibrated;
  await call('cancel');
  if (leave) location.href = '/';
});

// During the speed steps: follow the sweep, and notice when Stow, the joystick
// or another phone ended it.
function followSpeed(state) {
  calibrated = state.calibrated;
  const next = state.activity === 'tuning_speed' && state.tuning ? state.tuning : { active: false };
  const info = SPEED_STEPS[speedIndex];
  if (info && runs(next, info)) {
    const degrees = Math.round(next.axis === 'horizontal' ? state.h : state.v);
    $('speed-position').textContent = I18n.t('tuning.position', { degrees });
  }
  const changed = next.active !== tuning.active || next.speed !== tuning.speed || next.accel !== tuning.accel ||
    next.target !== tuning.target;
  tuning = next;
  if (speedIndex < SPEED_STEPS.length) {
    if (changed || !next.active) renderSpeed();
    maybeStartSweep();
  }
}

function connect() {
  const socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onmessage = event => {
    const state = JSON.parse(event.data);
    if (state.t !== 'state') return;
    position = { horizontal: state.h, vertical: state.v };
    activity = state.activity;
    if (speedIndex !== null && state.calibration === 'none') {
      followSpeed(state);
      return;
    }
    speedIndex = null;  // a new Calibration started, from here or another phone
    if (state.calibrated !== calibrated) {
      calibrated = state.calibrated;
      const current = step;
      step = null;  // redraw the buttons with the new wording
      show(current);
    }
    renderPositions();
    if (state.calibration !== step) {
      show(state.calibration);
      fetch('/api/calibration').then(r => r.json()).then(c => { draft = c.draft; renderMarked(); }).catch(() => {});
    }
  };
  socket.onclose = () => setTimeout(connect, 2000);
}

I18n.load().then(async () => {
  const calibration = await (await fetch('/api/calibration')).json();
  draft = calibration.draft;
  await loadWiring();
  try {
    calibrated = (await (await fetch('/api/settings')).json()).calibrated === true;
  } catch { /* the state over /ws will tell */ }
  show(calibration.step);
  renderMarked();
  connect();
});
