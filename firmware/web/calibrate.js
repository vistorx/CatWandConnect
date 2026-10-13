// Calibration wizard: drives /api/calibration and follows the robot over /ws
// (docs/api.md).
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
// Steps where the axis can be reversed (minimum and maximum of each axis).
const REVERSIBLE = ['horizontal_min', 'horizontal_max', 'vertical_min', 'vertical_max'];

let step = null;
let position = { horizontal: null, vertical: null };
let draft = null;  // values recorded so far, from /api/calibration
let calibrated = false;  // a Calibration is already saved (from the state over /ws)

// Steps already marked in this Calibration, before `step`.
const ORDER = ['horizontal_min', 'horizontal_max', 'vertical_min', 'vertical_max', 'stow'];

async function call(action, extra = {}) {
  const { ok, data: result } = await Api.call('POST', '/api/calibration', { action, ...extra });
  if (result.error === 'pin_required') return;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    if (result.error === 'travel_order' && draft && result.axis) {
      say('error', I18n.t('error.travel_order_at', { axis, min: Math.round(draft[result.axis].min) }));
    } else {
      say('error', I18n.t(`error.${result.error}`, { axis }));
    }
    return;
  }
  say('', '');
  if (result.draft) draft = result.draft;
  show(result.step);
  renderMarked();
}

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

  $('progress').textContent = info.number ? I18n.t('calibration.progress', { n: info.number, total: 6 }) : '';
  $('step-title').textContent = I18n.t(`calibration.${step}.title`);
  $('step-text').textContent = I18n.t(`calibration.${step}.text`);

  $('nudges').innerHTML = info.axes.map(axis => `
    <fieldset>
      <legend>${I18n.t(`axis.${axis}`)} <output id="pos-${axis}"></output></legend>
      ${step === 'stow' && draft ? `<p class="hint">${I18n.t('calibration.limits', { min: Math.round(draft[axis].min), max: Math.round(draft[axis].max) })}</p>` : ''}
      <div class="nudge-row">
        ${NUDGES.map(d => `<button type="button" class="secondary" data-axis="${axis}" data-degrees="${d}">${d > 0 ? '+' : '−'}${Math.abs(d)}°</button>`).join('')}
      </div>
      ${REVERSIBLE.includes(step) ? `<p class="hint">${I18n.t(`calibration.reverse.${axis}`)}</p>
      <button type="button" class="secondary" data-reverse="${axis}">${I18n.t('calibration.reverse.button')}</button>` : ''}
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
    primary.onclick = async () => {
      await call('confirm');
      say('ok', I18n.t('calibration.done'));
    };
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

$('nudges').addEventListener('click', event => {
  const reverse = event.target.closest('button[data-reverse]');
  if (reverse) {
    call('reverse', { axis: reverse.dataset.reverse }).then(() => {
      if ($('message').dataset.kind !== 'error') say('ok', I18n.t('calibration.reverse.done'));
    });
    return;
  }
  const button = event.target.closest('button[data-axis]');
  if (button) call('nudge', { axis: button.dataset.axis, degrees: Number(button.dataset.degrees) });
});
// With a Calibration already saved, cancelling the new one goes back home.
$('cancel').addEventListener('click', async () => {
  const leave = calibrated;
  await call('cancel');
  if (leave) location.href = '/';
});

function connect() {
  const socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onmessage = event => {
    const state = JSON.parse(event.data);
    if (state.t !== 'state') return;
    position = { horizontal: state.h, vertical: state.v };
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
  try {
    calibrated = (await (await fetch('/api/settings')).json()).calibrated === true;
  } catch { /* the state over /ws will tell */ }
  show(calibration.step);
  renderMarked();
  connect();
});
