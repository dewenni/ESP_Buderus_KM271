# Zusammenarbeit an den ESP-Projekten

## Geltungsbereich

Diese Datei gilt für `ESP_Buderus_KM271`. Gemeinsame Komponenten liegen in
`EspWebUI`; das verwandte Projekt heißt `ESP32-Jarolift-Controller`.
Änderungen an einem Repository sind nicht automatisch ein Auftrag, die anderen
Repositories ebenfalls zu ändern. Den Umfang aus dem aktuellen Auftrag ableiten.

## Arbeitsweise und Kommunikation

- Auf Deutsch, konkret und knapp kommunizieren. Technische Entscheidungen so
  erklären, dass der Maintainer den Code selbst weiterentwickeln kann.
- Ideen kritisch bewerten. Widersprechen, wenn ein Vorschlag Bedienbarkeit,
  Wartbarkeit oder das vereinbarte Design verschlechtert; eine einfache
  Alternative nennen.
- Bei größeren Aufgaben zuerst ein Konzept für die Umsetzung erarbeiten und vorstellen.
  Zum Abschluss sagen, was geändert und geprüft wurde und was noch am echten Gerät zu testen ist.
- Ein Konzept oder Mockup ist noch kein Auftrag zur produktiven Umsetzung.
  Vorschauen unter `work/` ablegen und von der eigentlichen WebUI trennen.
- Bereits erteilte Freigaben berücksichtigen und für notwendige, abgedeckte
  Arbeitsschritte nicht erneut nach inhaltlicher Bestätigung fragen.

## Branches und Commits

- Vor Änderungen den aktuellen Branch und offene Änderungen prüfen.
- Für größere Features, Umbauten und Experimente früh einen eigenen Branch
  vorschlagen. Ist die Umsetzung beauftragt, vor den Änderungen einen passenden
  Feature- oder Test-Branch anlegen, sofern noch keiner besteht.
- Kleine, zusammengehörige Korrekturen benötigen nicht zwingend einen neuen
  Branch. Größere unfertige Änderungen nicht auf `main` ansammeln.
- Unabhängige Themen getrennt halten. Wenn eine Aufgabe ein neues Thema berührt,
  die Trennung direkt ansprechen, bevor weitere Änderungen vermischt werden.
- Nach abgeschlossenen Teilaufgaben aktiv einen sinnvollen Commit und eine kurze
  englische Commit-Message vorschlagen. Nur die zugehörigen Dateien einbeziehen;
  kein pauschales `git add .` bei gemischten Änderungen.
- Die Bitte um eine Commit-Message ist kein Auftrag, selbst zu committen.
- Vor einem Merge Umfang, offene Änderungen und relevante Prüfungen kontrollieren.
  Merge, Push, Tag und Release nur im beauftragten Umfang durchführen. Ein lokaler
  Merge bedeutet nicht automatisch, dass auch gepusht werden soll.
- Nutzeränderungen erhalten. Nichts ungefragt verwerfen und keine bestehenden
  Commits umschreiben. Nach einem Auftrag den tatsächlichen Git-Stand verifizieren.

## Builds und Releases

- `web/output/` und die Firmware-Binaries im jeweiligen Release-Verzeichnis
  werden durch Build-Skripte erzeugt. Änderungen an den Quellen vornehmen,
  nicht ausschließlich an generierten Dateien.
- Während der Entwicklung generierte Web-Dateien und Binaries standardmäßig
  nicht in normale Quellcode-Commits aufnehmen. Lokal dürfen sie für Tests
  aktualisiert werden.
- Bei einem fertigen Release gehören sowohl die erzeugten Web-Dateien als auch
  die passenden `.bin`-Dateien in den Release-Stand und müssen mit gepusht werden.
- Keine Versionsanhebung, Tags oder Veröffentlichung ohne entsprechenden Auftrag.
- Build-Skripte können alte Binaries entfernen oder gemeinsame erzeugte Header
  überschreiben. Solche Nebeneffekte prüfen und keine fremden Änderungen bereinigen.
