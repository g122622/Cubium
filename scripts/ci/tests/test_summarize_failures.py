"""验证 nightly 完整读取报告，且失败和跳过不会被误报为全部通过。"""

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "summarize_failures.py"
SPEC = importlib.util.spec_from_file_location("summarize_failures", SCRIPT_PATH)
summary = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(summary)


class SummarizeFailuresTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.artifacts = Path(self.temp_dir.name)

    def write_report(self, filename, cases):
        path = self.artifacts / filename
        path.write_text(f"<testsuite>{cases}</testsuite>", encoding="utf-8")
        return path

    def test_failure_and_skip_after_four_million_characters(self):
        self.write_report(
            "ctest-results.xml",
            '<testcase name="Passed"><system-out>' + "x" * 4_100_000
            + '</system-out></testcase><testcase name="Crash"><failure message="SEGFAULT"/></testcase>'
            '<testcase name="Skipped"><skipped message="No audio device"/></testcase>',
        )
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(results["passed"], ["Passed"])
        self.assertEqual(results["failures"], ["Crash — SEGFAULT"])
        self.assertEqual(results["skipped"], ["Skipped — No audio device"])
        self.assertTrue(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_xml_errors_entities_and_output_are_not_confused(self):
        self.write_report(
            "ctest-results.xml",
            '<testcase name="A&amp;B"><error message="Process failed"/></testcase>'
            '<testcase name="Clean"><system-out>&lt;failure&gt;</system-out></testcase>',
        )
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(results["failures"], ["A&B — Process failed"])
        self.assertEqual(results["passed"], ["Clean"])

    def test_missing_report_and_log_tail_preserve_crashes_and_skips(self):
        (self.artifacts / "ctest-output.log").write_text(
            "x" * 4_100_000 + "\n1 - Crash (SEGFAULT)\n2 - Blocked (Not Run)\n3 - Optional (Skipped)\n",
            encoding="utf-8",
        )
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(results["failures"], ["Crash — SEGFAULT", "Blocked — Not Run"])
        self.assertEqual(results["skipped"], ["Optional — Skipped"])
        self.assertTrue(results["errors"])
        self.assertTrue(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_malformed_or_empty_report_cannot_be_clean(self):
        for text in ("<testsuite>", "<testsuite/>"):
            with self.subTest(text=text):
                (self.artifacts / "ctest-results.xml").write_text(text, encoding="utf-8")
                self.assertTrue(summary.collect_ctest_results(self.artifacts)["errors"])
                self.assertTrue(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_skipped_are_visible_and_not_counted_as_passed_or_failed(self):
        self.write_report("ctest-results.xml", '<testcase name="UnitOptional"><skipped/></testcase>')
        self.write_report(
            "gametest-round1.xml",
            '<testcase name="Optional" classname="structure"><skipped message="Not required"/></testcase>',
        )
        (self.artifacts / "e2e-tests.log").write_text(
            "  [跳过] inventory/blocked\n      Waiting for implementation\n", encoding="utf-8",
        )
        body = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", True)
        self.assertIn("通过 0；失败 0；跳过 1", body)
        self.assertIn("UnitOptional", body)
        self.assertIn("structure:Optional — Not required", body)
        self.assertIn("inventory/blocked — Waiting for implementation", body)
        self.assertFalse(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_full_report_keeps_every_failure_and_skip(self):
        cases = "".join(
            f'<testcase name="Fail{i}"><failure/></testcase>'
            f'<testcase name="Skip{i}"><skipped/></testcase>' for i in range(230)
        )
        self.write_report("ctest-results.xml", cases)
        limited = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", True)
        full = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", False)
        self.assertNotIn("Fail229", limited)
        self.assertNotIn("Skip229", limited)
        self.assertIn("Fail229", full)
        self.assertIn("Skip229", full)
        self.assertIn("失败 230；跳过 230", full)

    def test_long_integrated_log_tail_is_detected(self):
        (self.artifacts / "integrated-tests.log").write_text(
            "x" * 4_100_000 + "\nFATAL CRASH DETECTED\nReason: SIGSEGV\nrun-gametests exit code: 2\n",
            encoding="utf-8",
        )
        self.assertTrue(summary.collect_integrated_failures(self.artifacts))
        self.assertIn("SIGSEGV", summary.extract_crash_reports(self.artifacts)[0])

    def test_timeout_snapshot_reports_errors_failures_and_unstarted_tests(self):
        self.write_report("gametest-round1.xml",
            '<testcase name="Failed"><failure message="assertion"/></testcase>'
            '<testcase name="Interrupted"><error message="tick exceeded 2 seconds"/></testcase>'
            '<testcase name="NotRun"><skipped message="Not started"/></testcase>')
        (self.artifacts / "integrated-tests.log").write_text(
            "[GameTest] WALL-CLOCK TIMEOUT: tick (2 seconds), batch=default_1\n", encoding="utf-8")
        body = summary.build_body({"integrated-tests": "success"}, self.artifacts,
                                  "https://example/run/1", "main", "abc", False)
        self.assertIn("Failed", body)
        self.assertIn("Interrupted", body)
        self.assertIn("NotRun", body)
        self.assertIn("tick (2 seconds)", body)
        self.assertTrue(summary.has_failures({"integrated-tests": "success"}, self.artifacts))

    def test_repeated_ctest_names_keep_correct_total_and_benchmark_renders(self):
        self.write_report("ctest-results.xml", '<testcase name="Same"/><testcase name="Same"/>')
        (self.artifacts / "benchmark-compare.md").write_text("Benchmark comparison", encoding="utf-8")
        body = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", True)
        self.assertIn("通过 2；失败 0；跳过 0", body)
        self.assertIn("Benchmark comparison", body)

    def test_ctest_skip_reason_is_taken_from_gtest_output(self):
        self.write_report(
            "ctest-results.xml",
            '<testcase name="MissingItem"><skipped message="SKIP_REGULAR_EXPRESSION_MATCHED"/>'
            '<system-out>test.cpp:42: Skipped\nStone item not registered\n\n[ SKIPPED ] MissingItem</system-out>'
            '</testcase>',
        )
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(results["skipped"], ["MissingItem — Stone item not registered"])


if __name__ == "__main__":
    unittest.main()
