// Serves firmware/web/ with a simulated robot, to work on the Device interface
// without hardware: `node tools/dev_server.mjs [port]` from firmware/, then
// open http://localhost:8080/. No dependencies.
//
// The simulation is rough: it moves the axes at constant speed within the
// limits and accepts settings with only basic checks. Safety behaviour is
// specified and tested in the Robot core, not here.

import { createHash } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { extname, join, normalize } from 'node:path';
import { fileURLToPath } from 'node:url';

const WEB = fileURLToPath(new URL('../web/', import.meta.url));
const PORT = Number(process.argv[2] || 8080);
let pendingReport = null;  // the Pending report, kept until sent or discarded
const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json',
  '.svg': 'image/svg+xml', '.png': 'image/png', '.webmanifest': 'application/manifest+json' };

// --- Simulated robot ----------------------------------------------------------

const PAUSES = { horizontal: [0.5, 3], vertical: [1, 5] };  // default Pause ranges, s

const robot = {
  settings: {
    playSpeed: 0.6,
    calibrated: false,
    axes: Object.fromEntries(['horizontal', 'vertical'].map(axis =>
      [axis, { min: 60, max: 120, stow: 90, maxSpeed: 90, minSpeed: 9, maxAccel: 360,
        pauseMin: PAUSES[axis][0], pauseMax: PAUSES[axis][1] }])),
    sessionMinutes: 10,
  },
  position: { horizontal: 90, vertical: 90 },
  command: { horizontal: 0, vertical: 0 },
  activity: 'idle',
  lastCommand: 0,
  calibration: { step: 'none', draft: null, target: null, sweep: [] },
  tuning: null,  // { axis, bound, speed, towardMax } while tuning a speed
  goto: {},      // { horizontal: degrees } while moving to a position (POST /api/position)
  play: null,    // { endsAt, axes: { horizontal: { target, speed, pauseUntil } } } during a Play session
};

let deviceName = 'CatWandConnect';
let mqtt = { enabled: false, host: '', port: 1883, user: '', passwordSet: false, connected: false, error: '',
  baseTopic: 'catwandconnect/catwandconnect' };
let pin = '';
let wrongPins = 0;
let lockedUntil = 0;  // simulated lockout: 30 s after 5 wrong PINs
let scanToggle = false;
const tokens = new Set();

// Mirrors the robot's settings PIN: protected routes need a session token.
function authorized(request) {
  if (!pin) return true;
  const header = request.headers.authorization || '';
  return tokens.has(header.replace(/^Bearer /, ''));
}

const PROTECTED = new Set(['POST /api/calibration', 'POST /api/speed-tuning', 'PUT /api/hardware', 'PUT /api/device', 'PUT /api/auth/pin',
  'POST /api/wifi', 'PUT /api/mqtt']);
const startedAt = Date.now();
// docs/api.md: the API version and the Device ID, as GET /api/info and the
// _catwandconnect._tcp TXT records give them.
const API_VERSION = 1;
const DEVICE_ID = '3f9a12c4';

function info() {
  const host = deviceName.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase()
    .replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '') || 'catwandconnect';
  return { name: deviceName, hostname: `${host}.local`, version: '0.0.0-dev (simulator)', api: API_VERSION, id: DEVICE_ID,
    chip: 'ESP32-D0WD-V3',
    chipRevision: 3, cores: 2, cpuMhz: 240, flashMb: 4, freeHeap: 182000, mac: '24:6F:28:00:00:01',
    ssid: 'Simulated', ip: '127.0.0.1', rssi: -61, uptime: Math.floor((Date.now() - startedAt) / 1000), temperature: 47.5 };
}

let hardware = {
  chip: 'ESP32',
  allowedPins: [4, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33],
  axes: { horizontal: { pin: 18, inverted: false, minPulse: 400, maxPulse: 2200 },
    vertical: { pin: 19, inverted: false, minPulse: 600, maxPulse: 2400 } },
};

