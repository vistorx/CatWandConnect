// Discord interactions endpoint: the moderator-only /issue command run in a
// #support thread turns the thread into a GitHub issue (#57).

import { neutralize } from './report.js';
import { discord, threadUrl } from './discord.js';
import { createIssue, installationToken, searchIssues } from './github.js';

const PING = 1;
const APPLICATION_COMMAND = 2;
const PONG = 1;
const CHANNEL_MESSAGE = 4;
const DEFERRED_CHANNEL_MESSAGE = 5;
const EPHEMERAL = 1 << 6;
const MANAGE_THREADS = 1n << 34n;
const REPORT_LIMIT = 14 * 1024;

export const COMMANDS = [{
  name: 'issue',
  description: 'Turn this #support thread into a GitHub issue (confirmed bugs only)',
  type: 1,
  default_member_permissions: String(MANAGE_THREADS),
  dm_permission: false,
}];

function hexToBytes(hex) {
  return Uint8Array.from(hex.match(/../g) || [], byte => parseInt(byte, 16));
}

// Discord signs every interaction with the application's Ed25519 key.
export async function verify(request, publicKey, body) {
  const signature = request.headers.get('X-Signature-Ed25519');
  const timestamp = request.headers.get('X-Signature-Timestamp');
  if (!signature || !timestamp || !publicKey) return false;
  try {
    const key = await crypto.subtle.importKey('raw', hexToBytes(publicKey), { name: 'Ed25519' }, false, ['verify']);
    return await crypto.subtle.verify('Ed25519', key, hexToBytes(signature), new TextEncoder().encode(timestamp + body));
  } catch {
    return false;
  }
}

const reply = (content, flags = EPHEMERAL) => Response.json({
  type: CHANNEL_MESSAGE, data: { content, flags, allowed_mentions: { parse: [] } },
});

export async function interactions(request, env, deps = {}) {
  const body = await request.text();
  if (!(await verify(request, env.DISCORD_PUBLIC_KEY, body))) return new Response('invalid request signature', { status: 401 });
  const interaction = JSON.parse(body);
  if (interaction.type === PING) return Response.json({ type: PONG });
  if (interaction.type !== APPLICATION_COMMAND || interaction.data?.name !== 'issue') return reply('Unknown command.');

  const permissions = BigInt(interaction.member?.permissions || '0');
  if ((permissions & MANAGE_THREADS) === 0n) return reply('Only moderators can turn a thread into an issue.');
  const channel = interaction.channel || {};
  if (channel.parent_id !== env.DISCORD_SUPPORT_FORUM) return reply('Run /issue inside a #support thread.');

  // GitHub can take longer than Discord's 3 s: answer now, finish in the background.
  const work = convert(env, interaction, deps.fetch ?? fetch).catch(error => {
    console.error(error.message);
    return followUp(env, interaction, 'Could not create the issue. Try again later.', deps.fetch ?? fetch);
  });
  if (deps.waitUntil) deps.waitUntil(work);
  else await work;
  return Response.json({ type: DEFERRED_CHANNEL_MESSAGE });
}

async function followUp(env, interaction, content, fetcher) {
  await fetcher(`https://discord.com/api/v10/webhooks/${interaction.application_id}/${interaction.token}/messages/@original`, {
    method: 'PATCH',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ content, allowed_mentions: { parse: [] } }),
  });
}

// The Diagnostic report attached to the thread's first message, if any.
async function attachedReport(message, fetcher) {
  for (const file of message.attachments || []) {
    if (!/\.txt$/i.test(file.filename) || file.size > REPORT_LIMIT) continue;
    const response = await fetcher(file.url);
    if (!response.ok) continue;
    const text = (await response.text()).replace(/\r\n/g, '\n');
    if (text.startsWith('CatWandConnect diagnostic report\n')) return text;
  }
  return '';
}

function fence(text) {
  const longest = Math.max(2, ...(text.match(/`+/g) || []).map(run => run.length));
  return '`'.repeat(longest + 1);
}

export function issueFromThread(thread, message, report, url) {
  const author = message.author ? (message.author.global_name || message.author.username) : 'unknown';
  const lines = [
    `_Converted from a Discord #support thread by a moderator: ${url}_`,
    '',
    `### What happened (reported by ${neutralize(author)} on Discord)`,
    '',
    neutralize(message.content || '(no text)'),
    '',
  ];
  if (report) {
    const marks = fence(report);
    lines.push('### Diagnostic report', '', `${marks}text`, report.trimEnd(), marks, '');
  }
  lines.push(`Discord thread: ${url}`, '');
  const title = (thread.name || 'Problem reported on Discord').slice(0, 90);
  return { title: `[discord] ${neutralize(title)}`, body: lines.join('\n'), labels: ['needs-triage'] };
}

async function answeredIssue(env, interaction, threadId, fetcher) {
  const messages = await discord(env, 'GET', `/channels/${threadId}/messages?limit=100`, null, fetcher).catch(() => []);
  const prefix = `https://github.com/${env.REPOSITORY}/issues/`;
  for (const message of messages) {
    if (message.author?.id !== interaction.application_id) continue;
    const at = (message.content || '').indexOf(`tracked in ${prefix}`);
    if (at < 0) continue;
    const number = /^\d+/.exec(message.content.slice(at + 'tracked in '.length + prefix.length));
    if (number) return `${prefix}${number[0]}`;
  }
  return null;
}

async function convert(env, interaction, fetcher) {
  const threadId = interaction.channel_id;
  const url = threadUrl(env.DISCORD_GUILD_ID, threadId);
  const token = await installationToken(env, fetcher);

  // Running /issue twice links the existing issue instead of a duplicate. The
  // relay's own earlier answer in the thread is checked first, because GitHub
  // search takes a while to index a new issue.
  const existing = await answeredIssue(env, interaction, threadId, fetcher)
    || (await searchIssues(env, token, `is:issue in:body "${url}"`, fetcher)).find(item => item.body && item.body.includes(url))?.html_url;
  if (existing) {
    return followUp(env, interaction, `This thread is already tracked in ${existing}`, fetcher);
  }

  const thread = interaction.channel;
  // A forum thread's first message has the thread's ID.
  const message = await discord(env, 'GET', `/channels/${threadId}/messages/${threadId}`, null, fetcher).catch(() => ({}));
  const report = await attachedReport(message, fetcher);
  const issue = await createIssue(env, token, issueFromThread(thread, message, report, url), fetcher);
  return followUp(env, interaction, `Thanks! This is now tracked in ${issue.url}. We will post here when a fix ships.`, fetcher);
}
