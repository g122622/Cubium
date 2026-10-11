"""验证缺失、重复和不一致分片不能被汇报为完整测试。"""

import json
from pathlib import Path
import sys
import shutil
import subprocess
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_test_coverage import verify


class GameTestCoverageTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("ctest"), "Requires actual CTest selection")
    def test_ctest_stride_selection_and_recording_cover_every_test(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "CTestTestfile.cmake").write_text(
                "\n".join(f'add_test(case_{index} "{sys.executable}" "-c" "pass")' for index in range(19)))
            def inventory(arguments):
                result = subprocess.run(["ctest", "--test-dir", str(root), "--show-only=json-v1", *arguments],
                                        check=True, capture_output=True, text=True, timeout=10)
                return result.stdout
            (root / "all.json").write_text(inventory([]))
            recorder = Path(__file__).resolve().parents[1] / "record_ctest_partition.py"
            for shard in range(8):
                selected = root / "selected.json"
                selected.write_text(inventory(["-I", f"{shard + 1},,8"]))
                directory = root / str(shard)
                directory.mkdir()
                subprocess.run([sys.executable, str(recorder), "--all", str(root / "all.json"),
                                "--selected", str(selected), "--index", str(shard), "--count", "8",
                                "--out", str(directory / "ctest-partition.json")], check=True, timeout=10)
            self.assertEqual(verify(root, 8, "ctest-partition.json")["registered_tests"], 19)

    def test_all_registered_tests_must_appear_exactly_once(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            inventory = ["Class.first", "Class.second", "Class.disabled"]
            records = [{"shard_index": 0, "shard_count": 2, "all_test_ids": inventory,
                        "selected_test_ids": inventory[:2]},
                       {"shard_index": 1, "shard_count": 2, "all_test_ids": inventory,
                        "selected_test_ids": inventory[2:]}]
            for index, record in enumerate(records):
                (root / f"{index}.partition.json").write_text(json.dumps(record))
            self.assertEqual(verify(root, 2, "*.partition.json")["registered_tests"], 3)
            for broken in (dict(records[1], shard_index=0),
                           dict(records[1], selected_test_ids=["Class.first", "Class.disabled"]),
                           dict(records[1], selected_test_ids=[]),
                           dict(records[1], all_test_ids=["Other.test"])):
                (root / "1.partition.json").write_text(json.dumps(broken))
                with self.assertRaises(RuntimeError):
                    verify(root, 2, "*.partition.json")
            (root / "1.partition.json").unlink()
            with self.assertRaises(RuntimeError):
                verify(root, 2, "*.partition.json")


if __name__ == "__main__":
    unittest.main()