function putHardware(body) {
  const next = structuredClone(hardware);
  for (const axis of ['horizontal', 'vertical']) Object.assign(next.axes[axis], body.axes?.[axis] ?? {});
  for (const axis of ['horizontal', 'vertical']) {
    const a = next.axes[axis];
    if (!next.allowedPins.includes(a.pin)) return [400, { error: 'pin_not_allowed', axis }];
    if (a.minPulse < 300 || a.maxPulse > 2700 || a.minPulse + 500 > a.maxPulse) return [400, { error: 'pulse_out_of_range', axis }];
  }
  if (next.axes.horizontal.pin === next.axes.vertical.pin) return [400, { error: 'pin_duplicate', axis: 'vertical' }];
  const reset = ['horizontal', 'vertical'].some(axis => ['inverted', 'minPulse', 'maxPulse']
    .some(key => next.axes[axis][key] !== hardware.axes[axis][key]));
  if (reset) robot.settings.calibrated = false;
  hardware = next;
  return [200, { ...hardware, restart: true, calibrationReset: reset }];
}

const STEP_ORDER = ['horizontal_min', 'horizontal_max', 'vertical_min', 'vertical_max', 'stow', 'sweep', 'confirm'];

function calibrationView() {
  const c = robot.calibration;
  const draft = c.draft ?? Object.fromEntries(['horizontal', 'vertical'].map(axis => {
    const { min, max, stow } = robot.settings.axes[axis];
    return [axis, { min, max, stow }];
  }));
  return { step: c.step, draft };
}

// Like the Robot core: each step moves its axis to the value it is about to set.
function enterStep(step) {
  const c = robot.calibration;
  c.step = step;
  const [axis, end] = step.split('_');
  if (end === 'min' || end === 'max') c.target[axis] = c.draft[axis][end];
  if (step === 'stow') for (const a of ['horizontal', 'vertical']) c.target[a] = c.draft[a].stow;
}

function calibrate(body) {
  const c = robot.calibration;
  const pos = robot.position;
  if (body.action === 'start') {
    c.draft = calibrationView().draft;
    c.target = { ...pos };
    enterStep('horizontal_min');
    robot.activity = 'calibrating';
  } else if (body.action === 'nudge' && c.step !== 'none') {
    c.target[body.axis] = Math.max(0, Math.min(180, c.target[body.axis] + Number(body.degrees)));
  } else if (body.action === 'mark') {
    const [axis, end] = c.step.split('_');
    if (end === 'max' && pos[axis] <= c.draft[axis].min) return [400, { error: 'travel_order', axis }];
    if (end) c.draft[axis][end] = round(pos[axis]);
    if (c.step === 'stow') {
      for (const a of ['horizontal', 'vertical']) {
        if (pos[a] < c.draft[a].min || pos[a] > c.draft[a].max) return [400, { error: 'stow_out_of_travel', axis: a }];
        c.draft[a].stow = round(pos[a]);
      }
      const d = c.draft;
      c.sweep = [[d.horizontal.min, d.vertical.min], [d.horizontal.max, d.vertical.min], [d.horizontal.max, d.vertical.max],
        [d.horizontal.min, d.vertical.max], [d.horizontal.min, d.vertical.min], [d.horizontal.stow, d.vertical.stow]];
    }
    enterStep(STEP_ORDER[STEP_ORDER.indexOf(c.step) + 1]);
  } else if (body.action === 'confirm' && c.step === 'confirm') {
    for (const a of ['horizontal', 'vertical']) Object.assign(robot.settings.axes[a], c.draft[a]);
    robot.settings.calibrated = true;
    c.step = 'none';
    stow();
  } else if (body.action === 'cancel') {
    c.step = 'none';
    stow();
  } else if (body.action === 'reverse') {
    // Like the Robot core: mirror every angle of the axis so the wand stays put,
    // flip the wiring, and start the axis over at its minimum.
    const axis = body.axis;
    if (!['horizontal', 'vertical'].includes(axis) || !c.step.startsWith(axis + '_')) return [400, { error: 'invalid_request' }];
    const mirror = a => round(180 - a);
    for (const values of [c.draft[axis], robot.settings.axes[axis]]) {
      const { min, max, stow } = values;
      Object.assign(values, { min: mirror(max), max: mirror(min), stow: mirror(stow) });
    }
    robot.position[axis] = mirror(robot.position[axis]);
    c.target[axis] = robot.position[axis];
    hardware.axes[axis].inverted = !hardware.axes[axis].inverted;
    enterStep(axis + '_min');
  }
  return [200, calibrationView()];
}

// --- Speed tuning (lib/robot_core Robot::startSpeedTuning and friends) ------------

function tuningView() {
  const t = robot.tuning;
  return robot.activity === 'tuning_speed' && t
    ? { active: true, axis: t.axis, bound: t.bound, speed: t.speed } : { active: false };
}

