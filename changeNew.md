# v6.0.1

## Improvements and fixes

- **Wi-Fi power saving is enabled by default again**, restoring the behavior of v5.x and reducing the continuous Wi-Fi load compared with v6.0.0. This addresses the suspected cause of brownouts reported on boards powered from the Logamatic (#192). The effect on affected hardware still needs confirmation.
- Added a **Wi-Fi power saving switch** under the Wi-Fi settings. If you experience connection problems, for example in a mesh network, you can disable power saving selectively. Existing configurations default to enabled. Changes apply on the next Wi-Fi connection or after a restart.
- **OTA firmware validation after 30 seconds of uptime**, independent of network availability. The rollback hook is now correctly overridden; validation pauses during an active update and retries after API errors.
- Avoided a large temporary configuration copy on the ESP32 stack when initializing default settings.

## Notes for existing installations

For the full v6 feature overview and important upgrade notes, please read the detailed [v6.0.0 release notes](https://github.com/dewenni/ESP_Buderus_KM271/releases/tag/v6.0.0), especially when upgrading from v5.x.

The later freezes reported despite an external power supply remain a separate, unconfirmed issue.
