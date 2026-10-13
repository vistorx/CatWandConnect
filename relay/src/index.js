// CatWandConnect Support relay: a Cloudflare Worker that turns a redacted
// Diagnostic report sent by the owner's browser into a GitHub issue
// (ADR 0009). The robot never calls it; the phone's browser does.

import { LIMITS, buildIssue, threadMessage, threadTitle, validate } from './report.js';
import { createIssue, getIssue, installationToken, reportsToday, updateIssue } from './github.js';
import { discord, findMember, threadUrl } from './discord.js';
import { interactions } from './interactions.js';

// The Device interface is served by the robot over plain HTTP, from its
// mDNS name, its IP on the home network or its hotspot address.
export function allowedOrigin(origin) {
  if (!origin) return false;
  let url;
  try {
    url = new URL(origin);
  } catch {
    return false;
  }
  if (url.protocol !== 'http:') return false;
  const host = url.hostname;
  if (host.endsWith('.local') || host === 'localhost') return true;
  const ip = host.split('.').map(Number);
  if (ip.length !== 4 || ip.some(n => !Number.isInteger(n) || n < 0 || n > 255)) return false;
  return ip[0] === 10 || ip[0] === 127 || (ip[0] === 192 && ip[1] === 168) || (ip[0] === 172 && ip[1] >= 16 && ip[1] <= 31);
}

function corsHeaders(origin) {
  return allowedOrigin(origin) ? {
    'Access-Control-Allow-Origin': origin,
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
    'Access-Control-Max-Age': '86400',
    Vary: 'Origin',
  } : { Vary: 'Origin' };
}

function json(status, data, origin) {
  return new Response(JSON.stringify(data), {
    status,
    headers: { 'Content-Type': 'application/json', ...corsHeaders(origin) },
  });
}

const discordReady = env => Boolean(env.DISCORD_BOT_TOKEN && env.DISCORD_SUPPORT_FORUM && env.DISCORD_GUILD_ID);

// Opens a #support forum thread (tag bug) for the issue, and links both ways.
async function openThread(env, token, report, issue, fetcher) {
  const mentionId = report.contact ? await findMember(env, report.contact, fetcher) : null;
  const thread = await discord(env, 'POST', `/channels/${env.DISCORD_SUPPORT_FORUM}/threads`, {
    name: threadTitle(report.description),
    applied_tags: env.DISCORD_BUG_TAG ? [env.DISCORD_BUG_TAG] : [],
    message: {
      content: threadMessage(report, issue.url, mentionId),
      allowed_mentions: { parse: [], users: mentionId ? [mentionId] : [] },
    },
  }, fetcher);
  const url = threadUrl(env.DISCORD_GUILD_ID, thread.id);
  const current = await getIssue(env, token, issue.number, fetcher);
  await updateIssue(env, token, issue.number, { body: `${current.body}\n\nDiscord thread: ${url}\n` }, fetcher);
  return url;
}

const enabled = env => String(env.ENABLED ?? 'true').toLowerCase() !== 'false';

export async function handle(request, env, deps = {}) {
  const fetcher = deps.fetch ?? fetch;
  const url = new URL(request.url);
  const origin = request.headers.get('Origin');

  // Discord calls this server to server, signed; no CORS involved.
  if (url.pathname === '/discord/interactions' && request.method === 'POST') {
    return interactions(request, env, { fetch: fetcher, waitUntil: deps.waitUntil });
  }
  if (request.method === 'OPTIONS') {
    return new Response(null, { status: allowedOrigin(origin) ? 204 : 403, headers: corsHeaders(origin) });
  }
  // Lets the Device interface know whether to offer "Send to support".
  if (request.method === 'GET' && url.pathname === '/status') {
    return json(200, { available: enabled(env) }, origin);
  }
  if (url.pathname !== '/report') return json(404, { error: 'not_found' }, origin);
  if (request.method !== 'POST') return json(405, { error: 'method_not_allowed' }, origin);
  if (!allowedOrigin(origin)) return json(403, { error: 'origin_not_allowed' }, origin);
  if (!enabled(env)) return json(503, { error: 'unavailable' }, origin);

  const ip = request.headers.get('CF-Connecting-IP') || 'unknown';
  if (env.PER_IP) {
    const { success } = await env.PER_IP.limit({ key: ip });
    if (!success) return json(429, { error: 'rate_limited' }, origin);
  }

  const length = Number(request.headers.get('Content-Length') || 0);
  if (length > LIMITS.body) return json(413, { error: 'too_large' }, origin);
  const text = await request.text();
  if (text.length > LIMITS.body) return json(413, { error: 'too_large' }, origin);
  let input;
  try {
    input = JSON.parse(text);
  } catch {
    return json(400, { error: 'invalid_report' }, origin);
  }
  const checked = validate(input);
  if (!checked.ok) return json(400, { error: checked.error }, origin);

  try {
    const token = await installationToken(env, fetcher);
    if (await reportsToday(env, token, fetcher) >= Number(env.DAILY_CAP || 30)) {
      return json(429, { error: 'daily_cap' }, origin);
    }
    const issue = await createIssue(env, token, buildIssue(checked.value), fetcher);
    if (checked.value.followOnDiscord && discordReady(env)) {
      // The issue exists: a Discord failure must not turn it into an error.
      try {
        issue.thread = await openThread(env, token, checked.value, issue, fetcher);
      } catch (error) {
        console.error(error.message);
      }
    }
    return json(201, issue, origin);
  } catch (error) {
    console.error(error.message);
    // The GitHub step and status only (no secret, no report) to diagnose the setup.
    return json(502, { error: 'github_unavailable', detail: error.detail || 'key' }, origin);
  }
}

export default { fetch: (request, env, ctx) => handle(request, env, { waitUntil: promise => ctx.waitUntil(promise) }) };
