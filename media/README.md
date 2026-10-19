# Media

The project's visual assets: logo, banners, photos and screenshots. Drop the source files here; the README, the docs, the web flasher and the Device interface use them from this folder.

| Folder | What goes in it | Format |
|--------|-----------------|--------|
| `logo/` | The CatWandConnect icon: `logo.svg` (source, used by the website and the flasher), `favicon.ico`, PNG exports at 16, 32, 180 (Apple touch icon) and 512 px | SVG first, PNG exports |
| `banner/` | `banner.svg` (source) and `banner.png`: top of the README | 1200×315 |
| `discord/` | Avatars of the Discord bots (Admin, Support), kept here as sources | SVG + PNG 512 px |
| `photos/` | Photos of the robot, of the build steps and of cats playing with it | JPEG or WebP |
| `screenshots/` | Screenshots of the Device interface, the flasher and Home Assistant | PNG |
| `buttons/` | Call-to-action buttons used in the README: the install button and the sponsor button (`sponsor.svg`, same shape, rose) | SVG |
| `video/` | Short presentation and demo videos, each with a `-poster.jpg` still used as its clickable preview (GitHub does not play videos stored in the repository inline) | MP4 (H.264, `+faststart`), under about 5 MB |

## Naming

Lowercase, words joined by hyphens, starting with what the image shows: `logo-full-dark.svg`, `banner-readme.png`, `mk3-assembled-front.jpg`, `assembly-step-03-bearing.jpg`, `ui-main-screen-phone.png`. Use the [glossary](../CONTEXT.md) words (Mk3, Stow, Calibration…), not their synonyms.

## Keep the repository light

Every image stays in the git history forever, even after it is deleted.

- Resize photos to at most 1600 px on the long side and keep each file under about 500 KB.
- Strip location data (EXIF GPS) from phone photos before adding them.
- Prefer SVG for anything drawn (logo, diagrams).

## Where each place takes its images

| Place | How |
|-------|-----|
| `README.md` | `media/<folder>/<file>` |
| `docs/*.md` | `../media/<folder>/<file>` (technical diagrams such as the wiring stay in `docs/images/`) |
| Web flasher | The release workflow copies this folder to the flasher site, so `flasher/index.html` uses `media/<folder>/<file>` |
| Device interface | Not directly: `firmware/web/` is embedded in the robot's flash, where space is tight. Put a small, optimised copy in `firmware/web/` (as with the icons) and keep the source here |

## Licence

Photos, banners and screenshots are under [CC BY-SA 4.0](../LICENSES/CC-BY-SA-4.0.txt) like the documentation. The logo file is shared under the same licence, but the name and the logo themselves are covered by the [trademark policy](../TRADEMARK.md).

Only add images you made yourself or have the right to share under these terms.
