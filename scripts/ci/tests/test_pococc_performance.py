"""验证真实 CPU 计量、并行日志归组和性能报告证据门禁。"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
if sys.platform.startswith("linux"):
    import pococc_performance as performance


@unittest.skipUnless(sys.platform.startswith("linux"), "Requires Linux resource accounting")
class PococcPerformanceTests(unittest.TestCase):
    def test_interleaved_worker_logs_exclude_probe_and_count_compressed_payload(self):
        log = "\n".join([
            "distccd[10] (dcc_r_token_int) got DOTI000000ff",
            "distccd[11] (dcc_r_token_int) got DOTI00000001",
            "distccd[10] (dcc_r_file_timed) 255 bytes received in 0.125000s, rate 2kB/s",
            "distccd[10] (dcc_collect_child) cc child 100 terminated with status 0",
            "distccd[11] (dcc_collect_child) cc child 101 terminated with status 0",
            "distccd[10] (dcc_x_token_int) send DOTO00000200",
            "distccd[10] (dcc_x_token_int) send SERR00000008",
            "distccd[11] (dcc_job_summary) client: 127.0.0.1:1 COMPILE_OK exit:0 sig:0 core:0 ret:0 time:1ms clang++ /tmp/pococc-probe-abc/probe.cpp",
            "distccd[10] (dcc_job_summary) client: 127.0.0.1:2 COMPILE_OK exit:0 sig:0 core:0 ret:0 time:2000ms clang++ /workspace/third_party/lib.cpp",
        ])
        timing = {"pid": 100, "start_ns": 100, "end_ns": 200,
                  "user_seconds": 1.25, "system_seconds": 0.25, "wall_seconds": 1.75}
        report = performance.worker_metrics(log, [timing])
        self.assertEqual(report["remote_compiles"], 1)
        self.assertEqual(report["input_bytes"], 255)
        self.assertEqual(report["output_bytes"], 520)
        self.assertEqual(report["compiler_cpu_seconds"], 1.5)
        self.assertEqual(report["input_receive_seconds"], 0.125)
        with self.assertRaisesRegex(RuntimeError, "Missing compiler CPU"):
            performance.worker_metrics(log, [])

    def test_measured_command_keeps_failure_and_real_cpu_time(self):
        result = performance.measured_command([sys.executable, "-c", "sum(i*i for i in range(2000000));raise SystemExit(7)"])
        self.assertEqual(result["exit_code"], 7)
        self.assertGreater(result["user_seconds"], 0)
        self.assertGreaterEqual(result["wall_seconds"], result["user_seconds"])

    def test_peak_uses_completed_intervals_and_handles_touching_boundaries(self):
        self.assertEqual(performance.peak_parallelism([[1, 3], [2, 4], [3, 5]]), 2)
        self.assertEqual(performance.peak_parallelism([]), 0)

    def test_timed_worker_driver_records_real_compiler_and_preserves_arguments(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            bootstrap = state / "Bootstrap.py"
            bootstrap.write_text('''import os
from pathlib import Path
def configure_compiler_commands(state, compiler):
    os.environ["DISTCC_CMDLIST"] = str(state / "compiler-commands")
    return [Path(compiler)]
def worker(options):
    configure_compiler_commands(options.state, os.environ["CI_COMPILER"])
''')
            # 用 Python 的退出码验证包装器行为，避免把测试变成编译器行为镜像。
            with patch.dict(os.environ, CI_COMPILER=sys.executable):
                performance.start_worker(argparse.Namespace(state=state, worker=1, bootstrap=bootstrap))
            wrapper = Path((state / "compiler-commands").read_text().strip())
            result = subprocess.run([str(wrapper), "-c", "sum(range(1000000));raise SystemExit(9)"], check=False)
            self.assertEqual(result.returncode, 9)
            record = json.loads((state / "compiler-times.jsonl").read_text())
            self.assertEqual(record["exit_code"], 9)
            self.assertGreater(record["user_seconds"], 0)

    def test_final_report_requires_matching_current_run_and_worker_completions(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for pool in ("test", "release", "fuzz"):
                coordinator = dict(kind="coordinator", pool=pool, run_id="123", workers=1,
                                   slots_per_worker=2, ninja_jobs=4, preprocess_jobs=4, exit_code=0,
                                   failures=0, distcc_submissions=1, local_invocations=0, wall_seconds=60,
                                   user_seconds=10, system_seconds=2)
                worker = dict(kind="worker", pool=pool, run_id="123", worker=1, remote_compiles=1,
                              failed_compiles=0, compiler_cpu_seconds=30, intervals=[[1, 2]],
                              input_bytes=100, output_bytes=200, input_receive_seconds=0.1)
                performance.write_json(root / pool / "coordinator" / "pococc-performance.json", coordinator)
                performance.write_json(root / pool / "worker" / "pococc-performance.json", worker)
            markdown, _, errors = performance.performance_report(root, "123")
            self.assertEqual(errors, [])
            self.assertIn("CPU-min", markdown)
            (root / "test/worker/pococc-performance.json").unlink()
            self.assertTrue(performance.performance_report(root, "123")[2])
            with self.assertRaisesRegex(RuntimeError, "different run"):
                performance.performance_report(root, "999")


if __name__ == "__main__":
    unittest.main()
