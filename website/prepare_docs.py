"""Stages the website sources: python website/prepare_docs.py [<out-dir>]

The website is built from the repository's own Markdown, so the repo and the
site never drift. This script copies it into <out-dir> (default
website/build/docs, the `docs_dir` of website/mkdocs.yml):

- README.md becomes the home page (index.md), README.fr.md the French one
  (index.fr.md);
- docs/** is copied as is (pages, diagrams, French pages `page.fr.md`);
- SUPPORT.md, SECURITY.md and CONTEXT.md become support.md, security.md and
  glossary.md (SUPPORT.fr.md becomes support.fr.md);
- media/** (logo, banners, photos, video) is copied for the images.

GitHub-only content is adapted: a bare user-attachments video URL (GitHub
renders it as a player) becomes a <video> of the local copy in media/video/,
and blocks between `<!-- github-only -->` and `<!-- /github-only -->` (such
as the README's "read this on the website" link) are dropped.

Links are rewritten so they work on the site the way they work on GitHub:
links to staged pages point at the staged copy, links to anything else in the
repository (STL files, source code, licences, AGENTS.md...) become github.com
URLs. GitHub alerts (`> [!NOTE]`) become MkDocs admonitions. Folders linked as
pages (docs/adr/, docs/agents/) get a generated index.md listing their pages.
"""

import os
import posixpath
import re
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GITHUB = "https://github.com/vistorx/CatWandConnect"

# Repository file -> staged page, for the pages that live outside docs/.
ROOT_PAGES = {
    "README.md": "index.md",
    "README.fr.md": "index.fr.md",
    "SUPPORT.md": "support.md",
    "SUPPORT.fr.md": "support.fr.md",
    "SECURITY.md": "security.md",
    "CONTEXT.md": "glossary.md",
    "CHANGELOG.md": "changelog.md",
}
# Folders whose link opens a generated list of their pages.
INDEXED_DIRS = {"docs/adr": "Design decisions", "docs/agents": "Agent conventions"}
# Extra front matter per staged page.
HOME_FRONT_MATTER = "hide:\n  - navigation\n  - toc\n"
FRONT_MATTER = {"index.md": HOME_FRONT_MATTER, "index.fr.md": HOME_FRONT_MATTER}
# French pages (`page.fr.md`) are built under fr/ by the i18n plugin.
FR_SUFFIX = ".fr.md"

MD_LINK = re.compile(r"(\]\()([^)\s]+)(\))")
HTML_LINK = re.compile(r"""((?:href|src|poster)=")([^"]+)(")""")
ALERT = re.compile(r"^> \[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*$")
# GitHub user-attachments video -> its copy in media/, with its poster.
VIDEOS = {
    "https://github.com/user-attachments/assets/914092e7-1f09-4832-85f9-01eeb9a11499": (
        "media/video/mk2-presentation.mp4", "media/video/mk2-presentation-poster.jpg"),
}
ATTACHMENT = re.compile(r"^(https://github\.com/user-attachments/assets/[0-9a-f-]+)\s*$", re.M)
GITHUB_ONLY = re.compile(r"<!-- github-only -->.*?<!-- /github-only -->\n?", re.S)
ADMONITION = {"NOTE": "note", "TIP": "tip", "IMPORTANT": "info", "WARNING": "warning", "CAUTION": "danger"}


def staged_path(repo_path):
    """Where a repository file lands in the site sources, or None if it is not copied."""
    if repo_path in ROOT_PAGES:
        return ROOT_PAGES[repo_path]
    if repo_path.startswith("docs/"):
        return repo_path[len("docs/"):]
    if repo_path.startswith("media/") and not repo_path.endswith(".md"):
        return repo_path
    return None


def page_url(staged):
    """The site URL of a staged page, relative to the site root (use_directory_urls)."""
    if staged.endswith(FR_SUFFIX):
        return "fr/" + page_url(staged[:-len(FR_SUFFIX)] + ".md")
    if staged == "index.md":
        return ""
    if staged.endswith("/index.md"):
        return staged[:-len("index.md")]
    return staged[:-len(".md")] + "/" if staged.endswith(".md") else staged


