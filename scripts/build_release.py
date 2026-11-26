Import("env")
import os
import re
import shutil

OTA_NAME = "buderus_km271_ota_update"           # prefix for ota release image
INSTALL_NAME = "buderus_km271_esp32_flash"      # prefix for merged flash image

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
RELEASE_PATH = "$PROJECT_DIR/release"
BOARD_CONFIG = env.BoardConfig()


# extract program version from /include/config.h
def extract_version():
    config_path = env.subst("$PROJECT_DIR/include/config.h")
    with open(config_path, "r") as file:
        content = file.read()
        match = re.search(r'#define VERSION\s+"(.+)"', content)
        if match:
            return match.group(1)
        else:
            return None

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]
    # Run esptool to merge images into a single binary
    result = env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge-bin",
                "--flash-size", #optional: --fill-flash-size
                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "-o",
                MERGED_BIN,
            ]
            + flash_images
        )
    )

    if result:
        return result

    merged_file, ota_update_file = [node.get_abspath() for node in target]
    os.makedirs(os.path.dirname(merged_file), exist_ok=True)
    shutil.copyfile(env.subst(MERGED_BIN), merged_file)
    shutil.copyfile(env.subst(APP_BIN), ota_update_file)


# Track exported files separately from firmware.bin. Missing exports are rebuilt
# even when the firmware is current; other environments' exports stay untouched.
version = extract_version()
if not version:
    raise ValueError("Keine Version in include/config.h gefunden!")

release_path = env.subst(RELEASE_PATH)
flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", []))
release_files = env.Command(
    [os.path.join(release_path, f"{INSTALL_NAME}_{version}.bin"),
     os.path.join(release_path, f"{OTA_NAME}_{version}.bin")],
    [APP_BIN, "$PROJECT_DIR/include/config.h"] + flash_images[1::2],
    merge_bin,
)
env.Depends("buildprog", release_files)

# Archive the exact linked ELF; rebuilding a tag later need not reproduce the
# same addresses or ELF hash. Keep this alongside the corresponding binaries.
def archive_debug(source, target, env):
    import hashlib
    import json
    import zipfile
    elf = source[0].get_abspath()
    digest = hashlib.sha256(open(elf, "rb").read()).hexdigest()
    metadata = {"version": version, "environment": env.subst("$PIOENV"),
                "elf_sha256": digest, "platform": env.GetProjectOption("platform")}
    archive = target[0].get_abspath()
    os.makedirs(os.path.dirname(archive), exist_ok=True)
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
        bundle.write(elf, "firmware.elf")
        bundle.writestr("build-info.json", json.dumps(metadata, indent=2) + "\n")


debug_archive = env.Command(
    os.path.join(release_path, f"buderus_km271_debug_{version}.zip"),
    ["$BUILD_DIR/${PROGNAME}.elf", "$PROJECT_DIR/scripts/build_release.py", "$PROJECT_DIR/platformio.ini"],
    archive_debug,
)
env.Depends("buildprog", debug_archive)
