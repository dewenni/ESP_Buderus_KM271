# Schaltzeiten von HK1 und HK2 (ID04)

## Bedienung

Auf der Seite des Heizkreises zeigt **Eigen-Programm** sieben kompakte
Tageszeilen. Das Raster hat Stundenlinien, kräftigere Linien alle sechs Stunden
und eine gemeinsame Skala 00–24. Orange Balken zeigen die Heizzeiten. Mouse-over
oder Tastaturfokus zeigt den vollständigen Zeitraum; ein Klick öffnet den Editor.

**Plus** am Tagesende fügt eine Heizzeit hinzu. Dabei lassen sich mehrere Tage
wählen; pro Tag werden eigenständige Ein-/Aus-Punkte angelegt. Vorhandene Zeiten
werden nicht überschrieben. Die Auswahlfelder verwenden Zehnminutenschritte;
Aus um 24:00 und ein Ende am Folgetag sind möglich. Über Mitternacht oder den
Wochenwechsel dargestellte Teilbalken öffnen denselben Zeitraum.

**Speichern** überträgt die abgeschlossene Dialogaktion direkt. Die Auswahl eines
Wertes allein sendet nichts. **Abbrechen** verwirft die lokale Eingabe.
Entfernen benötigt eine Bestätigung im Dialog. Es gibt keinen zentralen
Wochenentwurf, keine anklickbaren Tagesnamen, keine zusätzliche Einzelpunktliste
und keine Anzeige des aktiven Programms in dieser Karte. Das Speichern ändert
auch den Programmselektor nicht; die Auswahl des aktiven Programms bleibt in der
vorhandenen Bedienung. Aus einem unveränderten Selektor darf keine garantierte
Wirkungslosigkeit der neuen Zeiten auf das Laufzeitverhalten abgeleitet werden.

Während eines Auftrags sind weitere Timeränderungen gesperrt. Der Dialog darf
währenddessen geschlossen werden; das bricht eine bereits begonnene Übertragung
nicht ab. „Gespeichert“ erscheint erst nach allen zugehörigen Transportquittungen.
Es handelt sich um angenommene Übernahme nach ACK, nicht um unabhängiges Rücklesen.

## Daten und Ablauf im Code

- `include/km271Timers.h`: kompakter Rohdaten-Decoder und Änderungs-Encoder ohne
  Heap-Allokationen. 14 Blöcke × drei Zweibytepunkte = 42 Punkte je Heizkreis,
  höchstens 21 vollständige Heizzeiten. Die physische Position eines vorhandenen
  Punktes bleibt erhalten. Eindeutige Ein-/Aus-Zuordnung erfolgt chronologisch
  und zyklisch über die Woche, nicht über sprachabhängige Timertexte.
- `src/km271.cpp`: Rohcache von 84 Bytes je Heizkreis, Gültigkeit pro Block,
  Revision und Sitzung sowie Status des letzten Timerauftrags. Ein vollständiger
  Stand aus der aktuellen Sitzung ist Voraussetzung zum Schreiben. Boot-spezifische
  Sitzungskennung plus Revision schützen vor alten Browserentwürfen.
- `km271WriteTimers()`: prüft den vollständigen Auftrag und erzeugt nur geänderte
  Blöcke. Schreibtyp 11/12, Offset `7 × (Blockindex + 1)`, sechs vollständige
  Nutzbytes. Spätere Blöcke werden zuerst übertragen. Eine gesamte Aktion wird
  angenommen oder vollständig abgewiesen, bevor ein Telegramm gesendet wird.
- Der vorhandene Queue-/3964-Pfad übernimmt Übertragung, DLE-Verdopplung,
  Prüfsumme, Wiederholungen und Abschlussquittung. Während eines Timerauftrags
  werden andere Schreibanforderungen abgewiesen. Timeraufträge beginnen nur bei
  leerer Queue; rohe Service-Timertelegramme werden abgewiesen, damit sie die
  Prüfung aktueller Nachbarbytes nicht umgehen können.
- Empfang und ACK-Nachführung verwenden denselben Timerblock-Handler. Dieser
  erhält die vorhandenen MQTT-Timertopics, Sprache und Veröffentlichung über
  `km271Msg(KM_TYP_CONFIG, ...)`. Zeitbyte `65` bedeutet hier 16:50 und wird nie
  als Keep-Marker ausgewertet. Unbenutzte Punkte sind `C2 90`, Sonntag 24:00 Aus
  ist dagegen `C0 90`.
- `src/webUIcallback.cpp`: `timer_write` nimmt den Änderungsauftrag über die
  bestehende WebSocket-/Loop-Queue an und meldet Annahme oder Ablehnung mit ID.
- `src/webUIupdates.cpp`: `km271Timers` liefert bei Änderungen bzw. Browserabruf
  den Rohstand beider Heizkreise, Verbindung, Sitzung, Revision, Gültigkeit,
  Auftrags-ID und Zustand. Kein zusätzlicher Timer oder serielles Abfrageintervall.
- `web/js/weekly_program.js`: Diagramm, gemeinsamer nativer Dialog, lokale
  Vorprüfung und Auswertung der ESP-Rückmeldung. Eine Annahme ist noch kein
  Erfolg; maßgeblich ist der zur ID gehörige Abschlusszustand. Die Firmware
  wiederholt die Eingabeprüfung unabhängig vom Browser.

Die WebUI ruft keinen neuen MQTT-Schreibbefehl auf. Bestätigte Timeränderungen
werden über die bestehenden Config-Timertopics veröffentlicht.

## Fehler und Grenzen

