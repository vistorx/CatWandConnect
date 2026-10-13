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

To update a robot that is already installed, on either channel, download the `-ota.bin` file of the release you want and upload it in **Settings → Robot → Firmware update**. Settings and Calibration are kept.

## What a release does

Pushing a `vX.Y.Z` or `vX.Y.Z-beta.N` tag runs `.github/workflows/release.yml`:

1. Runs the Robot core tests, the language check and the flasher tests. If any fails, nothing is published.
2. Builds the three chips with `CWC_VERSION` set from the tag (`0.1.0` or `0.1.0-beta.1`).
3. Publishes a GitHub release with two files per chip, marked as a pre-release for a beta tag:
   - `catwandconnect-<chip>-factory.bin`: the full flash image, written at offset 0 by `esptool`. It also covers the settings partition (NVS) with blank bytes, so writing it whole wipes the robot's settings;
   - `catwandconnect-<chip>-ota.bin`: the app only, for **Settings → Robot → Firmware update**.
4. Starts the **Pages** workflow on `main`, which republishes the website and the web flasher with the new release (see [The website and the web flasher](#the-website-and-the-web-flasher)).

Running the Release workflow by hand (**Actions → Release → Run workflow**) is a dry run: it builds everything and keeps a flasher preview as an artifact, but publishes nothing.

## The website and the web flasher

The GitHub Pages site, <https://vistorx.github.io/CatWandConnect/>, is the project website, with the web flasher at <https://vistorx.github.io/CatWandConnect/flasher/>. Only `.github/workflows/pages.yml` deploys it:

1. `website/prepare_docs.py` stages the pages into `website/build/docs`: `README.md` becomes the home page, `docs/` is copied as is, `SUPPORT.md`, `SECURITY.md` and `CONTEXT.md` become *Getting help*, *Security* and *Glossary*, and `media/` is copied for the images. Links are rewritten for the site; links to files that are not pages (STL files, code, licences) go to github.com. Never edit `website/build/`.
2. MkDocs Material builds the site (`website/mkdocs.yml`, versions pinned in `website/requirements.txt`) with `--strict`, so a broken link or anchor fails the build.
3. `firmware/tools/flasher_site.py` adds the web flasher under `flasher/`: `flasher/index.html` and its logo, then `stable/` and `beta/` folders with each channel's images and manifest, and `channels.json`, all rebuilt from the published releases. Each factory image is split into a `-boot.bin` (offset 0, up to the settings partition) and an `-app.bin` (after it), and the manifest lists those two parts, so installing without "Erase device" keeps the robot's settings (`firmware/tools/flasher_manifest.py`).
4. The site is deployed to GitHub Pages. Runs share the `pages` concurrency group, so two deployments never overlap.

It runs when `main` changes the README, `SUPPORT.md`, `SECURITY.md`, `CONTEXT.md`, `docs/`, `media/`, `flasher/`, `website/` or the flasher tools, after each release, and by hand (**Actions → Pages → Run workflow**), for example to republish the flasher after deleting a bad release. On pull requests it only builds the site, to catch broken links.

To preview the site locally (`uvx` comes with [uv](https://docs.astral.sh/uv/); `pip install -r website/requirements.txt` works too):

```sh
python website/prepare_docs.py
uvx --with-requirements website/requirements.txt mkdocs serve -f website/mkdocs.yml
```

## Checklist

### Beta

1. Make sure `main` is green.
2. Tag the next beta and push it:

   ```sh
   git tag v0.1.0-beta.1
   git push origin v0.1.0-beta.1
   ```

3. Check that the release is marked as a pre-release, that the Pages workflow ran, and that the web flasher's Beta channel shows the new version.
4. Install it (web flasher, or OTA from Settings) and run the [acceptance checklist](acceptance-checklist.md). Each fix gets a new beta.

### Stable

1. The acceptance checklist passed on the last beta, and `main` is green.
2. Tag and push:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

3. Check the release page, and edit its notes if needed (they are generated from merged PRs).
4. Open the web flasher: Stable is selected by default and shows the new version. Install on a board to check it.

## One-time setup for GitHub Pages

GitHub Pages is free for public repositories. Once the repository is public:

1. **Settings → Pages → Build and deployment → Source: GitHub Actions.**
2. **Settings → Environments → github-pages → Deployment branches and tags:** allow `main` (and a tag rule `v*`).

The website is then served at <https://vistorx.github.io/CatWandConnect/> and the flasher at <https://vistorx.github.io/CatWandConnect/flasher/>. Each release rebuilds them with the current Stable and Beta.

## Building release files locally

`pio run -e <chip>` writes `firmware.bin` (OTA) and `factory.bin` (full image) to `firmware/.pio/build/<chip>/`, or to `$PLATFORMIO_BUILD_DIR/<chip>/`. Without `CWC_VERSION`, the version comes from `git describe`, for example `0.1.0-3-gabc1234`.
