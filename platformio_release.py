Import("env")
import os
import re
import shutil

def extract_project():
    config_path = env.subst("$PROJECT_DIR/include/config.h")
    with open(config_path, "r") as file:
        content = file.read()
        match = re.search(r'#define PROJECT\s+"(.+)"', content)
        if match:
            return match.group(1)
        else:
            return None

def extract_version():
    config_path = env.subst("$PROJECT_DIR/include/config.h")
    with open(config_path, "r") as file:
        content = file.read()
        match = re.search(r'#define VERSION\s+"(.+)"', content)
        if match:
            return match.group(1)
        else:
            return None

def merge_bin(target, source, env):
    project = extract_project()
    version = extract_version()

    build_dir = env.subst("$BUILD_DIR")
    binary_path = os.path.join(build_dir, "firmware.bin")
    bootloader_path = os.path.join(build_dir, "bootloader.bin")
    partitions_path = os.path.join(build_dir, "partitions.bin")
    release_path = os.path.join(env.subst("$PROJECT_DIR"), "release")

    # Leeren des release-Ordners
    if os.path.exists(release_path):
        shutil.rmtree(release_path)
    os.makedirs(release_path)

    merged_file = os.path.join(release_path, f"{project}_{version}_install.bin")
    ota_update_file = os.path.join(release_path, f"{project}_{version}_OTA_update.bin")

    env.Execute(f'esptool.py --chip ESP32 merge_bin -o {merged_file} --flash_mode dio --flash_size 4MB 0x1000 {bootloader_path} 0x8000 {partitions_path} 0x10000 {binary_path}')
    shutil.copyfile(binary_path, ota_update_file)

env.AddPostAction("buildprog", merge_bin)