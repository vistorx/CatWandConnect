"""Release notes from CHANGELOG.md.

    python tools/release_notes.py notes <tag> [previous-tag]   # GitHub release body
    python tools/release_notes.py discord <tag> <title> <url>  # Discord announcement

Each version has a `## <version> - <date>` section in CHANGELOG.md, written for
owners: a pitch paragraph, `### Highlights`, then anything else (update steps,
fixes). The release workflow fails when the tag has no section, so every
release ships with its changelog.
"""

import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FOOTER = ("**Found a problem?** On the robot, open Settings → Robot → Help to report it with the "
          "Diagnostic report, ask on [Discord](https://discord.gg/Rbqge3v9dk), or see "
          "[Getting help](https://github.com/vistorx/CatWandConnect/blob/main/SUPPORT.md).")
DISCORD_LIMIT = 2000


def _version(tag):
    return tag[1:] if tag.startswith("v") else tag


def section(changelog, tag):
    """The body of the `## <version>` section, without its heading."""
    version = re.escape(_version(tag))
    match = re.search(rf"^## {version}(?: |$)[^\n]*\n(.*?)(?=^## |\Z)", changelog, re.M | re.S)
    if not match:
        raise KeyError(f"CHANGELOG.md has no section for {_version(tag)}")
    body = match.group(1).strip()
    if not body:
        raise KeyError(f"CHANGELOG.md section {_version(tag)} is empty")
    return body


def release_notes(changelog, tag, repo, previous=None):
    parts = [section(changelog, tag), FOOTER]
    if previous:
        parts.append(f"**Full Changelog**: https://github.com/{repo}/compare/{previous}...{tag}")
    return "\n\n".join(parts) + "\n"


def _pitch_and_highlights(body):
    pitch = body.split("\n\n", 1)[0] if not body.startswith("#") else ""
    highlights = ""
    match = re.search(r"^### Highlights\n(.*?)(?=^### |\Z)", body, re.M | re.S)
    if match:
        highlights = match.group(1).strip()
    return pitch.strip(), highlights


def discord_message(changelog, tag, title, url):
    """A short announcement: title, pitch, highlights, link; within Discord's limit."""
    pitch, highlights = _pitch_and_highlights(section(changelog, tag))
    head = f"## {title} {tag}" + (" (beta)" if "-" in tag else "") + "\n"
    tail = f"\nFull notes and download: <{url}>"
    body = "\n\n".join(p for p in (pitch, highlights) if p)
    room = DISCORD_LIMIT - len(head) - len(tail)
    if len(body) > room:
        lines, kept = body.split("\n"), []
        for line in lines:
            if len("\n".join(kept + [line, "…"])) > room:
                break
            kept.append(line)
        body = "\n".join(kept + ["…"])
    return head + body + "\n" + tail


def main(argv):
    with open(os.path.join(REPO_ROOT, "CHANGELOG.md"), encoding="utf-8") as f:
        changelog = f.read()
    if len(argv) >= 2 and argv[0] == "notes":
        sys.stdout.write(release_notes(changelog, argv[1], os.environ.get("GITHUB_REPOSITORY", "vistorx/CatWandConnect"),
                                       argv[2] if len(argv) > 2 and argv[2] else None))
    elif len(argv) == 4 and argv[0] == "discord":
        sys.stdout.write(discord_message(changelog, argv[1], argv[2], argv[3]))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
