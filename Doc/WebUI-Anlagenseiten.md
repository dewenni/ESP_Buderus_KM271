# Bedienung auf den Anlagenseiten

HK1 und HK2 besitzen die Tabs Einstellungen, Timer, Betriebswerte und Diagnose.
Die bisherigen Steuerelemente und ihre IDs bleiben erhalten. Der Timer nutzt
unverändert den vorhandenen Editor. Warmwasser verwendet dieselben Bereiche
ohne Timer. Der Ölzähler wird unter Einstellungen → Gerät → Ölzähler gesetzt;
die zentrale Bedienseite entfällt aus beiden Navigationen.

Messwerte stehen in eigenen Gruppen; veränderbare Konfigurationswerte werden
über die vorhandenen Eingaben gezeigt. Empfangsfelder, die bisher zusätzlich
in Konfigurationstabellen standen, bleiben verborgen als Ziele der bestehenden
WebSocket-Aktualisierungen erhalten. Kein zusätzlicher Abruf und kein neues
Geräteprotokoll. Änderungen an Reglern verwenden weiterhin den bisherigen
Übertragungsweg; ein Bereichswechsel sendet nichts.

Hilfe erscheint nach kurzem Hover über die Einstellung oder bei Tastaturfokus.
Die gepunktete Beschriftung lässt sich auch anklicken/antippen, ohne den Wert
zu ändern. Escape, Scrollen, Bereichswechsel oder ein Klick außerhalb schließen
sie. Betriebsart erhält keine Hilfe. Die vorhandenen DE/EN-Erklärungen werden
wie die übrigen Texte lokalisiert; Eingaben haben zusätzlich aria-describedby.
Die Tooltip-Anzeige ist in der WebUI implementiert und benötigt keine Funktion
der Konzeptvorschau. Bernsteinlinie und dezente Trennlinien entsprechen dem
abgestimmten Entwurf. Die Regler wechseln je nach Breite zwischen 4/3/2/1 Spalten.

## Solar: Vorbereitung

Solar verwendet Einstellungen, Betriebswerte und Diagnose. Die Oberfläche
enthält keinen Entwicklungshinweis zu den noch ausstehenden Schreibbefehlen. Betriebsart, Min
Solar und Max Solar bleiben zunächst Empfangswerte. Es gibt keine vorgetäuschten
Regler und keine unbestätigten Schreibadressen. Vor Aktivierung sind die
Schreibbefehle, Wertebereiche, Einheiten, Sonderwerte und die Zuordnung zum
ACK-Update der Empfangswerte anhand der Regelung zu klären. Anschließend am
Testsystem einzeln schreiben, ACK und Rückmeldung/MQTT prüfen und Ausgangswerte
wiederherstellen. Erst danach die vorhandenen Bedienelemente anbinden.

## Prüfungen

`node --test tests/equipment_pages.test.cjs` prüft den tatsächlichen Code mit
einem kleinen DOM-Modell: Bereichswechsel, Pfeiltasten/Home/End, Tooltip bei
Hover/Fokus/Klick, übersetzten Text, Schließen und ausbleibende Gerätebefehle.
`python3 -B -m unittest discover -s tests -p 'test_*.py'` schließt diese Prüfung
und die bisherigen Timer-/seriellen Tests ein. Das Modell prüft keine Darstellung,
keinen echten Touchscreen und keine Kommunikation mit einer Logamatic.

Der ESP32-Build erzeugt die WebUI und prüft die Übersetzungsschlüssel.
Noch am Browser/Gerät prüfen: Desktop und schmale Ansichten, Hover/Touch-Hilfe,
DE/EN, WebSocket-Aktualisierung in allen Bereichen, reale Parameteränderungen,
Timer nach Bereichswechsel und Ölzähler unter Einstellungen. Nicht geflasht.

Stand 27.11.2026: ESP32-Build und vollständige Host-Testsuite erfolgreich
(6 Python-Prüfungen einschließlich der Node-Tests). Keine fehlenden
Übersetzungsschlüssel; Hinweise auf ungenutzte Texte der entfernten zentralen
Bedienseite bleiben. JavaScript-Syntax und doppelte IDs der zusammengefügten
Bereiche geprüft. Die lokale Safari-Prüfung konnte nicht abgeschlossen werden.
