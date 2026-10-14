"""Checks the release notes extractor: python tools/test_release_notes.py"""

import unittest

from release_notes import discord_message, release_notes, section

CHANGELOG = """# Changelog

Intro.

## Unreleased

- Something coming.

## 0.2.0-beta.2 - 2026-10-14

Home Assistant without MQTT.

### Highlights

- **Discovery.** The robot announces itself.

### Update

OTA as usual.

## 0.2.0-beta.1 - 2026-10-14

Play sessions.

## 0.1.0 - 2026-10-13

First stable.
"""


class Section(unittest.TestCase):
    def test_takes_the_version_without_the_v(self):
        body = section(CHANGELOG, "v0.2.0-beta.2")
        self.assertTrue(body.startswith("Home Assistant without MQTT."))
        self.assertIn("### Highlights", body)
        self.assertIn("OTA as usual.", body)
        self.assertNotIn("Play sessions", body)

    def test_last_section_runs_to_the_end(self):
        self.assertEqual(section(CHANGELOG, "v0.1.0"), "First stable.")

    def test_beta_does_not_match_its_stable(self):
        self.assertEqual(section(CHANGELOG, "0.2.0-beta.1"), "Play sessions.")
        with self.assertRaises(KeyError):
            section(CHANGELOG, "v0.2.0")

    def test_empty_section_is_an_error(self):
        with self.assertRaises(KeyError):
            section(CHANGELOG.replace("- Something coming.", ""), "Unreleased")

    def test_missing_version_is_an_error(self):
        with self.assertRaises(KeyError):
            section(CHANGELOG, "v9.9.9")


class Notes(unittest.TestCase):
    def test_notes_add_footer_and_compare_link(self):
        notes = release_notes(CHANGELOG, "v0.2.0-beta.2", "vistorx/CatWandConnect", previous="v0.2.0-beta.1")
        self.assertTrue(notes.startswith("Home Assistant without MQTT."))
        self.assertIn("**Found a problem?**", notes)
        self.assertIn("https://github.com/vistorx/CatWandConnect/compare/v0.2.0-beta.1...v0.2.0-beta.2", notes)

    def test_notes_without_previous_tag(self):
        notes = release_notes(CHANGELOG, "v0.1.0", "vistorx/CatWandConnect")
        self.assertNotIn("compare", notes)


class Discord(unittest.TestCase):
    def test_message_has_title_pitch_highlights_and_link(self):
        msg = discord_message(CHANGELOG, "v0.2.0-beta.2", "CatWandConnect firmware",
                              "https://example.org/r")
        self.assertTrue(msg.startswith("## CatWandConnect firmware v0.2.0-beta.2 (beta)"))
        self.assertIn("Home Assistant without MQTT.", msg)
        self.assertIn("- **Discovery.**", msg)
        self.assertNotIn("OTA as usual", msg)  # only the pitch and highlights
        self.assertTrue(msg.rstrip().endswith("https://example.org/r>"))

    def test_stable_has_no_beta_mark(self):
        msg = discord_message(CHANGELOG, "v0.1.0", "X", "https://e/r")
        self.assertTrue(msg.startswith("## X v0.1.0\n"))

    def test_message_fits_discord_limit(self):
        long = CHANGELOG.replace("- **Discovery.** The robot announces itself.",
                                 "\n".join(f"- item {i} " + "x" * 80 for i in range(60)))
        msg = discord_message(long, "v0.2.0-beta.2", "X", "https://e/r")
        self.assertLessEqual(len(msg), 2000)
        self.assertIn("https://e/r", msg)


if __name__ == "__main__":
    unittest.main()
