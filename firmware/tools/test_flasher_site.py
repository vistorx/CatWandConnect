"""Checks the flasher channel rules and image parts: python tools/test_flasher_site.py"""

import json
import os
import struct
import tempfile
import unittest

from flasher_manifest import CHIPS, split_image
from flasher_site import build, copy_page, pick_channels, summary

NVS = (0x9000, 0x5000)


def entry(name, kind, subtype, offset, size):
    return struct.pack("<HBBII16sI", 0x50AA, kind, subtype, offset, size, name.encode(), 0)


def factory_image(app_size=0x3000):
    """A merged image like merge_bin's: bootloader, min_spiffs table, otadata, app."""
    image = bytearray(b"\xff" * (0x10000 + app_size))
    image[0x1000:0x1100] = b"\xe9" * 0x100                       # bootloader
    table = (entry("nvs", 1, 2, *NVS) + entry("otadata", 1, 0, 0xE000, 0x2000) +
             entry("app0", 0, 0x10, 0x10000, 0x1E0000) + entry("app1", 0, 0x11, 0x1F0000, 0x1E0000))
    image[0x8000:0x8000 + len(table)] = table
    image[0xE000:0xE020] = b"\x01" * 0x20                         # otadata
    image[0x10000:] = b"\xaa" * app_size                          # app
    return bytes(image)


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


class SplitImage(unittest.TestCase):
    def test_no_part_touches_the_settings_partition(self):
        start, size = NVS
        for offset, data in split_image(factory_image()):
            self.assertTrue(offset + len(data) <= start or offset >= start + size,
                            f"part at {offset:#x} overlaps NVS")

    def test_parts_rebuild_everything_but_the_settings_partition(self):
        image = factory_image()
        rebuilt = bytearray(b"\xff" * len(image))
        for offset, data in split_image(image):
            rebuilt[offset:offset + len(data)] = data
        start, size = NVS
        self.assertEqual(rebuilt[:start], image[:start])
        self.assertEqual(rebuilt[start + size:], image[start + size:])

    def test_an_image_without_a_partition_table_is_refused(self):
        with self.assertRaises(ValueError):
            split_image(b"\xff" * 0x20000)


CHANGELOG = """# Changelog

## Unreleased

## 0.2.0 - 2026-10-20

**One-tap updates.** Your robot tells you when a new version is out, and
installs it in [one tap](https://example.org/docs) from `Settings`.

### Highlights

- Something.

## 0.1.0-beta.1 - 2026-10-12

### Highlights

- First beta.
"""


def fake_download(downloaded=None):
    def download(tag, folder):
        if downloaded is not None:
            downloaded.append((tag, folder))
        for env in CHIPS:
            with open(os.path.join(folder, f"catwandconnect-{env}-factory.bin"), "wb") as f:
                f.write(factory_image())
            with open(os.path.join(folder, f"catwandconnect-{env}-ota.bin"), "wb") as f:
                f.write(f"{tag} {env}".encode())
    return download


class Summary(unittest.TestCase):
    def test_is_the_pitch_in_plain_text_on_one_line(self):
        self.assertEqual(summary(CHANGELOG, "v0.2.0"),
                         "One-tap updates. Your robot tells you when a new version is out, and installs it in "
                         "one tap from Settings.")

    def test_is_empty_when_the_section_starts_with_a_heading(self):
        self.assertEqual(summary(CHANGELOG, "v0.1.0-beta.1"), "")

    def test_is_empty_without_a_section(self):
        self.assertEqual(summary(CHANGELOG, "v0.0.9"), "")


class Build(unittest.TestCase):
    def test_writes_a_manifest_per_channel_and_the_channel_list_under_flasher(self):
        releases = [release("v0.1.0-beta.1", 12, pre=True)]
        downloaded = []
        with tempfile.TemporaryDirectory() as root:
            site = os.path.join(root, "flasher")
            channels = build(releases, root, fake_download(downloaded), CHANGELOG)
            self.assertEqual(channels["beta"]["version"], "0.1.0-beta.1")
            self.assertEqual(channels["beta"]["tag"], "v0.1.0-beta.1")
            self.assertEqual(downloaded, [("v0.1.0-beta.1", os.path.join(site, "beta"))])
            with open(os.path.join(site, "beta", "manifest.json")) as f:
                manifest = json.load(f)
            self.assertEqual(manifest["version"], "0.1.0-beta.1")
            for chip in manifest["builds"]:
                for part in chip["parts"]:
                    self.assertTrue(os.path.exists(os.path.join(site, "beta", part["path"])), part["path"])
                    self.assertNotEqual(part["offset"], 0x9000)
            with open(os.path.join(site, "channels.json")) as f:
                self.assertEqual(json.load(f), channels)
            self.assertFalse(os.path.exists(os.path.join(site, "stable")))
            self.assertFalse(os.path.exists(os.path.join(root, "channels.json")))

    # The Update feed (docs/adr/0012): what the Device interface and the
    # Native integration read to offer a one-tap update.
    def test_channels_json_is_the_update_feed(self):
        releases = [release("v0.2.0", 20), release("v0.3.0-beta.1", 25, pre=True)]
        with tempfile.TemporaryDirectory() as root:
            channels = build(releases, root, fake_download(), CHANGELOG)
            self.assertEqual(channels["stable"], {
                "version": "0.2.0", "tag": "v0.2.0",
                "notes": "https://github.com/vistorx/CatWandConnect/releases/tag/v0.2.0",
                "summary": summary(CHANGELOG, "v0.2.0"),
                "ota": {"ESP32": "stable/catwandconnect-esp32-ota.bin",
                        "ESP32-S3": "stable/catwandconnect-esp32s3-ota.bin",
                        "ESP32-C3": "stable/catwandconnect-esp32c3-ota.bin"},
            })
            self.assertEqual(channels["beta"]["summary"], "")
            self.assertEqual(channels["beta"]["ota"]["ESP32-C3"], "beta/catwandconnect-esp32c3-ota.bin")

    def test_every_ota_path_is_the_release_s_app_image_for_that_chip(self):
        releases = [release("v0.2.0", 20), release("v0.3.0-beta.1", 25, pre=True)]
        with tempfile.TemporaryDirectory() as root:
            channels = build(releases, root, fake_download(), CHANGELOG)
            for channel in ("stable", "beta"):
                for family, path in channels[channel]["ota"].items():
                    env = next(e for e, f in CHIPS.items() if f == family)
                    with open(os.path.join(root, "flasher", path), "rb") as f:
                        self.assertEqual(f.read(), f"{channels[channel]['tag']} {env}".encode())

    def test_a_release_without_ota_images_fails(self):
        def download(tag, folder):
            for env in CHIPS:
                with open(os.path.join(folder, f"catwandconnect-{env}-factory.bin"), "wb") as f:
                    f.write(factory_image())
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                build([release("v0.2.0", 20)], root, download, CHANGELOG)

    def test_copies_the_page_and_the_files_it_loads_under_flasher(self):
        with tempfile.TemporaryDirectory() as root:
            copy_page(root)
            page = os.path.join(root, "flasher", "index.html")
            with open(page) as f:
                html = f.read()
            for ref in ("media/logo/logo-64.png", "media/logo/logo-320.png"):
                self.assertIn(f'"{ref}"', html)
                self.assertTrue(os.path.exists(os.path.join(root, "flasher", ref)), ref)
            self.assertIn("fetch('channels.json')", html)


if __name__ == "__main__":
    unittest.main()
