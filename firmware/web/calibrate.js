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

let step = null;
let position = { horizontal: null, vertical: null };

async function call(action, extra = {}) {
  const { ok, data: result } = await Api.call('POST', '/api/calibration', { action, ...extra });
  if (result.error === 'pin_required') return;
  if (!ok) {
    const axis = result.axis ? I18n.t(`axis.${result.axis}`) : '';
    say('error', I18n.t(`error.${result.error}`, { axis }));
    return;
  }
  say('', '');
  show(result.step);
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
      <div class="nudge-row">
        ${NUDGES.map(d => `<button type="button" class="secondary" data-axis="${axis}" data-degrees="${d}">${d > 0 ? '+' : '−'}${Math.abs(d)}°</button>`).join('')}
      </div>
    </fieldset>`).join('');
  renderPositions();

  const primary = $('primary');
  const secondary = $('secondary');
  primary.hidden = false;
  primary.disabled = false;
  secondary.hidden = true;
  $('cancel').hidden = step === 'none';

  if (step === 'none') {
    primary.textContent = I18n.t('calibration.start');
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
  const button = event.target.closest('button[data-axis]');
  if (button) call('nudge', { axis: button.dataset.axis, degrees: Number(button.dataset.degrees) });
});
$('cancel').addEventListener('click', () => call('cancel'));

function connect() {
  const socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onmessage = event => {
    const state = JSON.parse(event.data);
    if (state.t !== 'state') return;
    position = { horizontal: state.h, vertical: state.v };
    renderPositions();
    show(state.calibration);
  };
  socket.onclose = () => setTimeout(connect, 2000);
}

I18n.load().then(async () => {
  const calibration = await (await fetch('/api/calibration')).json();
  show(calibration.step);
  connect();
});
