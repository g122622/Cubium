"""验证机器配置采集的真实工具选择、容量单位和有界降级。"""

import importlib.util
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location("collect_runner_info", Path(__file__).resolve().parents[1] / "collect_runner_info.py")
collector = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(collector)


class CollectRunnerInfoTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)

    def test_cmake_cache_only_reads_configuration_whitelist(self):
        """构建配置保留编译与链接参数，其他缓存字段不进入归档。"""
        (self.folder / "CMakeCache.txt").write_text(
            "// comment\nCMAKE_CXX_COMPILER:FILEPATH=/usr/lib/llvm-22/bin/clang++\n"
            "MC_ENABLE_NATIVE_ARCH:BOOL=OFF\nMC_FUZZ_ASAN:BOOL=ON\nSECRET:STRING=do-not-record\n"
            "CMAKE_GENERATOR:INTERNAL=Ninja Multi-Config\n"
            "CMAKE_EXE_LINKER_FLAGS_RELEASE:STRING=-Wl,--strip-all\n"
            "CMAKE_SHARED_LINKER_FLAGS_RELEASE:STRING=-Wl,--strip-all -Wl,--gc-sections\n", encoding="utf-8")
        result = collector.read_build_config(self.folder)
        self.assertEqual(result["CMAKE_CXX_COMPILER"], "/usr/lib/llvm-22/bin/clang++")
        self.assertEqual(result["MC_ENABLE_NATIVE_ARCH"], "OFF")
        self.assertEqual(result["MC_FUZZ_ASAN"], "ON")
        self.assertEqual(result["CMAKE_EXE_LINKER_FLAGS_RELEASE"], "-Wl,--strip-all")
        self.assertEqual(result["CMAKE_SHARED_LINKER_FLAGS_RELEASE"], "-Wl,--strip-all -Wl,--gc-sections")
        self.assertNotIn("SECRET", result)
        self.assertEqual(collector.read_build_config(self.folder / "missing"), {})
        self.assertEqual(collector.read_build_config(None), {})

    def test_tool_version_records_selected_executable_and_short_version(self):
        completed = subprocess.CompletedProcess([], 0, "Ubuntu clang version 22.1.8 (build metadata)\nTarget: x86_64", "")
        with patch.object(collector.shutil, "which", return_value="/usr/lib/llvm-22/bin/clang++"), \
                patch.object(collector.subprocess, "run", return_value=completed) as run:
            result = collector.tool_version("clang++")
        self.assertEqual(result["name"], "clang++")
        self.assertEqual(result["version"], "22.1.8")
        self.assertEqual(result["status"], "ok")
        self.assertEqual(run.call_args.args[0], ["/usr/lib/llvm-22/bin/clang++", "--version"])
        self.assertEqual(run.call_args.kwargs["timeout"], 3)

    def test_missing_failed_or_blocked_tool_does_not_abort_collection(self):
        with patch.object(collector.shutil, "which", return_value=None):
            self.assertEqual(collector.tool_version("missing")["status"], "unavailable")
        with patch.object(collector.shutil, "which", return_value="/tool"), \
                patch.object(collector.subprocess, "run", side_effect=subprocess.TimeoutExpired("/tool", 3)):
            self.assertEqual(collector.tool_version("blocked")["status"], "error")
        with patch.object(collector.shutil, "which", return_value="/tool"), \
                patch.object(collector.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, "", "failed")):
            self.assertEqual(collector.tool_version("broken")["status"], "error")

    def test_test_runner_does_not_claim_to_compile_downloaded_binaries(self):
        with patch.object(collector, "tool_version", return_value={"status": "ok", "version": "22.3.0"}) as version:
            result = collector.collect("integrated-tests", None)
        self.assertEqual(set(result["tools"]), {"node", "npm", "python"})
        self.assertEqual([call.args[0] for call in version.call_args_list], ["node", "npm"])
        self.assertEqual(result["build"], {})

    def test_build_uses_configured_compiler_and_records_memory_in_bytes(self):
        config = {"CMAKE_CXX_COMPILER": "/configured/clang++", "CMAKE_LINKER": "/configured/ld.lld"}
        def read(path):
            return "model name : Test CPU\n" if path.name == "cpuinfo" else "MemTotal: 16777216 kB\n"
        with patch.object(collector, "read_build_config", return_value=config), \
                patch.object(collector, "_read_text", side_effect=read), \
                patch.object(collector, "tool_version", return_value={"status": "ok", "version": "22.1.8"}) as version, \
                patch.dict(os.environ, {"ImageOS": "ubuntu24", "ImageVersion": "20261009.1.0", "UNRELATED_SECRET": "do-not-record"}):
            result = collector.collect("build", self.folder)
        self.assertEqual(result["cpu"]["model"], "Test CPU")
        self.assertEqual(result["memory_bytes"], 16 * 2**30)
        commands = [call.args[0] for call in version.call_args_list]
        self.assertIn("/configured/clang++", commands)
        self.assertIn("/configured/ld.lld", commands)
        self.assertEqual(result["runner"]["image_version"], "20261009.1.0")
        self.assertNotIn("do-not-record", json.dumps(result))

    def test_cli_creates_artifact_directory_and_valid_json(self):
        output = self.folder / "artifact" / "runner-info.json"
        with patch.object(collector, "collect", return_value={"schema_version": 1, "job": "report"}):
            self.assertEqual(collector.main(["--job", "report", "--out", str(output)]), 0)
        self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["job"], "report")
