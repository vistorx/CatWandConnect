"""Checks the ready-made Home Assistant dashboards in the docs:
python website/test_ha_dashboard.py

docs/home-assistant.md and its French page have one dashboard per connection:
the YAML block right after `<!-- ha-dashboard -->` (MQTT connection) and the
one right after `<!-- ha-dashboard-native -->` (Native integration). Each must
parse, use only built-in cards (plus, for the Native integration, the
joystick card it ships), and control every entity its connection creates,
under the default ids. Each also opens the robot's page: a button badge in the
first heading and a long press on every tile and button, all at the same
placeholder address the guide tells owners to replace.
"""

import os
import re
import unittest

import yaml

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGES = ["docs/home-assistant.md", "docs/home-assistant.fr.md"]

# Entity ids Home Assistant gives a robot under its default name,
# "CatWandConnect": device name + entity name, from MQTT discovery
# (firmware/lib/home_assistant), which names entities in English.
MQTT_ENTITIES = {
    "sensor.catwandconnect_activity",
    "button.catwandconnect_stow",
    "switch.catwandconnect_play_session",
    "number.catwandconnect_session_duration",
    "number.catwandconnect_horizontal_position",
    "number.catwandconnect_vertical_position",
    "number.catwandconnect_horizontal_maximum_speed",
    "number.catwandconnect_vertical_maximum_speed",
    "update.catwandconnect_firmware",
}

# The Native integration's 9 entities (vistorx/ha-catwandconnect, entity
# translation keys in comments). Home Assistant builds their ids from the
# device name + the English entity name, whatever the user's language. They
# match the MQTT ones on purpose: with both connections, Home Assistant
# appends `_2` to the second set.
NATIVE_ENTITIES = {
    "sensor.catwandconnect_activity",                  # activity
    "button.catwandconnect_stow",                      # stow
    "switch.catwandconnect_play_session",              # play_session
    "number.catwandconnect_session_duration",          # session_duration
    "number.catwandconnect_horizontal_position",       # horizontal_position
    "number.catwandconnect_vertical_position",         # vertical_position
    "number.catwandconnect_horizontal_maximum_speed",  # horizontal_max_speed
    "number.catwandconnect_vertical_maximum_speed",    # vertical_max_speed
    "update.catwandconnect_firmware",                  # firmware
}

# Custom cards a dashboard may use: the Native integration ships its joystick
# card (vistorx/ha-catwandconnect, frontend/); the MQTT connection has none.
CUSTOM_CARDS = {
    "ha-dashboard": set(),
    "ha-dashboard-native": {"custom:catwandconnect-joystick-card"},
}

# Marker before each dashboard's YAML block -> entities of its connection.
DASHBOARDS = {
    "ha-dashboard": MQTT_ENTITIES,
    "ha-dashboard-native": NATIVE_ENTITIES,
}


# The robot's address in the published YAML. The guide tells owners to
# replace it with their robot's: the Robot page button in the first heading
# and the long press on every robot card open it.
ROBOT_URL = "http://catwandconnect.local/"

# Card types whose long press opens the robot's page (the joystick card has
# no hold_action: a long press there drives the wand).
HOLD_CARDS = {"tile", "button"}


def first_heading(config):
    return next(d for d in walk(config) if d.get("type") == "heading")


def dashboard(page, marker):
    block = re.compile(rf"<!-- {re.escape(marker)} -->\s*```yaml\n(.*?)```", re.S)
    with open(os.path.join(ROOT, page), encoding="utf-8") as f:
        found = block.findall(f.read())
    if len(found) != 1:
        raise AssertionError(f"{page}: expected one ```yaml block after <!-- {marker} -->, found {len(found)}")
    return yaml.safe_load(found[0])


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


def every_dashboard():
    for page in PAGES:
        for marker, expected in DASHBOARDS.items():
            yield page, marker, expected


class ReadyMadeDashboards(unittest.TestCase):
    def test_uses_only_entities_its_connection_creates(self):
        for page, marker, expected in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                self.assertEqual(set(), entities(dashboard(page, marker)) - expected)

    def test_controls_every_entity_of_its_connection_by_its_default_id(self):
        for page, marker, expected in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                self.assertEqual(expected, entities(dashboard(page, marker)))

    def test_uses_only_built_in_cards_and_its_connections_own(self):
        for page, marker, _ in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                types = {d["type"] for d in walk(dashboard(page, marker)) if isinstance(d.get("type"), str)}
                self.assertEqual(set(), {t for t in types if t.startswith("custom:")} - CUSTOM_CARDS[marker])

    def test_native_dashboard_has_the_joystick(self):
        for page in PAGES:
            with self.subTest(page=page):
                types = {d["type"] for d in walk(dashboard(page, "ha-dashboard-native")) if isinstance(d.get("type"), str)}
                self.assertIn("custom:catwandconnect-joystick-card", types)

    def test_heading_has_a_robot_page_button(self):
        for page, marker, _ in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                heading = first_heading(dashboard(page, marker))
                buttons = [b for b in heading.get("badges", []) if b.get("type") == "button"]
                self.assertEqual(1, len(buttons))
                self.assertEqual({"action": "url", "url_path": ROBOT_URL}, buttons[0].get("tap_action"))
                self.assertTrue(buttons[0].get("text"))

    def test_long_press_on_every_robot_card_opens_the_robot_page(self):
        for page, marker, _ in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                cards = [d for d in walk(dashboard(page, marker)) if d.get("type") in HOLD_CARDS and "entity" in d]
                self.assertTrue(cards)
                for card in cards:
                    self.assertEqual({"action": "url", "url_path": ROBOT_URL}, card.get("hold_action"), card["entity"])

    def test_every_url_action_uses_the_placeholder_the_guide_names(self):
        for page, marker, _ in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                urls = {d["url_path"] for d in walk(dashboard(page, marker)) if d.get("action") == "url"}
                self.assertEqual({ROBOT_URL}, urls)
                with open(os.path.join(ROOT, page), encoding="utf-8") as f:
                    guide = f.read().split(f"<!-- {marker} -->")[0]
                self.assertIn(f"`{ROBOT_URL}`", guide)

    def test_is_one_sections_view(self):
        for page, marker, _ in every_dashboard():
            with self.subTest(page=page, dashboard=marker):
                config = dashboard(page, marker)
                self.assertEqual(1, len(config["views"]))
                self.assertEqual("sections", config["views"][0]["type"])


if __name__ == "__main__":
    unittest.main()
