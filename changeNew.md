# v5.4.0

## what's new

this brings different improvements and some new function.

### WebUI-Demo

For a first impression of the functions and the WebUI, a limited demo is also available.  
This can be accessed via the following link: [WebUI-DEMO](https://dewenni.github.io/ESP_Buderus_KM271/)

### Changed condition for Setup-Mode

Setup mode is now activated when the ESP is restarted **5** times.
A maximum of 5 seconds may elapse after each restart.

Example: restart 1/5 - wait 2s - restart 2/5 - wait 2s - restart 3/5 - wait 2s - restart 4/5 - wait 2s - restart /5/5 => Setup-Mode

### Extended Logger

The Logger source is now split into "System" and "Logamatic" log source.
System and Logamatic is recorded in parallel. You can switch between this two sources without deleting the buffer.
But the buffer will be deleted if you change the log filter for the Logamatic log.

### Flexible Oil-Meter Configuration

For the Hardware Oil-Meter you can now configure the "pulses per litre" and a "debouncing time"
For the virtual Oil-Meter you can noe define a offset value to correct the calculated value to your needs.

## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.5
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.7.0
- [UPDATE]  update ArduinoJSON @ 7.4.0
- [CHANGE]  Basic WebUI functions were outsourced to a separate [EspWebUI](https://github.com/dewenni/EspWebUI) library
- [IMPROVE] rework internal logging functions
- [IMPROVE] Added validation for IP input fields in the setup area to avoid wrong syntax
- [IMPROVE] Conversion of the storage of the oil counter from EEPROM to NVS
- [IMPROVE] change "Double-Reset-Detection" to "Multi-Reset-Detection" now it needs 5 restarts within 5s timeout to enter Setup-Mode
- [FEATURE] Add a [WebUI-DEMO](https://dewenni.github.io/ESP_Buderus_KM271/) under github-pages
- [FEATURE] For the Hardware Oil-Meter you can now configure the "pulses per litre" and a "debouncing time" #148
- [FEATURE] For the virtual Oil-Meter you can noe define a offset value to correct the calculated value to your needs. #147
