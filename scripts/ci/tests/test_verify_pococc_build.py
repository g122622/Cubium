"""确保 pococc 配置或分发退化时不会生成成功证据。"""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import verify_pococc_build as verifier


class VerifyPococcBuildTests(unittest.TestCase):
    def test_pch_and_missing_sanitizer_are_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            cache = (
                "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo\nMC_ENABLE_SANITIZERS:BOOL=ON\n"
                "MC_ENABLE_NATIVE_ARCH:BOOL=OFF\nCMAKE_DISABLE_PRECOMPILE_HEADERS:BOOL=ON\n"
                "CMAKE_CXX_SCAN_FOR_MODULES:BOOL=OFF\n"
                "CMAKE_C_FLAGS:STRING=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer\n"
                "CMAKE_CXX_FLAGS:STRING=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer\n"
            )
            (root / "CMakeCache.txt").write_text(cache)
            commands = root / "compile_commands.json"
            commands.write_text(json.dumps([{"file": "test.cpp", "command": "clang++ -c test.cpp -o test.o"}]))
            self.assertEqual(verifier.verify_configuration_without_pch(root), 1)
            commands.write_text(json.dumps([{
                "file": "test.cpp", "arguments": ["clang++", "-Xclang", "-include-pch", "-c", "test.cpp"],
            }]))
            with self.assertRaises(RuntimeError):
                verifier.verify_configuration_without_pch(root)
            commands.write_text(json.dumps([{"file": "test.cpp", "command": "clang++ -c test.cpp"}]))
            for broken in (cache.replace("PRECOMPILE_HEADERS:BOOL=ON", "PRECOMPILE_HEADERS:BOOL=OFF"),
                           cache.replace("-fsanitize=address,undefined ", "")):
                (root / "CMakeCache.txt").write_text(broken)
                with self.assertRaises(RuntimeError):
                    verifier.verify_configuration_without_pch(root)

    def test_cache_and_local_routes_do_not_hide_remote_failures(self):
        with tempfile.TemporaryDirectory() as temporary:
            journal = Path(temporary) / "pococc.tsv"
            remote = "1\tdistcc\t0\t100\ttest.cpp\tplain-object-compile\tstrict\n"
            local = "2\tlocal\t0\t100\tlibrary.c\tunrecognized-option\tstrict\n"
            journal.write_text(remote + local)
            report = verifier.verify_journal(journal)
            self.assertEqual(report["distcc_submissions"], 1)
            self.assertEqual(report["local_invocations"], 1)
            for broken in (local, remote.replace("\t0\t", "\t1\t"),
                           remote.replace("strict", "fallback-allowed"), "incomplete\n"):
                journal.write_text(broken)
                with self.assertRaises(RuntimeError):
                    verifier.verify_journal(journal)


if __name__ == "__main__":
    unittest.main()
