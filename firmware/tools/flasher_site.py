"""Assembles the web flasher site with a Stable and a Beta channel.

Usage: python tools/flasher_site.py <releases.json> <site-dir>

<releases.json> is the output of
`gh release list --json tagName,isPrerelease,isDraft,publishedAt`.
For each channel it downloads the factory images of the chosen release into
<site-dir>/<channel>/ and writes a manifest there:

- stable: the newest release that is not a pre-release;
- beta: the newest release of any kind (so a stable newer than every beta is
  also what Beta users get).

<site-dir>/channels.json lists the channels with their versions, for the page.
A channel with no release is left out. Downloading uses the `gh` CLI.
"""

import json
import os
import subprocess
import sys

from flasher_manifest import CHIPS, manifest


def pick_channels(releases):
    """Returns {"stable": tag or None, "beta": tag or None}."""
    published = [r for r in releases if not r.get("isDraft")]
    published.sort(key=lambda r: r["publishedAt"], reverse=True)
    stable = next((r["tagName"] for r in published if not r["isPrerelease"]), None)
    beta = published[0]["tagName"] if published else None
    return {"stable": stable, "beta": beta}


def build(releases, site, download):
    channels = {}
    for channel, tag in pick_channels(releases).items():
        if tag is None:
            continue
        version = tag[1:] if tag.startswith("v") else tag
        folder = os.path.join(site, channel)
        os.makedirs(folder, exist_ok=True)
        download(tag, folder)
        with open(os.path.join(folder, "manifest.json"), "w") as f:
            json.dump(manifest(version), f, indent=2)
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
    print(json.dumps(build(releases, sys.argv[2], gh_download), indent=2))