function tuningStep(speed, faster) {
  if (faster ? speed < 30 : speed <= 30) return 2;
  if (faster ? speed < 90 : speed <= 90) return 5;
  return 10;
}

function speedTuning(body) {
  const t = robot.tuning;
  const tuning = robot.activity === 'tuning_speed';
  if (body.action === 'start') {
    if (!['horizontal', 'vertical'].includes(body.axis) || !['min', 'max'].includes(body.bound)
        || !robot.settings.calibrated || robot.activity === 'stowing' || robot.activity === 'calibrating') {
      return [400, { error: 'invalid_request' }];
    }
    const a = robot.settings.axes[body.axis];
    robot.command = { horizontal: 0, vertical: 0 };
    robot.tuning = { axis: body.axis, bound: body.bound, speed: body.bound === 'max' ? a.maxSpeed : a.minSpeed, towardMax: true };
    robot.activity = 'tuning_speed';
  } else if (body.action === 'faster' || body.action === 'slower') {
    if (tuning) {
      const faster = body.action === 'faster';
      const lowest = t.bound === 'max' ? 10 : 1;
      const next = t.speed + (faster ? 1 : -1) * tuningStep(t.speed, faster);
      t.speed = Math.max(lowest, Math.min(360, next));
    }
  } else if (body.action === 'keep') {
    if (tuning) {
      const a = { ...robot.settings.axes[t.axis] };
      if (t.bound === 'max') {
        a.maxSpeed = t.speed;
        if (a.minSpeed >= a.maxSpeed) a.minSpeed = Math.max(1, a.maxSpeed / 10);
      } else {
        if (t.speed >= a.maxSpeed) return [400, { error: 'min_speed_out_of_range', axis: t.axis }];
        a.minSpeed = t.speed;
      }
      robot.settings.axes[t.axis] = a;
      stow();
    }
  } else if (body.action === 'cancel') {
    if (tuning) stow();
  } else {
    return [400, { error: 'invalid_request' }];
  }
  return [200, tuningView()];
}

// --- Play session (lib/robot_core Robot::startPlaySession and friends) ------------

const uniform = (low, high) => low + (high - low) * Math.random();

// One axis's next move: a target anywhere within the Travel limits, a speed
// within the Speed range.
function drawMove(axis) {
  const a = robot.settings.axes[axis];
  const { min, max } = travel(axis);
  return { target: uniform(min, max), speed: uniform(a.minSpeed, a.maxSpeed), pauseUntil: 0 };
}

function playView() {
  const playing = robot.activity === 'playing';
  return { active: playing, remaining: playing ? Math.max(0, Math.ceil((robot.play.endsAt - Date.now()) / 1000)) : 0,
    minutes: robot.settings.sessionMinutes };
}

const validMinutes = value => Number.isInteger(value) && value >= 1 && value <= 60;

function play(body) {
  if (body.action === 'start') {
    const minutes = body.sessionMinutes ?? robot.settings.sessionMinutes;
    if (!validMinutes(minutes)) return [400, { error: 'session_duration_out_of_range' }];
    if (!robot.settings.calibrated) return [409, { error: 'not_calibrated' }];
    if (['stowing', 'calibrating', 'tuning_speed'].includes(robot.activity)) return [409, { error: 'busy' }];
    robot.command = { horizontal: 0, vertical: 0 };
    robot.settings.sessionMinutes = minutes;
    robot.play = { endsAt: Date.now() + minutes * 60000,
      axes: { horizontal: drawMove('horizontal'), vertical: drawMove('vertical') } };
    robot.activity = 'playing';
  } else if (body.action === 'stop') {
    if (robot.activity === 'playing') stow();
  } else {
    return [400, { error: 'invalid_request' }];
  }
  return [200, playView()];
}

// Moves `value` toward `target` by at most `step`.
const approach = (value, target, step) => value + Math.max(-step, Math.min(step, target - value));

function travel(axis) {
  return robot.settings.calibrated ? robot.settings.axes[axis] : { min: 60, max: 120 };
}

