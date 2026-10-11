"""Sets CWC_VERSION for the firmware build.

The release workflow passes the tag's version in the CWC_VERSION
environment variable (v0.1.0 -> "0.1.0"). Local builds derive it from
git: "0.1.0-3-gabc1234" three commits after v0.1.0, or "0.0.0-dev+abc1234"
before any tag.
"""

import os
import subprocess

Import("env")  # noqa: F821  (provided by PlatformIO)


def version():
    if os.environ.get("CWC_VERSION"):
        return os.environ["CWC_VERSION"]
    try:
        described = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty", "--match", "v[0-9]*"],
            cwd=env["PROJECT_DIR"], stderr=subprocess.DEVNULL, text=True).strip()  # noqa: F821
    except (OSError, subprocess.CalledProcessError):
        return "0.0.0-dev"
    return described[1:] if described.startswith("v") else f"0.0.0-dev+{described}"


env.Append(CPPDEFINES=[("CWC_VERSION", env.StringifyMacro(version()))])  # noqa: F821
