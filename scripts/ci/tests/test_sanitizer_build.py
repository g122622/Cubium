"""使用小型真实 C/C++ 构建，验证同一目录启停 sanitizer 不残留编译或链接参数。"""

import json
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


@unittest.skipIf(sys.platform == "win32", "Global ASan/UBSan is supported on Linux/macOS")
class SanitizerBuildTest(unittest.TestCase):
    def test_toggle_c_cpp_and_link_instrumentation_in_one_directory(self):
        tools = {name: shutil.which(name) for name in ("cmake", "ninja", "clang", "clang++")}
        if not all(tools.values()):
            self.skipTest("CMake, Ninja and Clang are required")
        with tempfile.TemporaryDirectory(prefix="cubium-sanitizer-build-") as temporary:
            root = Path(temporary)
            build = root / "build"
            (root / "c_probe.c").write_text("int c_probe(void) { return 40; }\n")
            (root / "cpp_probe.cpp").write_text("int cpp_probe() { return 2; }\n")
            (root / "main.cpp").write_text('extern "C" int c_probe(void);\nint cpp_probe();\n'
                                           'int main() { return c_probe() + cpp_probe() != 42; }\n')
            (root / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.26)
project(SanitizerFixture LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include("{(REPO / 'cmake/Sanitizers.cmake').as_posix()}")
add_library(probe_objects OBJECT c_probe.c cpp_probe.cpp)
add_executable(probe main.cpp $<TARGET_OBJECTS:probe_objects>)
''')
            for enabled in ("ON", "OFF"):
                with self.subTest(sanitizer=enabled):
                    configured = subprocess.run([
                        tools["cmake"], "-S", str(root), "-B", str(build), "-G", "Ninja",
                        f"-DCMAKE_MAKE_PROGRAM={tools['ninja']}",
                        f"-DCMAKE_C_COMPILER={tools['clang']}",
                        f"-DCMAKE_CXX_COMPILER={tools['clang++']}",
                        f"-DMC_ENABLE_SANITIZERS={enabled}",
                    ], capture_output=True, text=True, timeout=60)
                    self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
                    entries = json.loads((build / "compile_commands.json").read_text())
                    for entry in entries:
                        arguments = entry.get("arguments") or shlex.split(entry["command"])
                        self.assertEqual("-fsanitize=address,undefined" in arguments, enabled == "ON")
                    commands = subprocess.check_output(
                        [tools["ninja"], "-C", str(build), "-t", "commands", "probe"], text=True)
                    self.assertEqual("-fsanitize=address,undefined" in commands.splitlines()[-1], enabled == "ON")
                    compiled = subprocess.run([tools["cmake"], "--build", str(build)],
                                              capture_output=True, text=True, timeout=60)
                    self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
                    completed = subprocess.run([str(build / "probe")], capture_output=True, text=True, timeout=10)
                    self.assertEqual(completed.returncode, 0, completed.stderr)
                    cache = (build / "CMakeCache.txt").read_text()
                    self.assertIn("CMAKE_C_FLAGS:STRING=\n", cache)
                    self.assertIn("CMAKE_CXX_FLAGS:STRING=\n", cache)


if __name__ == "__main__":
    unittest.main()
