# Support relay

The Support relay lets people without a GitHub account report a bug from their robot. The owner's browser sends the redacted Diagnostic report to the relay, and the relay creates the GitHub issue as a GitHub App. The robot never calls it (see [ADR 0009](adr/0009-support-flows-end-in-github-issues.md)).

It is a Cloudflare Worker whose code lives in [`relay/`](../relay). The [Support relay workflow](../.github/workflows/relay.yml) tests it and deploys it whenever `relay/` changes on `main`, or when it is run by hand from the Actions tab.

## API

| Request | Answer |
|---|---|
| `GET /status` | `{"available": true}`, or `false` while the kill switch is on. |
| `POST /report` with `{"description": "...", "report": "...", "contact": "..."}` | `201 {"number": 12, "url": "https://github.com/…/issues/12"}` |

`report` must be a redacted Diagnostic report: it starts with `CatWandConnect diagnostic report` and `Redacted: yes`, and is at most 14 KB. `description` is required (2,000 characters at most) and `contact` is optional (100 characters). The request body is limited to 20 KB.

Errors are returned as `{"error": "<code>"}`:

| Status | Code | Meaning |
|---|---|---|
| 400 | `invalid_report`, `invalid_description`, `invalid_contact` | The request does not have the expected format. |
| 403 | `origin_not_allowed` | The page is not a Device interface. |
| 413 | `too_large` | The body is over 20 KB. |
| 429 | `rate_limited`, `daily_cap` | Too many reports from this address, or for the day. |
| 503 | `unavailable` | The kill switch is on. |
| 502 | `github_unavailable` | GitHub refused or did not answer. |

On any error the Device interface falls back to the prefilled GitHub issue.

**CORS.** Only pages served over `http://` from a `.local` name, `localhost` or a private IPv4 address (`10.x`, `172.16–31.x`, `192.168.x`, which covers the hotspot `192.168.4.1`) get an `Access-Control-Allow-Origin` header.

**Issues.** Each issue is titled `[device] <first line of the description>` and labelled `from-device` and `needs-triage`. It contains the description, firmware, chip, the robot's random Device ID (never its MAC address), the optional contact, and the full report in a code block. `@` mentions and `fixes #N` keywords in user text are neutralised.

**Abuse limits.** There is no captcha, because Cloudflare Turnstile cannot run on a `.local` page. Instead the relay allows 3 reports per minute per IP address, a global cap of `DAILY_CAP` issues per UTC day (counted on GitHub with the `from-device` label, 30 by default), and the format and size checks above.

## Setup (maintainer, once)

The agent never sees the secrets. They are pasted only into the repository's Actions secrets.

1. **Cloudflare.**
   - In Workers & Pages, pick a `workers.dev` subdomain if the account has none. The relay will be `https://catwandconnect-support.<subdomain>.workers.dev`.
   - Copy the **Account ID** from the account home page.
   - Under My Profile → API Tokens → Create Token, use the **Edit Cloudflare Workers** template, limited to this account.
2. **GitHub App.** Under Settings → Developer settings → GitHub Apps → New GitHub App:
   - Name it `CatWandConnect Support`, with the repository URL as its homepage.
   - Untick **Webhook → Active**.
   - Set Repository permissions → **Issues: Read and write**.
   - Choose "Only on this account", then create it.
   - Note the **App ID**, then **Generate a private key**: a `.pem` file downloads.
   - **Install App** on `vistorx/CatWandConnect` only.
3. **Secrets.** Under Settings → Secrets and variables → Actions → New repository secret, add:
   - `CLOUDFLARE_ACCOUNT_ID`
   - `CLOUDFLARE_API_TOKEN`
   - `SUPPORT_APP_ID`
   - `SUPPORT_APP_PRIVATE_KEY`: the whole `.pem` file, including its `BEGIN` and `END` lines.

   Then delete the downloaded `.pem`. A new one can be generated at any time.
4. **Deploy.** Run the *Support relay* workflow (Actions tab → Run workflow). Its log ends with the relay's URL and `{"available":true}`.
5. **Wire it.** Put the relay URL in `firmware/web/links.js` (`relay`). The Help page then checks `GET /status` and offers "Send to support" next to the prefilled issue. With `relay` empty, the relay unreachable or the kill switch on, the Help page offers only the prefilled issue.

## Operating it

- **Kill switch.** In the Cloudflare dashboard, open Workers → `catwandconnect-support` → Settings → Variables and set `ENABLED` to `false`. This takes effect immediately. Then set it in `relay/wrangler.toml` too, so the next deploy keeps it. Robots fall back to the prefilled issue.
- **Daily cap.** Change `DAILY_CAP` the same way.
- **Logs.** Cloudflare dashboard → the Worker → Observability. The relay logs only GitHub errors, never reports.
- **Rotating the GitHub key.** Generate a new private key on the App, replace `SUPPORT_APP_PRIVATE_KEY`, run the workflow, then delete the old key on the App.
- **Tests.** Run `cd relay && npm test`. They also run in CI with the Robot core tests.
