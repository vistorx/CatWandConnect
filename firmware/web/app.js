// Device interface: Manual control over the WebSocket API (docs/api.md).
const $ = id => document.getElementById(id);

const SEND_INTERVAL_MS = 100;  // well under the robot's 500 ms Manual control timeout

let socket = null;

function setLink(state) {
  const link = $('link');
  link.dataset.state = state;
  link.textContent = I18n.t(`link.${state}`);
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
  const stowing = state.activity === 'stowing';
  pad.classList.toggle('locked', stowing);
  $('stow').classList.toggle('busy', stowing);
  $('stow').textContent = I18n.t(stowing ? 'stow.busy' : 'stow.button');
}

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
