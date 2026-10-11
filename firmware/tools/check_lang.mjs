// Checks that every language file in web/lang/ has exactly the keys of
// en.json, and the same {placeholders}. Run by CI: `node tools/check_lang.mjs`.
import { readdirSync, readFileSync } from 'node:fs';

const dir = new URL('../web/lang/', import.meta.url);
const load = name => JSON.parse(readFileSync(new URL(name, dir), 'utf8'));
const placeholders = text => [...text.matchAll(/\{(\w+)\}/g)].map(m => m[1]).sort().join(',');

const reference = load('en.json');
let failed = false;
for (const file of readdirSync(dir).filter(f => f.endsWith('.json') && f !== 'en.json')) {
  const lang = load(file);
  for (const key of Object.keys(reference)) {
    if (!(key in lang)) { console.error(`${file}: missing "${key}"`); failed = true; }
    else if (placeholders(lang[key]) !== placeholders(reference[key])) {
      console.error(`${file}: "${key}" placeholders differ from en.json`); failed = true;
    }
  }
  for (const key of Object.keys(lang)) {
    if (!(key in reference)) { console.error(`${file}: "${key}" is not in en.json`); failed = true; }
  }
}
if (failed) process.exit(1);
console.log('Language files match en.json');
