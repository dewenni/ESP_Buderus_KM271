"""Compile production discovery with real ArduinoJson; capture MQTT locally."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
JSON_INCLUDE = ROOT / '.pio/libdeps/esp32/ArduinoJson/src'


class MqttDiscoveryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('c++') and (JSON_INCLUDE / 'ArduinoJson.h').exists(),
                         'Discovery replay requires C++11 and PlatformIO ArduinoJson dependencies')
    def test_controls_and_sensor_migration(self):
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            for header in ('config.h', 'message.h', 'mqtt.h', 'mqttDiscovery.h'):
                (temp / header).write_text('#pragma once\n')
            (temp / 'basics.h').write_text('''#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ArduinoJson.h>
#define VERSION "test"
struct muTimer { bool cycleTrigger(unsigned) { return true; } };
static struct {
  struct { const char *ha_topic = "homeassistant", *topic = "buderus", *ha_device = "TestKM271"; uint8_t lang = 0; } mqtt;
  struct { bool use_hc1 = true, use_hc2 = true, use_ww = true, use_solar = false, use_alarmMsg = false; } km271;
  struct { bool use_hardware_meter = false, use_virtual_meter = false; } oilmeter;
  struct { bool ch1_enable = false, ch2_enable = false; const char *ch1_name = "one", *ch2_name = "two"; } sensor;
} config;
struct ConfigValues { uint8_t hc2_heating_system = 1; } configValues;
static ConfigValues *km271GetConfigValueAdr() { return &configValues; }
namespace EspStrUtil {
static void replace_underscores(const char *in, char *out, unsigned size) {
  snprintf(out, size, "%s", in);
  for (char *p = out; *p; ++p) if (*p == '_') *p = ' ';
}
namespace EspStrUtil {
static void replace_whitespace(const char *in, char *out, unsigned size) { snprintf(out, size, "%s", in); }
}
}
void mqttPublish(const char *, const char *, bool);
''')
            (temp / 'discovery.cpp').write_text((ROOT / 'src/mqttDiscovery.cpp').read_text())
            executable = temp / 'discovery'
            subprocess.run([shutil.which('c++'), '-std=c++11', '-Wall', '-Wextra',
                            '-Wno-unused-function', '-Wno-deprecated-declarations', '-I', str(temp), '-I', str(JSON_INCLUDE),
                            '-I', str(ROOT / 'include'), str(ROOT / 'tests/mqtt_discovery.cpp'),
                            '-o', str(executable)], check=True)
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
        reports = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(len(reports), 16)
        for language, language_reports in enumerate((reports[:8], reports[8:])):
            by_phase = {r['phase']: r['messages'] for r in language_reports}
            baseline = by_phase['baseline']
            # The harness labels the eight migration targets from the real dictionaries.
            controls = [m for m in baseline if m['migrated'] and m['payload']]
            self.assertEqual(len(controls), 8)
            self.assertEqual(baseline, by_phase['repeat'])
            for control in controls:
                doc = json.loads(control['payload'])
                name = control['name']
                self.assertEqual(doc['stat_t'], 'buderus/config/' + name)
                self.assertEqual(doc['cmd_t'], 'buderus/setvalue/' + name)
                self.assertEqual(doc['uniq_id'], name)
                self.assertEqual(doc['avty_t'], 'buderus/status')
                self.assertEqual(doc['ent_cat'], 'config')
                self.assertEqual(doc['dev']['ids'], 'TestKM271')
                self.assertNotIn('optimistic', doc)
                old_topic = f'homeassistant/sensor/TestKM271/{name}/config'
                deletion = next(m for m in baseline if m['topic'] == old_topic)
                self.assertEqual(deletion['payload'], '')
                self.assertTrue(deletion['retained'])
                self.assertLess(baseline.index(deletion), baseline.index(control))
                if control['kind'] in ('max', 'offset'):
                    self.assertIn('/number/', control['topic'])
                    self.assertEqual(doc['mode'], 'slider')
                    self.assertEqual(doc['val_tpl'], "{{value.split(' ')[0]}}")
                    self.assertEqual([float(doc[k]) for k in ('min', 'max', 'step')],
                                     [20, 90, 1] if control['kind'] == 'max' else [-5, 5, .5])
                else:
                    self.assertIn('/select/', control['topic'])
                    expected = (['Aus', 'Heizkoerper'] if language == 0 else ['off', 'radiator'])
                    if control['kind'] == 'hc2_system':
                        expected += ['Fussboden' if language == 0 else 'underfloor']
                    if control['kind'] == 'on_off':
                        expected = ['Aus', 'An'] if language == 0 else ['off', 'on']
                    self.assertEqual(doc['options'], expected)
                    self.assertNotIn('-', doc['options'])
            for phase, limit in [('underfloor', 60), ('radiator', 90), ('mid_pass_change', 60)]:
                maximum = [m for m in by_phase[phase] if m.get('kind') == 'max' and m['hc2'] and m['payload']]
                self.assertTrue(maximum)
                self.assertEqual(float(json.loads(maximum[-1]['payload'])['max']), limit)
            self.assertTrue(by_phase['reset'])
            self.assertTrue(all(not m['payload'] and m['retained'] for m in by_phase['reset']))
            disabled = [m for m in by_phase['disabled'] if m['migrated']]
            self.assertEqual(len(disabled), 16)  # Eight old sensors and eight controls.
            self.assertTrue(all(not m['payload'] and m['retained'] for m in disabled))
            self.assertEqual(by_phase['empty_prefix'], [])


if __name__ == '__main__':
    unittest.main()
