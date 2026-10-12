"""验证图表的统计口径、单位、缺失数据与实际输出。"""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import compare_benchmark as comparison
import report_plots as plots


class ReportPlotTest(unittest.TestCase):
    def test_median_is_normalized_before_sorting_mixed_units(self):
        data = {"benchmarks": [{"name": "Case", "real_time": value, "time_unit": unit}
                               for value, unit in [(1, "ms"), (100, "us"), (150, "us")]]}
        self.assertEqual(comparison.index_entries(data)["Case"]["time_ns"], 150000)

    def test_cv_and_stddev_are_not_plotted_as_time(self):
        data = {"benchmarks": [{"name": f"Case_{kind}", "run_name": "Case", "aggregate_name": kind,
                                "real_time": value, "time_unit": "ns", "aggregate_unit": unit}
                               for kind, value, unit in [("mean", 220, "time"), ("median", 200, "time"),
                                                         ("stddev", 50, "time"), ("cv", .25, "percentage")]]}
        self.assertEqual(list(comparison.index_entries(data)), ["Case"])
        self.assertEqual(comparison.index_entries(data)["Case"]["time_ns"], 200)
        excluded = {"benchmarks": [entry for entry in data["benchmarks"]
                                    if entry["aggregate_name"] in ("stddev", "cv")]}
        self.assertEqual(comparison.index_entries(excluded), {})

    def test_zero_baseline_and_different_statistics_have_no_false_percentage(self):
        current = {"context": {"ci_build_profile": "same"}, "benchmarks": [
            {"name": "Zero", "real_time": 10, "time_unit": "ns"},
            {"name": "Case_median", "aggregate_name": "median", "real_time": 20, "time_unit": "ns"}]}
        previous = {"context": {"ci_build_profile": "same"}, "benchmarks": [
            {"name": "Zero", "real_time": 0, "time_unit": "ns"},
            {"name": "Case_mean", "aggregate_name": "mean", "real_time": 10, "time_unit": "ns"}]}
        data = comparison.build_comparison(current, previous, 10, 20)
        self.assertTrue(all(row["change_pct"] is None for row in data["rows"]))
        self.assertEqual(data["warnings"], [])
        json.dumps(data, allow_nan=False)

    def test_measurement_errors_remain_visible_beside_valid_results(self):
        current = {"benchmarks": [{"name": "Valid", "real_time": 20},
                                   {"name": "Startup", "error_occurred": True, "error_message": "bind failed"}]}
        data = comparison.build_comparison(current, None, 10, 20)
        self.assertEqual(data["status"], "error")
        self.assertEqual(data["errors"], ["Startup: bind failed"])
        self.assertEqual(len(data["rows"]), 1)
        with tempfile.TemporaryDirectory() as temporary:
            images = plots.plot_benchmark(data, Path(temporary))
            self.assertIn("benchmark-error.png", images)
            self.assertIn("benchmark-times-01.png", images)
            svg = (Path(temporary) / "benchmark-error.svg").read_text(encoding="utf-8")
            self.assertIn("Startup: bind failed", svg)

    def test_invalid_time_cannot_be_reported_as_a_valid_measurement(self):
        for value in (True, -1, float("inf"), float("nan")):
            with self.subTest(value=value), self.assertRaises(ValueError):
                comparison.index_entries({"benchmarks": [{"name": "Case", "real_time": value}]})

    def test_invalid_optional_counters_do_not_discard_valid_timings(self):
        for value in (None, True, "invalid", float("inf"), float("nan")):
            with self.subTest(value=value):
                entry = comparison.index_entries({"benchmarks": [
                    {"name": "Case", "real_time": 20, "max_bytes_used": value, "num_allocs": 5}]})["Case"]
                self.assertEqual(entry["time_ns"], 20)
                self.assertEqual(entry["memory"], {"num_allocs": 5})

    def test_worker_matrix_is_preserved_and_runtime_snapshots_are_not_double_counted(self):
        def info(job, captured, artifact):
            return {"job": job, "captured_at": captured, "artifact": artifact, "runner": {},
                    "cpu": {}, "memory_bytes": None, "disk": {}, "tools": {}, "system": {}, "build": {}}
        data = plots.runner_plot_data({
            "fuzz/a": info("fuzz", "2026-01-01", "a"),
            "fuzz/b": info("fuzz", "2026-01-02", "b"),
            "workers/test-1": info("workers", "2026-01-01", "test-1"),
            "workers/test-2": info("workers", "2026-01-01", "test-2")}, {"benchmark": "skipped"}, [])
        self.assertEqual(len(data["rows"]), 3)
        self.assertEqual(data["snapshots"], 4)
        self.assertEqual(next(row["info"]["artifact"] for row in data["rows"] if row["info"]["job"] == "fuzz"), "b")
        self.assertEqual(data["missing"]["benchmark"], "未运行")
        self.assertIsNone(data["rows"][0]["info"]["memory_bytes"])

    def test_graphics_are_standalone_png_and_svg_files(self):
        from PIL import Image
        current = {"context": {"ci_build_profile": "same"}, "benchmarks": [
            {"name": "Case", "real_time": 2, "time_unit": "us"}]}
        previous = {"context": {"ci_build_profile": "same"}, "benchmarks": [
            {"name": "Case", "real_time": 1, "time_unit": "us"}]}
        data = comparison.build_comparison(current, previous, 10, 20)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            names = plots.plot_benchmark(data, directory)
            self.assertEqual(len(names), 2)
            for name in names:
                with Image.open(directory / name) as graphic:
                    self.assertGreater(graphic.width, 800)
                    self.assertGreater(graphic.height, 400)
                self.assertTrue((directory / Path(name).with_suffix(".svg")).is_file())
            links = "\n".join(plots.image_links(names, "https://example.com/plots"))
            self.assertNotIn("|---", links)
            self.assertIn("![", links)


if __name__ == "__main__":
    unittest.main()
