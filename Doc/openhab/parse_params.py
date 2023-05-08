from typing import List

from pydantic import BaseModel
from pydantic_yaml import YamlModel

CONFIG = 'CONFIG'
COMMAND = 'command'
COMMAND_KEY = COMMAND + ':'
# de = 0 or en = 1
LANGUAGE = 0
LANGUAGE_HK = ["HK1", "hc1"]
LANGUAGE_HK2 = ["HK2", "hc2"]
LOGAMATIC = 'Logamatic'
MQTT_COMMANDS = 'MQTT commands'
MQTT_KEY = 'esp_heizung/'
NUMBER = 'number'
STATUS = 'STATUS'
VALUE_KEY = 'value:'


# Configuration model for an OpenHab channel
class Configuration(BaseModel):
    stateTopic: str
    transformationPattern: str | None = None
    unit: str | None = None


# Configuration command model for command option of an OpenHab channel
class ConfigurationCommand(BaseModel):
    commandTopic: str


# Channel model
class Channel(BaseModel):
    id: str
    channelTypeUID: str
    label: str
    description: str
    configuration: Configuration | ConfigurationCommand


# List of channels required to create the output list of all channels
class Configs(YamlModel):
    channels: List[Channel]


# Initialize list with frequently updated channels which are not part of params.txt.
channel_list = [Channel(id='burner_status', channelTypeUID='mqtt:number', label='Brenner_Status',
                        description='"Burner running 1 or 0"', configuration=
                        Configuration(stateTopic=MQTT_KEY + 'info', transformationPattern='JSONPATH:$.burner')),
                Channel(id='pump_status', channelTypeUID='mqtt:number', label='Pumpen_Status',
                        description='"Pump running 1 or 0"', configuration=
                        Configuration(stateTopic=MQTT_KEY + 'info', transformationPattern='JSONPATH:$.pump')),
                Channel(id='ww_temp', channelTypeUID='mqtt:number', label='WW_Temperatur',
                        description='"Warm water temperature"', configuration=
                        Configuration(stateTopic=MQTT_KEY + 'info', transformationPattern='JSONPATH:$.ww_temp',
                                      unit="°C")),
                Channel(id='boiler_temp', channelTypeUID='mqtt:number', label='Kessel_Temperatur',
                        description='"Boiler temperature"', configuration=
                        Configuration(stateTopic=MQTT_KEY + 'info', transformationPattern='JSONPATH:$.boiler_temp',
                                      unit='°C'))
                ]

if __name__ == '__main__':
    # 1. Load params file
    with open('../param.txt') as infile:
        mode = ''
        mode_key = ''
        mqtt_type = 'string'
        for index, row in enumerate(infile):
            # 2. Check if we should switch between modes: command, config values or status values
            if row.__contains__(LOGAMATIC) or row.__contains__(MQTT_COMMANDS):
                if row.__contains__(MQTT_COMMANDS):
                    mode = COMMAND
                    mode_key = COMMAND_KEY
                elif row.__contains__(CONFIG):
                    mode = CONFIG.lower()
                    mode_key = VALUE_KEY
                elif row.__contains__(STATUS):
                    mode = STATUS.lower()
                    mode_key = VALUE_KEY
                    mqtt_type = NUMBER
            # 3. Check if there is a value block
            if (not mode_key.__eq__('')) and row.startswith(mode_key):
                # 3.1. Value string used as channel description
                description_index = row.index(mode_key) + mode_key.__len__()
                description_text = row[description_index:].lstrip()
                description_text = description_text.replace('"', '\'')
                description_text = '"' + description_text.replace('\n', '') + '"'
                # 3.2. Topic string used for MQTT topic value
                topic_row = infile.readline()
                topic_text = topic_row.split('"')[1::2]
                topic = topic_text[LANGUAGE]
                topic_id = topic.replace('/', '_')
                # 3.3. Set degree if there is any.
                unit = ''
                if infile.readline(index + 1).__contains__('°C') and mqtt_type == NUMBER:
                    unit = '°C'
                else:
                    unit = None
                # 3.4. Use other configuration key which is required for MQTT commands
                if mode.__eq__(COMMAND):
                    configuration_instance = ConfigurationCommand(commandTopic=MQTT_KEY + topic)
                else:
                    configuration_instance = Configuration(stateTopic=MQTT_KEY + mode + '/' + topic, unit=unit)
                # 3.5. Create channel and append it to the list
                config_instance = Channel(id=topic_id, channelTypeUID='mqtt:' + mqtt_type,
                                          label=topic_id, description=description_text,
                                          configuration=configuration_instance)
                channel_list.append(config_instance)
                # 3.6. Check if channel is a heating circuit value
                if (not mode.__eq__(COMMAND)) and topic.__contains__(LANGUAGE_HK[LANGUAGE]):
                    # 3.6.1. Duplicate everything for heating circuit 2
                    value_text_hk2 = description_text.replace('1', '2')
                    topic_hk2 = topic.replace(LANGUAGE_HK[LANGUAGE], LANGUAGE_HK2[LANGUAGE])
                    topic_hk2_id = topic_hk2.replace('/', '_')
                    configuration_instance_hk2 = Configuration(stateTopic=MQTT_KEY + mode + '/' + topic_hk2,
                                                               unit=unit)
                    config_instance_hk2 = Channel(id=topic_hk2_id, channelTypeUID='mqtt:' + mqtt_type,
                                                  label=topic_hk2_id, description=value_text_hk2,
                                                  configuration=configuration_instance_hk2)
                    channel_list.append(config_instance_hk2)
        # 4. Create yaml file
        yaml = Configs(channels=channel_list).yaml(exclude_none=True, width=1024)
        # 5. Replace some error chars
        yaml = yaml.replace('\'', '')
        yaml = yaml.replace('\\xB0', '°')
        # 6. Write to file
        config_file = open('channels.yaml', 'w')
        config_file.write(yaml)
        config_file.close()