Ungültiges Raster, Überschneidungen, gemeinsame Schaltpunkte zum gleichen
Zeitpunkt, fehlender Platz, deaktivierte Heizkreise, Simulation, unvollständiger
Stand und veraltete Revisionen werden vor dem Schreiben abgewiesen. Nach einer
Browser-Wiederverbindung muss zuerst ein neuer Snapshot eintreffen.

Mehrblockänderungen sind auf dem Gerät **nicht atomar**. Nach endgültigem NAK,
Handshake-Timeout oder abweichenden empfangenen Timerdaten während eines Auftrags
werden noch ausstehende Telegramme verworfen, der Auftrag als fehlgeschlagen
markiert und die aktuelle Gültigkeit zurückgesetzt. Der Logging-Aufbau fordert
wieder einen vollständigen Stand an. Bereits quittierte Rohbytes bleiben im
Cache; es gibt kein blindes Zurückrollen. Die WebUI verlangt das Schließen und
erneute Prüfen mit aktuellen Daten.

Ein Verbindungsabbruch oder eine ausbleibende Anwendungsantwort wird im Browser
als unbekanntes Ergebnis angezeigt. Nach spätestens 120 Sekunden endet das
lokale Warten, ohne automatisches erneutes Senden. Diese Wartebegrenzung ist kein
Beweis, dass ein Geräteauftrag fehlgeschlagen ist.

Nicht eindeutig paarbare, redundante oder unzulässige Gerätedaten werden nicht
normalisiert und nicht zur Bearbeitung freigegeben. Zeiträume, deren Ende später
als 24:00 des Folgetages liegt, sind mit dem vereinbarten Dialog nicht bearbeitbar.
Es gibt bewusst keinen zusätzlichen Einzelpunkteditor. Sehr kurze Balken bleiben
maßstabsgetreu und können auf Touchgeräten schwer zu treffen sein; die zusätzliche
Tagesauswahl wurde entsprechend dem abgestimmten Konzept entfernt.

## Durchgeführte Prüfungen

- Firmware-/WebUI-Build mit `pio run -e esp32`, einschließlich Textprüfung.
- Python-Hostsuite einschließlich C++-Replay des produktiven Transportcodes und
  Node.js-Prüfungen des tatsächlichen Browsercodes.
- 18 kompakte Before-/Wire-/After-Fälle aus den direkten Hardwareaufzeichnungen
  vom 27.11.2026: beide Heizkreise, alle 14 Blöcke, leere Punkte, Vollbelegung,
  Wiederherstellung, DLE-Zeitbyte und Sonntag 24:00. Die aufgezeichneten
  Drahttelegramme werden mit dem produktiven Framer und ACK-Updater abgespielt.
- Zusätzliche Hostprüfungen für Encoder, Blockreihenfolge, unveränderte Nachbarn,
  0x65, MQTT-Topic und -Text, Mehrtages-Anlage, Kapazität, stabile Positionen,
  alte Sitzung/Revision, konkurrierende Aufträge, verkürzten Empfang,
  Teil-NAK, Timeout, verspätete ACKs und externe Änderungen während eines Auftrags.
- UI-Tests: Öffnen/Ändern/Abbrechen ohne Senden, Plus, lokale Vorprüfung,
  Auftragskennung, Warten auf Abschluss, Teilfehler, unvollständiger Stand,
  Browserverlust und ausbleibende Antwort ohne Wiederholung.
- Safari-Sichtprüfung der gebauten Demo. Separater lokaler Dialogtest mit echten
  UI-Quellen und simulierten ESP-Snapshots: Zeitänderung, Warten, Bestätigung,
  Mehrtagesauswahl, Deutsch/Englisch, Raster bei 390 Pixel Komponentenbreite und
  Tooltip bei Tastaturfokus. Dabei wurden keine Gerätebefehle gesendet. Der
  temporäre Browser-Testaufbau wurde anschließend entfernt.

## Noch am echten ESP/Testsystem prüfen

Die früheren direkten RS232-Tests des originalen KM271 sind Protokollnachweise.
Sie ersetzen keinen Test des jetzt ergänzten ESP-/WebUI-/MQTT-Wegs.

1. Ausgangsprogramm und beide Wochenpläne sichern; auf vollständige Timerdaten
   warten. Ein Intervall innerhalb eines Blocks ändern und am Gerät prüfen.
2. Ein blockübergreifendes Intervall ändern und entfernen. Nachbarn und Programm-
   selektor vor/nach der Änderung vergleichen. Config-Timertopics am Broker prüfen.
3. Mehrere Tage ergänzen, beide Heizkreise getrennt prüfen. 02:40, 16:50,
   Sonntag/Montag und Aus um 24:00 prüfen; Ausgangsstand wiederherstellen.
4. Zwei Browser mit demselben Ausgangsstand öffnen. Im ersten ändern; ein im
   zweiten bereits offener Dialog muss vor dem Senden als veraltet erkannt werden.
5. Browserverbindung während einer Übertragung unterbrechen und danach den
   tatsächlich eingelesenen Stand prüfen. Teil-NAK/Timeout bleiben zunächst im
   Host-Replay geprüft; keine unbekannten Gerätebefehle zur Fehlerprovokation.
6. Auf einem Smartphone insbesondere sehr kurze Balken, Dialoghöhe, Tastatur-
   bzw. Touchbedienung und Darstellung in beiden Sprachen prüfen.

Kein automatisches Flashen, kein Neustart der Steuerung und keine Prüfung der
Dauerhaftigkeit nach Stromausfall sind Bestandteil der lokalen Implementierung.
