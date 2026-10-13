// Registers the relay's /issue command on the CatWandConnect server.
// Run by the Support relay workflow: node scripts/register-commands.mjs
// Env: DISCORD_BOT_TOKEN, DISCORD_APPLICATION_ID, DISCORD_GUILD_ID.

import { COMMANDS } from '../src/interactions.js';

const { DISCORD_BOT_TOKEN, DISCORD_APPLICATION_ID, DISCORD_GUILD_ID } = process.env;
if (!DISCORD_BOT_TOKEN || !DISCORD_APPLICATION_ID || !DISCORD_GUILD_ID) {
  console.log('Discord secrets not set: /issue not registered.');
  process.exit(0);
}
const response = await fetch(`https://discord.com/api/v10/applications/${DISCORD_APPLICATION_ID}/guilds/${DISCORD_GUILD_ID}/commands`, {
  method: 'PUT',
  headers: { Authorization: `Bot ${DISCORD_BOT_TOKEN}`, 'Content-Type': 'application/json', 'User-Agent': 'DiscordBot (cwc, 1)' },
  body: JSON.stringify(COMMANDS),
});
console.log(`register commands: ${response.status}`);
if (!response.ok) {
  console.log(await response.text());
  process.exit(1);
}
