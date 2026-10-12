"""验证 nightly 完整读取报告，且失败和跳过不会被误报为全部通过。"""

import json
import sys
import tempfile
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import summarize_failures as summary


class SummarizeFailuresTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.artifacts = Path(self.temp_dir.name)

    def write_report(self, filename, cases):
        path = self.artifacts / filename
        path.write_text(f"<testsuite>{cases}</testsuite>", encoding="utf-8")
        return path

    def write_runner(self, job):
        folder = self.artifacts / job
        folder.mkdir(parents=True, exist_ok=True)
        info = {"schema_version": 1, "job": job,
                "cpu": {"model": "Intel(R) Xeon | Test <CPU>", "logical_cpus": 4},
                "memory_bytes": 16 * 2**30,
                "disk": {"total_bytes": 80 * 2**30, "free_bytes": 50 * 2**30},
                "system": {"os": "Ubuntu 24.04.3 LTS", "kernel": "6.14.0-azure", "architecture": "x86_64", "libc": "glibc 2.39"},
                "runner": {"image_os": "ubuntu24", "image_version": "20261009.1.0"},
                "tools": {"compiler": {"name": "clang++", "status": "ok", "version": "22.1.8"},
                          "python": {"status": "ok", "version": "3.12.3"}},
                "build": {"MC_ENABLE_NATIVE_ARCH": "OFF", "MC_ENABLE_SANITIZERS": "OFF"}}
        path = folder / "runner-info.json"
        path.write_text(json.dumps(info), encoding="utf-8")
        return path

    def chart_data(self, statuses):
        """生成机器配置图表，并读取其机器可读数据。"""
        links = "\n".join(summary.runner_info_section(statuses, self.artifacts, "https://example/run/1"))
        data = json.loads((self.artifacts / "_report_charts/runner-chart-data.json").read_text(encoding="utf-8"))
        return links, data

    def test_runner_charts_preserve_all_jobs_without_text_tables(self):
        for job in summary._JOB_LABELS:
            self.write_runner(job)
        links, data = self.chart_data({})
        self.assertEqual(len(data["rows"]), len(summary._JOB_LABELS))
        info = data["rows"][0]["info"]
        self.assertEqual(info["memory_bytes"], 16 * 2**30)
        self.assertEqual(info["disk"]["free_bytes"], 50 * 2**30)
        self.assertEqual(info["cpu"]["model"], "Intel(R) Xeon | Test <CPU>")
        self.assertNotIn("|---", links)
        self.assertNotIn("<CPU>", links)
        self.assertIn("![", links)
        for limited in (True, False):
            body = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", limited)
            self.assertIn(links, body)

    def test_missing_runner_info_distinguishes_skipped_and_not_collected(self):
        self.write_runner("build")
        _, data = self.chart_data({"unit-tests": "skipped", "fuzz": "failure"})
        self.assertEqual(data["missing"]["unit-tests"], "未运行")
        self.assertEqual(data["missing"]["fuzz"], "未采集")

    def test_matrix_and_build_runtime_configurations_are_all_preserved(self):
        for job, sources in (("unit-tests", ("unit-0", "unit-1")),
                             ("fuzz", ("fuzz-build", "fuzz-runtime"))):
            for source in sources:
                path = self.write_runner(job)
                directory = self.artifacts / source
                directory.mkdir()
                path.rename(directory / path.name)
        runners, errors = summary.collect_runner_info(self.artifacts)
        self.assertEqual(errors, [])
        self.assertEqual(len(runners), 4)
        self.assertEqual({r["artifact"] for r in runners.values()},
                         {"unit-0", "unit-1", "fuzz-build", "fuzz-runtime"})
        _, data = self.chart_data({})
        self.assertEqual(data["snapshots"], 4)
        self.assertEqual(len(data["rows"]), 3)
        self.assertEqual(data["errors"], [])

    def test_fuzz_sanitizer_is_separate_from_global_sanitizer_flag(self):
        path = self.write_runner("fuzz")
        info = json.loads(path.read_text(encoding="utf-8"))
        info["build"]["MC_FUZZ_ASAN"] = "ON"
        path.write_text(json.dumps(info), encoding="utf-8")
        _, data = self.chart_data({})
        self.assertEqual(data["rows"][0]["info"]["build"]["MC_ENABLE_SANITIZERS"], "OFF")
        self.assertEqual(data["rows"][0]["info"]["build"]["MC_FUZZ_ASAN"], "ON")

    def test_invalid_runner_info_is_visible_without_changing_test_status(self):
        self.write_report("ctest-results.xml", '<testcase name="Passed"/>')
        for text in ("{broken", "[]", '{"schema_version":1,"job":[]}', '{"schema_version":1,"job":"build"}'):
            with self.subTest(text=text):
                (self.artifacts / "runner-info.json").write_text(text, encoding="utf-8")
                runners, errors = summary.collect_runner_info(self.artifacts)
                self.assertFalse(runners)
                self.assertTrue(errors)
                _, data = self.chart_data({})
                self.assertTrue(data["errors"])
                self.assertFalse(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_complete_runner_json_is_archived_by_cli(self):
        self.write_runner("build")
        output = self.artifacts / "nightly-runners.json"
        self.assertEqual(summary.main(["--artifacts-dir", str(self.artifacts), "--runner-info-out", str(output),
                                       "--out", str(self.artifacts / "body.md"),
                                       "--status-out", str(self.artifacts / "status.txt")]), 0)
        info = json.loads(output.read_text(encoding="utf-8"))
        self.assertEqual(info["jobs"]["build"]["cpu"]["logical_cpus"], 4)
        self.assertEqual(info["errors"], [])

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
        (self.artifacts / "benchmark-overview.png").write_bytes(b"png")
        (self.artifacts / "benchmark-overview.svg").write_text("<svg/>", encoding="utf-8")
        (self.artifacts / "benchmark-compare.json").write_text(
            json.dumps({"status": "ok", "images": ["benchmark-overview.png"]}), encoding="utf-8")
        body = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", True)
        self.assertIn("通过 2；失败 0；跳过 0", body)
        self.assertIn("benchmark-overview.png", body)
        self.assertNotIn("Benchmark comparison", body)

    def test_ctest_skip_reason_is_taken_from_gtest_output(self):
        self.write_report(
            "ctest-results.xml",
            '<testcase name="MissingItem"><skipped message="SKIP_REGULAR_EXPRESSION_MATCHED"/>'
            '<system-out>test.cpp:42: Skipped\nStone item not registered\n\n[ SKIPPED ] MissingItem</system-out>'
            '</testcase>',
        )
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(results["skipped"], ["MissingItem — Stone item not registered"])

    def test_benchmark_error_cannot_be_clean_even_when_junit_passes(self):
        self.write_report("ctest-results.xml", '<testcase name="Passed"/>')
        (self.artifacts / "benchmark-compare.json").write_text(
            json.dumps({"status": "error", "errors": ["bind failed"]}), encoding="utf-8")
        self.assertTrue(summary.has_failures({"benchmark": "success"}, self.artifacts))

    def test_corrupt_benchmark_manifest_keeps_the_full_failure_report(self):
        self.write_report("ctest-results.xml", '<testcase name="ActualFailure"><failure/></testcase>')
        (self.artifacts / "benchmark-compare.json").write_text("{broken", encoding="utf-8")
        body = summary.build_body({}, self.artifacts, "https://github.com/g122622/Cubium/actions/runs/1", "main", "abc", False)
        self.assertIn("ActualFailure", body)
        self.assertIn("/image-data/ci-reports/1/benchmark-error.png", body)
        self.assertTrue(summary.has_failures({}, self.artifacts))

    def test_sanitizer_error_in_skipped_case_still_fails_report(self):
        """用例跳过不能掩盖进程结束时的 sanitizer 错误。"""
        self.write_report("ctest-results.xml", '<testcase name="Skipped"><skipped/>'
                          '<system-out>==42==ERROR: LeakSanitizer: detected memory leaks</system-out></testcase>')
        results = summary.collect_ctest_results(self.artifacts)
        self.assertEqual(len(results["skipped"]), 1)
        self.assertIn("Sanitizer failure", results["errors"][0])
        self.assertTrue(summary.has_failures({"unit-tests": "success"}, self.artifacts))

    def test_sanitizer_log_cannot_be_clean_even_when_junit_passes(self):
        """直接日志中的 sanitizer 错误优先于全部通过的 JUnit。"""
        self.write_report("ctest-results.xml", '<testcase name="Passed"/>')
        (self.artifacts / "ctest-output.log").write_text(
            "==42==ERROR: AddressSanitizer: heap-use-after-free\n", encoding="utf-8")
        self.assertTrue(summary.has_failures({"unit-tests": "success"}, self.artifacts))
        body = summary.build_body({}, self.artifacts, "https://example/run/1", "main", "abc", True)
        self.assertIn("ASan / UBSan 检测结果", body)
        self.assertIn("heap-use-after-free", body)


if __name__ == "__main__":
    unittest.main()
