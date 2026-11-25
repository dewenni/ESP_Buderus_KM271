"""Run with python3 -B -m unittest discover -s tests."""
import json
from pathlib import Path
import runpy
import tempfile
import unittest
import os

ROOT = Path(__file__).resolve().parents[1]
KEYS = ('p00_version', 'p09_sw_version', 'p00_dialog_version')


class SimulationGenerationTests(unittest.TestCase):
    def test_clean_generation_unchanged_run_and_version_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'web').mkdir()
            source = root / 'web/sim.json'
            source.write_bytes((ROOT / 'web/sim.json').read_bytes())
            original = source.read_bytes()
            previous = Path.cwd()
            os.chdir(root)
            try:
                def generate(version):
                    (root / 'include/config.h').write_text(f'#define VERSION "{version}"\n')
                    (root / 'changeNew.md').write_text(f'# {version}\n')
                    runpy.run_path(str(ROOT / 'scripts/checkVersion.py'))

                generate('v6.0.1')
                output = root / 'web/output/sim.json'
                data = json.loads(output.read_text())
                self.assertTrue(all(data[key] == 'v6.0.1' for key in KEYS))
                # A known old timestamp detects rewriting without a timed sleep.
                os.utime(output, ns=(1000000000, 1000000000))
                generate('v6.0.1')
                self.assertEqual(output.stat().st_mtime_ns, 1000000000)
                generate('v6.0.2')
                updated = json.loads(output.read_text())
                self.assertTrue(all(updated[key] == 'v6.0.2' for key in KEYS))
                self.assertEqual({k: v for k, v in data.items() if k not in KEYS},
                                 {k: v for k, v in updated.items() if k not in KEYS})
                self.assertEqual(source.read_bytes(), original)
            finally:
                os.chdir(previous)


if __name__ == '__main__':
    unittest.main()
