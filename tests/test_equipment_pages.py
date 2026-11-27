"""Run local navigation/help behavior checks without a browser or controller."""
from pathlib import Path
import shutil
import subprocess
import unittest

class EquipmentPagesTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('node'), 'Requires Node.js 18+')
    def test_local_equipment_actions(self):
        subprocess.run([shutil.which('node'), '--test', str(Path(__file__).with_name('equipment_pages.test.cjs'))], check=True)
