# v5.1.0

## what's new

This is a minor update version, but it is based on a major update 5.0.0
Please see here what´s changed in [release v5.0.0](https://github.com/dewenni/ESP_Buderus_KM271/releases/tag/v5.0.0)

> [!IMPORTANT]   
> unfortunately there was a bug in the MQTT handling in v5.0.0 which led to commands not being accepted correctly. Therefore please update to this version if you are already using 5.0.0!

### reminder Ethernet Extention

I would also like to point out again that since V5 it is possible to connect the ESP via Ethernet.  
Daniel already has a great expansion board designed for this, which you are welcome to register for.  
https://www.tindie.com/products/the78mole/km271-wifi-ethernet-extension

### new functions

- now the active Page is highlighted in the navigation bar.
- due to the increased number of possible used GPIO´s - there is a check whether the GPIOs are valid and individual GPIOs are not configured multiple times

## changelog

- bugfix incorrect handling of received mqtt payload #131
- bugfix incorrect heap usage value
- change mqtt client-id back to WiFi-Hostname #128
- no restart of the ESP if the WiFi connection cannot be established but there is an Ethernet connection.
- interval for renewed mqtt connection attempt increases with each attempt by a further 10s (max. 50s). The ESP restarts after 5 failed attempts
- change previous used Libraries for DS18B20 sensors to [mathieucarbou/MycilaDS18](https://github.com/mathieucarbou/MycilaDS18)
- add function to check whether the configured GPIOs are valid and have not been assigned twice
- highlight active page in sidebar navigation
- update Readme.md