# v5.3.1

## what's new

this is a minor bugfix version.

> [!CAUTION]
> Please see also the [changelog of v5.3.0](https://github.com/dewenni/ESP_Buderus_KM271/releases/tag/v5.3.0) if you update from version <5.3.0

## changelog

- [FIX] missing status values in Home Assistant after adding `"state_class": "measurement"` #140
- [IMPROVE] when Home Assistant or the mqtt plugin of HASS restarts, now all values are send again
