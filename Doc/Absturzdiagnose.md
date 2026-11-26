# Absturzdiagnose

Unter **Einstellungen → Zugriff & Wartung → Absturzdiagnose** wird angezeigt, ob ein
Core-Dump gespeichert ist. Der Neustartgrund steht weiterhin unter **System**.
Ein Panic-Reset bezeichnet einen Absturz; ein normaler Software-Neustart ist
kein Nachweis für einen Panic.

**Details / Aktualisieren**, **Core-Dump herunterladen** und **Bericht löschen**
fragen das separate Service-Passwort ab. Ohne gesetztes Service-Passwort sind
Details, Download und Löschen gesperrt. Die normale WebUI-Anmeldung gilt zusätzlich,
wenn sie aktiviert ist. Der Browser speichert das Service-Passwort nicht dauerhaft.
Die Übertragung verwendet den vorhandenen WebSocket und dessen Transport.

Ein Download löscht den Dump nicht. Ein Neustart erhält ihn; ein weiterer Panic
kann ihn überschreiben. Löschen erfordert eine Bestätigung. Unvollständige oder
beschädigte Dumps werden als solche angezeigt und können gelöscht werden. Es
wird kein Absturzzeitpunkt behauptet: Ein gespeicherter Dump kann aus einem
früheren Firmwarestand stammen. Task, PC, Panic-Text und ELF-Hash werden angezeigt,
soweit sie aus dem Dump ausgelesen werden können.

## Voraussetzungen und Grenzen

Die aktuelle `min_spiffs.csv` enthält eine 64-KB-Core-Dump-Partition; im verwendeten
Arduino-ESP32-SDK ist Core-Dump nach Flash im ELF-Format aktiviert. Auf älteren
Geräten muss auch die tatsächlich geflashte Partitionstabelle dazu passen:
Ein OTA-Update der Anwendung ersetzt diese Tabelle nicht.

Der Dump enthält Task-Stacks und Register, keinen garantierten vollständigen
Speicherabzug. Bei stark beschädigtem Speicher, zu wenig Platz oder Stromausfall
kann er fehlen oder unvollständig sein. Ein Diagnosebericht kann Zugangsdaten
und andere Speicherinhalte enthalten und ist entsprechend vertraulich zu behandeln.

Die Anwendung prüft den Dump einmal beim Start. Lesen und Löschen erfolgen auf
dem Loop-Task; ein Download fordert jeweils maximal 384 Bytes an. Während OTA
sind geschützte Diagnoseoperationen gesperrt. Es gibt keine regelmäßigen
Flash-Schreibzugriffe für die Diagnose und keinen eigenen Panic-Handler.

## Passende Firmware sichern und auswerten

Jeder Build erzeugt zusätzlich
`release/buderus_km271_debug_<Version>.zip` mit `firmware.elf` und `build-info.json`.
Der Release-Workflow lädt dieses Archiv zusammen mit den Binaries hoch. Version
und Environment allein identifizieren einen Build nicht eindeutig: Entscheidend
ist die zur abgestürzten Anwendung passende ELF-SHA256. Bei lokalen Builds das
Archiv vor dem nächsten Build separat sichern.

Das ZIP entpacken und den ELF-Hash mit der Anzeige im Diagnosebereich vergleichen.
Das SDK zeigt derzeit nur die ersten neun Zeichen der ELF-SHA256 an; diese
müssen mit dem Anfang des vollständigen Hashs aus `build-info.json` übereinstimmen.
Ältere Dump-Formate können gültig sein, ohne dass das aktuelle SDK eine
Task-Zusammenfassung daraus liefert. Der binäre Download bleibt dann möglich.

Der heruntergeladene Dump ist ein unveränderter binärer Flash-Dump einschließlich
Header und Prüfsumme. Mit einem zum verwendeten ESP-IDF passenden `esp-coredump`
kann er beispielsweise so ausgewertet werden (Werkzeug separat installieren):

```sh
esp-coredump info_corefile -t raw -c buderus-coredump-<Hash>.bin firmware.elf
```

Für vollständige ROM-Symbole kann zusätzlich die passende ESP32-ROM-ELF nötig sein.
Siehe https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/core_dump.html

## Prüfung am Testgerät

Nur ausdrücklich freigegebene Testgeräte verwenden. Vor einem Testabsturz einen
bestehenden Bericht herunterladen und die passende Firmware-ELF sichern.

1. Status ohne Dump, falsches Service-Passwort und normale WebUI-Anmeldung prüfen.
2. Per authentifiziertem Telnet `test crash` ausführen. Das startet den ESP neu;
   keine Heizungsparameter schreiben.
3. Nach dem Wiederverbinden Panic-Reset, Dump-Status, Task und ELF-Hash prüfen.
4. Dump herunterladen, Prüfsumme und Auswertung mit der archivierten ELF prüfen.
5. Normal neu starten: Bericht muss erhalten bleiben. Download abbrechen und
   erneut versuchen; ein Teil-Download darf nicht als fertige Datei erscheinen.
6. Löschdialog abbrechen: Bericht bleibt. Löschen bestätigen: kein Bericht mehr,
   auch nach Neustart. Auf Desktop und schmalem Bildschirm sowie Deutsch/Englisch prüfen.

Host-Tests ersetzen diese Hardwareprüfung nicht. Ein künstlicher Panic beweist
auch nicht, dass die Ursache der ursprünglich sporadischen Abstürze behoben ist.
