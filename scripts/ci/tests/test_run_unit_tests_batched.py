"""验证中断批次保留完成结果、隔离故障用例，并继续批量执行未开始的用例。"""

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from run_unit_tests_batched import Runner
from summarize_failures import collect_ctest_failures


class BatchedUnitTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.runner = Runner(argparse.Namespace(build_dir=self.root, config="RelWithDebInfo",
                                               batch_timeout=300, test_timeout=60,
                                               out_log=self.root / "ctest-output.log",
                                               out_junit=self.root / "ctest-results.xml"))
        self.names = ["Suite.Passed", "Suite.Failed", "Suite.Skipped", "Suite.Hangs", "Suite.Later"]
        self.runner.filter_of = {name: name for name in self.names}

    def progress(self):
        return "\n".join([
            "[ RUN      ] Suite.Passed", "x" * 9000,
            "[       OK ] Suite.Passed (12 ms)",
            "[ RUN      ] Suite.Failed", "assertion evidence",
            "[  FAILED  ] Suite.Failed (23 ms)",
            "[ RUN      ] Suite.Skipped", "skip reason",
            "[  SKIPPED ] Suite.Skipped (0 ms)",
            "[ RUN      ] Suite.Hangs", "watchdog fired",
        ])

    def test_abort_and_timeout_recover_prefix_and_keep_unstarted_tests_batched(self):
        for timed_out in (False, True):
            with self.subTest(timed_out=timed_out), patch.object(
                self.runner, "_run_gtest", return_value=(-6, timed_out, self.progress())
            ):
                results, pending = self.runner._execute_subset(self.names, self.root / "batch.xml")
            self.assertEqual([result["name"] for result in results], self.names[:3])
            self.assertEqual([result["status"] for result in results], ["passed", "failure", "skipped"])
            self.assertEqual(results[0]["time"], 0.012)
            self.assertEqual(results[1]["message"], "assertion evidence")
            self.assertEqual(results[2]["message"], "skip reason")
            self.assertEqual(pending, [["Suite.Hangs"], ["Suite.Later"]])

    def test_missing_progress_keeps_bisection_and_timeout_fallback(self):
        for timed_out in (False, True):
            with self.subTest(timed_out=timed_out), patch.object(
                self.runner, "_run_gtest", return_value=(-6, timed_out, "startup failed")
            ):
                results, pending = self.runner._execute_subset(self.names, self.root / "batch.xml")
            self.assertEqual(results, [])
            self.assertEqual(pending, [[name] for name in self.names] if timed_out else
                             [self.names[:2], self.names[2:]])

    def test_parameter_names_and_summary_lines_do_not_create_false_results(self):
        self.runner.filter_of["Param.Case/readable value"] = "Param.Case/0"
        output = "\n".join([
            "[ RUN      ] Param.Case/0", "parameter assertion",
            "[  FAILED  ] Param.Case/0, where GetParam() = 42 (2 ms)",
            "[  FAILED  ] Param.Case/0", "[       OK ] Suite.Passed (1 ms)",
        ])
        results, interrupted = self.runner._parse_progress(output, ["Param.Case/readable value"])
        self.assertIsNone(interrupted)
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0]["name"], "Param.Case/readable value")
        self.assertEqual(results[0]["status"], "failure")

    def test_single_uses_test_timeout_and_is_not_rechecked_twice(self):
        with patch.object(self.runner, "_run_gtest", return_value=(-6, True, "watchdog")) as run:
            results, pending = self.runner._execute_subset(["Suite.Hangs"], self.root / "single.xml")
        self.assertEqual(run.call_args.args[2], 60)
        self.assertEqual(pending, [])
        self.assertTrue(results[0]["isolated"])
        self.assertTrue(results[0]["timed_out"])
        with patch.object(self.runner, "_run_single") as rerun:
            self.assertEqual(self.runner._recheck_single(results[0]), results[0])
        rerun.assert_not_called()

    def test_capture_keeps_progress_before_large_output(self):
        output = self.progress()
        with patch("run_unit_tests_batched.subprocess.run",
                   return_value=subprocess.CompletedProcess([], -6, output, "sanitizer evidence")):
            _, _, captured = self.runner._run_gtest(self.names, self.root / "unused.xml", 300)
        self.assertTrue(captured.startswith("[ RUN      ] Suite.Passed"))
        self.assertTrue(captured.endswith("sanitizer evidence"))

    def test_timeout_keeps_complete_progress_and_decodes_bytes(self):
        expired = subprocess.TimeoutExpired([], 300, output=self.progress().encode(), stderr=b"killed")
        with patch("run_unit_tests_batched.subprocess.run", side_effect=expired):
            code, timed_out, captured = self.runner._run_gtest(self.names, self.root / "unused.xml", 300)
        self.assertEqual(code, -1)
        self.assertTrue(timed_out)
        results, interrupted = self.runner._parse_progress(captured, self.names)
        self.assertEqual(len(results), 3)
        self.assertEqual(interrupted, "Suite.Hangs")

    def test_existing_xml_remains_authoritative_when_process_exits_abnormally(self):
        def execute(names, path, timeout):
            path.write_text('<testsuites><testsuite><testcase classname="Suite" name="Passed" '
                            'time="1.25"/><testcase classname="Suite" name="Failed" time="2">'
                            '<failure message="XML evidence"/></testcase></testsuite></testsuites>')
            return -6, False, self.progress()

        with patch.object(self.runner, "_run_gtest", side_effect=execute):
            results, pending = self.runner._execute_subset(self.names[:2], self.root / "batch.xml")
        self.assertEqual(results[0]["time"], 1.25)
        self.assertEqual(results[1]["message"], "XML evidence")
        self.assertEqual(pending, [])

    def test_exit_failure_after_passing_xml_is_archived_separately(self):
        """全部用例完成后的泄漏、析构崩溃和超时均不能被通过 XML 掩盖。"""
        for code, timed_out, evidence in ((23, False, "ERROR: LeakSanitizer: leaks"),
                                         (-6, False, "destructor aborted"),
                                         (0, True, "shutdown timed out")):
            with self.subTest(code=code, timed_out=timed_out):
                self.runner.process_issues.clear()

                def execute(names, output_path, timeout):
                    output_path.write_text('<testsuites><testsuite><testcase classname="Suite" '
                                           'name="Passed" time="1"/><testcase classname="Suite" '
                                           'name="Later" time="2"/></testsuite></testsuites>')
                    return code, timed_out, evidence

                with patch.object(self.runner, "_run_gtest", side_effect=execute):
                    results, pending = self.runner._execute_subset(
                        ["Suite.Passed", "Suite.Later"], self.root / "batch.xml")
                self.assertEqual([result["status"] for result in results], ["passed", "passed"])
                self.assertEqual(pending, [])
                summary = self.runner.write_outputs([(1, "Suite.Passed"), (2, "Suite.Later")], results)
                self.assertEqual(summary["total"], 3)
                self.assertEqual(summary["errors"], 1)
                self.assertIn(evidence, self.runner.options.out_log.read_text(encoding="utf-8"))
                xml = ET.parse(self.runner.options.out_junit)
                self.assertIn(evidence, xml.findtext(".//error"))
                self.assertTrue(collect_ctest_failures(self.root))

    def test_single_passing_xml_does_not_hide_sanitizer_exit(self):
        def execute(names, output_path, timeout):
            output_path.write_text('<testsuites><testsuite><testcase classname="Suite" '
                                   'name="Passed" time="1"/></testsuite></testsuites>')
            return 1, False, "ERROR: AddressSanitizer: heap-use-after-free"

        with patch.object(self.runner, "_run_gtest", side_effect=execute):
            result = self.runner._run_single("Suite.Passed")
        self.assertEqual(result["status"], "passed")
        self.assertEqual(len(self.runner.process_issues), 1)

    def test_regular_assertion_failure_is_not_a_process_error(self):
        self.runner._record_process_issue(["Suite.Failed"], 1, False, "assertion failed",
                                          [{"status": "failure"}])
        self.assertEqual(self.runner.process_issues, [])


if __name__ == "__main__":
    unittest.main()
