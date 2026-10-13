// The few Discord REST calls the Support relay makes, with the relay's own
// least-privilege bot (docs/support-relay.md). Never the admin bot.

const API = 'https://discord.com/api/v10';

export const threadUrl = (guildId, threadId) => `https://discord.com/channels/${guildId}/${threadId}`;

// Every link to a Discord thread of this server in a text.
export function threadLinks(text, guildId) {
  const found = new Set();
  for (const match of String(text || '').matchAll(/https:\/\/(?:www\.|ptb\.|canary\.)?discord(?:app)?\.com\/channels\/(\d+)\/(\d+)/g)) {
    if (match[1] === String(guildId)) found.add(match[2]);
  }
  return [...found];
}

export async function discord(env, method, path, body, fetcher = fetch) {
  const response = await fetcher(`${API}${path}`, {
    method,
    headers: {
      Authorization: `Bot ${env.DISCORD_BOT_TOKEN}`,
      'User-Agent': 'DiscordBot (https://github.com/vistorx/CatWandConnect, 1)',
      ...(body ? { 'Content-Type': 'application/json' } : {}),
    },
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = response.status === 204 ? {} : await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error(`Discord ${method} ${path} answered ${response.status}: ${data.message || ''}`);
    error.detail = `discord ${path.split('/')[1]}: ${response.status}`;
    throw error;
  }
  return data;
}

// The member whose username or display name is exactly `name`, if any. Needs
// the Server Members intent; without it, the caller keeps the plain text.
export async function findMember(env, name, fetcher = fetch) {
  const wanted = name.replace(/^@/, '').trim().toLowerCase();
  if (!/^[a-z0-9_.]{2,32}$/.test(wanted)) return null;
  try {
    const members = await discord(env, 'GET', `/guilds/${env.DISCORD_GUILD_ID}/members/search?query=${encodeURIComponent(wanted)}&limit=5`, null, fetcher);
    const member = members.find(m => [m.user.username, m.user.global_name, m.nick].some(n => n && n.toLowerCase() === wanted));
    return member ? member.user.id : null;
  } catch {
    return null;
  }
}