setInterval(() => {
  if (Date.now() - robot.lastCommand > 500) robot.command = { horizontal: 0, vertical: 0 };
  const c = robot.calibration;
  if (robot.activity === 'calibrating') {
    if (c.step === 'sweep' || c.step === 'confirm') {
      const [h, v] = c.sweep[0] ?? [robot.position.horizontal, robot.position.vertical];
      robot.position.horizontal = approach(robot.position.horizontal, h, 1);
      robot.position.vertical = approach(robot.position.vertical, v, 1);
      if (c.sweep.length && robot.position.horizontal === h && robot.position.vertical === v) c.sweep.shift();
      if (c.step === 'sweep' && !c.sweep.length) c.step = 'confirm';
    } else {
      for (const axis of ['horizontal', 'vertical']) robot.position[axis] = approach(robot.position[axis], c.target[axis], 1);
    }
    return broadcast(state());
  }
  if (robot.activity === 'tuning_speed') {
    const t = robot.tuning;
    const { min, max } = travel(t.axis);
    const target = t.towardMax ? max : min;
    robot.position[t.axis] = approach(robot.position[t.axis], target, t.speed * 0.05);
    if (robot.position[t.axis] === target) t.towardMax = !t.towardMax;
    return broadcast(state());
  }
  if (robot.activity === 'playing') {
    if (Date.now() >= robot.play.endsAt) {
      stow();
      return broadcast(state());
    }
    for (const axis of ['horizontal', 'vertical']) {
      const p = robot.play.axes[axis];
      if (p.pauseUntil) {
        if (Date.now() < p.pauseUntil) continue;
        robot.play.axes[axis] = drawMove(axis);
        continue;
      }
      robot.position[axis] = approach(robot.position[axis], p.target, p.speed * 0.05);
      if (robot.position[axis] === p.target) {
        const a = robot.settings.axes[axis];
        p.pauseUntil = Date.now() + uniform(a.pauseMin, a.pauseMax) * 1000;
      }
    }
    return broadcast(state());
  }
  if (robot.activity === 'stowing') {
    let done = true;
    for (const axis of ['horizontal', 'vertical']) {
      const target = robot.settings.axes[axis].stow;
      const step = robot.settings.axes[axis].maxSpeed * 0.05;
      const delta = target - robot.position[axis];
      robot.position[axis] += Math.max(-step, Math.min(step, delta));
      if (robot.position[axis] !== target) done = false;
    }
    if (done) robot.activity = 'idle';
    return broadcast(state());
  }
  for (const axis of ['horizontal', 'vertical']) {
    const a = robot.settings.axes[axis];
    const along = (robot.settings.playSpeed - 0.1) / 0.9;
    const speed = a.minSpeed + (a.maxSpeed - a.minSpeed) * along;
    const { min, max } = travel(axis);
    if (axis in robot.goto) {
      const target = Math.min(max, Math.max(min, robot.goto[axis]));
      robot.position[axis] = approach(robot.position[axis], target, speed * 0.05);
      if (robot.position[axis] === target) delete robot.goto[axis];
      continue;
    }
    const next = robot.position[axis] + robot.command[axis] * speed * 0.05;
    robot.position[axis] = Math.min(max, Math.max(min, next));
  }
  if (robot.activity === 'manual' && !robot.command.horizontal && !robot.command.vertical
    && !Object.keys(robot.goto).length) robot.activity = 'idle';
  broadcast(state());
}, 50);

function state() {
  return { t: 'state', h: round(robot.position.horizontal), v: round(robot.position.vertical),
    activity: robot.activity, held: robot.activity !== 'idle', playSpeed: robot.settings.playSpeed,
    calibrated: robot.settings.calibrated, calibration: robot.calibration.step, name: deviceName,
    sessionMinutes: robot.settings.sessionMinutes,
    ...(robot.activity === 'tuning_speed' ? { tuning: tuningView() } : {}),
    ...(robot.activity === 'playing' ? { play: { remaining: playView().remaining } } : {}) };
}

const round = value => Math.round(value * 10) / 10;

function onMessage(message) {
  if (message.t === 'move' && robot.activity !== 'stowing') {
    robot.calibration.step = 'none';
    robot.play = null;  // Manual control ends a Play session, without Stow
    robot.command = { horizontal: clampUnit(message.h), vertical: clampUnit(message.v) };
    robot.goto = {};
    robot.activity = 'manual';
    robot.lastCommand = Date.now();
  } else if (message.t === 'stop') {
    robot.command = { horizontal: 0, vertical: 0 };
  } else if (message.t === 'stow') {
    stow();
  }
}

