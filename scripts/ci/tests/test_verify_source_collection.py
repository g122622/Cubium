"""验证游离源文件扫描结论的校验、降级与摘要生成，且发现游离时不阻塞流水线。"""

import json
import sys
import tempfile
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import verify_source_collection as verifier


def make_report(verdict="complete", orphans=(), warnings=(), counts=None):
    """构造一份结构合法的扫描报告；counts 缺省时按游离清单长度补齐。"""
    report_counts = {"compiled": 100, "declared": 20, "aggregated": 3, "orphan": len(orphans)}
    if counts is not None:
        report_counts.update(counts)
    return {
        "schema_version": 1,
        "verdict": verdict,
        "compile_commands_used": counts is None,
        "total": sum(report_counts.values()),
        "counts": report_counts,
        "warnings": list(warnings),
        "orphans": list(orphans),
        "sources": [],
    }


class VerifySourceCollectionTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root = Path(self.temp_dir.name)

    def write_report(self, payload):
        path = self.root / "scan" / "source-collection.json"
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(payload, str):
            path.write_text(payload, encoding="utf-8")
        else:
            path.write_text(json.dumps(payload), encoding="utf-8")
        return path

    def run_cli(self, payload, *extra):
        report = self.write_report(payload)
        return verifier.main(["--report", str(report), *extra])

    def test_clean_scan_is_reported_complete_and_needs_no_attention(self):
        report = verifier.load_report(self.write_report(make_report()))
        self.assertFalse(verifier.needs_attention(report))
        markdown = verifier.render_markdown(report)
        self.assertIn("✅ 无游离源文件", markdown)
        self.assertIn("✅ 完整", markdown)

    def test_orphans_are_listed_but_do_not_fail_the_step(self):
        payload = make_report(orphans=["src/a.cpp", "tests/unit/b.cpp"])
        self.assertEqual(self.run_cli(payload), 0)
        report = verifier.load_report(self.root / "scan" / "source-collection.json")
        self.assertTrue(verifier.needs_attention(report))
        markdown = verifier.render_markdown(report)
        self.assertIn("❌ 存在游离源文件", markdown)
        self.assertIn("`src/a.cpp`", markdown)
        self.assertIn("**2**", markdown)

    def test_degraded_scan_without_compile_database_is_visible(self):
        report = verifier.load_report(self.write_report(make_report(verdict="degraded")))
        markdown = verifier.render_markdown(report)
        self.assertIn("⚠️ 降级", markdown)
        self.assertIn("compile_commands.json", markdown)

    def test_parsing_warnings_make_the_verdict_untrustworthy(self):
        """解析告警意味着结论可能既虚高也虚低，必须显式标记且要求跟进。"""
        payload = make_report(verdict="unverifiable", warnings=["x/CMakeLists.txt: unknown variable ${A}"])
        report = verifier.load_report(self.write_report(payload))
        self.assertTrue(verifier.needs_attention(report))
        markdown = verifier.render_markdown(report)
        self.assertIn("❌ 不可采信", markdown)
        self.assertIn("unknown variable", markdown)

    def test_zero_orphans_with_warnings_still_needs_attention(self):
        """零游离 + 告警不能算干净：漏报风险仍在。"""
        payload = make_report(verdict="unverifiable", warnings=["a: ${B}"])
        self.assertTrue(verifier.needs_attention(verifier.load_report(self.write_report(payload))))

    def test_large_orphan_lists_are_truncated_in_the_summary(self):
        orphans = [f"src/generated_{index:04d}.cpp" for index in range(verifier._MAX_LISTED_ORPHANS + 5)]
        markdown = verifier.render_markdown(verifier.load_report(self.write_report(make_report(orphans=orphans))))
        self.assertIn(f"另有 {5} 个未列出", markdown)
        self.assertEqual(markdown.count("- `src/generated_"), verifier._MAX_LISTED_ORPHANS)

    def test_count_mismatch_is_a_corrupt_report(self):
        """计数与清单不一致说明报告写坏，不能展示自相矛盾的数字。"""
        payload = make_report(orphans=["src/a.cpp"], counts={"orphan": 7})
        with self.assertRaises(verifier.ReportError):
            verifier.load_report(self.write_report(payload))

    def test_unsupported_schema_and_malformed_json_are_rejected(self):
        with self.assertRaises(verifier.ReportError):
            verifier.load_report(self.write_report(dict(make_report(), schema_version=99)))
        with self.assertRaises(verifier.ReportError):
            verifier.load_report(self.write_report("{not json"))
        with self.assertRaises(verifier.ReportError):
            verifier.load_report(self.write_report(dict(make_report(), verdict="maybe")))

    def test_missing_report_does_not_fabricate_a_conclusion(self):
        """报告缺失时不得虚构“无游离”，默认也不阻塞流水线。"""
        argv = ["--report", str(self.root / "missing.json"), "--markdown-out", str(self.root / "out.md")]
        self.assertEqual(verifier.main(argv), 0)
        self.assertFalse((self.root / "out.md").exists())
        argc = argv + ["--fail-on-report-error"]
        self.assertEqual(verifier.main(argc), 1)

    def test_markdown_and_step_summary_are_both_written(self):
        payload = make_report(orphans=["src/a.cpp"])
        step_summary = self.root / "summary.md"
        step_summary.write_text("existing\n", encoding="utf-8")
        markdown_out = self.root / "scan" / "source-collection.md"
        self.assertEqual(self.run_cli(payload, "--markdown-out", str(markdown_out),
                                      "--step-summary", str(step_summary)), 0)
        self.assertIn("存在游离源文件", markdown_out.read_text(encoding="utf-8"))
        combined = step_summary.read_text(encoding="utf-8")
        self.assertTrue(combined.startswith("existing\n"))
        self.assertIn("游离源文件扫描", combined)


if __name__ == "__main__":
    unittest.main()