import re
import logging
import sys
import json

# Logger konfigurieren
logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')

# Version aus changeNew.md extrahieren
def get_version_from_changenew(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            # Suche nach der Version in der Form "# v4.x.x"
            match = re.match(r'#\s*v([\d]+)\.([\d]+)\.([\d]+)', line)
            if match:
                return f'v{match.group(1)}.{match.group(2)}.{match.group(3)}'
    return None

# Version aus include/config.h extrahieren
def get_version_from_config(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            # Suche nach der Definition in der Form "#define VERSION "v4.x.x""
            match = re.match(r'#define\s+VERSION\s+"(v[\d]+\.[\d]+\.[\d]+)"', line)
            if match:
                return match.group(1)
    return None


# Hauptfunktion
changenew_version = get_version_from_changenew('changeNew.md')  # Pfad zum Root-Verzeichnis
config_version = get_version_from_config('include/config.h')

if changenew_version is None:
    logging.error("Konnte die Version aus changeNew.md nicht finden.")
    sys.exit(1)

if config_version is None:
    logging.error("Konnte die Version aus include/config.h nicht finden.")
    sys.exit(1)

if changenew_version != config_version:
    logging.warning(f"Versionskonflikt: changeNew.md hat die Version '{changenew_version}', "
                    f"aber include/config.h hat die Version '{config_version}'.")


# Funktion zum Ersetzten der Version in der sim.json für github-pages
def update_json_version(json_path, config_path, keys):

    version = get_version_from_config(config_path)
    if not version:
        raise ValueError("Keine Version in der Config-Datei gefunden!")

    with open(json_path, 'r') as f:
        data = json.load(f)

    for key in keys:
        if key in data:
            data[key] = version
        else:
            print(f"Warnung: Schlüssel '{key}' nicht in der JSON-Datei gefunden!")

    with open(json_path, 'w') as f:
        json.dump(data, f, indent=4)
    
keys= [
    'p00_version',
    'p09_sw_version',
    'p00_dialog_version',
]
update_json_version("web/output/sim.json", "include/config.h", keys)