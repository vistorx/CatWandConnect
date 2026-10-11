# Releasing

How to publish a new firmware version. Versions follow [SemVer](https://semver.org/): `vMAJOR.MINOR.PATCH`.

## What a release does

Pushing a `vX.Y.Z` tag runs `.github/workflows/release.yml`:

1. Runs the Robot core tests and the language check. If either fails, nothing is published.
2. Builds the three chips with `CWC_VERSION=X.Y.Z`.
3. Publishes a GitHub release with two files per chip:
   - `catwandconnect-<chip>-factory.bin`: the full flash image, written at offset 0 by the web flasher or `esptool`;
   - `catwandconnect-<chip>-ota.bin`: the app only, for **Settings → Firmware update**.
4. Publishes the web flasher (`flasher/index.html`, a generated `manifest.json` and the factory images) to GitHub Pages. This step only runs once the repository is public.

Running the workflow by hand (**Actions → Release → Run workflow**) is a dry run: it builds everything and keeps the flasher site as an artifact, but publishes nothing.

## Checklist

1. Run the [acceptance checklist](acceptance-checklist.md) on a real robot, with a build of the commit you will tag.
2. Make sure `main` is green.
3. Tag and push:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

4. Check the release page, and edit its notes if needed (they are generated from merged PRs).
5. Open the web flasher and install on a board to check it.

## One-time setup for the web flasher

GitHub Pages is free for public repositories. Once the repository is public:

1. **Settings → Pages → Build and deployment → Source: GitHub Actions.**
2. **Settings → Environments → github-pages → Deployment branches and tags:** add a tag rule `v*`, so tag builds may deploy.

The flasher is then served at <https://vistorx.github.io/CatWandConnect/>. It always offers the latest release, because each release replaces the site.

## Building release files locally

`pio run -e <chip>` writes `firmware.bin` (OTA) and `factory.bin` (full image) to `firmware/.pio/build/<chip>/`, or to `$PLATFORMIO_BUILD_DIR/<chip>/`. Without `CWC_VERSION`, the version comes from `git describe`, for example `0.1.0-3-gabc1234`.
