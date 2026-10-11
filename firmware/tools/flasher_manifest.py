"""Writes the ESP Web Tools manifest for the web flasher.

Usage: python tools/flasher_manifest.py <version> <output.json>
The factory images it names sit next to it on the flasher site.
"""

import json
import sys

CHIPS = {"esp32": "ESP32", "esp32s3": "ESP32-S3", "esp32c3": "ESP32-C3"}


def manifest(version):
    return {
        "name": "CatWandConnect",
        "version": version,
        "new_install_prompt_erase": True,
        "builds": [
            {"chipFamily": family, "parts": [{"path": f"catwandconnect-{env}-factory.bin", "offset": 0}]}
            for env, family in CHIPS.items()
        ],
    }


if __name__ == "__main__":
    version, output = sys.argv[1], sys.argv[2]
    with open(output, "w") as f:
        json.dump(manifest(version), f, indent=2)
        f.write("\n")
