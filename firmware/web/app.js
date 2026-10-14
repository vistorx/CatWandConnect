// Device interface: Manual control over the WebSocket API (docs/api.md).
const $ = id => document.getElementById(id);

const SEND_INTERVAL_MS = 100;  // well under the robot's 500 ms Manual control timeout

let socket = null;

let offlineSince = null;

function setLink(state) {
  const link = $('link');
  link.dataset.state = state;
  link.textContent = I18n.t(`link.${state}`);
  link.title = link.textContent;  // the text is hidden on narrow phones
  // Offer help once the connection has been lost for a while.
  if (state === 'online') offlineSince = null;
  else offlineSince ??= Date.now();
  $('link-help').hidden = state === 'online' || Date.now() - offlineSince < 10000;
}

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onopen = () => setLink('online');
  socket.onclose = () => {
    setLink('offline');
    setTimeout(connect, 2000);
  };
  socket.onmessage = event => {
    const message = JSON.parse(event.data);
    if (message.t === 'state') render(message);
  };
}

function send(message) {
  if (socket && socket.readyState === WebSocket.OPEN) socket.send(JSON.stringify(message));
}

function render(state) {
  $('pos-h').textContent = `${Math.round(state.h)}°`;
  $('pos-v').textContent = `${Math.round(state.v)}°`;
  if (!speedDragging) showSpeed(Math.round(state.playSpeed * 100));
  $('banner').hidden = state.calibrated !== false;
  if (state.name && $('name').textContent !== state.name) {
    $('name').textContent = state.name;
    document.title = state.name;
  }
  const stowing = state.activity === 'stowing';
  pad.classList.toggle('locked', stowing);
  $('stow').classList.toggle('busy', stowing);
  $('stow').textContent = I18n.t(stowing ? 'stow.busy' : 'stow.button');
  renderPlay(state);
}

// --- Play session (POST /api/play) ---------------------------------------------

const MINUTES_MIN = 1;
const MINUTES_MAX = 60;
const BUSY = ['stowing', 'calibrating', 'tuning_speed'];

let playEndsAt = null;   // local estimate of the session's end, from /ws play.remaining
let playTimer = null;
let minutesSaveTimer = null;
let minutesEditing = false;  // don't let /ws overwrite a value being chosen
let playing = false;
let playPending = false;     // a start/stop request is on its way
let lastActivity = 'idle';

const clampMinutes = value => Math.min(MINUTES_MAX, Math.max(MINUTES_MIN, Math.round(value) || MINUTES_MIN));

function formatRemaining(seconds) {
  const s = Math.max(0, seconds);
  return `${String(Math.floor(s / 60)).padStart(2, '0')}:${String(s % 60).padStart(2, '0')}`;
}

function tickRemaining() {
  if (playEndsAt === null) return;
  $('play-remaining').textContent = formatRemaining(Math.ceil((playEndsAt - Date.now()) / 1000));
}

function playMessage(html) {
  $('play-message').innerHTML = html;
}

function renderPlay(state) {
  const wasPlaying = playing;
  lastActivity = state.activity;
  playing = state.activity === 'playing';
  if (playing && state.play) {
    // The robot rounds up: resync only when the local countdown drifts, so it ticks smoothly.
    const local = playEndsAt === null ? null : Math.ceil((playEndsAt - Date.now()) / 1000);
    if (local === null || Math.abs(local - state.play.remaining) > 1) playEndsAt = Date.now() + state.play.remaining * 1000;
    if (!playTimer) playTimer = setInterval(tickRemaining, 250);
    tickRemaining();
  } else {
    playEndsAt = null;
    clearInterval(playTimer);
    playTimer = null;
  }
  if (playing && !wasPlaying) playMessage('');
  if (!minutesEditing && Number.isInteger(state.sessionMinutes) && document.activeElement !== $('play-minutes')) {
    $('play-minutes').value = state.sessionMinutes;
  }
  $('play-setup').hidden = playing;
  $('play-running').hidden = !playing;
  const toggle = $('play-toggle');
  toggle.textContent = I18n.t(playing ? 'play.stop' : 'play.start');
  toggle.classList.toggle('stop', playing);
  updateToggle();
}

function updateToggle() {
  $('play-toggle').disabled = playPending || (!playing && BUSY.includes(lastActivity));
}

