# KM271: Parameter schreiben und Vorbereitung für ID04

ID03 führt nach einer erfolgreichen Schreibquittung direkt in den laufenden
Empfang zurück. Der Logmode-Befehl `EE 00 00` wird beim Verbindungsaufbau und
nach einer notwendigen Synchronisierung gesendet, nicht nach jedem Parameter.
Die bestehende Empfangs- und Telegrammkodierung bleibt erhalten.

## Aufträge und Speicher

`km271QueueWrite(const uint8_t (*telegrams)[8], size_t count)` nimmt einen
vollständigen Auftrag mit einem oder mehreren fertig kodierten
8-Byte-Telegrammen an. Die Daten werden beim Einreihen kopiert.

Die feste FIFO hat 32 Plätze einschließlich des aktiven Telegramms. Das ist
ein vorläufiges Speicherbudget, kein Nachweis, dass ein vollständiges
ID04-Schaltprogramm hineinpasst. Der spätere Encoder muss seine maximale
Auftragsgröße dagegen prüfen; bei Bedarf ist das Budget gezielt anzupassen.
Leere/ungültige Aufträge, Platzmangel und eine nicht aufgebaute Verbindung
führen zu `false`; dabei wird kein Teil des Auftrags eingereiht. Die bisherigen
Parametersender verwenden denselben Weg mit genau einem Telegramm. Die
bisherige Ablehnung von Service-Telegrammen mit erstem Byte `00` bleibt erhalten.

Einreihen und Entnehmen sind zwischen AsyncTCP- und Loop-Task geschützt.
Serielle Ausgabe, JSON-Verarbeitung und Meldungen laufen außerhalb der
kritischen Abschnitte. Andere Schreibaufträge stehen vollständig vor oder
hinter einem mehrteiligen Auftrag; dessen Telegramme werden nicht vermischt.
Bereits begonnene Datenempfänge werden weiterhin abgearbeitet. Neue ausgehende
Telegramme werden wie bisher bei einer empfangenen STX-Anfrage angefordert.

## Quittierung und Fehler

Sendefreigabe und Abschlussquittung sind getrennte Zustände. Das aktive
Telegramm wird erst nach seiner Abschlussquittung aus der FIFO entfernt.
Ein unaufgefordertes `DLE` im normalen Empfang löst keinen Versand aus;
einzelne Steuerzeichen gelangen nicht in den Wertedecoder.

- Für Sendefreigabe und Abschlussquittung gelten jeweils 2000 ms Wartezeit,
  auch beim Logmode-Aufbau. Die Prüfung erfolgt in `cyclicKM271()`; der
  bestehende blockierende UART-Leseaufruf kann die Erkennung um bis zu dessen
  200 ms Lese-Timeout verzögern.
- Ein ausdrückliches `NAK` erlaubt höchstens zwei Wiederholungen desselben
  Telegramms. Danach wird der Rest dieses Auftrags verworfen. Bereits
  quittierte Teile bleiben als möglicherweise wirksam zu berücksichtigen;
  getrennte Folgeaufträge können weiterlaufen.
- Bei fehlender Quittung oder unerwarteter Antwort wird der Zustand als
  unklar behandelt: keine automatische Wiederholung, alle offenen Aufträge
  verwerfen und den Logmode neu synchronisieren.
- Auch der vorhandene Empfangsausfall nach mehr als 120 Sekunden verwirft
  alle offenen Aufträge, bevor neue empfangene Bytes verarbeitet werden.
  Während der Synchronisierung werden neue Schreibaufträge abgelehnt.

Ein erfolgreicher Schreibvorgang erhält den Verbindungsstatus, die
Alarmgültigkeit und die Diagrammgültigkeit. Er startet keinen vollständigen
Refresh. Erst eine echte Synchronisierung invalidiert die Live-Gültigkeit.

Meldungen unterscheiden das Einreihen, die Transportquittierung eines ganzen
Auftrags und einen Abbruch einschließlich der Anzahl bereits quittierter
Telegramme. Erfolgreiches Einreihen und Quittieren sind Debug-Meldungen und
werden nicht per Pushover versendet. Die eigentliche Befehlsmeldung sowie
Ablehnungen und Abbrüche bleiben normale Informationsmeldungen.
Eine Transportquittung bestätigt keine unabhängig zurückgelesene
Übernahme. Für die 35 unterstützten Parameter wird nach der einzelnen
Abschlussquittung eine Übernahme angenommen: Nur die tatsächlich geschriebenen
Felder werden im numerischen Cache, Anzeigetext und per `KM_TYP_CONFIG` über
den bestehenden MQTT-Weg aktualisiert (einschließlich Topic, Sprache und
Retain-Einstellung). Die WebUI übernimmt die Caches über ihren bisherigen
Aktualisierungsweg. Echte Empfangstelegramme und quittierte Schreibtelegramme
verwenden dieselbe Feldverarbeitung; spätere echte Rückmeldungen überschreiben
die angenommenen Werte immer.

