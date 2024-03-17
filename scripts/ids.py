def extract_ids_from_html(file_path):
    # Öffnen der HTML-Datei und Lesen des Inhalts
    with open(file_path, 'r') as file:
        html_content = file.read()

    # Finden aller Vorkommen von id=""
    id_list = []
    start_pos = 0
    while True:
        start_pos = html_content.find('id="', start_pos)
        if start_pos == -1:
            break  # Beenden, wenn keine weiteren id="" gefunden werden
        start_pos += 4  # Überspringen von id="

        end_pos = html_content.find('"', start_pos)
        if end_pos == -1:
            break  # Sicherstellen, dass ein Endzitat vorhanden ist

        id_value = html_content[start_pos:end_pos]
        id_list.append(id_value)

        start_pos = end_pos  # Fortsetzen der Suche nach dem letzten Endzitat

    return id_list

# Angenommen, Sie haben eine Funktion, die die IDs extrahiert
# Beispiel: Ihre HTML-Datei heißt 'example.html'

file_path = 'web/temp/index.html'  # Pfad zur HTML-Datei
ids = extract_ids_from_html(file_path)

# Schreiben der extrahierten IDs in eine neue Datei
with open('ids.txt', 'w') as file:
    for id in ids:
        file.write(id + '\n')
