# Getting started

How to pick up CatWandConnect on a new machine, whether you are the maintainer or a new contributor.

## Where things stand

The dedicated firmware in `firmware/` replaces the old ESPHome proof of concept. It already provides Wi-Fi provisioning, Manual control, speed settings, Stow and a gentle power-up. The remaining V1 work is tracked under [issue #1](https://github.com/vistorx/CatWandConnect/issues/1). None of it has been run on real hardware yet; [`acceptance-checklist.md`](acceptance-checklist.md) lists what to verify.

Some docs are out of date: `README.md`, `docs/assembly.md`, `docs/electronics.md` and `docs/troubleshooting.md` still describe ESPHome and a paid V2. [#3](https://github.com/vistorx/CatWandConnect/issues/3) rewrites them. Until then, trust the sources below.

## What to read first

| Read | For |
|------|-----|
| [`CONTEXT.md`](../CONTEXT.md) | The glossary. Use these words in code, issues and docs. |
| [`docs/adr/`](adr/) | Why the big decisions were made: open everything, standalone first, Arduino + PlatformIO, MQTT for Home Assistant, licences, plain web interface. |
| [Issue #1](https://github.com/vistorx/CatWandConnect/issues/1) | The V1 spec. Its sub-issues are the tickets, with native "blocked by" links. |
| [`AGENTS.md`](../AGENTS.md) and [`docs/agents/`](agents/) | How AI agents work with the issue tracker, labels and domain docs. |

## What is in the repo and what is not

In the repo:

- Domain docs, ADRs and agent config.
- Printable parts: Mk2 in `stls/mk2/` (reference), Mk1 in `archive/stls/mk1/` (retired).
- The Claude Code project settings that enable the `mattpocock-skills` plugin (`.claude/settings.json`).

Not in the repo:

- **CAD sources.** Only STL/3MF exports are committed, and `cads/` is empty. To change a part, you need the original CAD project.
- **The ESPHome PoC.** It was removed in `fa58005`. Recover it with `git show 3cda6f7:sources/cat-playing-platform.yaml`. It is the reference for the PoC motion logic.
- **Secrets.** None are committed, and the new firmware does not need any at build time. Wi-Fi credentials are entered on the device.
- **Per-machine setup.** Your SSH key, GitHub login, git identity and `.claude/settings.local.json` must be recreated on each machine. See below.

## Set up a machine

### 1. Tools

- Git and the [GitHub CLI](https://cli.github.com/) (`gh`).
- VS Code with the **PlatformIO IDE** extension. It provides the toolchain for ESP32, ESP32-S3 and ESP32-C3, plus the native test environment.
- Optional: [Claude Code](https://claude.com/claude-code). The `mattpocock-skills` plugin is enabled for this workspace by the committed settings. Accept the install prompt the first time.

### 2. Clone

Contributors can clone normally:

```sh
git clone git@github.com:vistorx/CatWandConnect.git
```

Do not clone into a folder synced by Nextcloud, Dropbox or iCloud. Syncing a `.git` folder between machines corrupts it. Each machine gets its own clone, and git is the sync.

### 3. Maintainer only: use the `vistorx` account on a machine that has another GitHub account

On a work machine, the default GitHub account is someone else's. These steps keep `vistorx` scoped to this repo only.

**SSH key, with an alias host so the default `github.com` is untouched:**

```sh
ssh-keygen -t ed25519 -C "vistorx" -f ~/.ssh/github_vistorx
# Add ~/.ssh/github_vistorx.pub at https://github.com/settings/keys (logged in as vistorx)
```

Append this to `~/.ssh/config`. Port 443 gets through networks that block port 22.

```
Host github-vistorx
  HostName ssh.github.com
  Port 443
  User git
  IdentityFile ~/.ssh/github_vistorx
  IdentitiesOnly yes
  AddKeysToAgent yes
  UseKeychain yes
```

Check it with `ssh -T github-vistorx`, which should greet `vistorx`.

**Clone through the alias and set a repo-local identity:**

```sh
git clone git@github-vistorx:vistorx/CatWandConnect.git
cd CatWandConnect
git config user.name "vistorx"
git config user.email "vistorx@users.noreply.github.com"
```

**A separate `gh` login, stored in its own config folder:**

```sh
GH_CONFIG_DIR=~/.config/gh-vistorx gh auth login --web
```

Then point this workspace at that folder:

- For Claude Code, create `.claude/settings.local.json`. It is git-ignored.

  ```json
  { "env": { "GH_CONFIG_DIR": "/Users/<you>/.config/gh-vistorx" } }
  ```

- For the VS Code terminal, add the following to the `settings` of `CatWandConnect.code-workspace`, and keep that change uncommitted:

  ```json
  "terminal.integrated.env.osx": { "GH_CONFIG_DIR": "${env:HOME}/.config/gh-vistorx" }
  ```

Check it with `gh auth status` in the workspace terminal, which should show `vistorx`.

### 4. Build and test

The firmware is one PlatformIO project in `firmware/`. Run these from that folder, or use the PlatformIO sidebar in VS Code. The VS Code extension installs `pio` at `~/.platformio/penv/bin/pio`.

```sh
pio test -e native     # Robot core tests on your computer, no board needed
pio run                # build for ESP32, ESP32-S3 and ESP32-C3
pio run -e esp32 -t upload -t monitor   # flash a board over USB and watch its logs
```

To work on the Device interface without a robot, run `node tools/dev_server.mjs` (Node 18+, no install needed) and open <http://localhost:8080/>. It serves `firmware/web/` with a rough simulated robot behind the same API.

The environments are `esp32`, `esp32s3`, `esp32c3` and `native`. CI (`.github/workflows/ci.yml`) runs the tests and the three builds on every push and pull request.

Code layout:

- `firmware/lib/robot_core/` is the Robot core. It is pure C++17 and never includes Arduino headers. It reaches hardware only through the `MotorDriver`, `Clock` and `Storage` interfaces in `ports.h`.
- `firmware/src/` is the Arduino side. It implements those interfaces and wires them to the Robot core.
- `firmware/src/network/` gets the robot onto Wi-Fi: Improv provisioning, the fallback hotspot with its setup page, and mDNS. `firmware/lib/improv/` is the Improv protocol framing, as pure, tested C++.
- `firmware/web/` is the Device interface: plain HTML, CSS and JS with no build step. Text lives in `web/lang/*.json`. `tools/embed_web.py` compresses it into the firmware at build time. The interface talks to the robot only through the [Device API](api.md).
- `firmware/test/` holds the native tests, with in-memory fakes of the interfaces.
- [`docs/acceptance-checklist.md`](acceptance-checklist.md) lists what to check on the real robot. Extend it whenever you add behaviour that tests cannot cover.

If your clone sits in a synced folder, set `PLATFORMIO_BUILD_DIR` to a folder outside it. Builds write hundreds of megabytes to `firmware/.pio/`.

## Working with an AI agent

You can use any coding agent. The project's instructions live in [`AGENTS.md`](../AGENTS.md), the cross-tool convention, and everything it points to is plain Markdown.

| Tool | How it picks up `AGENTS.md` |
|------|-----------------------------|
| Claude Code | Automatically. It also loads the `mattpocock-skills` plugin (`/tdd`, `/triage`, …). |
| OpenAI Codex, Cursor, GitHub Copilot (VS Code), Zed, Jules | Automatically. |
| Aider | Through the committed `.aider.conf.yml`. |
| Gemini CLI | Set `"contextFileName": "AGENTS.md"` in `.gemini/settings.json` (local, or in your user settings). |
| Anything else | Tell it to read `AGENTS.md` first. |

The slash-command skills are specific to Claude Code. Agents without them follow the same conventions by reading `docs/agents/`, `CONTEXT.md` and the ADRs.

## How work flows

1. Pick an open ticket under #1 that has no open blocker. Tickets labelled `ready-for-agent` are fully specified.
2. Branch from `main`, implement test-first where the Robot core is involved, and open a PR that references the ticket.
3. CI must pass. Behaviour that tests cannot cover goes in the physical acceptance checklist (from #3).
4. If you settle a new term or decision, update `CONTEXT.md` or add an ADR in the same PR.

Hardware you need to test on the robot: an ESP32, ESP32-S3 or ESP32-C3 board, two MG996R servos, a 5 V supply that can deliver several amps, and a printed Mk2 Mechanism.

**ESP8266 boards are not supported, and almost certainly never will be.** See [ADR 0008](adr/0008-no-esp8266-support.md).