// Manual control by position, like the MQTT position command: clamped into
// the Travel limits, ends a Play session without Stow, ignored while stowing.
function position(body) {
  if (!['horizontal', 'vertical'].includes(body?.axis) || typeof body.degrees !== 'number'
    || !Number.isFinite(body.degrees)) return [400, { error: 'invalid_request' }];
  if (robot.activity !== 'stowing') {
    if (robot.activity !== 'manual') robot.command = { horizontal: 0, vertical: 0 };
    robot.calibration.step = 'none';
    robot.tuning = null;
    robot.play = null;
    robot.goto[body.axis] = Math.max(0, Math.min(180, body.degrees));
    robot.activity = 'manual';
  }
  return [202, {}];
}

function stow() {
  robot.goto = {};
  robot.play = null;
  robot.calibration.step = 'none';
  robot.command = { horizontal: 0, vertical: 0 };
  robot.activity = 'stowing';
}

const clampUnit = value => Math.max(-1, Math.min(1, Number(value) || 0));

function putSettings(body) {
  const next = structuredClone(robot.settings);
  if (typeof body.playSpeed === 'number') next.playSpeed = body.playSpeed;
  if ('sessionMinutes' in body) next.sessionMinutes = body.sessionMinutes;
  for (const axis of ['horizontal', 'vertical']) Object.assign(next.axes[axis], body.axes?.[axis] ?? {});
  if (next.playSpeed < 0.1 || next.playSpeed > 1) return [400, { error: 'play_speed_out_of_range' }];
  if (!validMinutes(next.sessionMinutes)) return [400, { error: 'session_duration_out_of_range' }];
  for (const axis of ['horizontal', 'vertical']) {
    const a = next.axes[axis];
    if (a.min >= a.max) return [400, { error: 'travel_order', axis }];
    if (a.stow < a.min || a.stow > a.max) return [400, { error: 'stow_out_of_travel', axis }];
    if (a.maxSpeed < 10 || a.maxSpeed > 360) return [400, { error: 'speed_out_of_range', axis }];
    if (a.minSpeed < 1 || a.minSpeed >= a.maxSpeed) return [400, { error: 'min_speed_out_of_range', axis }];
    if (!(a.pauseMin >= 0 && a.pauseMin <= 60 && a.pauseMax >= 0 && a.pauseMax <= 60)) {
      return [400, { error: 'pause_out_of_range', axis }];
    }
    if (a.pauseMin > a.pauseMax) return [400, { error: 'pause_order', axis }];
  }
  robot.settings = next;
  return [200, robot.settings];
}

// --- HTTP ---------------------------------------------------------------------

