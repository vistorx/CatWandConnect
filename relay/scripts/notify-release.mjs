// Run by the Release workflow once a release is published:
//   node scripts/notify-release.mjs <tag>
// Env: GITHUB_TOKEN, GITHUB_REPOSITORY, DISCORD_BOT_TOKEN, DISCORD_GUILD_ID.
// Posts once per thread and release (a marker line makes reruns harmless).

import { discord } from '../src/discord.js';
import { marker, previousTag, referencedNumbers, releaseMessage, threadsOf } from '../src/release.js';

const tag = process.argv[2];
const { GITHUB_TOKEN, GITHUB_REPOSITORY: repo, DISCORD_BOT_TOKEN, DISCORD_GUILD_ID } = process.env;
if (!tag || !GITHUB_TOKEN || !repo) throw new Error('usage: notify-release.mjs <tag> with GITHUB_TOKEN and GITHUB_REPOSITORY');
if (!DISCORD_BOT_TOKEN) {
  console.log('DISCORD_BOT_TOKEN not set: nothing to notify.');
  process.exit(0);
}

async function gh(path, init = {}) {
  const response = await fetch(`https://api.github.com${path}`, {
    ...init,
    headers: { Accept: 'application/vnd.github+json', Authorization: `Bearer ${GITHUB_TOKEN}`, 'User-Agent': 'CatWandConnect-release', ...init.headers },
  });
  if (!response.ok) throw new Error(`GitHub ${path}: ${response.status}`);
  return response.json();
}

async function closingIssues(pr) {
  const [owner, name] = repo.split('/');
  const data = await gh('/graphql', {
    method: 'POST',
    body: JSON.stringify({
      query: 'query($o:String!,$n:String!,$p:Int!){repository(owner:$o,name:$n){pullRequest(number:$p){closingIssuesReferences(first:20){nodes{number}}}}}',
      variables: { o: owner, n: name, p: pr },
    }),
  });
  return data.data?.repository?.pullRequest?.closingIssuesReferences?.nodes.map(n => n.number) ?? [];
}

const env = { DISCORD_BOT_TOKEN };
const release = await gh(`/repos/${repo}/releases/tags/${tag}`);
const tags = (await gh(`/repos/${repo}/tags?per_page=100`)).map(t => t.name);
const previous = previousTag(tags, tag);
const commits = previous
  ? (await gh(`/repos/${repo}/compare/${previous}...${tag}`)).commits
  : (await gh(`/repos/${repo}/commits?sha=${tag}&per_page=100`));
const numbers = referencedNumbers(commits.map(c => c.commit.message));
console.log(`${tag}: ${commits.length} commits since ${previous || 'the start'}, references ${numbers.join(', ') || 'none'}`);

const issues = new Map();
for (const number of numbers) {
  const item = await gh(`/repos/${repo}/issues/${number}`);
  const closed = item.pull_request ? await closingIssues(number) : [number];
  for (const n of closed) {
    if (issues.has(n)) continue;
    const issue = n === number ? item : await gh(`/repos/${repo}/issues/${n}`);
    if (issue.pull_request || issue.state !== 'closed') continue;
    const comments = await gh(`/repos/${repo}/issues/${n}/comments?per_page=100`);
    issues.set(n, { number: n, url: issue.html_url, texts: [issue.body || '', ...comments.map(c => c.body || '')] });
  }
}

let posted = 0;
for (const issue of issues.values()) {
  for (const thread of threadsOf(issue.texts, DISCORD_GUILD_ID)) {
    try {
      const recent = await discord(env, 'GET', `/channels/${thread}/messages?limit=50`);
      if (recent.some(m => (m.content || '').includes(marker(tag)))) continue;
      await discord(env, 'POST', `/channels/${thread}/messages`, {
        content: releaseMessage(tag, issue, release.prerelease),
        allowed_mentions: { parse: [] },
      });
      posted += 1;
      console.log(`notified thread ${thread} for #${issue.number}`);
    } catch (error) {
      console.log(`thread ${thread} (#${issue.number}): ${error.message}`);
    }
  }
}
console.log(`${posted} thread(s) notified.`);
