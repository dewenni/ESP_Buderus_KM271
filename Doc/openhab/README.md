# OpenHab thing configuration

## Prerequisites: 
1. MQTT binding installed (https://www.openhab.org/addons/bindings/mqtt/)
2. MQTT broker created (Thing - Bridge)
3. MQTT thing created with broker as bridge (Generic MQTT Thing)
4. Optional: Python and pip installed to your PC

## Use pre generated YAML (Default Language: German)
1. Go to your created MQTT thing
2. Switch to the "Code" tab
3. Copy and paste the channels.yaml content to the channels section
4. Start adding OpenHab items

## Optional: Use python script
1. Optional: Change language by setting the LANGUAGE variable to 0 for German and 1 for English
2. Run ```pip install -r requirements.txt```
3. Run ```python parse_params.py```
4. Open .yaml file with 'Windows 1252' encoding 
5. Reformat .yaml file. (Indent channels with one tab)
6. Follow steps from above