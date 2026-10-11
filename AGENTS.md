# AGENTS.md

CatWandConnect is an open-hardware cat toy robot: an ESP32 drives a wand on two servos. The dedicated firmware (Arduino + PlatformIO) is being built ticket by ticket. These instructions apply to every coding agent, whichever tool or model runs it.

## Before you change anything

- Read `CONTEXT.md` and use its terms everywhere: code, tests, issues, commits. Each entry lists the synonyms to avoid.
- Read the ADRs in `docs/adr/` that touch your area. If your change contradicts one, say so explicitly in the PR rather than overriding it silently.
- Work from a GitHub issue. The V1 spec is issue #1. Its sub-issues are the tickets, and a ticket is ready when all its "blocked by" issues are closed.

## How to work

- Write Robot core logic test-first. The core is pure C++ with injected motor driver, clock and storage, and it is tested on the host with PlatformIO's `native` environment. That is the single test seam.
- Behaviour that only real hardware can show goes in the physical acceptance checklist.
- One ticket per branch and PR. Reference the ticket in the PR.
- When you settle a new term or decision, update `CONTEXT.md` or add an ADR in the same PR.
- Target the ESP32 family only (ESP32, ESP32-S3, ESP32-C3). ESP8266 support is out of scope (ADR 0008); close requests for it with a link to that ADR.
- `README.md`, `docs/assembly.md`, `docs/electronics.md` and `docs/troubleshooting.md` are stale (they describe ESPHome) until issue #3 lands. Trust `CONTEXT.md`, the ADRs and the issues instead.

## Agent skills

Claude Code loads the `mattpocock-skills` plugin for this workspace. Other agents can follow the same conventions by reading these files directly.

### Issue tracker

Issues are tracked in GitHub Issues on `vistorx/CatWandConnect` (via the `gh` CLI). See `docs/agents/issue-tracker.md`.

### Triage labels

Default canonical labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
