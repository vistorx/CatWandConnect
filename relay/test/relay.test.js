// Support relay tests: `node --test relay/test` (no dependencies).
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { generateKeyPairSync, createVerify } from 'node:crypto';

import { allowedOrigin, handle } from '../src/index.js';
import { buildIssue, neutralize, validate } from '../src/report.js';
import { appJwt, pemToPkcs8 } from '../src/github.js';

const REPORT = [
  'CatWandConnect diagnostic report',
  'Redacted: yes',
  '',
  'Name: CatWandConnect',
  'Firmware: 0.1.0-beta.5',
  'Chip: ESP32-D0WD-V3 rev 3',
  'Device ID: 3f9a12c4',
  '',
  'Recent log:',
  '[    1.204] [wifi] connecting to "<wifi-network>"',
  '',
].join('\n');

const { privateKey, publicKey } = generateKeyPairSync('rsa', { modulusLength: 2048 });
const PKCS1 = privateKey.export({ type: 'pkcs1', format: 'pem' });
const PKCS8 = privateKey.export({ type: 'pkcs8', format: 'pem' });

const ENV = { SUPPORT_APP_ID: '123', SUPPORT_APP_PRIVATE_KEY: PKCS1, REPOSITORY: 'vistorx/CatWandConnect', DAILY_CAP: '3' };

// A fake GitHub API recording the calls.
function fakeGitHub({ today = 0, fail = false } = {}) {
  const calls = [];
  const fetcher = async (url, init) => {
    calls.push({ url, method: init.method, body: init.body && JSON.parse(init.body), auth: init.headers.Authorization });
    if (fail) return new Response('{"message":"boom"}', { status: 500 });
    if (url.endsWith('/repos/vistorx/CatWandConnect/installation')) return Response.json({ id: 42 });
    if (url.endsWith('/app/installations/42/access_tokens')) return Response.json({ token: 'ghs_test' }, { status: 201 });
    if (url.includes('/search/issues')) return Response.json({ total_count: today });
    if (url.endsWith('/repos/vistorx/CatWandConnect/issues')) {
      return Response.json({ number: 7, html_url: 'https://github.com/vistorx/CatWandConnect/issues/7' }, { status: 201 });
    }
    return new Response('{}', { status: 404 });
  };
  return { calls, fetcher };
}

function post(body, { origin = 'http://catwandconnect.local', ip = '1.2.3.4' } = {}) {
  return new Request('https://relay.example/report', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', Origin: origin, 'CF-Connecting-IP': ip },
    body: typeof body === 'string' ? body : JSON.stringify(body),
  });
}

const GOOD = { description: 'The arm stops halfway\nafter a minute', report: REPORT, contact: 'pierre#1234' };

test('origins of the Device interface are allowed, others are not', () => {
  for (const origin of ['http://catwandconnect.local', 'http://catwandconnect-salon.local', 'http://192.168.4.1',
    'http://192.168.1.37', 'http://10.0.0.5', 'http://172.20.1.2', 'http://localhost:8099']) {
    assert.ok(allowedOrigin(origin), origin);
  }
  for (const origin of [null, '', 'https://evil.example', 'http://evil.example', 'http://8.8.8.8', 'http://172.32.0.1',
    'https://catwandconnect.local', 'file://', 'null']) {
    assert.ok(!allowedOrigin(origin), String(origin));
  }
});

test('only a redacted CatWandConnect report with a description is accepted', () => {
  assert.ok(validate(GOOD).ok);
  assert.equal(validate({ ...GOOD, description: '  ' }).error, 'invalid_description');
  assert.equal(validate({ ...GOOD, report: 'hello' }).error, 'invalid_report');
  assert.equal(validate({ ...GOOD, report: REPORT.replace('Redacted: yes', 'Redacted: no') }).error, 'invalid_report');
  assert.equal(validate({ ...GOOD, report: REPORT + 'x'.repeat(15000) }).error, 'invalid_report');
  assert.equal(validate({ ...GOOD, contact: 'x'.repeat(101) }).error, 'invalid_contact');
  assert.equal(validate([GOOD]).error, 'invalid_report');
});