// The last choice is the robot's Session duration, saved without the PIN.
function saveMinutesSoon() {
  minutesEditing = true;
  clearTimeout(minutesSaveTimer);
  minutesSaveTimer = setTimeout(async () => {
    minutesSaveTimer = null;
    try {
      await fetch('/api/settings', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ sessionMinutes: clampMinutes(Number($('play-minutes').value)) }),
      });
    } catch { /* offline: the next Start carries the duration anyway */ }
    minutesEditing = false;
  }, 800);
}

function stepMinutes(delta) {
  $('play-minutes').value = clampMinutes(Number($('play-minutes').value) + delta);
  saveMinutesSoon();
}

$('play-less').addEventListener('click', () => stepMinutes(-1));
$('play-more').addEventListener('click', () => stepMinutes(1));
$('play-minutes').addEventListener('input', () => { minutesEditing = true; });
$('play-minutes').addEventListener('change', () => stepMinutes(0));

async function playAction(body) {
  playPending = true;
  $('play-toggle').disabled = true;
  playMessage('');
  try {
    const response = await fetch('/api/play', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    if (!response.ok) {
      let error = 'invalid_request';
      try { error = (await response.json()).error || error; } catch { /* empty body */ }
      let text = I18n.t(`error.${error}`);
      if (error === 'not_calibrated') text += ` <a href="calibrate.html">${I18n.t('banner.calibrate')}</a>`;
      playMessage(text);
    }
  } catch {
    playMessage(I18n.t('link.offline'));
  }
  playPending = false;
  updateToggle();
}

$('play-toggle').addEventListener('click', () => {
  if (playing) return playAction({ action: 'stop' });
  // The duration rides with Start, which also makes it the Session duration.
  clearTimeout(minutesSaveTimer);
  minutesSaveTimer = null;
  minutesEditing = false;
  const minutes = clampMinutes(Number($('play-minutes').value));
  $('play-minutes').value = minutes;
  return playAction({ action: 'start', sessionMinutes: minutes });
});

// --- Stow ------------------------------------------------------------------------

// Stow wins over everything: drop the joystick first so no move follows it.
$('stow').addEventListener('click', () => {
  release();
  send({ t: 'stow' });
});

// --- Speed slider -------------------------------------------------------------

let speedDragging = false;

function showSpeed(percent) {
  $('speed').value = percent;
  $('speed-value').textContent = `${percent} %`;
}

$('speed').addEventListener('input', () => {
  speedDragging = true;
  showSpeed(Number($('speed').value));
});

// Saved on release only: every save is a flash write.
$('speed').addEventListener('change', async () => {
  speedDragging = false;
  await fetch('/api/settings', {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ playSpeed: Number($('speed').value) / 100 }),
  });
});

// --- Joystick ---------------------------------------------------------------

const pad = $('joystick');
const knob = pad.querySelector('.knob');
let deflection = null;
let sendTimer = null;

function track(event) {
  const box = pad.getBoundingClientRect();
  const radius = box.width / 2;
  let x = (event.clientX - box.left - radius) / radius;
  let y = -(event.clientY - box.top - radius) / radius;
  const length = Math.hypot(x, y);
  if (length > 1) { x /= length; y /= length; }
  deflection = { x, y };
  const travel = radius * 0.6;  // knob edge stays inside the pad
  knob.style.transform = `translate(${x * travel}px, ${-y * travel}px)`;
}

function sendMove() {
  if (!deflection) return;
  const round = value => Math.round(value * 100) / 100;
  send({ t: 'move', h: round(deflection.x), v: round(deflection.y) });
}

function release() {
  if (!deflection) return;
  deflection = null;
  clearInterval(sendTimer);
  pad.classList.remove('active');
  knob.style.transform = '';
  send({ t: 'stop' });
}

pad.addEventListener('pointerdown', event => {
  pad.setPointerCapture(event.pointerId);
  pad.classList.add('active');
  track(event);
  sendMove();
  clearInterval(sendTimer);
  sendTimer = setInterval(sendMove, SEND_INTERVAL_MS);
});
pad.addEventListener('pointermove', event => { if (deflection) track(event); });
for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) pad.addEventListener(type, release);
document.addEventListener('visibilitychange', () => { if (document.hidden) release(); });

I18n.load().then(() => {
  setLink('offline');
  connect();
});

// A report saved on the hotspot waits for the owner, never sent by itself.
async function checkPending() {
  try {
    const [pending, wifi] = await Promise.all([
      fetch('/api/diagnostics/pending').then(r => r.json()),
      fetch('/api/wifi/status').then(r => r.json()),
    ]);
    $('pending-banner').hidden = !(pending.pending && wifi.mode === 'connected');
  } catch { /* older firmware or offline */ }
}
checkPending();
