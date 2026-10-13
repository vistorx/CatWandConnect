// Help page: a question goes to Discord, a problem becomes a prefilled public
// GitHub issue built from the redacted Diagnostic report (docs/api.md).
const $ = id => document.getElementById(id);

// GitHub refuses very long URLs; stay well under its limit once encoded.
const MAX_URL = 7000;

let report = '';  // redacted Diagnostic report, as served by the robot
let issueUrl = '';

function say(kind, text) {
  $('message').dataset.kind = kind;
  $('message').textContent = text;
}

// Splits the report into the summary and its log lines.
function splitReport(text) {
  const at = text.indexOf('\nRecent log:\n');
  if (at < 0) return { summary: text.trim(), log: [] };
  return {
    summary: text.slice(0, at).trim(),
    log: text.slice(at + '\nRecent log:\n'.length).split('\n').filter(Boolean),
  };
}

function field(report, name) {
  const match = report.match(new RegExp(`^${name}: (.*)$`, 'm'));
  return match ? match[1].trim() : '';
}

function chipOption(chip) {
  if (chip.startsWith('ESP32-S3')) return 'ESP32-S3';
  if (chip.startsWith('ESP32-C3')) return 'ESP32-C3';
  if (chip.startsWith('ESP32')) return 'ESP32';
  return "I don't know";
}

function buildUrl(what, body) {
  const params = new URLSearchParams({
    template: 'bug.yml',
    title: what.split('\n')[0].slice(0, 80) || 'Problem reported from the robot',
    'what-happened': what,
    'diagnostic-report': body,
    version: field(report, 'Firmware'),
    chip: chipOption(field(report, 'Chip')),
  });
  return `${Links.newBug}?${params}`;
}

// The summary plus the most recent log lines that fit in the URL.
function fitReport(what) {
  const { summary, log } = splitReport(report);
  for (let keep = log.length; keep >= 0; keep--) {
    const lines = log.slice(log.length - keep);
    const body = lines.length ? `${summary}\n\nRecent log (last ${lines.length} of ${log.length} lines):\n${lines.join('\n')}` : summary;
    const url = buildUrl(what, body);
    if (url.length <= MAX_URL) return { body, url, trimmed: keep < log.length };
  }
  return { body: summary, url: buildUrl(what, summary.slice(0, 2000)), trimmed: true };
}

async function preview() {
  const what = $('what').value.trim();
  if (!what) return say('error', I18n.t('help.bug.what.missing'));
  say('', '');
  try {
    report = await (await fetch('/api/diagnostics')).text();
  } catch {
    return say('error', I18n.t('diag.unavailable'));
  }
  const fitted = fitReport(what);
  issueUrl = fitted.url;
  $('summary').textContent = `${what}\n\n${fitted.body}`;
  $('trimmed').hidden = !fitted.trimmed;
  $('publish').hidden = false;
  $('publish').scrollIntoView({ behavior: 'smooth', block: 'start' });
}

function download() {
  const stamp = new Date().toISOString().slice(0, 16).replace(/[-:T]/g, '');
  const link = document.createElement('a');
  link.href = URL.createObjectURL(new Blob([report], { type: 'text/plain' }));
  link.download = `catwandconnect-report-${stamp}.txt`;
  link.click();
  URL.revokeObjectURL(link.href);
}

$('preview').addEventListener('click', preview);
$('what').addEventListener('input', () => { $('publish').hidden = true; });
$('open-issue').addEventListener('click', () => {
  download();
  window.open(issueUrl, '_blank', 'noopener');
  say('ok', I18n.t('help.bug.opened'));
});
$('download').addEventListener('click', download);

I18n.load().then(() => {
  $('discord').href = Links.discord;
  $('troubleshooting').href = Links.troubleshooting;
  $('security').href = Links.security;
  $('what').placeholder = I18n.t('help.bug.what.placeholder');
});
