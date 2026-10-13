// Discord parts of the Support relay: follow on Discord (#56), /issue (#57),
// release notifications (#58).
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { generateKeyPairSync, sign as edSign } from 'node:crypto';

import { handle } from '../src/index.js';
import { issueFromThread } from '../src/interactions.js';
import { threadLinks } from '../src/discord.js';
import { marker, previousTag, referencedNumbers, releaseMessage, threadsOf } from '../src/release.js';
import { threadMessage } from '../src/report.js';

const GUILD = '1559112591748366357';
const FORUM = '1559207553940848680';
const REPORT = 'CatWandConnect diagnostic report\nRedacted: yes\n\nFirmware: 0.1.0-beta.5\nChip: ESP32\nDevice ID: 3f9a12c4\n';

const rsa = generateKeyPairSync('rsa', { modulusLength: 2048 }).privateKey.export({ type: 'pkcs1', format: 'pem' });
const ed = generateKeyPairSync('ed25519');
const PUBLIC_KEY = ed.publicKey.export({ format: 'der', type: 'spki' }).subarray(-32).toString('hex');

const ENV = {
  SUPPORT_APP_ID: '1', SUPPORT_APP_PRIVATE_KEY: rsa, REPOSITORY: 'vistorx/CatWandConnect', DAILY_CAP: '30',
  DISCORD_BOT_TOKEN: 'bot', DISCORD_GUILD_ID: GUILD, DISCORD_SUPPORT_FORUM: FORUM, DISCORD_BUG_TAG: '681', DISCORD_PUBLIC_KEY: PUBLIC_KEY,
};

function fakeApis({ members = [], existing = [], firstMessage = { content: 'It stops', author: { username: 'pierre' } }, attachment = null } = {}) {
  const calls = [];
  let body = 'issue body';
  const fetcher = async (url, init = {}) => {
    const call = { url, method: init.method || 'GET', body: init.body && JSON.parse(init.body) };
    calls.push(call);
    if (url.endsWith('/installation')) return Response.json({ id: 42 });
    if (url.endsWith('/access_tokens')) return Response.json({ token: 't' });
    if (url.includes('/search/issues')) {
      return Response.json(url.includes('label%3Afrom-device') ? { total_count: 0 } : { items: existing });
    }
    if (url.endsWith('/repos/vistorx/CatWandConnect/issues') && call.method === 'POST') {
      return Response.json({ number: 7, html_url: 'https://github.com/vistorx/CatWandConnect/issues/7' }, { status: 201 });
    }
    if (url.endsWith('/issues/7') && call.method === 'GET') return Response.json({ number: 7, body });
    if (url.endsWith('/issues/7') && call.method === 'PATCH') { body = call.body.body; return Response.json({}); }
    if (url.includes('/members/search')) return Response.json(members);
    if (url.endsWith(`/channels/${FORUM}/threads`)) return Response.json({ id: '999' }, { status: 201 });
    if (url.includes('/channels/555/messages/555')) {
      return Response.json({ ...firstMessage, attachments: attachment ? [{ filename: 'catwandconnect-report.txt', size: 200, url: 'https://cdn/report.txt' }] : [] });
    }
    if (url === 'https://cdn/report.txt') return new Response(attachment);
    if (url.includes('/webhooks/')) return Response.json({});
    return new Response('{}', { status: 404 });
  };
  return { calls, fetcher, body: () => body };
}

function post(payload) {
  return new Request('https://relay/report', {
    method: 'POST', headers: { Origin: 'http://catwandconnect.local', 'Content-Type': 'application/json' }, body: JSON.stringify(payload),
  });
}

test('follow on Discord opens a #support thread with the bug tag and links both ways', async () => {
  const apis = fakeApis();
  const response = await handle(post({ description: 'Arm stops\nmore', report: REPORT, followOnDiscord: true }), ENV, { fetch: apis.fetcher });
  assert.equal(response.status, 201);
  const result = await response.json();
  assert.equal(result.thread, `https://discord.com/channels/${GUILD}/999`);
  const thread = apis.calls.find(c => c.url.endsWith('/threads'));
  assert.equal(thread.body.name, 'Arm stops');
  assert.deepEqual(thread.body.applied_tags, ['681']);
  assert.match(thread.body.message.content, /issues\/7/);
  assert.deepEqual(thread.body.message.allowed_mentions, { parse: [], users: [] });
  assert.match(apis.body(), /Discord thread: https:\/\/discord\.com\/channels\/1559112591748366357\/999/);
});

test('a Discord username in the contact is mentioned, and only that user', async () => {
  const apis = fakeApis({ members: [{ user: { id: '42', username: 'pierre_m' } }] });
  await handle(post({ description: 'x', report: REPORT, contact: '@pierre_m', followOnDiscord: true }), ENV, { fetch: apis.fetcher });
  const thread = apis.calls.find(c => c.url.endsWith('/threads'));
  assert.match(thread.body.message.content, /^<@42> reported/);
  assert.deepEqual(thread.body.message.allowed_mentions, { parse: [], users: ['42'] });
});

test('user text in the thread cannot mass-mention', () => {
  const text = threadMessage({ description: '@everyone @here look', contact: '@everyone' }, 'u', null);
  assert.ok(!/@everyone|@here/.test(text.replace(/@\u200b/g, '')));
});

