"""Include timer model and WebUI state tests in the host suite."""
from pathlib import Path
import shutil
import subprocess
import unittest


class WeeklyProgramTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("node"), "Timer WebUI checks require Node.js 18+")
    def test_timer_webui(self):
        subprocess.run([
            shutil.which("node"), "--test", str(Path(__file__).with_name("weekly_program.test.cjs")),
        ], check=True)
