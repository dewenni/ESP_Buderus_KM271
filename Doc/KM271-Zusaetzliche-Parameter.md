# Zusätzliche Konfigurationsbefehle

Die vorhandenen Schreibqueue-, WebSocket- und MQTT-Wege unterstützen zusätzlich
Maximaltemperatur, Raumtemperaturabgleich und Heizsystem für HK1/HK2 sowie
Warmwasserbereitung aktiv und Warmwasservorrang. Bestehende MQTT-Topics bleiben
bei den zusätzlichen Schreibbefehlen erhalten.

Alle 35 MQTT-Konfigurationsbefehle werden gemeinsam in
`processMqttConfigCommand()` verarbeitet. Eine nach HK1, HK2 und Warmwasser
geordnete Tabelle bestimmt Topic, Treiberbefehl, Dezimalwerte und übersetzte
Auswahltexte. Zahlen werden vollständig geprüft; leere, nicht endliche,
überlaufende oder unvollständige Eingaben werden verworfen. Ganzzahlbefehle
akzeptieren keine Nachkommastellen ungleich null. Die Sommer-/Winter-Auswahl
verwendet ausdrücklich den Indexversatz 9; fachliche Wertebereiche prüft
weiterhin der KM271-Treiber. Es gibt keine separate Behandlung neuer Befehle.

Die WebUI verwendet entsprechend `processWebConfigCommand()` mit einer nach
denselben Gruppen geordneten Tabelle. Sie deckt alle 35 Befehle über 41
Element-IDs ab. Die neun Betriebsart-Tasten liefern feste Werte 0/1/2 aus
der Tabelle; die übrigen Eingaben verwenden dieselbe vollständige Zahlenprüfung
wie MQTT. Bestehende IDs bleiben erhalten. Unbekannte IDs werden nicht als
Konfigurationsbefehl behandelt; lokale Hilfe-/Anzeige-IDs lösen kein Schreiben aus.

## Zuordnung

Alle Adressen hexadezimal; Nutzbyte ab null nach Typ/Offset beziehungsweise
den zwei Registerbytes. Unveränderte Parameterbytes werden mit `65` gesendet.

| Feld | Leseblock | Schreibtyp / Offset | Nutzbyte | Werte |
|---|---|---|---|---|
| HK1 Maximaltemperatur | `000E` | `07 / 0E` | 2 | 20–90 °C |
| HK2 Maximaltemperatur | `0046` | `08 / 0E` | 2 | 20–90 °C, Fußboden höchstens 60 °C |
| HK1 Temperaturabgleich | `0031` | `07 / 31` | 3 | −5 bis +5 °C, vorzeichenbehaftet ×2 |
| HK2 Temperaturabgleich | `0069` | `08 / 31` | 3 | −5 bis +5 °C, vorzeichenbehaftet ×2 |
| HK1 Heizsystem | `001C` | `07 / 1C` | 2 | 0=Aus, 1=Heizkörper |
| HK2 Heizsystem | `0054` | `08 / 1C` | 2 | 0=Aus, 1=Heizkörper, 3=Fußboden |
| WW-Bereitung aktiv | `0085` | `0C / 0E` | 3 | 0=Aus, 1=An |
| WW-Vorrang für HK2 | `004D` | `08 / 15` | 1 | 0=Aus, 1=An |

Innerhalb dieser bestätigten beziehungsweise durch die Referenz belegten
Gruppen sind Blockoffset und Byteposition zugeordnet. Ein unbekanntes
Leseregister allein bestimmt keinen Schreibtyp. Empfang und angenommene
Übernahme nach Transportquittierung verwenden dieselbe Feldverarbeitung;
echter Empfang überschreibt angenommene Werte weiterhin.

Der Temperaturabgleich wird zuerst vorzeichenbehaftet dekodiert und danach
durch zwei geteilt. Die bisherige Rückwandlung in `uint8_t` vor der Skalierung
ist entfernt; negative Werte erscheinen damit korrekt.

Die HK-Schreibzuordnungen stammen aus der Referenzimplementierung:
https://github.com/the78mole/esphome_components/blob/main/components/km271_wifi/km271_communication_components.cpp

## Bestätigter direkter Gerätetest am 27.11.2026

Mac → originale KM271, 2400 Baud, 8N1. Frischer Stand und jeder Rücklesestand:
138 Register, davon 60 Config-Blöcke. Beide WW-Felder wurden einzeln von 1
auf 0 geändert und wieder auf 1 zurückgestellt. Jedes Telegramm erhielt
die Abschlussquittung DLE; die Änderung wurde zusätzlich zurückgelesen.

