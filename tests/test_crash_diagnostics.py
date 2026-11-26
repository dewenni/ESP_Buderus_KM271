"""Include browser crash-download regressions in the host suite."""
from pathlib import Path
import shutil
import subprocess
import unittest


class CrashDiagnosticsTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("node"), "Crash download checks require Node.js 18+")
    def test_crash_download(self):
        subprocess.run(
            [shutil.which("node"), "--test", str(Path(__file__).with_name("crash_diagnostics.test.cjs"))],
            check=True,
        )
