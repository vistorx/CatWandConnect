"""Checks that every English user page has its French page: python website/check_translations.py

The website is in English and French (website/mkdocs.yml). A user page
`page.md` has its French version `page.fr.md` next to it: README.fr.md for the
home page, SUPPORT.fr.md, docs/<page>.fr.md. A pull request that changes an
English user page updates its French page too (AGENTS.md).

Contributor docs are exempt: they are listed in EXEMPT below, the one place
that decides it, and the French site falls back to their English page.

Exits with 1 and lists the missing files when a French page is missing, like
firmware/tools/check_lang.mjs does for the Device interface's language files.
"""

import fnmatch
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# English pages of the website, as repository paths (globs).
PAGES = ["README.md", "SUPPORT.md", "SECURITY.md", "CONTEXT.md", "docs/**.md"]

# Contributor docs: English only, the French site shows the English page.
EXEMPT = [
    "CONTEXT.md",            # Glossary: the project's words, for contributors
    "CHANGELOG.md",          # Release notes, published as written (GitHub, Discord)
    "SECURITY.md",           # Security policy
    "docs/adr/**",           # Design decisions
    "docs/agents/**",        # Agent conventions
    "docs/releasing.md",     # Publishing a version
    "docs/support-relay.md", # Support relay internals
]

FR = ".fr.md"


def matches(path, patterns):
    return any(fnmatch.fnmatchcase(path, p) for p in patterns)


def markdown_files(repo):
    """Markdown files at the root of `repo` and under docs/, as repository paths."""
    files = [n for n in os.listdir(repo) if n.endswith(".md")]
    for root, _, names in os.walk(os.path.join(repo, "docs")):
        files += [os.path.relpath(os.path.join(root, n), repo).replace(os.sep, "/") for n in names if n.endswith(".md")]
    return sorted(files)


def english_pages(repo):
    """Every English website page in `repo`, as sorted repository paths."""
    return [p for p in markdown_files(repo) if not p.endswith(FR) and matches(p, PAGES)]


def missing_translations(repo):
    """English user pages of `repo` that have no French page, as the missing paths."""
    missing = []
    for page in english_pages(repo):
        if matches(page, EXEMPT):
            continue
        french = page[:-len(".md")] + FR
        if not os.path.exists(os.path.join(repo, french)):
            missing.append(french)
    return missing


def orphan_translations(repo):
    """French pages whose English page no longer exists."""
    return [p for p in markdown_files(repo)
            if p.endswith(FR) and matches(p[:-len(FR)] + ".md", PAGES)
            and not os.path.exists(os.path.join(repo, p[:-len(FR)] + ".md"))]


def main(repo):
    missing, orphans = missing_translations(repo), orphan_translations(repo)
    for path in missing:
        print(f"missing French page: {path}", file=sys.stderr)
    for path in orphans:
        print(f"French page without its English page: {path}", file=sys.stderr)
    if missing or orphans:
        print("Translate the page into French, or add it to EXEMPT in website/check_translations.py "
              "if it is a contributor doc.", file=sys.stderr)
        return 1
    print(f"Every user page has its French page ({len(english_pages(repo))} English pages checked)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else REPO))
