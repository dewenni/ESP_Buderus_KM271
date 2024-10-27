import re
import logging
import sys

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

