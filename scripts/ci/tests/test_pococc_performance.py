"""验证真实 CPU 计量、并行日志归组和性能报告证据门禁。"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
if sys.platform.startswith("linux"):
    import pococc_performance as performance


@unittest.skipUnless(sys.platform.startswith("linux"), "Requires Linux resource accounting")
class PococcPerformanceTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("ccache") and shutil.which("clang++-22"), "Requires ccache and Clang 22")
    def test_actual_warm_ccache_ignores_restored_counters_and_creates_zero_remote_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "sample.cpp"
            source.write_text("int value() { return 42; }\n")
            command = ["ccache", "clang++-22", "-c", str(source), "-o", str(root / "sample.o")]
            with patch.dict(os.environ, CCACHE_DIR=str(root / "ccache"), CCACHE_PREFIX=""):
                subprocess.run(command, check=True)
                before = performance.ccache_counters()
                self.assertEqual(before["misses"], 1)
                options = argparse.Namespace(command=command, out=root / "performance.json",
                    pool="test", workers=1, slots=2, jobs=4, preprocess_jobs=4,
                    journal=root / "missing.tsv", client_log=root / "missing.log")
                self.assertEqual(performance.build(options), 0)
            record = json.loads(options.out.read_text())
            self.assertEqual(record["ccache"]["direct_hits"], 1)
            self.assertEqual(record["ccache"]["misses"], 0)
            self.assertEqual(record["distcc_submissions"], 0)
            self.assertEqual(record["remote_attempts"], 0)
            self.assertEqual(record["submitted_by_source"], {})

    def test_remote_retry_preserves_both_attempts_and_one_delivered_result(self):
        log = '\n'.join([
            'distcc[10] (dcc_scan_args) found input file "/workspace/a.cpp"',
            'distcc[11] (dcc_scan_args) found input file "/workspace/b.cpp"',
            'distcc[10] exec on host-a/2,lzo: clang++ -c a.cpp',
            'distcc[11] exec on host-b/2,lzo: clang++ -c b.cpp',
            'distcc[10] (dcc_readx) ERROR: unexpected eof on fd8',
            'distcc[10] exec on host-c/2,lzo: clang++ -c a.cpp',
            'distcc[11] compile /workspace/b.cpp on host-b/2,lzo completed ok',
            'distcc[10] compile /workspace/a.cpp on host-c/2,lzo completed ok',
        ])
        result = performance.client_metrics(log)
        self.assertEqual(result['remote_attempts'], 3)
        self.assertEqual(result['remote_retries'], 1)
        self.assertEqual(result['attempted_by_source'], {'/workspace/a.cpp': 2, '/workspace/b.cpp': 1})
        self.assertEqual(result['delivered_by_source'], {'/workspace/a.cpp': 1, '/workspace/b.cpp': 1})
        with self.assertRaisesRegex(RuntimeError, 'no input source'):
            performance.client_metrics('distcc[12] exec on host/2,lzo: clang++ -c c.cpp')

    def test_composite_diagnostics_call_actual_runner_collector_for_each_pool(self):
        repository = Path(__file__).resolve().parents[3]
        action = (repository / ".github/actions/build-pococc/action.yml").read_text()
        # 执行真正的 action shell 与 collector 的 CLI，捕获两层接口不一致。
        lines = action[action.index('        case "$POOL" in'):].splitlines()
        body = []
        for line in lines:
            if not line.startswith("        "):
                break
            body.append(line[8:])
        body = "\n".join(body)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").symlink_to(repository / "scripts", target_is_directory=True)
            (root / "pococc-diagnostics").mkdir()
            for pool, expected in (("test", "build"), ("release", "release-build"), ("fuzz", "fuzz")):
                result = subprocess.run(["bash", "-e", "-c", body], cwd=root,
                                        env=dict(os.environ, POOL=pool, BUILD_DIR=str(root / "not-built")),
                                        capture_output=True, text=True, check=False, timeout=30)
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads((root / "pococc-diagnostics/runner-info.json").read_text())
                self.assertEqual(record["job"], expected)

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
                                   user_seconds=10, system_seconds=2, remote_attempts=1, remote_retries=0,
                                   submitted_by_source={"/a.cpp": 1}, attempted_by_source={"/a.cpp": 1},
                                   delivered_by_source={"/a.cpp": 1}, ccache_restore_key="",
                                   ccache={"direct_hits": 0, "preprocessed_hits": 0, "misses": 1, "uncacheable": 0})
                worker = dict(kind="worker", pool=pool, run_id="123", worker=1, remote_compiles=1,
                              failed_compiles=0, compiler_cpu_seconds=30, intervals=[[1, 2]],
                              input_bytes=100, output_bytes=200, input_receive_seconds=0.1,
                              completed_by_source={"/a.cpp": 1}, probe_compiles=1)
                performance.write_json(root / pool / "coordinator" / "pococc-performance.json", coordinator)
                performance.write_json(root / pool / "worker" / "pococc-performance.json", worker)
            markdown, _, errors = performance.performance_report(root, "123")
            self.assertEqual(errors, [])
            self.assertIn("CPU-min", markdown)
            # 首个响应丢失后成功重试：两次 worker 完成都计费，但客户端只交付一个对象文件。
            coordinator.update(remote_attempts=2, remote_retries=1, attempted_by_source={"/a.cpp": 2})
            worker.update(remote_compiles=2, completed_by_source={"/a.cpp": 2})
            performance.write_json(root / "test/coordinator/pococc-performance.json", dict(coordinator, pool="test"))
            performance.write_json(root / "test/worker/pococc-performance.json", dict(worker, pool="test"))
            self.assertEqual(performance.performance_report(root, "123")[2], [])
            # 无对应尝试的额外完成、错误源码或未交付的输出仍必须阻塞报告。
            worker.update(completed_by_source={"/wrong.cpp": 2})
            performance.write_json(root / "test/worker/pococc-performance.json", dict(worker, pool="test"))
            self.assertTrue(performance.performance_report(root, "123")[2])
            (root / "test/worker/pococc-performance.json").unlink()
            self.assertTrue(performance.performance_report(root, "123")[2])
            with self.assertRaisesRegex(RuntimeError, "different run"):
                performance.performance_report(root, "999")

    def test_fully_cached_pool_keeps_idle_worker_and_reports_current_build_hits(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for pool in ("test", "release", "fuzz"):
                coordinator = dict(kind="coordinator", pool=pool, run_id="123", workers=1,
                    slots_per_worker=2, ninja_jobs=4, preprocess_jobs=4, exit_code=0, failures=0,
                    distcc_submissions=0, local_invocations=0, wall_seconds=1, user_seconds=0.1,
                    system_seconds=0.1, remote_attempts=0, remote_retries=0, submitted_by_source={},
                    attempted_by_source={}, delivered_by_source={}, ccache_restore_key="ccache-linux-nightly-122",
                    ccache={"direct_hits": 100, "preprocessed_hits": 0, "misses": 0, "uncacheable": 0})
                worker = dict(kind="worker", pool=pool, run_id="123", worker=1, remote_compiles=0,
                    failed_compiles=0, compiler_cpu_seconds=0, intervals=[], input_bytes=0, output_bytes=0,
                    input_receive_seconds=0, completed_by_source={}, probe_compiles=1)
                performance.write_json(root/pool/"coordinator/pococc-performance.json", coordinator)
                performance.write_json(root/pool/"worker/pococc-performance.json", worker)
            markdown, _, errors = performance.performance_report(root, "123")
            self.assertEqual(errors, [])
            self.assertIn("100 / 0 (100.0%)", markdown)
            coordinator["ccache"]["misses"] = 1
            performance.write_json(root/"fuzz/coordinator/pococc-performance.json", coordinator)
            self.assertTrue(performance.performance_report(root, "123")[2])


if __name__ == "__main__":
    unittest.main()