| Schritt | Gesendete Nutzdaten ohne Framing | Zurückgelesener Block |
|---|---|---|
| WW-Bereitung Aus | `0C 0E 65 65 65 00 65 65` | `00 85 02 65 65 00 65 04` |
| WW-Bereitung An wiederhergestellt | `0C 0E 65 65 65 01 65 65` | `00 85 02 65 65 01 65 04` |
| WW-Vorrang Aus | `08 15 65 00 65 65 65 65` | `00 4D 03 00 05 65 01 05` |
| WW-Vorrang An wiederhergestellt | `08 15 65 01 65 65 65 65` | `00 4D 03 01 05 65 01 05` |

Beim Ändern war jeweils nur das eine erwartete Config-Byte verändert.
Nach jeder Wiederherstellung waren alle 60 Config-Blöcke bytegenau identisch
zum frischen Ausgangsstand. Keine erkannten Protokollfehler. Port geschlossen.
Lokale vollständige Rohmitschnitte und Ergebnisdateien liegen unter
`work/id04/sessions/20261127-185530-timer-validation/`;
der historische Ordnername stammt vom wiederverwendeten Transportwerkzeug,
es wurden dabei keine Timer geschrieben.

### HK1-Temperaturabgleich und Maximaltemperatur

Am selben Tag direkt an der KM271 geprüft: Temperaturabgleich 1,0 → 0,5 →
1,0 K und Maximaltemperatur 90 → 89 → 90 °C. Alle vier Schreibtelegramme
erhielten DLE; jeder Zustand wurde mit 138 Registern frisch zurückgelesen.
Beim Ändern unterschied sich jeweils ausschließlich das erwartete Byte;
nach jeder Wiederherstellung waren alle 60 Config-Blöcke bytegenau identisch
zum frischen Ausgangsstand. Keine erkannten Protokollfehler.

| Schritt | Gesendete Nutzdaten ohne Framing |
|---|---|
| HK1 Abgleich 0,5 K | `07 31 65 65 65 01 65 65` |
| HK1 Abgleich 1,0 K wiederhergestellt | `07 31 65 65 65 02 65 65` |
| HK1 Maximum 89 °C | `07 0E 65 65 59 65 65 65` |
| HK1 Maximum 90 °C wiederhergestellt | `07 0E 65 65 5A 65 65 65` |

Rohmitschnitt und Rücklesestände:
`work/id04/sessions/20261127-193403-timer-validation/`.
Zusätzlich wurde der negative Abgleich 1,0 → −0,5 → 1,0 K geprüft:
`07 31 65 65 65 FF 65 65` wurde mit DLE quittiert und als `FF` im
Leseblock `0031` zurückgemeldet. Auch hier änderte sich nur das erwartete
Byte; nach dem Zurückstellen stimmten alle 60 Config-Blöcke wieder überein.
Keine erkannten Protokollfehler, Port anschließend geschlossen. Mitschnitt:
`work/id04/sessions/20261127-193715-timer-validation/`.
Der Test bestätigt die Speicherung der Konfiguration; die tatsächliche
Wirkung auf Vorlauftemperatur oder Raumregelung wurde nicht gemessen.

## Prüfung und Grenzen

Die ausführbaren Hosttests und Voraussetzungen stehen in `tests/README.md`.
Sie prüfen die produktiven Encoder, Quittierungs-/Empfangsnachführung,
feste Drahtbytes, Grenzen und Vorzeichen. Der Build erfolgt mit
`pio run -e esp32`, ohne Upload.

Der direkte Gerätetest bestätigt die beiden WW-Konfigurationsänderungen,
aber keine reale Speicherladung, Pumpenwirkung oder Vorrangregelung.
HK1-Maximaltemperatur und HK1-Temperaturabgleich sind zusätzlich durch direkte
Schreib-/Rücklesetests bestätigt. Für die drei HK2-Felder und HK1-Heizsystem
steht ein solcher Gerätetest noch aus; diese sind durch Referenz und
Softwaretests belegt.
Die auf dem ESP laufende WebUI-/MQTT-Befehlskette wurde nicht geflasht oder
praktisch geprüft. Nach Firmwareinstallation separat kontrollieren:

1. Neue Eingaben in WebUI und über MQTT, Quittierung und anschließender Empfang.
2. Temperaturabgleich negativ und positiv, max. 60 °C bei HK2-Fußboden.
3. Heizsystemänderung nur passend zur tatsächlichen Anlage; mögliche
   automatisch angepasste Nachbarparameter am Gerät vergleichen.
4. Mobile/Desktop-Darstellung und DE/EN sowie unveränderte lokale Hilfeaktionen.

Die aktuellen Sender und die MQTT-/WebSocket-Verarbeitung sind mit Hosttests
geprüft. Ein abschließender ESP32-Build der aktuellen Quellen und der Test
des vollständigen Firmwarepfads auf dem ESP stehen noch aus.
