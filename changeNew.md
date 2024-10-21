# v4.1.3

## what's new

There are some changes to MQTT Discovery for Home Assistant

> [!IMPORTANT]   
> with v4.1.2 the discovery config for "HK1_Sommer_ab", "hc1_summer_mode_threshold and "WW_Zirkulation", "ww_circulation" has changed.
> Therefore, it may be necessary to delete the old Discovery Config manually.
> This can be achieved by manually sending an MQTT message with the appropriate topic and an **empty** payload.
> Alternatively, with this version you can also reset all config messages via Telnet (ha resetconfig) and also resend the config via command (ha sendconfig)  
>
> Example for german Topics (payload empty):  
> `old Topic: homeassistant/number/Logamatic/HK1_Sommer_ab/config`  
> `old Topic: homeassistant/sensor/Logamatic/WW_Zirkulation/config`

## changelog

- bugfix Home Assistant setvalues not working - this bug was introduced in v4.1.2 #108
- add function to send MQTT discovery manually via Telnet (> ha sendconfig)
- add function to reset MQTT discovery manually via Telnet (> ha resetconfig)