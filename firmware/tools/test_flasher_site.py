"""Checks the flasher channel rules: python tools/test_flasher_site.py"""

import json
import os
import tempfile
import unittest

from flasher_site import build, pick_channels


def release(tag, day, pre=False, draft=False):
    return {"tagName": tag, "isPrerelease": pre, "isDraft": draft,
            "publishedAt": f"2026-10-{day:02d}T12:00:00Z"}


class PickChannels(unittest.TestCase):
    def test_no_release_offers_nothing(self):
        self.assertEqual(pick_channels([]), {"stable": None, "beta": None})

    def test_only_betas_offer_beta_only(self):
        releases = [release("v0.1.0-beta.1", 12, pre=True), release("v0.1.0-beta.2", 14, pre=True)]
        self.assertEqual(pick_channels(releases), {"stable": None, "beta": "v0.1.0-beta.2"})

    def test_a_newer_beta_does_not_replace_stable(self):
        releases = [release("v0.1.0", 20), release("v0.2.0-beta.1", 25, pre=True)]
        self.assertEqual(pick_channels(releases), {"stable": "v0.1.0", "beta": "v0.2.0-beta.1"})

    def test_beta_follows_a_stable_newer_than_every_beta(self):
        releases = [release("v0.1.0-beta.3", 18, pre=True), release("v0.1.0", 20)]
        self.assertEqual(pick_channels(releases), {"stable": "v0.1.0", "beta": "v0.1.0"})

    def test_drafts_are_ignored(self):
        releases = [release("v0.1.0", 20), release("v0.2.0", 25, draft=True)]
        self.assertEqual(pick_channels(releases)["stable"], "v0.1.0")


class Build(unittest.TestCase):
    def test_writes_a_manifest_per_channel_and_the_channel_list(self):
        releases = [release("v0.1.0-beta.1", 12, pre=True)]
        downloaded = []
        with tempfile.TemporaryDirectory() as site:
            channels = build(releases, site, lambda tag, folder: downloaded.append((tag, folder)))
            self.assertEqual(channels, {"beta": {"version": "0.1.0-beta.1", "tag": "v0.1.0-beta.1"}})
            self.assertEqual(downloaded, [("v0.1.0-beta.1", os.path.join(site, "beta"))])
            with open(os.path.join(site, "beta", "manifest.json")) as f:
                self.assertEqual(json.load(f)["version"], "0.1.0-beta.1")
            with open(os.path.join(site, "channels.json")) as f:
                self.assertEqual(json.load(f), channels)
            self.assertFalse(os.path.exists(os.path.join(site, "stable")))


if __name__ == "__main__":
    unittest.main()