- Builds beider Anwendungen bei gemeinsam erzeugten EspWebUI-Headern nacheinander
  ausführen. Prüfen, dass jeder Build seine eigenen Projektinhalte enthält.
- Lokale EspWebUI-Änderungen nur dann als übernommen betrachten, wenn die
  Abhängigkeit tatsächlich auf die lokale Library zeigt. Gepinnte Releases und
  Kopien unter `.pio/libdeps/` unterscheiden.

## Code und gemeinsame Komponenten

- Möglichst schlanke, verständliche Lösungen bevorzugen. Keine zusätzlichen
  Abstraktionen, Module, Absicherungen oder Abhängigkeiten ohne konkreten Bedarf.
- Die Ressourcen des ESP berücksichtigen: begrenzte Puffer, wenig dynamische
  Allokationen, keine unnötigen Kopien und keine zusätzlichen Abfragen oder
  Timer, wenn vorhandene Daten und Ereignisse ausreichen.
- Bestehende JSON-/WebSocket-Mechanismen weiterverwenden. Neue Transportwege oder
  Protokolle nur bei einem nachgewiesenen Bedarf einführen.
- Gemeinsame Basisfunktionen beider Anwendungen möglichst identisch halten.
  Bei einer beauftragten Übernahme identischen Code 1:1 übernehmen und nur die
  notwendigen projektspezifischen Teile anpassen.
- Allgemeine WebUI-Funktionen nach Möglichkeit in EspWebUI pflegen;
  anwendungsspezifische Datenverarbeitung im jeweiligen Projekt belassen.
- Änderungen an Libraries an deren eigentlicher Quelle vornehmen, nicht nur in
  einer generierten oder heruntergeladenen Kopie unter `.pio/libdeps/`.
- Umfangreiche Nebenbereinigungen getrennt vorschlagen und nicht mit einer
  funktionalen Änderung vermischen.

## WebUI und Bedienung

- Das vorhandene Grau-Blau-Design mit dezentem Orange weiterführen. Bestehende
  Theme-Variablen, Layouts und Komponenten nutzen, bevor neue CSS-Regeln entstehen.
- Gemeinsame UI-Teile in beiden Projekten bei Übernahmen identisch gestalten.
  Projektspezifische Seiten an denselben Gestaltungsregeln ausrichten.
- Desktop- und schmale/mobile Ansichten berücksichtigen. Optionale Elemente
  dürfen keine unbrauchbaren Lücken, Überläufe oder unnötigen Hinweise erzeugen.
- Deutsch und Englisch berücksichtigen. Vorhandene Übersetzungsmechanismen nutzen
  und das Text-Prüfskript auch für in JavaScript verwendete Texte beachten.
- Vorhandene IDs und Ereignis-Anbindungen erhalten oder ihre Verwendung vollständig
  anpassen. Rein lokale UI-Aktionen dürfen keine unbeabsichtigten Gerätebefehle senden.
- Status, Schätzwert und tatsächlicher Messwert klar unterscheiden. Datenlücken
  oder fehlende Rückmeldungen nicht als gesicherte Werte darstellen.

## Prüfung und Abschluss

- Prüfungen passend zur Änderung wählen: etwa Syntax und Darstellung bei UI-
  Änderungen, Builds und gezielte Verhaltenstests bei Firmware-Änderungen.
- Tests sollen relevantes Verhalten absichern. Keine umfangreichen Tests nur für
  triviale Format- oder Abstandsänderungen hinzufügen. Wenn die Funtkion fertig ist, können die zusätzlich erstellten Tests auch wieder gelöscht werden.
- Erfolgreicher Build, Softwaretest und Test auf realer Hardware sind verschiedene
  Nachweise. Nicht behaupten, ein OTA-Update, Funkempfang oder eine Motorfahrt sei
  praktisch getestet worden, wenn nur der Build oder ein Test mit Nachbildungen lief.
- Geräte nicht allein zur Prüfung ungefragt flashen, neu starten oder Aktoren bewegen.
- Am Ende verbleibende Einschränkungen nennen sowie klarstellen, ob nur lokal
  geändert, committet, gemerged oder bereits gepusht wurde.
