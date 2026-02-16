# WiFi Mesh Network Optimization Guide

## Problem Solved
ESP32-C3 was connecting to far away Deco XE75 Pro mesh APs instead of the nearest one with strong signal, resulting in poor and unstable WiFi performance.

## Changes Made

### 1. **Automatic AP Selection by RSSI** ✅
- Added WiFi network scanning before connection
- ESP32 now automatically selects the strongest signal AP
- Logs all available APs with their RSSI values for debugging

### 2. **Power Save Mode Disabled** ✅
- Set `WiFi.setSleep(WIFI_PS_NONE)` for stable connection
- Reduces latency and improves reliability
- (Can be changed to `WIFI_PS_MIN_MODEM` if battery powered)

### 3. **Protocol Optimization** ✅
- Limited to 802.11b/g/n protocols
- Better performance with nearby APs
- Removed long-range mode that causes issues with mesh

### 4. **Enhanced Logging** ✅
- Shows BSSID (MAC address) of connected AP
- Displays channel and RSSI on connection
- Lists all available APs during reconnection attempts

### 5. **Optional BSSID Locking** ✅ (Advanced)
- Can force connection to a specific AP by MAC address
- Useful if automatic selection still doesn't work perfectly
- Configured via `config.wifi.bssid` field

## How to Use

### Normal Operation (Recommended)
Just update your ESP32 with the new firmware. It will automatically:
1. Scan for all APs with your SSID
2. Log available options in the serial monitor
3. Connect to the strongest signal AP
4. Show you which AP it connected to

### Monitor the Connection
Connect via serial monitor at 115200 baud to see:
```
[SETUP] Scan complete: 3 networks found
[SETUP]   AP: YourSSID, Ch: 6, RSSI: -45 dBm, BSSID: AA:BB:CC:DD:EE:F1
[SETUP]   AP: YourSSID, Ch: 11, RSSI: -72 dBm, BSSID: AA:BB:CC:DD:EE:F2
[SETUP]   AP: YourSSID, Ch: 1, RSSI: -83 dBm, BSSID: AA:BB:CC:DD:EE:F3
[SETUP] WiFi connected
[SETUP] IP address: 192.168.1.100
[SETUP] Connected to BSSID: AA:BB:CC:DD:EE:F1, Channel: 6, RSSI: -45 dBm
```

### Advanced: Lock to Specific AP (If Needed)
If you still have issues and want to force a specific Deco unit:

1. Check the serial log to find the BSSID (MAC) of your preferred AP
2. Add this to your configuration file (config.json):
   ```json
   "wifi": {
     "ssid": "YourSSID",
     "password": "YourPassword",
     "bssid": "AA:BB:CC:DD:EE:F1"
   }
   ```
3. The format must be exactly: `"AA:BB:CC:DD:EE:FF"` (17 characters with colons)
4. Leave empty `""` or omit for automatic selection

## Additional Deco Mesh Recommendations

### On Your Deco XE75 Pro
1. **Disable "Fast Roaming" (802.11r)** - Some ESP32 devices don't handle it well
2. **Set "Beamforming" to ON** - Helps with signal quality
3. **Use "Connection Preference: Prefer Nearest"** in Deco app
4. **Avoid "Mesh Technology: Smart Roaming"** for this device
5. **Check Channel Selection** - Use separate channels for nearby Decos (6, 11, 1 for 2.4GHz)

### On the Deco App (Per-Device Settings)
1. Open Deco app → Clients → Find your ESP32-C3 device
2. Try setting "Connection Preference" to a specific Deco unit
3. If issues persist, use the BSSID lock method above

## Signal Strength Guide
- **-30 to -50 dBm**: Excellent (expected with nearby AP)
- **-50 to -60 dBm**: Good
- **-60 to -70 dBm**: Fair (acceptable)
- **-70 to -80 dBm**: Weak (may cause issues)
- **Below -80 dBm**: Very poor (should not connect)

## Troubleshooting

### Still connecting to wrong AP?
1. Check serial monitor logs to see which APs are available
2. Verify the nearby Deco is broadcasting on 2.4GHz (ESP32-C3 doesn't support 5GHz)
3. Try the BSSID locking method
4. Ensure the nearby Deco isn't on a different band/SSID

### Connection unstable?
- Check RSSI in logs - should be > -70 dBm
- Verify no WiFi interference (microwaves, Bluetooth, etc.)
- Check if the Deco unit itself has connectivity issues
- Try different WiFi channel on Deco (avoid overlap with neighbors)

### Device keeps rebooting?
- May indicate very poor signal or configuration issue
- Check power supply (ESP32-C3 needs stable 3.3V)
- Verify credentials are correct

## Files Modified
- `src/basics.cpp` - Main WiFi logic improvements
- `include/config.h` - Added BSSID field to config structure

## Technical Details
The implementation uses:
- `WiFi.scanNetworks()` - Active scanning for AP discovery
- `WiFi.begin(ssid, password, channel, bssid)` - BSSID-specific connection
- `esp_wifi_set_protocol()` - Protocol limitation for stability
- `WiFi.setSleep(WIFI_PS_NONE)` - Power save disabled
- RSSI-based AP selection via scan results
