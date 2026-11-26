"""Include the dependency-free Node.js alarm checks in the host test suite."""
from pathlib import Path
import shutil
import subprocess
import unittest


class AlarmTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("node"), "Alarm checks require Node.js 18+")
    def test_alarm_memory(self):
        subprocess.run(
            [shutil.which("node"), "--test", str(Path(__file__).with_name("alarms.test.cjs"))],
            check=True,
        )