const API = {
  'GET /api/settings': () => [200, robot.settings],
  'PUT /api/settings': putSettings,
  'GET /api/info': () => [200, info()],
  'GET /api/auth': (body, url, request) => [200, { enabled: !!pin, authorized: authorized(request), lockedFor: 0 }],
  'POST /api/auth/unlock': body => {
    if (Date.now() < lockedUntil) return [429, { error: 'locked_out', retryAfter: Math.ceil((lockedUntil - Date.now()) / 1000) }];
    if (!pin || body.pin !== pin) {
      if (++wrongPins >= 5) {
        lockedUntil = Date.now() + 30000;
        return [429, { error: 'wrong_pin', retryAfter: 30 }];
      }
      return [401, { error: 'wrong_pin' }];
    }
    wrongPins = 0;
    const token = Math.random().toString(16).slice(2);
    tokens.add(token);
    return [200, { token }];
  },
  'PUT /api/auth/pin': body => {
    const next = String(body.pin ?? '');
    if (next && !/^[0-9]{4,8}$/.test(next)) return [400, { error: 'invalid_pin' }];
    pin = next;
    tokens.clear();
    if (!pin) return [200, { enabled: false }];
    const token = Math.random().toString(16).slice(2);
    tokens.add(token);
    return [200, { enabled: true, token }];
  },
  'PUT /api/device': body => {
    const name = String(body.name ?? '').trim();
    if (!name || new TextEncoder().encode(name).length > 32 || !/[a-z0-9]/i.test(name.normalize('NFD'))) {
      return [400, { error: 'invalid_name' }];
    }
    deviceName = name;
    return [200, { name, hostname: info().hostname, restart: true }];
  },
  'GET /api/mqtt': () => [200, mqtt],
  'PUT /api/mqtt': body => {
    const next = { ...mqtt, ...body };
    if (next.enabled && !next.host) return [400, { error: 'mqtt_host_required' }];
    if (next.port < 1 || next.port > 65535) return [400, { error: 'mqtt_port_invalid' }];
    if ('password' in body) next.passwordSet = !!body.password;
    delete next.password;
    // No broker here: pretend it fails, which exercises the error display.
    mqtt = { ...next, connected: false, error: next.enabled ? 'unreachable' : '' };
    return [200, mqtt];
  },
  'GET /api/hardware': () => [200, hardware],
  'PUT /api/hardware': putHardware,
  'GET /api/calibration': () => [200, calibrationView()],
  'POST /api/calibration': calibrate,
  'GET /api/speed-tuning': () => [200, tuningView()],
  'POST /api/speed-tuning': speedTuning,
  'POST /api/stow': () => { stow(); return [202, {}]; },
  'POST /api/position': position,
  'GET /api/play': () => [200, playView()],
  'POST /api/play': play,
  // HOTSPOT=1 simulates a robot on its setup hotspot, with no internet on the phone.
  'GET /api/wifi/status': () => [200, process.env.HOTSPOT === '1' ? { mode: 'hotspot', failed: false }
    : { mode: 'connected', failed: false, ssid: 'Simulated', ip: '127.0.0.1', host: 'catwandconnect.local' }],
  'GET /api/diagnostics/pending': () => [200, pendingReport ? { pending: true, ...pendingReport } : { pending: false }],
  'PUT /api/diagnostics/pending': body => {
    if (!body.description) return [400, { error: 'invalid_request' }];
    pendingReport = { description: String(body.description).slice(0, 600), report: diagnostics(false) };
    return [200, { pending: true }];
  },
  'DELETE /api/diagnostics/pending': () => { pendingReport = null; return [200, { pending: false }]; },
  'GET /api/wifi/networks': () => {
    // Answers "scanning" once in two, like the robot's background scan.
    scanToggle = !scanToggle;
    if (scanToggle) return [200, { scanning: true }];
    return [200, { scanning: false, networks: [
      { ssid: 'Livebox-84B0', rssi: -57, secure: true }, { ssid: 'Simulated', rssi: -50, secure: true },
      { ssid: 'Guest', rssi: -72, secure: false }, { ssid: 'Simulated', rssi: -80, secure: true },
      { ssid: 'A very long network name that does not fit on a phone screen', rssi: -90, secure: true }] }];
  },
};

const server = createServer(async (request, response) => {
  const url = new URL(request.url, 'http://localhost');
  if (request.method === 'POST' && url.pathname === '/api/update') {
    for await (const chunk of request) void chunk;  // pretend to flash it
    if (!authorized(request)) return send(response, 401, { error: 'pin_required' });
    return send(response, 200, { ok: true, restart: true });
  }
  if (request.method === 'GET' && url.pathname === '/api/diagnostics') {
    response.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    return response.end(diagnostics(url.searchParams.get('full') === '1'));
  }
  const route = API[`${request.method} ${url.pathname}`];
  if (route) {
    let body = '';
    for await (const chunk of request) body += chunk;
    let parsed = {};
    try { parsed = body ? JSON.parse(body) : {}; } catch { return send(response, 400, { error: 'invalid_json' }); }
    const key = `${request.method} ${url.pathname}`;
    const settingsChange = key === 'PUT /api/settings'
      && (!Object.keys(parsed).length || Object.keys(parsed).some(k => k !== 'playSpeed' && k !== 'sessionMinutes'));
    if ((PROTECTED.has(key) || settingsChange) && !authorized(request)) return send(response, 401, { error: 'pin_required' });
    const [code, result] = route(parsed, url, request);
    return send(response, code, result);
  }

  const path = url.pathname === '/' ? '/index.html' : url.pathname;
  const file = normalize(join(WEB, path));
  if (!file.startsWith(WEB)) return send(response, 404, { error: 'not_found' });
  try {
    const data = await readFile(file);
    response.writeHead(200, { 'Content-Type': TYPES[extname(file)] || 'application/octet-stream' });
    response.end(data);
  } catch {
    send(response, 404, { error: 'not_found' });
  }
});

