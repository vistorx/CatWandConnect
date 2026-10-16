# Releasing

How to publish a new firmware version. Versions follow [SemVer](https://semver.org/): `vMAJOR.MINOR.PATCH`.

## Channels

There are two release channels:

| Channel | Tag | GitHub release | Who it is for |
|---------|-----|----------------|---------------|
| **Beta** | `vX.Y.Z-beta.N` | pre-release | Testing a version on real robots before it goes stable |
| **Stable** | `vX.Y.Z` | latest release | Everyone; the web flasher's default |

A version normally goes through one or more betas (`v0.2.0-beta.1`, `v0.2.0-beta.2`…), then becomes stable once the [acceptance checklist](acceptance-checklist.md) passes on a beta. The stable tag points at the same commit as the last good beta, or at a later commit with only fixes from that beta's testing.

The web flasher offers both channels: **Stable** is the newest non-pre-release, **Beta** the newest release of any kind. Publishing a beta never replaces the stable install. Before the first stable release, the flasher offers Beta only.

A robot that is already installed is offered the newest version of its own Update channel (Settings → Robot → Firmware update) on its main screen, and installs it in one tap; the browser reads the Update feed (`flasher/channels.json`) and downloads the `-ota.bin` copied there ([ADR 0012](adr/0012-the-browser-fetches-updates-for-the-robot.md)). Uploading a release's `-ota.bin` by hand still works. Settings and Calibration are kept.

## What a release does

Pushing a `vX.Y.Z` or `vX.Y.Z-beta.N` tag runs `.github/workflows/release.yml`:

1. Runs the Robot core tests, the language check and the flasher tests. If any fails, nothing is published.
2. Builds the three chips with `CWC_VERSION` set from the tag (`0.1.0` or `0.1.0-beta.1`).
3. Waits for the owner's approval: the publishing job runs in the `release` environment, whose only required reviewer is the owner. GitHub emails a review request; open the run (**Actions → Release**) and click **Review deployments → Approve**. Until then nothing is published, so a tag pushed by mistake (or by an agent) has no effect; reject the run and delete the tag.
4. Publishes a GitHub release with two files per chip, marked as a pre-release for a beta tag:
   - `catwandconnect-<chip>-factory.bin`: the full flash image, written at offset 0 by `esptool`. It also covers the settings partition (NVS) with blank bytes, so writing it whole wipes the robot's settings;
   - `catwandconnect-<chip>-ota.bin`: the app only, for **Settings → Robot → Firmware update**.
5. Starts the **Pages** workflow on `main`, which republishes the website and the web flasher with the new release (see [The website and the web flasher](#the-website-and-the-web-flasher)).

Running the Release workflow by hand (**Actions → Release → Run workflow**) is a dry run: it builds everything and keeps a flasher preview as an artifact, but publishes nothing.

## The website and the web flasher

The GitHub Pages site, <https://vistorx.github.io/CatWandConnect/>, is the project website, with the web flasher at <https://vistorx.github.io/CatWandConnect/flasher/>. Only `.github/workflows/pages.yml` deploys it:

1. `website/prepare_docs.py` stages the pages into `website/build/docs`: `README.md` becomes the home page, `docs/` is copied as is, `SUPPORT.md`, `SECURITY.md` and `CONTEXT.md` become *Getting help*, *Security* and *Glossary*, and `media/` is copied for the images. French pages sit next to their English page (`README.fr.md`, `SUPPORT.fr.md`, `docs/<page>.fr.md`) and are built under `/fr/`; contributor docs have no French page and the French site shows them in English. `website/check_translations.py` fails the build when a user page has no French page. Links are rewritten for the site; links to files that are not pages (STL files, code, licences) go to github.com. Never edit `website/build/`.
2. MkDocs Material builds the site (`website/mkdocs.yml`, versions pinned in `website/requirements.txt`) with `--strict`, so a broken link or anchor fails the build.
3. `firmware/tools/flasher_site.py` adds the web flasher under `flasher/`: `flasher/index.html` and its logo, then `stable/` and `beta/` folders with each channel's images, manifest and `-ota.bin` files, and `channels.json`, all rebuilt from the published releases. `channels.json` is also the Update feed robots are offered updates from (version, release notes link, the pitch of the version's `CHANGELOG.md` section as `summary`, and the `-ota.bin` path per chip family; see *Update feed* in [the Device API](api.md)). Each factory image is split into a `-boot.bin` (offset 0, up to the settings partition) and an `-app.bin` (after it), and the manifest lists those two parts, so installing without "Erase device" keeps the robot's settings (`firmware/tools/flasher_manifest.py`).
4. The site is deployed to GitHub Pages. Runs share the `pages` concurrency group, so two deployments never overlap.

It runs when `main` changes the README, `SUPPORT.md`, `SECURITY.md`, `CONTEXT.md`, `docs/`, `media/`, `flasher/`, `website/` or the flasher tools, after each release, and by hand (**Actions → Pages → Run workflow**), for example to republish the flasher after deleting a bad release. On pull requests it only builds the site, to catch broken links.

To preview the site locally (`uvx` comes with [uv](https://docs.astral.sh/uv/); `pip install -r website/requirements.txt` works too):

```sh
python website/prepare_docs.py
uvx --with-requirements website/requirements.txt mkdocs serve -f website/mkdocs.yml
```

## Changelog

`CHANGELOG.md` is what owners read before updating, so it sells the update: what they can now do and why it is worth it, in plain words, not a list of PRs.

- A PR with a change owners notice adds a line under `## Unreleased` (feature, visible fix, new requirement). Internal work (CI, refactors, contributor docs) doesn't.
- Before tagging, rename `## Unreleased` to `## X.Y.Z[-beta.N] - YYYY-MM-DD`, add a fresh empty `## Unreleased` above it, and shape the section:
  1. a one-paragraph **pitch**: the headline in bold, then what it means for the owner and their cat;
  2. `### Highlights`: one bullet per change, **bold name**, then the benefit; link the guide;
  3. optional `### Update` (requirements, steps, anything to do after updating) and `### Fixes`.
- A stable version that follows betas gets one section summing up all of them since the last stable.
- The release workflow publishes that section as the GitHub release notes (`firmware/tools/release_notes.py`), with the support footer and the compare link, and posts the pitch and highlights in Discord `#announcements`. **A tag without a section fails the release before anything is published.**

## Checklist

### Beta

1. Make sure `main` is green, and that `CHANGELOG.md` has the version's section (see [Changelog](#changelog)), merged.
2. Tag the next beta and push it:

   ```sh
   git tag v0.1.0-beta.1
   git push origin v0.1.0-beta.1
   ```

3. Approve the run in **Actions → Release → Review deployments** once the builds pass.
4. Check that the release is marked as a pre-release with the changelog as its notes, the Discord announcement, that the Pages workflow ran, and that the web flasher's Beta channel shows the new version.
5. Install it (web flasher, or OTA from Settings) and run the [acceptance checklist](acceptance-checklist.md). Each fix gets a new beta.

### Stable

1. The acceptance checklist passed on the last beta, `main` is green, and `CHANGELOG.md` has the stable version's section.
2. Tag and push:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

3. Approve the run in **Actions → Release → Review deployments**.
4. Check the release page and the Discord announcement.
5. Open the web flasher: Stable is selected by default and shows the new version. Install on a board to check it.

## One-time setup for GitHub Pages

GitHub Pages is free for public repositories. Once the repository is public:

1. **Settings → Pages → Build and deployment → Source: GitHub Actions.**
2. **Settings → Environments → github-pages → Deployment branches and tags:** allow `main` (and a tag rule `v*`).
3. **Settings → Environments → release:** required reviewer = the owner, *Prevent self-review* off, deployment tags `v*` only.

The website is then served at <https://vistorx.github.io/CatWandConnect/> and the flasher at <https://vistorx.github.io/CatWandConnect/flasher/>. Each release rebuilds them with the current Stable and Beta.

## Building release files locally

`pio run -e <chip>` writes `firmware.bin` (OTA) and `factory.bin` (full image) to `firmware/.pio/build/<chip>/`, or to `$PLATFORMIO_BUILD_DIR/<chip>/`. Without `CWC_VERSION`, the version comes from `git describe`, for example `0.1.0-3-gabc1234`.
