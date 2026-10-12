"""Writes the ESP Web Tools manifest for the web flasher, and the image parts it names.

Usage: python tools/flasher_manifest.py <version> <output.json>
The factory images (catwandconnect-<chip>-factory.bin) must sit next to
<output.json>; their parts are written there too.

The web flasher must not write over the settings partition (NVS: Wi-Fi, name,
PIN, settings, wiring, Calibration). factory.bin is one merged image from
offset 0 whose gap over NVS is filled with 0xFF, so writing it whole wipes the
settings. split_image() cuts it into parts that skip NVS; "Erase device" in the
flasher still wipes everything on purpose.
"""

import json
import os
import struct
import sys

CHIPS = {"esp32": "ESP32", "esp32s3": "ESP32-S3", "esp32c3": "ESP32-C3"}

PARTITION_TABLE = 0x8000
ENTRY = struct.Struct("<HBBII16sI")
ENTRY_MAGIC = 0x50AA
DATA, NVS = 0x01, 0x02


def settings_partition(image):
    """(offset, size) of the NVS partition, read from the image's partition table."""
    for at in range(PARTITION_TABLE, PARTITION_TABLE + 0xC00, ENTRY.size):
        magic, kind, subtype, offset, size, _, _ = ENTRY.unpack_from(image, at)
        if magic != ENTRY_MAGIC:
            break
        if kind == DATA and subtype == NVS:
            return offset, size
    raise ValueError("no settings (NVS) partition in the image's partition table")


def split_image(image):
    """[(offset, bytes)] covering the whole image except the NVS partition."""
    if len(image) < PARTITION_TABLE + 0xC00:
        raise ValueError("image too small to hold a partition table")
    start, size = settings_partition(image)
    return [(0, image[:start]), (start + size, image[start + size:])]


def part_names(env):
    return [f"catwandconnect-{env}-boot.bin", f"catwandconnect-{env}-app.bin"]


def write_parts(factory, folder, env):
    """Writes the parts of `factory` into `folder` and returns them as manifest parts."""
    with open(factory, "rb") as f:
        parts = split_image(f.read())
    listed = []
    for (offset, data), name in zip(parts, part_names(env)):
        with open(os.path.join(folder, name), "wb") as f:
            f.write(data)
        listed.append({"path": name, "offset": offset})
    return listed


def manifest(version, parts):
    """`parts` maps each chip env to its manifest parts."""
    return {
        "name": "CatWandConnect",
        "version": version,
        "new_install_prompt_erase": True,
        "builds": [{"chipFamily": family, "parts": parts[env]} for env, family in CHIPS.items()],
    }


def site_manifest(version, folder):
    """Splits every chip's factory image in `folder` and returns the manifest for them."""
    parts = {env: write_parts(os.path.join(folder, f"catwandconnect-{env}-factory.bin"), folder, env)
             for env in CHIPS}
    return manifest(version, parts)


if __name__ == "__main__":
    version, output = sys.argv[1], sys.argv[2]
    data = site_manifest(version, os.path.dirname(os.path.abspath(output)))
    with open(output, "w") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
