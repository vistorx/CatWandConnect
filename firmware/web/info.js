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

I18n.load().then(() => {
  refresh();
  setInterval(refresh, 5000);
});
