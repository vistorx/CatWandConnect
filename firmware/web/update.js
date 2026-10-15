// One-tap firmware update (docs/adr/0012). The robot makes no outbound
// connection: this page reads the Update feed, downloads the OTA image for the
// robot's chip and uploads it to POST /api/update like a manual upload.
const Updates = (() => {
  // The Update feed: the web flasher's channels.json on GitHub Pages (CORS *).
  // OTA paths in it are relative to this URL. The simulator rewrites this line.
  const FEED_URL = 'https://vistorx.github.io/CatWandConnect/flasher/channels.json';
  const CHECK_INTERVAL_MS = 60 * 60 * 1000;  // at most once an hour while open
  const BUSY = { playing: 'update.blocked.playing', calibrating: 'update.blocked.calibrating',
    tuning_speed: 'update.blocked.calibrating' };

  // --- Versions: mirrors lib/update_channel (semver with pre-release) ---------

  function parse(text) {
    const clean = String(text ?? '').replace(/^v/i, '').split(/[+ ]/)[0];
    const match = /^(\d{1,9})\.(\d{1,9})\.(\d{1,9})(?:-(.+))?$/.exec(clean);
    if (!match) return null;
    const pre = match[4] ? match[4].split('.') : [];
    if (pre.some(id => !id)) return null;
    return { core: match.slice(1, 4).map(Number), pre };
  }

  function compareIds(a, b) {
    const an = /^\d{1,9}$/.test(a);
    const bn = /^\d{1,9}$/.test(b);
    if (an && bn) return Math.sign(Number(a) - Number(b));
    if (an !== bn) return an ? -1 : 1;
    return a < b ? -1 : a > b ? 1 : 0;
  }

  function compare(a, b) {
    for (let i = 0; i < 3; i++) if (a.core[i] !== b.core[i]) return a.core[i] < b.core[i] ? -1 : 1;
    if (!a.pre.length !== !b.pre.length) return a.pre.length ? -1 : 1;  // a release beats its pre-releases
    for (let i = 0; i < Math.min(a.pre.length, b.pre.length); i++) {
      const order = compareIds(a.pre[i], b.pre[i]);
      if (order) return order;
    }
    return Math.sign(a.pre.length - b.pre.length);
  }

  // Whether `candidate` is strictly newer than `running`.
  function isNewer(candidate, running) {
    const c = parse(candidate);
    const r = parse(running);
    return !!(c && r && compare(c, r) > 0);
  }

  // Older firmware has no `channel`/`chipFamily` in /api/info.
  const channelOf = info => info.channel || (String(info.version).includes('-beta') ? 'beta' : 'stable');
  const familyOf = info => info.chipFamily || (/^ESP32-(S3|C3)/i.exec(info.chip || '')?.[0]?.toUpperCase() ?? 'ESP32');

  // --- Check ----------------------------------------------------------------------

  // { state: 'available' | 'current' | 'error', version, notes, summary, url }.
  // Also tells the robot the latest version, for its MQTT Firmware entity.
  async function check(info) {
    let feed;
    try {
      const response = await fetch(FEED_URL, { cache: 'no-store' });
      if (!response.ok) throw new Error(response.status);
      feed = await response.json();
    } catch {
      return { state: 'error' };
    }
    const entry = feed[channelOf(info)];
    const path = entry?.ota?.[familyOf(info)];
    report(entry && path ? entry.version : '', entry?.notes || '');
    if (!entry || !path || !isNewer(entry.version, info.version)) return { state: 'current' };
    return { state: 'available', version: entry.version, notes: entry.notes || '', summary: entry.summary || '',
      url: new URL(path, new URL(FEED_URL, location.href)).href };
  }

  function report(version, notes) {
    fetch('/api/update/available', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ version, notes }),
    }).catch(() => { /* older firmware: nothing to tell */ });
  }

  // Runs `check` now and then every hour; `render` gets each result.
  async function watch(render) {
    const run = async () => {
      try {
        const info = await (await fetch('/api/info')).json();
        render(await check(info), info);
      } catch { /* robot unreachable: the page says so already */ }
    };
    await run();
    setInterval(run, CHECK_INTERVAL_MS);
  }

  // The i18n key saying why updating must wait, or '' when it can go ahead.
  async function blocker() {
    try {
      const [play, calibration, tuning] = await Promise.all(
        ['/api/play', '/api/calibration', '/api/speed-tuning'].map(path => fetch(path).then(r => r.json())));
      if (play.active) return BUSY.playing;
      if (calibration.step && calibration.step !== 'none') return BUSY.calibrating;
      if (tuning.active) return BUSY.tuning_speed;
    } catch { /* let the robot answer */ }
    return '';
  }

  // --- Download, upload, restart --------------------------------------------------

  // Resolves to a Blob; `progress(percent)` while it arrives.
  async function download(url, progress) {
    const response = await fetch(url, { cache: 'no-store' });
    if (!response.ok || !response.body) throw new Error('download_failed');
    const total = Number(response.headers.get('Content-Length')) || 0;
    const reader = response.body.getReader();
    const chunks = [];
    let received = 0;
    for (;;) {
      const { done, value } = await reader.read();
      if (done) break;
      chunks.push(value);
      received += value.length;
      // Content-Length is the compressed size when Pages gzips the file.
      if (total) progress(Math.min(99, Math.round((received / total) * 100)));
    }
    progress(100);
    return new Blob(chunks, { type: 'application/octet-stream' });
  }

  // POST /api/update; XHR rather than fetch, for upload progress. Asks for the
  // PIN and retries once when the robot wants it. Resolves { status, data }.
  async function upload(file, name, progress) {
    const send = () => new Promise(resolve => {
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/update');
      if (Api.token()) xhr.setRequestHeader('Authorization', `Bearer ${Api.token()}`);
      xhr.upload.onprogress = event => {
        if (event.lengthComputable) progress(Math.round((event.loaded / event.total) * 100));
      };
      xhr.onload = () => {
        let data = {};
        try { data = JSON.parse(xhr.responseText); } catch { /* not JSON */ }
        resolve({ status: xhr.status, data });
      };
      xhr.onerror = () => resolve({ status: 0, data: { error: 'upload_interrupted' } });
      const form = new FormData();
      form.append('firmware', file, name);
      xhr.send(form);
    });
    let result = await send();
    if (result.status === 401 && result.data.error === 'pin_required') {
      Api.setToken('');
      if (await Api.askPin()) result = await send();
    }
    return result;
  }

  // Resolves to the new /api/info once the robot is back after restarting, or
  // null after `timeoutMs`.
  async function waitForRestart(before, timeoutMs = 90000) {
    const started = Date.now();
    await new Promise(resolve => setTimeout(resolve, 3000));
    while (Date.now() - started < timeoutMs) {
      try {
        const info = await (await fetch('/api/info', { cache: 'no-store' })).json();
        const expectedUptime = before.uptime + (Date.now() - started) / 1000;
        if (info.version !== before.version || info.uptime + 5 < expectedUptime) return info;
      } catch { /* restarting */ }
      await new Promise(resolve => setTimeout(resolve, 2000));
    }
    return null;
  }

  return { FEED_URL, isNewer, channelOf, familyOf, check, watch, blocker, download, upload, waitForRestart, BUSY };
})();