def rewrite_target(target, src_repo, dst_staged, html):
    """Returns the link to use on the site for `target`, found in `src_repo`."""
    if re.match(r"^[a-z][a-z0-9+.-]*:", target) or target.startswith(("#", "/")):
        return target
    path, _, anchor = target.partition("#")
    anchor = "#" + anchor if anchor else ""
    repo_path = os.path.normpath(os.path.join(os.path.dirname(src_repo), path)).replace(os.sep, "/")
    if repo_path.startswith(".."):
        return target
    is_dir = path.endswith("/") or os.path.isdir(os.path.join(REPO, repo_path))
    if is_dir and repo_path in INDEXED_DIRS:
        staged = staged_path(repo_path + "/index.md")
    elif is_dir:
        return f"{GITHUB}/tree/main/{repo_path}{anchor}"
    else:
        staged = staged_path(repo_path)
    if staged is None:
        return f"{GITHUB}/blob/main/{repo_path}{anchor}"
    if html:
        # Raw HTML is not rewritten by MkDocs: link to the built URL instead.
        here, there = page_url(dst_staged), page_url(staged)
        rel = posixpath.relpath(there or ".", here or ".")
        is_page = there == "" or there.endswith("/")
        return (rel + "/" if is_page and rel != "." else "./" if rel == "." else rel) + anchor
    rel = os.path.relpath(staged, os.path.dirname(dst_staged) or ".").replace(os.sep, "/")
    return rel + anchor


def convert_alerts(text):
    """GitHub alerts (`> [!NOTE]` blockquotes) -> MkDocs admonitions (`!!! note`)."""
    out, lines, i = [], text.split("\n"), 0
    while i < len(lines):
        match = ALERT.match(lines[i])
        if not match:
            out.append(lines[i])
            i += 1
            continue
        out.append(f"!!! {ADMONITION[match.group(1)]}")
        i += 1
        while i < len(lines) and lines[i].startswith(">"):
            body = lines[i][1:].lstrip(" ")
            out.append("    " + body if body else "")
            i += 1
    return "\n".join(out)


def convert_videos(text, src_repo):
    """Bare GitHub video URLs -> a <video> of the local copy (or a plain link).

    Paths are written relative to the source file, like any repo link, so the
    HTML link rewriting that follows turns them into built URLs."""
    def video(match):
        url = match.group(1)
        if url not in VIDEOS:
            return f"[{url}]({url})"
        up = "../" * src_repo.count("/")
        src, poster = VIDEOS[url]
        # Half the page width, centred; never narrower than a phone screen.
        return (f'<video controls preload="metadata" playsinline '
                f'style="display:block;margin:0 auto;width:50%;min-width:280px;max-width:100%" '
                f'src="{up}{src}" poster="{up}{poster}"></video>')
    return ATTACHMENT.sub(video, text)


def rewrite_page(text, src_repo, dst_staged):
    text = GITHUB_ONLY.sub("", text)
    text = convert_videos(text, src_repo)
    text = MD_LINK.sub(lambda m: m.group(1) + rewrite_target(m.group(2), src_repo, dst_staged, False) + m.group(3), text)
    text = HTML_LINK.sub(lambda m: m.group(1) + rewrite_target(m.group(2), src_repo, dst_staged, True) + m.group(3), text)
    text = convert_alerts(text)
    front = FRONT_MATTER.get(dst_staged)
    return f"---\n{front}---\n\n{text}" if front else text


def first_heading(path):
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("# "):
                return line[2:].strip()
    return os.path.basename(path)


def write_index(out, repo_dir, title):
    staged_dir = repo_dir[len("docs/"):]
    if os.path.exists(os.path.join(REPO, repo_dir, "index.md")):
        return
    lines = [f"# {title}", ""]
    for name in sorted(os.listdir(os.path.join(REPO, repo_dir))):
        if name.endswith(".md") and not name.endswith(FR_SUFFIX):
            lines.append(f"- [{first_heading(os.path.join(REPO, repo_dir, name))}]({name})")
    with open(os.path.join(out, staged_dir, "index.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def stage(out):
    if os.path.exists(out):
        shutil.rmtree(out)
    files = [p for p in ROOT_PAGES if os.path.exists(os.path.join(REPO, p))]
    for top in ("docs", "media"):
        for root, _, names in os.walk(os.path.join(REPO, top)):
            files += [os.path.relpath(os.path.join(root, n), REPO).replace(os.sep, "/") for n in names if not n.startswith(".")]
    for repo_path in sorted(files):
        staged = staged_path(repo_path)
        if staged is None:
            continue
        dst = os.path.join(out, staged)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        if repo_path.endswith(".md"):
            with open(os.path.join(REPO, repo_path), encoding="utf-8") as f:
                text = f.read()
            with open(dst, "w", encoding="utf-8") as f:
                f.write(rewrite_page(text, repo_path, staged))
        else:
            shutil.copyfile(os.path.join(REPO, repo_path), dst)
    for repo_dir, title in INDEXED_DIRS.items():
        write_index(out, repo_dir, title)


if __name__ == "__main__":
    stage(sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "website", "build", "docs"))
