"""Checks the translation check: python website/test_check_translations.py"""

import os
import tempfile
import unittest

from check_translations import main, missing_translations, orphan_translations


def make_repo(root, files):
    for path in files:
        full = os.path.join(root, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, "w", encoding="utf-8") as f:
            f.write("# Page\n")


COMPLETE = ["README.md", "README.fr.md", "SUPPORT.md", "SUPPORT.fr.md",
            "docs/install.md", "docs/install.fr.md",
            # Exempt contributor docs need no French page.
            "CONTEXT.md", "SECURITY.md", "docs/releasing.md", "docs/support-relay.md",
            "docs/adr/0001-x.md", "docs/agents/domain.md",
            # Not website pages.
            "AGENTS.md", "media/README.md", "relay/README.md"]


class CheckTranslations(unittest.TestCase):
    def test_passes_when_every_user_page_is_translated(self):
        with tempfile.TemporaryDirectory() as repo:
            make_repo(repo, COMPLETE)
            self.assertEqual(missing_translations(repo), [])
            self.assertEqual(main(repo), 0)

    def test_fails_on_a_missing_french_page(self):
        with tempfile.TemporaryDirectory() as repo:
            make_repo(repo, [f for f in COMPLETE if f != "docs/install.fr.md"] + ["docs/new-page.md"])
            self.assertEqual(missing_translations(repo), ["docs/install.fr.md", "docs/new-page.fr.md"])
            self.assertEqual(main(repo), 1)

    def test_fails_on_a_missing_french_home_page(self):
        with tempfile.TemporaryDirectory() as repo:
            make_repo(repo, [f for f in COMPLETE if f != "README.fr.md"])
            self.assertEqual(missing_translations(repo), ["README.fr.md"])

    def test_fails_on_a_french_page_left_behind(self):
        with tempfile.TemporaryDirectory() as repo:
            make_repo(repo, COMPLETE + ["docs/removed.fr.md"])
            self.assertEqual(orphan_translations(repo), ["docs/removed.fr.md"])
            self.assertEqual(main(repo), 1)

    def test_this_repository_is_complete(self):
        self.assertEqual(main(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 0)


if __name__ == "__main__":
    unittest.main()
