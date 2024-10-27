from collections import Counter
import re

print("\n>>>> Prüfung auf doppelte oder nicht verwendete Texte <<<<")

# Pfade zur JS- und HTML-Datei
js_file_path = 'web/js/lang.js'
html_file_path = 'web/temp/index.html'

# 1. Lang.js auslesen
with open(js_file_path, 'r', encoding='utf-8') as js_file:
    js_content = js_file.read()

# 2. Alle Schlüssel in der lang.js extrahieren
js_pattern = r'\b(\w+):\s*{'
js_keys = re.findall(js_pattern, js_content)

# Zähle die Vorkommen der Schlüssel, um doppelte Einträge zu finden
key_counts = Counter(js_keys)
duplicates = {key: count for key, count in key_counts.items() if count > 1}

if duplicates:
    print("Doppelte Einträge gefunden:")
    for key, count in duplicates.items():
        print(f"'{key}' kommt {count} mal vor")
else:
    print("Keine doppelten Einträge in lang.js gefunden.")

# 3. Index.html auslesen
with open(html_file_path, 'r', encoding='utf-8') as html_file:
    html_content = html_file.read()

# 4. Alle data-i18n-Tags in der HTML-Datei extrahieren (alle Varianten: alleinstehend, $ $ und $-$)
html_pattern = r'data-i18n="([\w-]+(?:\$[- ]\$\w+)*)"'  # Erfasst alleinstehend, $-$ und $ $
html_tags = re.findall(html_pattern, html_content)

# 5. Zerlege die data-i18n-Werte in einzelne Teile (z.B. hc1 und pump)
html_keys = set()
for tag in html_tags:
    html_keys.update(tag.split('$-$'))
    html_keys.update(tag.split('$ $'))
    html_keys.add(tag)

# 6. Überprüfen, ob die JS-Schlüssel in der HTML-Datei verwendet werden
unused_keys = set(js_keys) - html_keys  # Überprüft, welche JS-Schlüssel nicht in der HTML vorkommen

if unused_keys:
    print("\nDie folgenden Schlüssel aus lang.js werden in der HTML-Datei nicht verwendet:")
    for key in unused_keys:
        print(f"'{key}'")
else:
    print("\nAlle Schlüssel aus lang.js werden in der HTML-Datei verwendet.")

print("\n>>>> Prüfung beendet <<<<")