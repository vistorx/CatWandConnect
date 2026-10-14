"""Checks the ready-made Home Assistant dashboard in the docs:
python website/test_ha_dashboard.py

The YAML block right after the `<!-- ha-dashboard -->` marker in
docs/home-assistant.md and its French page must parse, use only built-in
cards, and control every entity the robot announces, under the default ids.
"""

import os
import re
import unittest

import yaml

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGES = ["docs/home-assistant.md", "docs/home-assistant.fr.md"]

# Entity ids Home Assistant gives a robot under its default name,
# "CatWandConnect": device name + entity name.
DEFAULT_ENTITIES = {
    "sensor.catwandconnect_activity",
    "button.catwandconnect_stow",
    "switch.catwandconnect_play_session",
    "number.catwandconnect_session_duration",
    "number.catwandconnect_horizontal_position",
    "number.catwandconnect_vertical_position",
    "number.catwandconnect_horizontal_maximum_speed",
    "number.catwandconnect_vertical_maximum_speed",
}

BLOCK = re.compile(r"<!-- ha-dashboard -->\s*```yaml\n(.*?)```", re.S)


def dashboard(page):
    with open(os.path.join(ROOT, page), encoding="utf-8") as f:
        match = BLOCK.search(f.read())
    if not match:
        raise AssertionError(f"{page}: no ```yaml block after <!-- ha-dashboard -->")
    return yaml.safe_load(match.group(1))


def walk(node):
    """Every dict in the config, depth first."""
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def entities(config):
    found = set()
    for d in walk(config):
        if isinstance(d.get("entity"), str):
            found.add(d["entity"])
        target = d.get("target")
        if isinstance(target, dict) and isinstance(target.get("entity_id"), str):
            found.add(target["entity_id"])
    return found


class ReadyMadeDashboard(unittest.TestCase):
    def test_controls_every_announced_entity_by_its_default_id(self):
        for page in PAGES:
            with self.subTest(page=page):
                self.assertEqual(DEFAULT_ENTITIES, entities(dashboard(page)))

    def test_uses_only_built_in_cards(self):
        for page in PAGES:
            with self.subTest(page=page):
                types = {d["type"] for d in walk(dashboard(page)) if isinstance(d.get("type"), str)}
                self.assertFalse([t for t in types if t.startswith("custom:")])

    def test_is_one_sections_view(self):
        for page in PAGES:
            with self.subTest(page=page):
                config = dashboard(page)
                self.assertEqual(1, len(config["views"]))
                self.assertEqual("sections", config["views"][0]["type"])


if __name__ == "__main__":
    unittest.main()
