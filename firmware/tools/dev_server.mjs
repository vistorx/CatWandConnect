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
const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json',
  '.svg': 'image/svg+xml', '.png': 'image/png', '.webmanifest': 'application/manifest+json' };

// --- Simulated robot ----------------------------------------------------------

const robot = {
  settings: {
    playSpeed: 0.6,
    calibrated: false,
    axes: Object.fromEntries(['horizontal', 'vertical'].map(axis =>
      [axis, { min: 60, max: 120, stow: 90, maxSpeed: 90, maxAccel: 360 }])),
  },
  position: { horizontal: 90, vertical: 90 },
  command: { horizontal: 0, vertical: 0 },
  activity: 'idle',
  lastCommand: 0,
};

function travel(axis) {
  return robot.settings.calibrated ? robot.settings.axes[axis] : { min: 60, max: 120 };
}

setInterval(() => {
  if (Date.now() - robot.lastCommand > 500) robot.command = { horizontal: 0, vertical: 0 };
  for (const axis of ['horizontal', 'vertical']) {
    const speed = robot.settings.axes[axis].maxSpeed * robot.settings.playSpeed;
    const { min, max } = travel(axis);
    const next = robot.position[axis] + robot.command[axis] * speed * 0.05;
    robot.position[axis] = Math.min(max, Math.max(min, next));
  }
  if (robot.activity === 'manual' && !robot.command.horizontal && !robot.command.vertical) robot.activity = 'idle';
  broadcast(state());
}, 50);

function state() {
  return { t: 'state', h: round(robot.position.horizontal), v: round(robot.position.vertical),
    activity: robot.activity, playSpeed: robot.settings.playSpeed };
}

const round = value => Math.round(value * 10) / 10;

function onMessage(message) {
  if (message.t === 'move') {
    robot.command = { horizontal: clampUnit(message.h), vertical: clampUnit(message.v) };
    robot.activity = 'manual';
    robot.lastCommand = Date.now();
  } else if (message.t === 'stop') {
    robot.command = { horizontal: 0, vertical: 0 };
  }
}

const clampUnit = value => Math.max(-1, Math.min(1, Number(value) || 0));

function putSettings(body) {
  const next = structuredClone(robot.settings);
  if (typeof body.playSpeed === 'number') next.playSpeed = body.playSpeed;
  for (const axis of ['horizontal', 'vertical']) Object.assign(next.axes[axis], body.axes?.[axis] ?? {});
  if (next.playSpeed < 0.1 || next.playSpeed > 1) return [400, { error: 'play_speed_out_of_range' }];
  for (const axis of ['horizontal', 'vertical']) {
    const a = next.axes[axis];
    if (a.min >= a.max) return [400, { error: 'travel_order', axis }];
    if (a.stow < a.min || a.stow > a.max) return [400, { error: 'stow_out_of_travel', axis }];
    if (a.maxSpeed < 10 || a.maxSpeed > 360) return [400, { error: 'speed_out_of_range', axis }];
  }
  robot.settings = next;
  return [200, robot.settings];
}

// --- HTTP ---------------------------------------------------------------------

const API = {
  'GET /api/settings': () => [200, robot.settings],
  'PUT /api/settings': putSettings,
  'GET /api/wifi/status': () => [200, { mode: 'connected', failed: false, ssid: 'Simulated', ip: '127.0.0.1',
    host: 'catwandconnect.local' }],
  'GET /api/wifi/networks': () => [200, { scanning: false, networks: [{ ssid: 'Simulated', rssi: -50, secure: true }] }],
};

const server = createServer(async (request, response) => {
  const url = new URL(request.url, 'http://localhost');
  const route = API[`${request.method} ${url.pathname}`];
  if (route) {
    let body = '';
    for await (const chunk of request) body += chunk;
    let parsed = {};
    try { parsed = body ? JSON.parse(body) : {}; } catch { return send(response, 400, { error: 'invalid_json' }); }
    const [code, result] = route(parsed, url);
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
