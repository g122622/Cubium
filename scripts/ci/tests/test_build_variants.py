"""验证发布/测试构建边界、ELF 检查和 benchmark 基线配置。"""

import json
import sys
import tempfile
import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch


CI_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CI_DIR))
import prepare_build_artifacts as artifacts
import compare_benchmark as comparison

PRESETS = json.loads((CI_DIR.parents[1] / "CMakePresets.json").read_text(encoding="utf-8"))


def resolve_preset(name):
    """展开 preset 继承后的配置，用于验证实际构建契约。"""
    preset = next(p for p in PRESETS["configurePresets"] if p["name"] == name)
    result = resolve_preset(preset["inherits"]) if "inherits" in preset else {}
    result.update(preset.get("cacheVariables", {}))
    return result


class BuildVariantsTest(unittest.TestCase):
    def test_test_preset_enables_global_c_and_cpp_instrumentation(self):
        """测试构建须对 C/C++ 和两类链接目标启用 ASan/UBSan。"""
        config = resolve_preset("linux-clang-ci-tests")
        artifacts.verify_configuration(config, "test")
        self.assertEqual(config["MC_BUILD_BENCHMARKS"], "OFF")
        self.assertEqual(config["MC_BUILD_TESTS"], "ON")
        self.assertEqual(config["CMAKE_EXE_LINKER_FLAGS"], "-fsanitize=address,undefined")
        self.assertEqual(config["CMAKE_SHARED_LINKER_FLAGS"], "-fsanitize=address,undefined")

    def test_release_preset_disables_debug_profiler_and_sanitizers(self):
        """Release 产物保留运行库定位，但关闭调试、profiler 和 sanitizer。"""
        config = resolve_preset("linux-clang-ci-release")
        artifacts.verify_configuration(config, "release")
        self.assertEqual(config["MC_BUILD_BENCHMARKS"], "ON")
        self.assertEqual(config["MC_BUILD_TESTS"], "OFF")
        self.assertEqual(config["CMAKE_BUILD_RPATH"], "$ORIGIN/lib")

    def test_fuzz_retains_local_instrumentation_and_original_base(self):
        """新增全局 sanitizer preset 不得改变 fuzz 的局部插桩策略。"""
        config = resolve_preset("linux-clang-fuzz")
        self.assertEqual(config["MC_ENABLE_SANITIZERS"], "OFF")
        self.assertEqual(config["MC_FUZZ_ASAN"], "ON")
        self.assertNotIn("CMAKE_CXX_FLAGS", config)
        self.assertEqual(config["CMAKE_BUILD_TYPE"], "RelWithDebInfo")

    def test_boolean_option_without_sanitizer_flags_is_rejected(self):
        """配置开关不能替代实际编译插桩参数。"""
        config = resolve_preset("linux-clang-ci-tests")
        config["CMAKE_CXX_FLAGS"] = ""
        with self.assertRaisesRegex(RuntimeError, "ASan and UBSan"):
            artifacts.verify_configuration(config, "test")

    def test_sanitized_binary_requires_both_runtimes_and_debug_information(self):
        """测试二进制必须同时具备 ASan、UBSan 和调试节。"""
        with patch.object(artifacts, "_run", side_effect=[".debug_info", "__asan_init __ubsan_handle_type_mismatch_v1"]):
            artifacts.verify_binary(Path("test-server"), "test")
        for sections, symbols in ((".text", "__asan_init __ubsan_handle_type_mismatch_v1"),
                                  (".debug_info", "__asan_init")):
            with self.subTest(sections=sections, symbols=symbols), \
                    patch.object(artifacts, "_run", side_effect=[sections, symbols]):
                with self.assertRaises(RuntimeError):
                    artifacts.verify_binary(Path("test-server"), "test")

    def test_release_binary_rejects_debug_links_and_sanitizer_symbols(self):
        """发布 ELF 不得包含调试节、外置调试链接或 sanitizer 入口。"""
        for sections, symbols in ((".debug_info", "main"), (".gnu_debuglink", "main"),
                                  (".text", "__asan_init"), (".text", "__ubsan_handle_add_overflow")):
            with self.subTest(sections=sections, symbols=symbols), \
                    patch.object(artifacts, "_run", side_effect=[sections, symbols]):
                with self.assertRaises(RuntimeError):
                    artifacts.verify_binary(Path("release-server"), "release")
        with patch.object(artifacts, "_run", side_effect=[".text .dynsym", "main malloc"]):
            artifacts.verify_binary(Path("release-server"), "release")

    def test_missing_dynamic_library_aborts_packaging(self):
        """依赖未解析时不能生成看似可运行的发布包。"""
        with patch.object(artifacts, "_run", return_value="libfmt.so => not found"):
            with self.assertRaisesRegex(RuntimeError, "Unresolved"):
                artifacts.collect_libraries(Path("server"), Path("build"), Path("libs"), "release")

    def test_packaging_retains_loader_name_and_excludes_system_libc(self):
        """复制库时保留动态加载器使用的名字，同时排除系统 libc。"""
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / "build/vcpkg_installed/x64-linux/lib/libexample.so.1.2"
            source.parent.mkdir(parents=True)
            source.write_bytes(b"example")
            ldd = f"libexample.so.1 => {source.as_posix()} (0x1)\nlibc.so.6 => /lib/libc.so.6 (0x2)"
            with patch.object(artifacts, "_run", return_value=ldd):
                artifacts.collect_libraries(Path("server"), root / "build", root / "libs", "test")
            self.assertEqual((root / "libs/libexample.so.1").read_bytes(), b"example")
            self.assertFalse((root / "libs/libc.so.6").exists())

    def test_comparison_resets_baseline_on_build_profile_change(self):
        """跨配置比较重建基线，同配置仍能识别性能劣化。"""
        def data(profile, time):
            """生成带构建配置标识的一条基准结果。"""
            return {"context": {"ci_build_profile": profile}, "benchmarks": [
                {"name": "Test", "real_time": time, "time_unit": "ns", "run_type": "iteration"}]}
        current = data("release-noprof-nosan-v1", 100)
        previous = data("legacy", 50)
        reset = comparison.build_report(current, previous, 10, 20)
        self.assertIn("重新建立基线", reset)
        self.assertNotIn("⚠️ 超过阈值", reset)
        previous["context"]["ci_build_profile"] = "release-noprof-nosan-v1"
        same = comparison.build_report(current, previous, 10, 20)
        self.assertIn("CPU 劣化", same)

    def test_release_rejects_sanitizer_compile_and_link_flags(self):
        """即使总开关关闭，也必须在 strip 前拒绝各层的显式插桩参数。"""
        for key in ("CMAKE_C_FLAGS", "CMAKE_CXX_FLAGS", "CMAKE_C_FLAGS_RELEASE", "CMAKE_CXX_FLAGS_RELEASE",
                    "CMAKE_EXE_LINKER_FLAGS", "CMAKE_SHARED_LINKER_FLAGS", "CMAKE_MODULE_LINKER_FLAGS",
                    "CMAKE_EXE_LINKER_FLAGS_RELEASE", "CMAKE_SHARED_LINKER_FLAGS_RELEASE", "CMAKE_MODULE_LINKER_FLAGS_RELEASE"):
            with self.subTest(key=key):
                config = resolve_preset("linux-clang-ci-release")
                config[key] = config.get(key, "") + " -fsanitize=address,undefined"
                with self.assertRaisesRegex(RuntimeError, "sanitizer instrumentation"):
                    artifacts.verify_configuration(config, "release")

    def test_invalid_release_configuration_is_rejected_before_modifying_binary(self):
        """错误配置不能先删除符号，再因丢失证据而通过二进制核验。"""
        config = resolve_preset("linux-clang-ci-release")
        config["CMAKE_CXX_FLAGS"] = "-fsanitize=address,undefined"
        with patch.object(artifacts, "read_build_config", return_value=config), patch.object(artifacts, "_run") as run:
            result = artifacts.main(["--variant", "release", "--build-dir", "build", "--binary", "server",
                                     "--library-dir", "libs", "--report", "report.txt"])
        self.assertEqual(result, 1)
        run.assert_not_called()

    def test_artifact_tools_use_argv_without_shell_interpretation(self):
        """文件名中的 shell 元字符保持为一个参数，不经过 shell 展开。"""
        name = "binary; $(touch marker) `echo unexpected`"
        with patch.object(artifacts.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "ELF", "")) as run:
            self.assertEqual(artifacts._run(["readelf", "--sections", "--", name]), "ELF")
        self.assertEqual(run.call_args.args[0], ["readelf", "--sections", "--", name])
        self.assertIs(run.call_args.kwargs["shell"], False)

    def test_artifact_tool_allowlist_rejects_arbitrary_programs(self):
        """工具名只能由固定的 ELF 检查工具集合提供。"""
        with patch.object(artifacts.subprocess, "run") as run:
            with self.assertRaisesRegex(RuntimeError, "Unsupported"):
                artifacts._run(["arbitrary-program", "server"])
        run.assert_not_called()
