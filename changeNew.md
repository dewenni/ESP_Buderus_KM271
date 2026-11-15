# v6.0.0

## What's new

**A major release with a redesigned WebUI, temperature trends and clearer operation and setup.** This release also improves MQTT recovery, oil-meter handling, Home Assistant discovery and update stability.

### Highlights

- **New WebUI design:** a consistent grey-blue theme with subtle orange accents, responsive layouts, clearer controls and a compact header with connection status. The project logo and favicons have also been refreshed.
- **Temperature trends:** charts for boiler flow, HC1/HC2 flow, hot water, solar and outdoor temperature. Burner operation and hot-water priority appear as overlays where applicable. The ESP records the last **60 minutes at one-minute intervals in a RAM ring buffer**, even without an open browser, and supplies the history when opening the page or reconnecting. History is cleared when the ESP restarts.
- **Custom-program weekly view:** switching times of the Logamatic's **Eigen/custom program** are displayed as a weekly schedule, with an expandable switching-point list. The currently selected program is shown separately and is also available on the dashboard. Editing individual switching times is not included.
- **New offline Help page:** German and English guidance for operation, setup, wiring, troubleshooting, MQTT/Home Assistant and service access, with links to the full documentation.
- **Grouped settings:** five categories — **General, Network, Integrations, Device, Access & maintenance** — replace the long settings list. Home Assistant has its own section under Integrations.
- **SSID and BSSID search:** scan for WiFi networks and access points, select an entry and copy it into the configuration. An optional fixed BSSID allows targeting a specific access point in a mesh network (#182).

## Improvements and fixes

### Connections and updates

- MQTT accepts **hostnames as well as IP addresses** for the broker (#166).
- Added an **MQTT connection test** using the current settings, with live connection status.
- An unavailable MQTT broker **no longer causes repeated ESP restarts**. Reconnection uses bounded backoff, connection timeouts and transport recovery.
- Improved WiFi recovery and mesh compatibility; an optional BSSID lock is supported. Short network outages no longer trigger the previous rapid restart cycle.
- Improved **OTA update stability**.
- Local-file and GitHub firmware updates are clearly separated on the Tools page. Version information, installation actions and update progress are shown directly there.
- Improved WebSocket reconnection, background-tab recovery and connection feedback. Controls are disabled while the browser is disconnected.
- W5500 Ethernet now uses the native ESP-IDF SPI driver.

### Access and configuration

- Separated **WebUI authentication, Setup Mode recovery access and service access**. Normal WebUI authentication remains optional and supports a custom username and password.
- **Telnet must be enabled explicitly** and uses a service password. Remote ArduinoOTA uses the same service password; web updates follow the normal WebUI access settings.
- Setup Mode uses fixed recovery credentials independently of `config.json`, including when the configuration is missing or damaged.
- More robust configuration saving and recovery after interrupted writes; missing configuration fields retain their defaults.
- Improved input validation and command handling, including protection against truncated numeric values and invalid date/time writes.

### Oil meter and Logamatic values

- Hardware oil pulses are counted after a **complete, debounced contact cycle**, on release. Debouncing covers activation and release, reducing false counts from contact bounce and incomplete cycles.
- Invalid MQTT oil-counter commands, such as `offline` or an empty payload, are rejected instead of resetting the counter to zero.
- Improved pulse handling and access to the stored oil-counter value. The oil-counter input uses `INPUT` instead of `INPUT_PULLUP`.
- Suppressed publication of uninitialized cached Logamatic values before the initial data set has been received, addressing the startup-zero problem affecting cumulative counters (#161, #172).
- Added detection and recovery for missing serial reception instead of continuing to report the KM271 connection as live.
- Expanded the Logamatic error-code table and improved handling of unknown codes.

### Home Assistant and languages

- Added missing Home Assistant discovery entities, including supported fault indicators and second-stage burner status (#171).
- Corrected binary-sensor ON/OFF payload mapping and cumulative-counter statistics metadata.
- Corrected the hardware oil-meter discovery value to **litres**, including conversion from the raw MQTT value in hundredths of a litre.
- WebUI status and configuration texts now follow the **WebUI language independently of the MQTT language**, including alarms and time-offset text (#177).
- Corrected the initial WebUI language selection in EspWebUI (#183) and localized the Tools file-selection controls.
- Updated English and German documentation, screenshots and wiring/troubleshooting guidance.

## Notes for existing installations

- **Export `config.json` before updating.** After the update, review the new **Access & maintenance** settings if you use Telnet or remote ArduinoOTA. In normal operation, these services require a non-empty service password; Telnet additionally requires its enable switch.
- **Setup Mode login:** `admin` / `km271-2026!`. These recovery credentials apply independently of the normal WebUI login. Web-based firmware updates do not require an additional service password.
- **Oil-meter wiring:** the configured counter GPIO is now an input **without an internal pull-up**. Reed contacts/open-collector outputs need a suitable external pull-up unless the board already provides one. GPIO35, for example, has no internal pull-up. The raw MQTT counter unit remains unchanged: **`12345` = 123.45 L**.
- **Home Assistant:** review oil-counter dashboards and existing statistics after the corrected litre scaling and counter metadata are applied. Raw MQTT topics retain their existing units.
- Do not retain messages on MQTT command topics. Old retained commands must be removed at the broker; a status update on a different topic does not overwrite them.
- Trends are a short-term RAM history, not a persistent archive. The weekly schedule is a read-only view of the custom program and can differ from the currently selected program.

## Updated dependencies

| Component | Version |
| --- | --- |
| pioarduino ESP32 platform | 55.03.312-1 |
| ESPAsyncWebServer | 3.12.1 |
| AsyncTCP | 3.6.1 |
| ArduinoJson | 7.4.3 |
| MycilaDS18 | 5.2.1 |
| EspWebUI | 0.0.14 |

## Thanks

Thanks to **@xn101de** for the reliability, Home Assistant discovery and issue fixes in #170, #171 and #172, and **@maxarndt** for MQTT hostname support and WiFi improvements in #166 and #182.
