// What the Support relay accepts from a browser, and the issue it builds.

export const LIMITS = {
  body: 20 * 1024,        // whole request, bytes
  report: 14 * 1024,      // the robot caps its report at 12 KB
  description: 2000,
  contact: 100,
};

const REPORT_HEADER = 'CatWandConnect diagnostic report\nRedacted: yes\n';

// Returns { ok: true, value } or { ok: false, error }.
export function validate(input) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) return { ok: false, error: 'invalid_report' };
  const { description, report, contact = '' } = input;
  if (typeof description !== 'string' || typeof report !== 'string' || typeof contact !== 'string') {
    return { ok: false, error: 'invalid_report' };
  }
  const what = description.trim();
  if (!what || what.length > LIMITS.description) return { ok: false, error: 'invalid_description' };
  // Only a redacted report from the robot: anything else is not ours to post.
  if (!report.replace(/\r\n/g, '\n').startsWith(REPORT_HEADER) || report.length > LIMITS.report) {
    return { ok: false, error: 'invalid_report' };
  }
  if (contact.length > LIMITS.contact) return { ok: false, error: 'invalid_contact' };
  return { ok: true, value: { description: what, report: report.replace(/\r\n/g, '\n'), contact: contact.trim() } };
}

function field(report, name) {
  const match = report.match(new RegExp(`^${name}: (.*)$`, 'm'));
  return match ? match[1].trim() : '';
}

// Text typed by strangers must not ping people or close issues.
export function neutralize(text) {
  return text
    .replace(/@(?=[A-Za-z0-9-])/g, '@\u200b')
    .replace(/\b(close[sd]?|fix(e[sd])?|resolve[sd]?)(\s+)#/gi, '$1$3#\u200b');
}

// A code fence longer than any backtick run inside the text.
function fence(text) {
  const longest = Math.max(2, ...(text.match(/`+/g) || []).map(run => run.length));
  return '`'.repeat(longest + 1);
}

export function buildIssue({ description, report, contact }) {
  const firstLine = description.split('\n')[0].trim();
  const title = firstLine.length > 80 ? `${firstLine.slice(0, 77)}…` : firstLine;
  const version = field(report, 'Firmware') || 'unknown';
  const chip = field(report, 'Chip') || 'unknown';
  const device = field(report, 'Device ID');
  const marks = fence(report);
  const lines = [
    '_Sent from a robot\'s Device interface through the Support relay._',
    '',
    '### What happened',
    '',
    neutralize(description),
    '',
    '### Details',
    '',
    `- Firmware: ${neutralize(version)}`,
    `- Chip: ${neutralize(chip)}`,
  ];
  if (device) lines.push(`- Device ID: \`${device.replace(/`/g, '')}\``);
  if (contact) lines.push(`- Contact: ${neutralize(contact)}`);
  lines.push('', '### Diagnostic report', '', `${marks}text`, report.trimEnd(), marks, '');
  return {
    title: `[device] ${neutralize(title)}`,
    body: lines.join('\n'),
    labels: ['from-device', 'needs-triage'],
  };
}
