"""Assembles the web flasher with a Stable and a Beta channel.

Usage: python tools/flasher_site.py <releases.json> <site-dir>

The flasher lives under <site-dir>/flasher/ (FLASHER), next to the project
website at the site root: its page (flasher/index.html), the logo it shows,
the channel folders and channels.json all go there.

<releases.json> is the output of
`gh release list --json tagName,isPrerelease,isDraft,publishedAt`.
For each channel it downloads the factory images of the chosen release into
<site-dir>/flasher/<channel>/, splits them into parts that leave the settings
partition alone (flasher_manifest.split_image) and writes a manifest there:

- stable: the newest release that is not a pre-release;
- beta: the newest release of any kind (so a stable newer than every beta is
  also what Beta users get).

<site-dir>/flasher/channels.json lists the channels with their versions, for the page.
A channel with no release is left out. Downloading uses the `gh` CLI.
"""

import json
import os
import shutil
import subprocess
import sys

from flasher_manifest import CHIPS, site_manifest

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# Where the flasher is served, relative to the site root (/CatWandConnect/flasher/).
FLASHER = "flasher"
# Repository files the flasher page loads, copied next to it.
PAGE_FILES = ["flasher/index.html", "media/logo/logo-64.png", "media/logo/logo-320.png"]


def pick_channels(releases):
    """Returns {"stable": tag or None, "beta": tag or None}."""
    published = [r for r in releases if not r.get("isDraft")]
    published.sort(key=lambda r: r["publishedAt"], reverse=True)
    stable = next((r["tagName"] for r in published if not r["isPrerelease"]), None)
    beta = published[0]["tagName"] if published else None
    return {"stable": stable, "beta": beta}


def copy_page(site):
    """Copies the flasher page and the files it loads into <site>/flasher/."""
    for path in PAGE_FILES:
        dst = os.path.join(site, FLASHER, os.path.relpath(path, "flasher") if path.startswith("flasher/") else path)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(os.path.join(REPO, path), dst)


def build(releases, site, download):
    """Writes the channels under <site>/flasher/ and returns them."""
    site = os.path.join(site, FLASHER)
    os.makedirs(site, exist_ok=True)
    channels = {}
    for channel, tag in pick_channels(releases).items():
        if tag is None:
            continue
        version = tag[1:] if tag.startswith("v") else tag
        folder = os.path.join(site, channel)
        os.makedirs(folder, exist_ok=True)
        download(tag, folder)
        with open(os.path.join(folder, "manifest.json"), "w") as f:
            json.dump(site_manifest(version, folder), f, indent=2)
            f.write("\n")
        channels[channel] = {"version": version, "tag": tag}
    with open(os.path.join(site, "channels.json"), "w") as f:
        json.dump(channels, f, indent=2)
        f.write("\n")
    return channels


def gh_download(tag, folder):
    subprocess.run(["gh", "release", "download", tag, "--dir", folder, "--clobber",
                    "--pattern", "catwandconnect-*-factory.bin"], check=True)
    missing = [env for env in CHIPS if not os.path.exists(
        os.path.join(folder, f"catwandconnect-{env}-factory.bin"))]
    if missing:
        sys.exit(f"{tag} has no factory image for {', '.join(missing)}")


if __name__ == "__main__":
    with open(sys.argv[1]) as f:
        releases = json.load(f)
    copy_page(sys.argv[2])
    print(json.dumps(build(releases, sys.argv[2], gh_download), indent=2))
