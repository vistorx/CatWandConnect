// GitHub App authentication and the few GitHub API calls the Support relay
// makes. Works in a Cloudflare Worker and in Node (WebCrypto + fetch).

const API = 'https://api.github.com';

const encoder = new TextEncoder();

function base64url(bytes) {
  let binary = '';
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

function derLength(length) {
  if (length < 0x80) return [length];
  const bytes = [];
  for (let n = length; n > 0; n >>= 8) bytes.unshift(n & 0xff);
  return [0x80 | bytes.length, ...bytes];
}

function der(tag, content) {
  return Uint8Array.from([tag, ...derLength(content.length), ...content]);
}

// GitHub hands out PKCS#1 keys ("BEGIN RSA PRIVATE KEY"); WebCrypto only
// imports PKCS#8, so wrap the PKCS#1 key in a PKCS#8 envelope.
export function pkcs1ToPkcs8(pkcs1) {
  const version = [0x02, 0x01, 0x00];
  const rsaEncryption = [0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00];
  const octets = der(0x04, pkcs1);
  return der(0x30, Uint8Array.from([...version, ...rsaEncryption, ...octets]));
}

// Accepts the .pem file as pasted into a secret: PKCS#1 or PKCS#8, real or
// escaped ("\n") line breaks.
export function pemToPkcs8(pem) {
  const text = pem.replace(/\\n/g, '\n').trim();
  const match = text.match(/-----BEGIN ((?:RSA )?PRIVATE KEY)-----([\s\S]+?)-----END \1-----/);
  if (!match) throw new Error('private key is not a PEM private key');
  const bytes = Uint8Array.from(atob(match[2].replace(/\s+/g, '')), c => c.charCodeAt(0));
  return match[1] === 'RSA PRIVATE KEY' ? pkcs1ToPkcs8(bytes) : bytes;
}

// A short-lived JSON Web Token identifying the GitHub App.
export async function appJwt(appId, privateKeyPem, now = Date.now()) {
  const key = await crypto.subtle.importKey('pkcs8', pemToPkcs8(privateKeyPem),
    { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-256' }, false, ['sign']);
  const seconds = Math.floor(now / 1000);
  const header = base64url(encoder.encode(JSON.stringify({ alg: 'RS256', typ: 'JWT' })));
  // Backdated a minute against clock drift; GitHub refuses more than 10 minutes.
  const payload = base64url(encoder.encode(JSON.stringify({ iat: seconds - 60, exp: seconds + 540, iss: String(appId) })));
  const signature = await crypto.subtle.sign('RSASSA-PKCS1-v1_5', key, encoder.encode(`${header}.${payload}`));
  return `${header}.${payload}.${base64url(new Uint8Array(signature))}`;
}

async function github(fetcher, method, path, token, body) {
  const response = await fetcher(`${API}${path}`, {
    method,
    headers: {
      Accept: 'application/vnd.github+json',
      Authorization: `Bearer ${token}`,
      'User-Agent': 'CatWandConnect-support-relay',
      'X-GitHub-Api-Version': '2022-11-28',
      ...(body ? { 'Content-Type': 'application/json' } : {}),
    },
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(`GitHub ${method} ${path} answered ${response.status}: ${data.message || ''}`);
  return data;
}

// An installation token for the repository, valid one hour.
export async function installationToken(env, fetcher = fetch) {
  const jwt = await appJwt(env.SUPPORT_APP_ID, env.SUPPORT_APP_PRIVATE_KEY);
  const installation = await github(fetcher, 'GET', `/repos/${env.REPOSITORY}/installation`, jwt);
  const { token } = await github(fetcher, 'POST', `/app/installations/${installation.id}/access_tokens`, jwt);
  return token;
}

// How many issues the relay created today (UTC), for the daily cap.
export async function reportsToday(env, token, fetcher = fetch, now = new Date()) {
  const day = now.toISOString().slice(0, 10);
  const query = encodeURIComponent(`repo:${env.REPOSITORY} is:issue label:from-device created:>=${day}`);
  const result = await github(fetcher, 'GET', `/search/issues?q=${query}&per_page=1`, token);
  return result.total_count;
}

export async function createIssue(env, token, issue, fetcher = fetch) {
  const created = await github(fetcher, 'POST', `/repos/${env.REPOSITORY}/issues`, token, issue);
  return { number: created.number, url: created.html_url };
}
