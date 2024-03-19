import json

# Pfad zur Originaldatei
input_file = 'web/js/lang.js'

# Pfad zur bereinigten Datei
output_file = 'lang_cleaned.js'

# Liste der nicht genutzten Schlüssel
unused_keys = [
    'opmodes', 'act_value', 'info_unit_l', 'oilmeter_act', 'voltage', 
    'esp_heapsize', 'esp_freeheap', 'esp_info', 'hostname', 'sprache',
    'automatic', 'manual', 'day_night', 'summer', 'winter', 
    'summer_winter', 'setpoint', 'opmode', 'info_summer1', 'info_summer2',
    'info_frost', 'info_designtemp', 'info_switchoff', 'info_wwtemp', 'info_unit_c',
    'info_ww_pump_circ1', 'info_ww_pump_circ2', 'esp_maxallocheap', 'esp_minfreeheap', 'esp_flash_usage',
    'esp_heap_usage', 'sysinfo', 'alarm', 'alarminfo', 'message'
]


# Laden der Originaldaten
with open(input_file, 'r') as file:
    data = json.load(file)

# Entfernen der nicht genutzten Schlüssel
for key in unused_keys:
    if key in data:
        del data[key]

# Speichern der bereinigten Daten
with open(output_file, 'w') as file:
    json.dump(data, file, indent=4, ensure_ascii=False)
