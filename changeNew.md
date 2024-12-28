# v5.2.0

## what's new

some minor bugfixes, updates and changes

## changelog

- fix [[BUG] ws.closeAll() causes a crash on Safari](https://github.com/mathieucarbou/ESPAsyncWebServer/issues/162)
- fix "Solar Collector Temperature wrong" #133
- fix HCx_MixingValue (change from unsigned to signed value) #133
- update ArduinoJSON @ 7.2.1
- update AsyncTCP @ 3.3.1
- update ESPAsyncWebServer @ 3.4.5
- update to Arduino core 3.1.0 based on IDF 5.3.2.241210
- reduced font sizes across viewports by 10% for a cleaner and less cluttered appearance.
- added a scrollbar to the sidebar navigation to ensure accessibility for content exceeding the viewport height.
- uploaded files via "config-upload" are now automatically renamed to config.json, regardless of the original filename.
- reorganize internal Libraries with new Libraries EspStrUtil and EspSysUtil
- added information about actual used HCx programs
