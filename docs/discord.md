# Discord server

The [CatWandConnect Discord server](https://discord.gg/Rbqge3v9dk) is where people ask questions, get help with a build and share it, without needing a GitHub account. This page describes how it is organised, for maintainers. Users only need [SUPPORT.md](../SUPPORT.md).

## Invite

`https://discord.gg/Rbqge3v9dk` is the permanent invite (no expiry, no use limit); it lands in `#welcome`. Use only this link in the firmware, the web flasher and the docs. To check an invite has not been replaced by an expiring one:

```sh
curl -s "https://discord.com/api/v10/invites/Rbqge3v9dk?with_expiration=true"
```

`expires_at` must be `null`.

## Channels

| Category | Channel | Type | Purpose |
|---|---|---|---|
| Information | `#welcome` | Text, read-only | Pinned welcome message: web flasher, where to get help, security reporting. |
| Information | `#announcements` | Text, read-only | Firmware releases, posted automatically by GitHub. |
| Support | `#support` | Forum | One post per problem or question. Tags: `bug`, `installation`, `question`, `idea`, aligned with the GitHub labels. |
| Community | One channel per language | Text | Chat in the members' own language, named `<flag>・<language>` with a topic written in that language. Only languages the maintainers can moderate. |
| Community | `📸・showroom` | Forum, gallery | One post per build, with photos or a video. Tags: `Mk2`, `custom build`, `ESP32`, `ESP32-S3`, `ESP32-C3`, `video`. |

Read-only means `@everyone` is denied Send Messages, Send Messages in Threads and Create Public Threads.

## From Discord to GitHub

A `#support` post that turns out to be a confirmed bug becomes a GitHub issue that links back to the post, and the post gets the issue link. Questions stay on Discord. See [ADR 0009](adr/0009-support-flows-end-in-github-issues.md).

## Release announcements

GitHub posts each published release to `#announcements` through a Discord webhook:

1. In `#announcements`: Edit Channel → Integrations → Webhooks → New Webhook → Copy Webhook URL.
2. In the repository: Settings → Webhooks → Add webhook. Payload URL is the copied URL followed by `/github`, content type `application/json`, and only the **Releases** event.

The webhook URL is a secret: anyone who has it can post in the channel. Never paste it into an issue, a commit or a chat. If it leaks, delete the webhook in Discord and create a new one.

## Administration

The server is administered through a private bot application ("CatWandConnect Admin", Administrator permission, **Public Bot** off, its role at the top of the role list) using the Discord REST API. Its token is kept outside the repository, on the maintainer's machine. The Support relay uses a separate bot with only the permissions it needs, and its token lives only in the repository's Actions secrets.