`0x65`-Felder bleiben unverändert. Ohne Abschlussquittung (NAK, Timeout oder
unerwartete Antwort) erfolgt keine lokale Config-Aktualisierung. Ein bereits
quittierter Teil eines später abgebrochenen Auftrags bleibt dagegen lokal
nachgeführt. Betriebswerte wie die aktuelle Warmwasser-Solltemperatur werden
nicht aus Config-Schreibbefehlen abgeleitet.

Unterstützt sind Betriebsart, Tages-/Nacht-/Urlaubstemperatur, Sommer-/Frostgrenze,
Auslegung, Aufschalttemperatur, Abschaltgrenze, Absenkungsart, Programm und
Urlaubstage beider Heizkreise sowie Warmwassertemperatur, -betriebsart und
Zirkulationsintervall. Zusätzlich sind HK-Maximaltemperatur, Temperaturabgleich
und Heizsystem beider Heizkreise sowie WW-Aktivierung und -Vorrang angebunden.
Zuordnung und Gerätetest: `Doc/KM271-Zusaetzliche-Parameter.md`. Bekannte Felder eines Service-Schreibtelegramms nutzen
auch diesen Weg; unbekannte Befehle, Datum/Uhrzeit und bislang nicht unterstützte
Timer-Schreibtelegramme erzeugen keine angenommenen Config-Werte. Datum/Uhrzeit
werden beim Einreihen weiterhin als vorgemerkt gemeldet.

Die automatische NTP-Erstsynchronisierung wartet auf den Logmode und setzt ihr
Erledigt-Flag erst nach erfolgreichem Einreihen. Ein abgelehnter Auftrag wird
über den vorhandenen Zehnsekundenzyklus erneut versucht. Bei einer abgelehnten
Sommerzeit-Anpassung bleibt die alte Referenz für den nächsten Stundenzyklus
erhalten. Ein späterer Transportabbruch wird damit nicht automatisch wiederholt.

## Grenzen für ID04

Die Schnittstelle stellt Reihenfolge und vollständige lokale Auftragsannahme
sicher. Sie bietet keine atomare Änderung oder Rücknahme in der Logamatic.
Ein Abbruch kann einen teilweise geänderten Zustand hinterlassen.

ID04 muss Telegramminhalt, Anzahl, Reihenfolge, mögliche Abschlussbefehle,
Wiederholbarkeit und Rückmeldung des „Eigen“-Programms an der Teststeuerung
klären. Falls einzelne Befehle nicht wiederholbar sind, muss deren Auftrag
vor dem Einsatz eine passende Wiederholungsregel erhalten. Bis dahin werden
keine neuen Schaltzeit-Telegramme erzeugt oder angeboten.

Die ESPHome-Referenz liefert Ansätze für getrennte Sendefreigabe/Quittierung
und eine feste Queue; ihr Code wurde nicht übernommen:

- https://github.com/the78mole/esphome_components/blob/main/components/km271_wifi/km271.cpp
- https://github.com/the78mole/esphome_components/blob/main/components/km271_wifi/3964r.cpp
- https://github.com/the78mole/esphome_components/blob/main/components/km271_wifi/telegramqueue.h

## Prüfung und noch nötiger Gerätetest

Ausführung der Hosttests und deren Grenzen stehen in `tests/README.md`.
Ein Firmware-Build erfolgt mit `pio run -e esp32`, ohne Upload.

Vor dem Merge an der Teststeuerung prüfen:

1. Einen geeigneten Parameter ändern: abschließendes `DLE`, kein erneutes
   `EE 00 00`, fortlaufender Empfang, lokal aktualisierter Config-Wert in WebUI
   und MQTT. Am Display der Steuerung die tatsächliche Übernahme vergleichen.
   Den Ausgangswert anschließend kontrolliert wiederherstellen.
2. Mehrere geeignete Änderungen nacheinander auslösen und Reihenfolge sowie
   lokale Config-Aktualisierungen prüfen. Nur quittierte Änderungen dürfen
   erscheinen, auch bei einem teilweise abgebrochenen Auftrag. Keine
   Befehlsverluste oder leeren Zusatztelegramme.
3. Verbindungs-/Refresh-Anzeige und Alarm-/Diagrammgültigkeit beobachten:
   erfolgreicher Versand löst keinen Verbindungsabbruch oder Vollabruf aus.
4. Eine vorher abgestimmte Unterbrechung testen: offene Aufträge werden
   abgebrochen, Wiederverbindung synchronisiert neu, keine alten Befehle laufen
   nach. Die 2000-ms-Frist anhand realer Antwortzeiten bestätigen.

Softwaretests und Firmware-Build ersetzen diese Prüfung nicht. Es wird allein
zum Testen weder automatisch geflasht noch eine Steuerung bedient.