// A Diagnostic report shaped like the robot's (lib/diagnostics), for UI work.
function diagnostics(full) {
  const i = info();
  const hide = (value, label) => (full ? value : `<${label}>`);
  const s = i.uptime;
  return `CatWandConnect diagnostic report
Redacted: ${full ? 'no' : 'yes'}

Name: ${i.name === 'CatWandConnect' ? i.name : hide(i.name, 'name')}
Address: http://${i.hostname === 'catwandconnect.local' ? 'catwandconnect' : hide(i.hostname.replace(/\.local$/, ''), 'hostname')}.local/
Firmware: ${i.version}
Chip: ${i.chip} rev ${i.chipRevision}, ${i.cores} x ${i.cpuMhz} MHz, ${i.flashMb} MB flash
Device ID: ${DEVICE_ID}
Last reset: power-on
Running for: ${Math.floor(s / 86400)} d ${Math.floor(s % 86400 / 3600)} h ${Math.floor(s % 3600 / 60)} min (${s} s)
Free memory: ${Math.floor(i.freeHeap / 1024)} KB (lowest ${Math.floor(i.freeHeap / 1024) - 20} KB)
Chip temperature: ${i.temperature.toFixed(1)} C
Settings PIN: ${pin ? 'on' : 'off'}

Wi-Fi: connected, signal ${i.rssi} dBm
Wi-Fi network: ${hide(i.ssid, 'wifi-network')}
IP address: ${hide(i.ip, 'ip')}
MAC address: ${hide(i.mac, 'mac')}
Home Assistant (MQTT): ${mqtt.enabled ? 'enabled, not connected' : 'disabled'}

Settings: ${JSON.stringify(robot.settings)}
Hardware: ${JSON.stringify(hardware)}

Recent log:
[    0.012] CatWandConnect ${i.version} starting, last reset: power-on
[    1.204] [wifi] connecting to "${hide(i.ssid, 'wifi-network')}"
[    3.410] [wifi] connected, open http://${hide(i.ip, 'ip')}/ or http://${i.hostname}/
`;
}

function send(response, code, body) {
  response.writeHead(code, { 'Content-Type': 'application/json' });
  response.end(JSON.stringify(body));
}

// --- Minimal WebSocket (RFC 6455, text frames only) -----------------------------

const sockets = new Set();

server.on('upgrade', (request, socket) => {
  if (request.url !== '/ws') return socket.destroy();
  const accept = createHash('sha1')
    .update(request.headers['sec-websocket-key'] + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
  socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n' +
    `Sec-WebSocket-Accept: ${accept}\r\n\r\n`);
  sockets.add(socket);
  socket.write(frame(JSON.stringify(state())));

  let buffer = Buffer.alloc(0);
  socket.on('data', chunk => {
    buffer = Buffer.concat([buffer, chunk]);
    for (;;) {
      const parsed = unframe(buffer);
      if (!parsed) break;
      buffer = buffer.subarray(parsed.length);
      if (parsed.opcode === 8) return socket.end();
      if (parsed.opcode === 1) {
        try { onMessage(JSON.parse(parsed.text)); } catch { /* ignore malformed messages */ }
      }
    }
  });
  const drop = () => {
    sockets.delete(socket);
    robot.command = { horizontal: 0, vertical: 0 };  // dead-man rule
  };
  socket.on('close', drop);
  socket.on('error', drop);
});

function frame(text) {
  const payload = Buffer.from(text);
  const header = payload.length < 126
    ? Buffer.from([0x81, payload.length])
    : Buffer.from([0x81, 126, payload.length >> 8, payload.length & 255]);
  return Buffer.concat([header, payload]);
}

function unframe(buffer) {
  if (buffer.length < 2) return null;
  const opcode = buffer[0] & 0x0f;
  let length = buffer[1] & 0x7f;
  let offset = 2;
  if (length === 126) {
    if (buffer.length < 4) return null;
    length = buffer.readUInt16BE(2);
    offset = 4;
  }
  const masked = buffer[1] & 0x80;
  const mask = masked ? buffer.subarray(offset, offset + 4) : null;
  if (masked) offset += 4;
  if (buffer.length < offset + length) return null;
  const payload = Buffer.from(buffer.subarray(offset, offset + length));
  if (mask) for (let i = 0; i < payload.length; i++) payload[i] ^= mask[i % 4];
  return { opcode, text: payload.toString(), length: offset + length };
}

function broadcast(message) {
  const data = frame(JSON.stringify(message));
  for (const socket of sockets) socket.write(data);
}

server.listen(PORT, () => console.log(`Simulated CatWandConnect at http://localhost:${PORT}/`));
