"""Checks the website staging: python website/test_prepare_docs.py"""

import os
import tempfile
import unittest

from prepare_docs import convert_alerts, page_url, rewrite_page, rewrite_target, stage


class RewriteTarget(unittest.TestCase):
    def md(self, target, src, dst):
        return rewrite_target(target, src, dst, False)

    def test_readme_links_to_docs_point_at_the_staged_pages(self):
        self.assertEqual(self.md("docs/install.md", "README.md", "index.md"), "install.md")
        self.assertEqual(self.md("docs/releasing.md#channels", "README.md", "index.md"), "releasing.md#channels")

    def test_root_pages_are_renamed(self):
        self.assertEqual(self.md("../SUPPORT.md", "docs/install.md", "install.md"), "support.md")
        self.assertEqual(self.md("CONTEXT.md", "README.md", "index.md"), "glossary.md")
        self.assertEqual(self.md("../../SECURITY.md", "docs/adr/0001-x.md", "adr/0001-x.md"), "../security.md")

    def test_images_point_at_the_staged_copies(self):
        self.assertEqual(self.md("media/banner/b.jpg", "README.md", "index.md"), "media/banner/b.jpg")
        self.assertEqual(self.md("images/wiring.svg", "docs/electronics.md", "electronics.md"), "images/wiring.svg")

    def test_files_outside_the_site_go_to_github(self):
        self.assertEqual(self.md("../stls/mk2/a.stl", "docs/hardware.md", "hardware.md"),
                         "https://github.com/vistorx/CatWandConnect/blob/main/stls/mk2/a.stl")
        self.assertEqual(self.md("AGENTS.md", "README.md", "index.md"),
                         "https://github.com/vistorx/CatWandConnect/blob/main/AGENTS.md")
        self.assertEqual(self.md("media/README.md", "README.md", "index.md"),
                         "https://github.com/vistorx/CatWandConnect/blob/main/media/README.md")

    def test_folders_go_to_github_unless_indexed(self):
        self.assertEqual(self.md("stls/mk2/", "README.md", "index.md"),
                         "https://github.com/vistorx/CatWandConnect/tree/main/stls/mk2")
        self.assertEqual(self.md("docs/adr/", "README.md", "index.md"), "adr/index.md")
        self.assertEqual(self.md("agents/", "docs/getting-started.md", "getting-started.md"), "agents/index.md")

    def test_urls_and_anchors_are_kept(self):
        for target in ("https://discord.gg/x", "#see-it", "mailto:a@b.c"):
            self.assertEqual(self.md(target, "README.md", "index.md"), target)

    def test_raw_html_links_use_built_urls(self):
        html = lambda t, src, dst: rewrite_target(t, src, dst, True)
        self.assertEqual(html("docs/install.md", "README.md", "index.md"), "install/")
        self.assertEqual(html("media/buttons/install.svg", "README.md", "index.md"), "media/buttons/install.svg")
        self.assertEqual(html("../media/buttons/install.svg", "docs/install.md", "install.md"),
                         "../media/buttons/install.svg")


    def test_french_pages_link_to_french_pages(self):
        self.assertEqual(self.md("docs/install.fr.md", "README.fr.md", "index.fr.md"), "install.fr.md")
        self.assertEqual(self.md("SUPPORT.fr.md", "README.fr.md", "index.fr.md"), "support.fr.md")
        self.assertEqual(self.md("../SUPPORT.fr.md", "docs/install.fr.md", "install.fr.md"), "support.fr.md")
        self.assertEqual(self.md("media/banner/b.jpg", "README.fr.md", "index.fr.md"), "media/banner/b.jpg")

    def test_raw_html_links_between_languages(self):
        html = lambda t, src, dst: rewrite_target(t, src, dst, True)
        self.assertEqual(html("README.fr.md", "README.md", "index.md"), "fr/")
        self.assertEqual(html("README.md", "README.fr.md", "index.fr.md"), "../")
        self.assertEqual(html("docs/install.fr.md", "README.fr.md", "index.fr.md"), "install/")
        self.assertEqual(html("media/buttons/install.svg", "README.fr.md", "index.fr.md"), "../media/buttons/install.svg")


class Pages(unittest.TestCase):
    def test_french_page_urls_live_under_fr(self):
        self.assertEqual(page_url("index.fr.md"), "fr/")
        self.assertEqual(page_url("install.fr.md"), "fr/install/")


    def test_github_alerts_become_admonitions(self):
        self.assertEqual(convert_alerts("> [!NOTE]\n> Line one\n>\n> Line two\n\nAfter"),
                         "!!! note\n    Line one\n\n    Line two\n\nAfter")

    def test_home_page_hides_the_side_navigation(self):
        self.assertTrue(rewrite_page("# Hi\n", "README.md", "index.md").startswith("---\nhide:"))
        self.assertTrue(rewrite_page("# Salut\n", "README.fr.md", "index.fr.md").startswith("---\nhide:"))

    def test_stage_builds_the_home_page_and_indexes(self):
        with tempfile.TemporaryDirectory() as out:
            stage(out)
            for page in ("index.md", "index.fr.md", "support.fr.md", "install.fr.md", "support.md", "glossary.md", "install.md", "adr/index.md",
                         "agents/index.md", "media/logo/logo-64.png", "images/wiring.svg"):
                self.assertTrue(os.path.exists(os.path.join(out, page)), page)
            with open(os.path.join(out, "index.md")) as f:
                self.assertIn("/CatWandConnect/flasher/", f.read())


if __name__ == "__main__":
    unittest.main()
