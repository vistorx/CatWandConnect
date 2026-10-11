"""After each build, writes factory.bin next to firmware.bin.

factory.bin is the whole flash image (bootloader, partition table, OTA
data and app) written at offset 0 by the web flasher. firmware.bin alone
is what OTA updates take.
"""

Import("env")  # noqa: F821  (provided by PlatformIO)


def merge(source, target, env):
    build = env.subst("$BUILD_DIR")
    parts = [(offset, image) for offset, image in env.get("FLASH_EXTRA_IMAGES", [])]
    parts.append((env.subst("$ESP32_APP_OFFSET"), f"{build}/firmware.bin"))
    args = " ".join(f"{env.subst(offset)} {env.subst(image)}" for offset, image in parts)
    command = (f'"$PYTHONEXE" "$OBJCOPY" --chip {env.BoardConfig().get("build.mcu")} merge_bin '
               f'--flash_mode keep --flash_freq keep --flash_size keep -o "{build}/factory.bin" {args}')
    return env.Execute(command)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge)  # noqa: F821