test('the issue carries the description, details, device ID, contact and report, labelled for triage', () => {
  const issue = buildIssue(validate(GOOD).value);
  assert.equal(issue.title, '[device] The arm stops halfway');
  assert.deepEqual(issue.labels, ['from-device', 'needs-triage']);
  assert.match(issue.body, /### What happened\n\nThe arm stops halfway\nafter a minute/);
  assert.match(issue.body, /- Firmware: 0\.1\.0-beta\.5/);
  assert.match(issue.body, /- Device ID: `3f9a12c4`/);
  assert.match(issue.body, /- Contact: pierre#1234/);
  assert.ok(issue.body.includes('```text\n' + REPORT.trimEnd() + '\n```'));
});

test('user text cannot mention people or close issues', () => {
  assert.equal(neutralize('ping @vistorx'), 'ping @\u200bvistorx');
  assert.equal(neutralize('fixes #3'), 'fixes #\u200b3');
  const issue = buildIssue(validate({ ...GOOD, description: '@everyone closes #1', contact: '@someone' }).value);
  assert.ok(!/@[a-z]/i.test(issue.title + issue.body.split('### Diagnostic report')[0]));
});

test('a report containing backticks cannot break out of its code block', () => {
  const report = REPORT + '```\n@vistorx\n';
  const issue = buildIssue(validate({ ...GOOD, report }).value);
  assert.ok(issue.body.includes('````text\n'));
});

test('the GitHub App token is a valid RS256 JWT from a PKCS#1 or PKCS#8 key', async () => {
  for (const pem of [PKCS1, PKCS8, PKCS1.replace(/\n/g, '\\n')]) {
    const jwt = await appJwt('123', pem, Date.UTC(2026, 9, 13));
    const [header, payload, signature] = jwt.split('.');
    const claims = JSON.parse(Buffer.from(payload, 'base64url'));
    assert.equal(claims.iss, '123');
    assert.ok(claims.exp - claims.iat <= 600);
    const verify = createVerify('RSA-SHA256');
    verify.update(`${header}.${payload}`);
    assert.ok(verify.verify(publicKey, Buffer.from(signature, 'base64url')));
  }
  assert.throws(() => pemToPkcs8('not a key'));
});

test('a valid report creates an issue and returns its link, with CORS', async () => {
  const github = fakeGitHub();
  const response = await handle(post(GOOD), ENV, { fetch: github.fetcher });
  assert.equal(response.status, 201);
  assert.deepEqual(await response.json(), { number: 7, url: 'https://github.com/vistorx/CatWandConnect/issues/7' });
  assert.equal(response.headers.get('Access-Control-Allow-Origin'), 'http://catwandconnect.local');
  const created = github.calls.find(c => c.url.endsWith('/issues') && c.method === 'POST');
  assert.equal(created.auth, 'Bearer ghs_test');
  assert.deepEqual(created.body.labels, ['from-device', 'needs-triage']);
});

test('preflight answers only the Device interface origins', async () => {
  const ok = await handle(new Request('https://relay.example/report', { method: 'OPTIONS', headers: { Origin: 'http://192.168.4.1' } }), ENV);
  assert.equal(ok.status, 204);
  assert.equal(ok.headers.get('Access-Control-Allow-Methods'), 'GET, POST, OPTIONS');
  const ko = await handle(new Request('https://relay.example/report', { method: 'OPTIONS', headers: { Origin: 'https://evil.example' } }), ENV);
  assert.equal(ko.status, 403);
  assert.equal(ko.headers.get('Access-Control-Allow-Origin'), null);
});

test('rejections: origin, method, format, size', async () => {
  const github = fakeGitHub();
  const deps = { fetch: github.fetcher };
  assert.equal((await handle(post(GOOD, { origin: 'https://evil.example' }), ENV, deps)).status, 403);
  assert.equal((await handle(new Request('https://relay.example/report'), ENV, deps)).status, 405);
  assert.equal((await handle(post('{nope'), ENV, deps)).status, 400);
  assert.equal((await handle(post({ ...GOOD, report: 'x' }), ENV, deps)).status, 400);
  assert.equal((await handle(post({ ...GOOD, report: REPORT + 'x'.repeat(30000) }), ENV, deps)).status, 413);
  assert.equal((await handle(new Request('https://relay.example/other'), ENV, deps)).status, 404);
  assert.equal(github.calls.length, 0);
});

test('the kill switch answers unavailable, and /status says so', async () => {
  const off = { ...ENV, ENABLED: 'false' };
  const response = await handle(post(GOOD), off, { fetch: fakeGitHub().fetcher });
  assert.equal(response.status, 503);
  assert.deepEqual(await response.json(), { error: 'unavailable' });
  const status = await handle(new Request('https://relay.example/status', { headers: { Origin: 'http://192.168.4.1' } }), off);
  assert.deepEqual(await status.json(), { available: false });
  const on = await handle(new Request('https://relay.example/status'), ENV);
  assert.deepEqual(await on.json(), { available: true });
});

test('per-IP rate limit and global daily cap', async () => {
  const seen = new Map();
  const PER_IP = { limit: async ({ key }) => { seen.set(key, (seen.get(key) || 0) + 1); return { success: seen.get(key) <= 2 }; } };
  const env = { ...ENV, PER_IP };
  const github = fakeGitHub();
  assert.equal((await handle(post(GOOD), env, { fetch: github.fetcher })).status, 201);
  assert.equal((await handle(post(GOOD), env, { fetch: github.fetcher })).status, 201);
  const limited = await handle(post(GOOD), env, { fetch: github.fetcher });
  assert.equal(limited.status, 429);
  assert.deepEqual(await limited.json(), { error: 'rate_limited' });
  assert.equal((await handle(post(GOOD, { ip: '5.6.7.8' }), env, { fetch: github.fetcher })).status, 201);

  const full = await handle(post(GOOD), ENV, { fetch: fakeGitHub({ today: 3 }).fetcher });
  assert.equal(full.status, 429);
  assert.deepEqual(await full.json(), { error: 'daily_cap' });
});

test('a GitHub failure answers 502 so the browser falls back to the prefilled issue', async () => {
  const response = await handle(post(GOOD), ENV, { fetch: fakeGitHub({ fail: true }).fetcher });
  assert.equal(response.status, 502);
  assert.deepEqual(await response.json(), { error: 'github_unavailable', detail: 'repos/vistorx: 500' });
});
