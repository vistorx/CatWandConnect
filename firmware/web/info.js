// Info page: shows /api/info (docs/api.md), refreshed every 5 s.
const $ = id => document.getElementById(id);

function signal(rssi) {
  if (!rssi) return '–';
  const quality = rssi >= -55 ? 'excellent' : rssi >= -67 ? 'good' : rssi >= -75 ? 'fair' : 'weak';
  return `${rssi} dBm (${I18n.t(`info.signal.${quality}`)})`;
}

function uptime(seconds) {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  return I18n.t('info.uptime.value', { d, h, m });
}

let rows = [];

async function refresh() {
  const info = await (await fetch('/api/info')).json();
  rows = [
    ['info.name', info.name],
    ['info.address', `http://${info.hostname}/`],
    ['info.version', info.version],
    ['info.chip', `${info.chip} rev ${info.chipRevision}, ${info.cores} × ${info.cpuMhz} MHz, ${info.flashMb} MB`],
    ['info.network', info.ssid || '–'],
    ['info.ip', info.ip || '–'],
    ['info.signal', signal(info.rssi)],
    ['info.uptime', uptime(info.uptime)],
    ['info.temperature', `${info.temperature} °C`],
    ['info.memory', `${Math.round(info.freeHeap / 1024)} KB`],
    ['info.mac', info.mac],
  ];
  $('info').innerHTML = '';
  for (const [key, value] of rows) {
    const term = document.createElement('dt');
    term.textContent = I18n.t(key);
    const detail = document.createElement('dd');
    detail.textContent = value;
    $('info').append(term, detail);
  }
}

$('copy').addEventListener('click', async () => {
  const text = rows.map(([key, value]) => `${I18n.t(key)}: ${value}`).join('\n');
  try {
    await navigator.clipboard.writeText(text);
    $('message').dataset.kind = 'ok';
    $('message').textContent = I18n.t('info.copied');
  } catch {
    // Clipboard access needs HTTPS on some browsers: select the text instead.
    getSelection().selectAllChildren($('info'));
  }
});

// --- Play session journal (GET /api/journal, new events on /ws) ---

let journal = null;  // the last /api/journal answer
let journalLoadedAt = 0;  // when it came, to age its events

function when(event) {
  const lang = document.documentElement.lang;
  if (event.at !== null) {
    const date = new Date(event.at * 1000);
    const sameDay = date.toDateString() === new Date().toDateString();
    return sameDay
      ? date.toLocaleTimeString(lang, { hour: '2-digit', minute: '2-digit' })
      : date.toLocaleString(lang, { weekday: 'short', day: 'numeric', month: 'short', hour: '2-digit', minute: '2-digit' });
  }
  if (event.ago === null) return I18n.t('journal.when.unknown');
  const ago = event.ago + Math.floor((Date.now() - journalLoadedAt) / 1000);
  const minutes = Math.floor(ago / 60);
  const relative = minutes < 1 ? I18n.t('journal.ago.now')
    : minutes < 60 ? I18n.t('journal.ago.minutes', { m: minutes })
      : I18n.t('journal.ago.hours', { h: Math.floor(minutes / 60), m: minutes % 60 });
  return I18n.t('journal.when.unset', { ago: relative });
}

function describe(event) {
  const what = I18n.t(`journal.what.${event.what}`);
  const source = I18n.t(`journal.source.${event.source}`);
  const reason = event.reason ? ` (${I18n.t(`journal.reason.${event.reason}`)})` : '';
  return `${what}${reason} · ${source}`;
}

function showJournal() {
  if (!journal) return;
  const today = journal.sessionsToday;
  $('journal-today').textContent = today === null || today === undefined
    ? I18n.t('journal.today.unknown') : I18n.t('journal.today', { n: today });
  $('journal').innerHTML = '';
  for (const event of journal.events) {
    const item = document.createElement('li');
    item.dataset.what = event.what;
    const time = document.createElement('span');
    time.className = 'journal-when';
    time.textContent = when(event);
    const text = document.createElement('span');
    text.textContent = describe(event);
    item.append(time, text);
    $('journal').append(item);
  }
  $('journal-empty').hidden = journal.events.length > 0;
}

async function loadJournal() {
  try {
    const response = await fetch('/api/journal');
    if (!response.ok) return;  // firmware without a journal
    journal = await response.json();
    journalLoadedAt = Date.now();
    showJournal();
  } catch {
    // Offline: keep what is shown.
  }
}

// New events arrive on /ws as {"t":"journal","event":{...}}: reloading keeps
// the ages, dates and Sessions today consistent with the robot's.
function listen() {
  const socket = new WebSocket(`ws://${location.host}/ws`);
  socket.addEventListener('message', message => {
    try {
      if (JSON.parse(message.data).t === 'journal') loadJournal();
    } catch {
      // Not ours.
    }
  });
  socket.addEventListener('close', () => setTimeout(listen, 3000));
}

// --- Diagnostic report (GET /api/diagnostics), redacted unless asked ---

async function report(full) {
  const response = await fetch(`/api/diagnostics${full ? '?full=1' : ''}`);
  if (!response.ok) throw new Error(String(response.status));
  return response.text();
}

async function showReport() {
  try {
    $('report').textContent = await report(false);
  } catch {
    $('report').textContent = I18n.t('diag.unavailable');
  }
}

function fileName(full) {
  const stamp = new Date().toISOString().slice(0, 16).replace(/[-:T]/g, '');
  return `catwandconnect-report-${stamp}${full ? '-full' : ''}.txt`;
}

function download(text, name) {
  const link = document.createElement('a');
  link.href = URL.createObjectURL(new Blob([text], { type: 'text/plain' }));
  link.download = name;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(link.href), 1000);
}

function reportMessage(key, kind = 'ok') {
  $('report-message').dataset.kind = kind;
  $('report-message').textContent = I18n.t(key);
}

$('report-copy').addEventListener('click', async () => {
  await showReport();
  try {
    await navigator.clipboard.writeText($('report').textContent);
    reportMessage('diag.copied');
  } catch {
    // No clipboard on plain http:// in most browsers: select the text instead.
    getSelection().selectAllChildren($('report'));
    reportMessage('diag.selected');
  }
});

$('report-download').addEventListener('click', async () => {
  try {
    const text = await report(false);
    $('report').textContent = text;
    download(text, fileName(false));
  } catch {
    reportMessage('diag.unavailable', 'error');
  }
});

$('report-full').addEventListener('click', async () => {
  try {
    download(await report(true), fileName(true));
  } catch {
    reportMessage('diag.unavailable', 'error');
  }
});

I18n.load().then(() => {
  refresh();
  setInterval(refresh, 5000);
  // After clock.js has set the Robot clock, so the events come dated.
  setTimeout(loadJournal, 300);
  setInterval(showJournal, 30000);
  listen();
  showReport();
});