test('without follow, or if Discord fails, the issue is still created', async () => {
  const apis = fakeApis();
  await handle(post({ description: 'x', report: REPORT }), ENV, { fetch: apis.fetcher });
  assert.ok(!apis.calls.some(c => c.url.includes('discord.com')));
  const failing = async (url, init) => (url.includes('discord.com') ? new Response('{}', { status: 500 }) : fakeApis().fetcher(url, init));
  const response = await handle(post({ description: 'x', report: REPORT, followOnDiscord: true }), ENV, { fetch: failing });
  assert.equal(response.status, 201);
  assert.equal((await response.json()).thread, undefined);
});

function interaction(payload, { key = ed.privateKey, timestamp = '1700000000' } = {}) {
  const body = JSON.stringify(payload);
  const signature = edSign(null, Buffer.from(timestamp + body), key).toString('hex');
  return new Request('https://relay/discord/interactions', {
    method: 'POST', headers: { 'X-Signature-Ed25519': signature, 'X-Signature-Timestamp': timestamp, 'Content-Type': 'application/json' }, body,
  });
}

const command = (extra = {}) => ({
  type: 2, application_id: 'app', token: 'tok', channel_id: '555', data: { name: 'issue' },
  channel: { id: '555', parent_id: FORUM, name: 'Wand stops halfway' },
  member: { permissions: String(1n << 34n) }, ...extra,
});

test('interactions: signature checked, PING answered', async () => {
  const pong = await handle(interaction({ type: 1 }), ENV);
  assert.deepEqual(await pong.json(), { type: 1 });
  const other = generateKeyPairSync('ed25519').privateKey;
  assert.equal((await handle(interaction({ type: 1 }, { key: other }), ENV)).status, 401);
});

test('/issue is for moderators, in #support threads', async () => {
  const notMod = await handle(interaction(command({ member: { permissions: '0' } })), ENV, { fetch: fakeApis().fetcher });
  assert.match((await notMod.json()).data.content, /Only moderators/);
  const elsewhere = await handle(interaction(command({ channel: { id: '555', parent_id: '1' } })), ENV, { fetch: fakeApis().fetcher });
  assert.match((await elsewhere.json()).data.content, /inside a #support thread/);
});

test('/issue creates the issue with the first message, attached report and thread link', async () => {
  const apis = fakeApis({ attachment: REPORT });
  const response = await handle(interaction(command()), ENV, { fetch: apis.fetcher });
  assert.deepEqual(await response.json(), { type: 5 });
  const created = apis.calls.find(c => c.url.endsWith('/issues') && c.method === 'POST');
  assert.equal(created.body.title, '[discord] Wand stops halfway');
  assert.deepEqual(created.body.labels, ['needs-triage']);
  assert.match(created.body.body, /It stops/);
  assert.match(created.body.body, /Device ID: 3f9a12c4/);
  assert.match(created.body.body, /Discord thread: https:\/\/discord\.com\/channels\/1559112591748366357\/555/);
  const answer = apis.calls.find(c => c.url.includes('/webhooks/app/tok/messages/@original'));
  assert.match(answer.body.content, /issues\/7/);
});

test('/issue twice links the existing issue instead of a duplicate', async () => {
  const url = `https://discord.com/channels/${GUILD}/555`;
  const apis = fakeApis({ existing: [{ html_url: 'https://github.com/x/issues/3', body: `Discord thread: ${url}` }] });
  await handle(interaction(command()), ENV, { fetch: apis.fetcher });
  assert.ok(!apis.calls.some(c => c.url.endsWith('/issues') && c.method === 'POST'));
  const answer = apis.calls.find(c => c.url.includes('/webhooks/'));
  assert.match(answer.body.content, /already tracked in https:\/\/github\.com\/x\/issues\/3/);
});

test('issue from a thread neutralises mentions', () => {
  const issue = issueFromThread({ name: '@everyone' }, { content: 'cc @vistorx, fixes #1', author: { username: 'a' } }, '', 'u');
  assert.ok(!/@[a-z]/i.test(issue.title + issue.body));
  assert.ok(!/fixes #1/.test(issue.body));
});

test('release: referenced numbers, previous tag, thread links, message', () => {
  assert.deepEqual(referencedNumbers(['Fix Wi-Fi (#62) (#64)\n\nbody (#99)', 'Docs (#70)', 'no ref']), [62, 64, 70]);
  const tags = ['v0.1.0-beta.1', 'v0.1.0-beta.2', 'v0.1.0-beta.4', 'v0.1.0', 'v0.1.1-beta.1'];
  assert.equal(previousTag(tags, 'v0.1.0-beta.5'), 'v0.1.0-beta.4');
  assert.equal(previousTag(tags, 'v0.1.0'), null);
  assert.equal(previousTag(tags, 'v0.1.1'), 'v0.1.0');
  assert.equal(previousTag(tags, 'v0.1.1-beta.2'), 'v0.1.1-beta.1');
  assert.deepEqual(threadLinks(`see https://discord.com/channels/${GUILD}/555 and https://discord.com/channels/1/2`, GUILD), ['555']);
  assert.deepEqual(threadsOf([`https://discord.com/channels/${GUILD}/5`, `again https://discord.com/channels/${GUILD}/5`], GUILD), ['5']);
  const text = releaseMessage('v0.1.0-beta.5', { number: 7, url: 'https://github.com/x/issues/7' }, true);
  assert.match(text, /v0\.1\.0-beta\.5\*\* \(Beta channel\)/);
  assert.ok(text.includes(marker('v0.1.0-beta.5')));
});
