"""Compile the production serial engine with a host UART/clock, then replay it."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Km271WriteTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("c++"), "KM271 replay requires a C++11 compiler")
    def test_serial_engine(self):
        source = (ROOT / "src/km271.cpp").read_text()
        # Keep the actual queue, byte receiver, handshake, timeouts and framing.
        # Only the application value decoder below this boundary is replaced.
        engine, decoder = source.split("void parseInfo(uint8_t *data, int len) {", 1)
        self.assertTrue(decoder)
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            (temp / "Arduino.h").write_text(
                "#pragma once\n#include <cstdint>\n#include <cstddef>\n"
                "#include <cstdio>\n#include <cstring>\n#include <ctime>\n#include <cmath>\n"
            )
            # Use the real DE/EN dictionaries; hardware config is provided by
            # km271_host.h instead of pulling in the ESP dependency graph.
            (temp / "config.h").write_text("#pragma once\n")
            (temp / "basics.h").write_text((ROOT / "tests/km271_host.h").read_text())
            def definition(signature):
                start = source.index(signature + " {")
                opening = source.index("{", start)
                depth = 1
                end = opening + 1
                while depth:
                    depth += (source[end] == "{") - (source[end] == "}")
                    end += 1
                return source[start:end] + "\n"

            for signature in ["float decode05cTemp(uint8_t data)",
                              "void decodeTimer(char *timerInfo, unsigned int size, uint8_t dateOnOff, uint8_t time)",
                              "int8_t decodeNegValue(uint8_t data)",
                              "uint8_t limit(uint8_t lower, uint8_t value, uint8_t upper)",
                              "void km271sendCmd(e_km271_sendCmd sendCmd, int cmdPara)",
                              "void km271sendCmdFlt(e_km271_sendCmd sendCmd, float cmdPara)",
                              "bool km271sendServiceCmd(uint8_t cmdPara[8])"]:
                engine += definition(signature)
            (temp / "engine.cpp").write_text(engine)
            fixtures = json.loads((ROOT / "tests/fixtures/km271_timers.json").read_text())
            cases = []
            for case in fixtures["cases"]:
                cases.append("{" + ",".join([
                    json.dumps(case["name"]),
                    "{" + ",".join(json.dumps(x) for x in case["before"]) + "}",
                    "{" + ",".join(json.dumps(x) for x in case["frames"]) + "}",
                    "{" + ",".join(json.dumps(x) for x in case["after"]) + "}",
                ]) + "}")
            (temp / "timer_fixtures.cpp").write_text(
                "struct TimerFixture { const char *name; const char *before[2]; std::vector<std::string> frames; const char *after[2]; };\n"
                "static const TimerFixture timerFixtures[] = {" + ",\n".join(cases) + "};\n"
            )
            # Replay the actual received WW config case as well. The remainder
            # of the application decoder is outside this transport test.
            ww_case = "case 0x007e:" + source.split("  case 0x007e:", 1)[1].split("\n  case ", 1)[0]
            (temp / "receive_config.cpp").write_text(
                "void parseInfo(uint8_t *data, int len) {\n"
                "received.emplace_back(data, data + len);\n"
                "unsigned kmregister = (data[0] << 8) | data[1];\n" +
                "  for (uint8_t circuit = 0; circuit < 2; ++circuit) {" +
                source.split("  for (uint8_t circuit = 0; circuit < 2; ++circuit) {", 1)[1].split(
                    "  switch (kmregister)", 1)[0] +
                "if (len < 8) return;\n"
                "switch ((data[0] << 8) | data[1]) {\n" + ww_case + "\n}\n}\n"
            )
            executable = temp / "km271-writes"
            subprocess.run([
                shutil.which("c++"), "-std=c++11", "-Wall", "-Wextra",
                "-Wno-unused-parameter", "-Wno-unused-variable",
                "-I", str(temp), "-I", str(ROOT / "include"),
                str(ROOT / "tests/km271_writes.cpp"), "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
