"""用真实 Ninja 构建图核验行为包增量依赖；fixture 不编译 C++ 或运行 GameTest。"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


class IntegratedPackBuildTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="cubium-pack-build-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name).resolve()
        self.packs = self.root / "packs"
        self.build = self.root / "build"
        self.node = shutil.which("node")
        self.cmake = shutil.which("cmake")
        self.ninja = shutil.which("ninja")
        cache = REPO / "build/CMakeCache.txt"
        if cache.exists():
            match = re.search(r"^CMAKE_MAKE_PROGRAM:[^=]+=(.+)$", cache.read_text(encoding="utf-8"), re.MULTILINE)
            if match and Path(match[1].strip()).is_file():
                self.ninja = match[1].strip()
        if not all((self.node, self.cmake, self.ninja)):
            self.skipTest("Node.js, CMake and Ninja are required")
        self.write("packs/packs.json", '["lighting"]')
        self.write("packs/package.json", '{"name":"pack-fixture","private":true}')
        self.write("packs/package-lock.json", '{"lockfileVersion":3,"packages":{}}')
        self.write("packs/tsconfig.base.json", '{}')
        self.write("packs/utils/helper.ts", "export const helper = 1;\n")
        self.write("packs/structures/gametests/room.mcstructure", "structure-v1")
        self.write("packs/lighting/manifest.json", '{}')
        self.write("packs/lighting/tsconfig.json", '{}')
        self.write("packs/lighting/src/main.ts", "export const value = 1;\n")
        shutil.copy2(REPO / "tests/integrated/build.mjs", self.packs / "build.mjs")
        # tsc 桩只模拟文件转换，真实 build.mjs 负责依赖安装、清理和资源复制。
        self.write("packs/node_modules/typescript/bin/tsc", """
const fs = require('node:fs');
const path = require('node:path');
const pack = path.dirname(process.argv[process.argv.indexOf('-p') + 1]);
const counter = path.join(pack, '..', 'compilations.txt');
fs.writeFileSync(counter, String((fs.existsSync(counter) ? Number(fs.readFileSync(counter, 'utf8')) : 0) + 1));
function emit(directory) {
  for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
    const source = path.join(directory, entry.name);
    if (entry.isDirectory()) emit(source);
    else if (source.endsWith('.ts') && !source.endsWith('.d.ts')) {
      const output = path.join(pack, 'scripts', path.relative(path.join(pack, 'src'), source).replace(/\\.ts$/, '.js'));
      fs.mkdirSync(path.dirname(output), {recursive: true});
      fs.copyFileSync(source, output);
    }
  }
}
emit(path.join(pack, 'src'));
""")
        fingerprint = hashlib.sha256((self.packs / "package.json").read_bytes()
                                     + (self.packs / "package-lock.json").read_bytes()).hexdigest()
        self.write("packs/node_modules/.cubium-dependencies.sha256", fingerprint)
        self.write("CMakeLists.txt", f'''cmake_minimum_required(VERSION 3.26)
project(PackDependencyFixture NONE)
set(NODE_EXECUTABLE "{Path(self.node).as_posix()}")
include("{(REPO / 'cmake/IntegratedPacks.cmake').as_posix()}")
mc_add_integrated_packs("${{CMAKE_SOURCE_DIR}}/packs")
''')
        self.run_command([self.cmake, "-S", str(self.root), "-B", str(self.build), "-G", "Ninja",
                          f"-DCMAKE_MAKE_PROGRAM={self.ninja}"])

    def write(self, relative, content):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        # WSL 的短命进程可能共享同一时钟刻度，编辑时间必须严格晚于上一轮 stamp。
        stamp = self.build / "tests/integrated/.build.stamp"
        if stamp.exists():
            edited = max(path.stat().st_mtime_ns, stamp.stat().st_mtime_ns + 1_000_000)
            os.utime(path, ns=(edited, edited))

    def run_command(self, command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def build_packs(self):
        return self.run_command([self.cmake, "--build", str(self.build), "--target", "mc_integrated_packs"])

    def count(self):
        return int((self.packs / "compilations.txt").read_text())

    def test_incremental_edits_additions_deletions_and_missing_products(self):
        self.build_packs()
        self.assertEqual(self.count(), 1)
        self.build_packs()
        self.assertEqual(self.count(), 1)
        self.write("packs/lighting/src/main.ts", "export const value = 2;\n")
        self.build_packs()
        self.assertEqual(self.count(), 2)
        product = self.packs / "lighting/scripts/main.js"
        self.assertIn("value = 2", product.read_text())
        product.unlink()
        self.build_packs()
        self.assertEqual(self.count(), 3)
        self.assertTrue(product.exists())
        self.write("packs/lighting/src/nested/added.ts", "export const added = 3;\n")
        self.build_packs()
        self.assertEqual(self.count(), 4)
        (self.packs / "lighting/src/nested/added.ts").unlink()
        self.build_packs()
        self.assertEqual(self.count(), 5)
        self.assertFalse((self.packs / "lighting/scripts/nested/added.js").exists())
        self.write("packs/structures/gametests/room.mcstructure", "structure-v2")
        self.build_packs()
        copied = self.packs / "lighting/structures/gametests/room.mcstructure"
        self.assertEqual(copied.read_text(), "structure-v2")
        (self.packs / "structures/gametests/room.mcstructure").unlink()
        (self.packs / "structures/gametests").rmdir()
        self.build_packs()
        self.assertFalse(copied.exists())
        compilations = self.count()
        self.build_packs()
        self.assertEqual(self.count(), compilations)
        # 损坏的 lockfile 必须触发重新安装并失败，不能复用已有 node_modules 绕过校验。
        self.write("packs/package-lock.json", '{"lockfileVersion":')
        rebuilt = subprocess.run([self.cmake, "--build", str(self.build), "--target", "mc_integrated_packs"],
                                 capture_output=True, text=True, timeout=30)
        self.assertNotEqual(rebuilt.returncode, 0)
        self.assertIn("installing locked dependencies with npm ci", rebuilt.stdout)

    def test_artifact_verification_does_not_install_or_compile(self):
        self.build_packs()
        dependencies = (self.packs / "node_modules").resolve()
        self.assertTrue(dependencies.is_relative_to(self.root))
        shutil.rmtree(dependencies)
        self.run_command([self.node, str(self.packs / "build.mjs"), "--verify"])
        self.assertFalse(dependencies.exists())
        self.assertEqual(self.count(), 1)
        (self.packs / "lighting/scripts/utils/helper.js").unlink()
        result = subprocess.run([self.node, str(self.packs / "build.mjs"), "--verify"],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertIn("helper.js", result.stderr)


if __name__ == "__main__":
    unittest.main()
