"""执行真实 gate shell，验证 gh 参数兼容、跨页检查和 API 失败时的关闭行为。"""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

WORKFLOW = Path(__file__).resolve().parents[3] / ".github/workflows/nightly-pococc.yml"


@unittest.skipUnless(sys.platform.startswith("linux") and shutil.which("jq"), "Requires Linux bash and jq")
class NightlyPococcGateTests(unittest.TestCase):
    def _run_gate(self, pages, api_failure):
        lines = WORKFLOW.read_text(encoding="utf-8").splitlines()
        start = next(index for index, line in enumerate(lines)
                     if line.strip() == "- name: Skip while existing Nightly CI is active")
        run = next(index for index in range(start, len(lines)) if lines[index].strip() == "run: |")
        body = []
        for line in lines[run + 1:]:
            if not line.startswith("          "):
                break
            body.append(line[10:])
        # GitHub 对显式 bash 启用 pipefail，默认 shell 只启用 errexit。
        explicit_bash = any(line.strip() == "shell: bash" for line in lines[start:run])
        command = ["bash", "-e"]
        if explicit_bash:
            command += ["-o", "pipefail"]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tools = root / "bin"
            tools.mkdir()
            gh = tools / "gh"
            gh.write_text(
                "#!/usr/bin/python3\nimport json,os,sys\n"
                "if '--slurp' in sys.argv and any(arg in sys.argv for arg in ('--jq','--template')):\n"
                " print('the --slurp option is not supported with --jq or --template',file=sys.stderr);sys.exit(1)\n"
                "if os.environ['API_FAILURE']=='1':sys.exit(1)\n"
                "pages=json.loads(os.environ['API_PAGES'])\n"
                "if '--paginate' not in sys.argv:pages=pages[:1]\n"
                "for page in pages:print(json.dumps(page))\n",
                encoding="utf-8",
            )
            gh.chmod(0o755)
            output = root / "output"
            environment = dict(os.environ, PATH=str(tools) + ":" + os.environ["PATH"],
                               GITHUB_REPOSITORY="g122622/Cubium", GITHUB_OUTPUT=str(output),
                               GITHUB_STEP_SUMMARY=str(root / "summary"),
                               API_PAGES=json.dumps(pages), API_FAILURE="1" if api_failure else "0")
            result = subprocess.run(command + ["-c", "\n".join(body)], env=environment,
                                    text=True, capture_output=True, timeout=10, check=False)
            return result.returncode, output.read_text() if output.exists() else ""

    def test_active_nightly_on_later_page_disables_cluster(self):
        pages = [{"workflow_runs": [{"status": "completed"}] * 100},
                 {"workflow_runs": [{"status": "in_progress"}]}]
        code, output = self._run_gate(pages, False)
        self.assertEqual(code, 0)
        self.assertEqual(output.strip(), "enabled=false")

    def test_completed_pages_enable_cluster(self):
        code, output = self._run_gate([{"workflow_runs": [{"status": "completed"}]}], False)
        self.assertEqual(code, 0)
        self.assertEqual(output.strip(), "enabled=true")

    def test_api_failure_cannot_enable_cluster(self):
        code, output = self._run_gate([], True)
        self.assertNotEqual(code, 0)
        self.assertEqual(output, "")


if __name__ == "__main__":
    unittest.main()
