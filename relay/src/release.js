// Release notification (#58): when a release is published, post in every
// Discord thread linked to an issue that release closes. Pure helpers here;
// relay/scripts/notify-release.mjs runs them from the Release workflow.

import { threadLinks } from './discord.js';

// Issue and PR numbers in squash-merge titles: "Fix the thing (#62) (#64)".
export function referencedNumbers(messages) {
  const numbers = new Set();
  for (const message of messages) {
    const title = message.split('\n')[0];
    for (const match of title.matchAll(/\(#(\d+)\)/g)) numbers.add(Number(match[1]));
  }
  return [...numbers].sort((a, b) => a - b);
}

// The previous release tag on the same line: for a beta, the latest earlier
// tag of any kind; for a stable, the previous stable.
export function previousTag(tags, current) {
  const parse = tag => {
    const m = tag.match(/^v(\d+)\.(\d+)\.(\d+)(?:-beta\.(\d+))?$/);
    return m ? [Number(m[1]), Number(m[2]), Number(m[3]), m[4] === undefined ? Infinity : Number(m[4])] : null;
  };
  const cmp = (a, b) => { for (let i = 0; i < 4; i++) if (a[i] !== b[i]) return a[i] < b[i] ? -1 : 1; return 0; };
  const now = parse(current);
  if (!now) return null;
  const stable = now[3] === Infinity;
  const earlier = tags.map(tag => [tag, parse(tag)])
    .filter(([, v]) => v && cmp(v, now) < 0 && (!stable || v[3] === Infinity))
    .sort((a, b) => cmp(a[1], b[1]));
  return earlier.length ? earlier[earlier.length - 1][0] : null;
}

export function marker(tag) {
  return `[release ${tag}]`;
}

export function releaseMessage(tag, issue, prerelease) {
  const channel = prerelease ? 'Beta' : 'Stable';
  return [
    `🎉 The fix for [#${issue.number}](<${issue.url}>) ships in **${tag}** (${channel} channel).`,
    `Update from your robot: Settings → Robot → Firmware update, with the \`-ota.bin\` file of the release: <https://github.com/vistorx/CatWandConnect/releases/tag/${tag}>`,
    'Or reinstall it with the web flasher: <https://vistorx.github.io/CatWandConnect/>',
    `-# ${marker(tag)}`,
  ].join('\n');
}

// Discord thread IDs found in an issue body and its comments.
export function threadsOf(texts, guildId) {
  return [...new Set(texts.flatMap(text => threadLinks(text, guildId)))];
}
