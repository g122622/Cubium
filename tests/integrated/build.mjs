// tests/integrated 构建脚本：把共享 utils 与共享结构复制到各行为包下，再用 tsc 编译每个包。
//
// 设计：行为包之间是独立模块（不能跨包 import），但多个包需要共享 utils 工具函数与 .mcstructure
// 结构文件。为兼顾"共享源码/资源"与"行为包隔离"，构建期把：
//   - tests/integrated/utils/           → 每个包的 src/utils/（共享工具函数）
//   - tests/integrated/structures/gametests/ → 每个包的 structures/gametests/（共享结构，与各包独有结构合并）
// 复制到每个包，再由各包 tsconfig 独立编译。这样每个包产出自包含的 scripts/ + structures/，可离线分发。
// 结构加载语义见 BehaviorPackStructureSource：按 structures/<ns>/<path>.mcstructure 在行为包内查找。
// 仅 gametests 命名空间走共享库；starter 用独立 startertests 命名空间，不在此共享体系内。
//
// 运行：node build.mjs         编译所有包
//       node build.mjs --clean 清理所有包的 scripts/ 与复制的 src/utils/（不清 structures/，独有结构是源文件）
//
// 由 cmake/IntegratedPacks.cmake 的独立资源目标在构建期调用，也支持开发者手动运行。

import * as fs from "node:fs";
import { createHash } from "node:crypto";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const integratedRoot = __dirname;
const utilsDir = path.join(integratedRoot, "utils");
const tscBin = path.join(integratedRoot, "node_modules", "typescript", "bin", "tsc");
const nodeModulesDir = path.join(integratedRoot, "node_modules");

// 共享结构库：跨行为包复用的 .mcstructure 放在此处（namespace 统一为 gametests）。
// 构建期由 stageSharedStructures 复制到每个包的 structures/gametests/ 下，与各包独有结构合并。
// 设计与 utils 共享一致：行为包之间是独立模块（不能跨包 import 资源），但结构文件可共享源文件——
// 构建期把共享结构复制进各包，使每个包产出自包含的 structures/（可离线分发），运行时各包仍从自己
// 目录加载（BehaviorPackStructureSource 按 structures/<ns>/<path>.mcstructure 在行为包内查找）。
// 仅 gametests 命名空间走共享；starter 用独立 startertests 命名空间，不在此共享体系内（教学示例）。
const sharedStructuresDir = path.join(integratedRoot, "structures", "gametests");

// 行为包目录名（每个目录是一个独立行为包，含 manifest.json + src/ + tsconfig.json）
const PACKS = JSON.parse(fs.readFileSync(path.join(integratedRoot, "packs.json"), "utf8"));

const isClean = process.argv.includes("--clean");
const isVerify = process.argv.includes("--verify");

/** 递归收集源文件，用于验证已归档的 JS，无需安装 npm 依赖。 */
function sourceFiles(directory) {
  const files = [];
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const filename = path.join(directory, entry.name);
    if (entry.isDirectory()) {
      files.push(...sourceFiles(filename));
    } else if (entry.isFile()) {
      files.push(filename);
    }
  }
  return files;
}

/** 验证测试构建携带的行为包产物，测试分片只消费同一份编译结果。 */
function verifyProducts() {
  const missing = [];
  for (const pack of PACKS) {
    const packDir = path.join(integratedRoot, pack);
    const srcDir = path.join(packDir, "src");
    const sources = sourceFiles(srcDir).filter((file) => !path.relative(srcDir, file).startsWith(`utils${path.sep}`));
    for (const source of sources) {
      if (source.endsWith(".ts") && !source.endsWith(".d.ts")) {
        const output = path.join(packDir, "scripts", path.relative(srcDir, source).replace(/\.ts$/, ".js"));
        if (!fs.existsSync(output)) {
          missing.push(output);
        }
      }
    }
    for (const source of fs.existsSync(utilsDir) ? sourceFiles(utilsDir) : []) {
      if (source.endsWith(".ts") && !source.endsWith(".d.ts")) {
        const output = path.join(packDir, "scripts", "utils", path.relative(utilsDir, source).replace(/\.ts$/, ".js"));
        if (!fs.existsSync(output)) {
          missing.push(output);
        }
      }
    }
    for (const name of fs.existsSync(sharedStructuresDir) ? fs.readdirSync(sharedStructuresDir) : []) {
      if (name.endsWith(".mcstructure") && !fs.existsSync(path.join(packDir, "structures", "gametests", name))) {
        missing.push(`${pack}/structures/gametests/${name}`);
      }
    }
  }
  if (missing.length > 0) {
    console.error(`[build] missing behavior pack products:\n${missing.join("\n")}`);
    process.exit(1);
  }
  console.log(`[build] verified ${PACKS.length} precompiled behavior packs`);
}

/**
 * 确保 node_modules 已安装（typescript + @minecraft 类型包）。
 * node_modules 不入 git（.gitignore），CMake 构建期首次跑或 clone 后首次跑时需先 npm ci。
 * package.json 与 lockfile 的摘要一致才复用依赖；变化时用 npm ci 严格恢复锁定版本。
 */
function ensureDeps() {
  const fingerprint = createHash("sha256")
    .update(fs.readFileSync(path.join(integratedRoot, "package.json")))
    .update(fs.readFileSync(path.join(integratedRoot, "package-lock.json")))
    .digest("hex");
  const stamp = path.join(nodeModulesDir, ".cubium-dependencies.sha256");
  if (fs.existsSync(tscBin) && fs.existsSync(stamp) && fs.readFileSync(stamp, "utf8") === fingerprint) {
    return;
  }
  console.log("[build] installing locked dependencies with npm ci...");
  const result = spawnSync("npm", ["ci", "--no-audit", "--no-fund"], {
    stdio: "inherit",
    cwd: integratedRoot,
    shell: process.platform === "win32",
  });
  if (result.status !== 0) {
    console.error("[build] npm ci failed (exit code {})".replace("{}", result.status));
    process.exit(1);
  }
  if (!fs.existsSync(tscBin)) {
    console.error("[build] npm ci completed but tsc still missing at " + tscBin);
    process.exit(1);
  }
  fs.writeFileSync(stamp, fingerprint);
}

/**
 * 递归复制目录。
 * @param src 源目录绝对路径
 * @param dest 目标目录绝对路径
 */
function copyDirRecursive(src, dest) {
  if (!fs.existsSync(src)) return;
  fs.mkdirSync(dest, { recursive: true });
  for (const entry of fs.readdirSync(src, { withFileTypes: true })) {
    const srcPath = path.join(src, entry.name);
    const destPath = path.join(dest, entry.name);
    if (entry.isDirectory()) {
      copyDirRecursive(srcPath, destPath);
    } else if (entry.isFile()) {
      fs.copyFileSync(srcPath, destPath);
    }
  }
}

/**
 * 递归删除目录（等同于 rm -rf）。
 * @param dir 目标目录绝对路径
 */
function removeDirRecursive(dir) {
  if (fs.existsSync(dir)) {
    fs.rmSync(dir, { recursive: true, force: true });
  }
}

/**
 * 把共享 utils 复制到指定包的 src/utils/ 下。
 * @param packDir 包目录绝对路径
 */
function stageUtils(packDir) {
  const destUtils = path.join(packDir, "src", "utils");
  removeDirRecursive(destUtils);
  copyDirRecursive(utilsDir, destUtils);
}

/**
 * 把共享结构库（structures/gametests/*.mcstructure）复制到指定包的 structures/gametests/ 下，
 * 与该包独有结构合并。幂等：每次构建覆盖同名共享结构，确保各包拿到最新共享版本。
 *
 * 不清理：clean 时不清 structures/（独有结构是源文件不可删，共享结构覆盖即可保持一致）。
 * 若包内已有同名独有结构（理论不应有——同名结构应统一走共享库），共享版覆盖之并 warn 提示。
 *
 * @param packDir 包目录绝对路径
 */
function stageSharedStructures(packDir) {
  const entries = fs.existsSync(sharedStructuresDir)
    ? fs.readdirSync(sharedStructuresDir, { withFileTypes: true })
    : [];
  const destDir = path.join(packDir, "structures", "gametests");
  const statePath = path.join(packDir, ".cubium-shared-structures.json");
  const previous = fs.existsSync(statePath) ? JSON.parse(fs.readFileSync(statePath, "utf8")) : [];
  const current = entries.filter((entry) => entry.isFile() && entry.name.endsWith(".mcstructure")).map((entry) => entry.name);
  for (const name of previous) {
    if (!current.includes(name)) {
      fs.rmSync(path.join(destDir, name), { force: true });
    }
  }
  for (const entry of entries) {
    if (!entry.isFile() || !entry.name.endsWith(".mcstructure")) {
      continue;
    }
    const srcPath = path.join(sharedStructuresDir, entry.name);
    fs.mkdirSync(destDir, { recursive: true });
    const destPath = path.join(destDir, entry.name);
    // 包内已有同名文件但内容不同（独有结构与共享结构撞名）→ warn 提示撞名，仍以共享版覆盖（统一来源）。
    if (fs.existsSync(destPath) && fs.readFileSync(destPath).length !== fs.readFileSync(srcPath).length) {
      console.warn(`[build] WARN: shared structure '${entry.name}' collides with pack-specific one in ${path.basename(packDir)}, overwriting with shared version`);
    }
    fs.copyFileSync(srcPath, destPath);
  }
  fs.writeFileSync(statePath, JSON.stringify(current) + "\n");
}

/**
 * 调用 tsc 编译指定包。
 * @param packDir 包目录绝对路径
 * @returns 编译是否成功（exit code === 0）
 */
function compilePack(packDir) {
  const tsconfigPath = path.join(packDir, "tsconfig.json");
  if (!fs.existsSync(tsconfigPath)) {
    console.warn(`[build] skip ${path.basename(packDir)}: no tsconfig.json`);
    return true;
  }
  // 清除已经删除的源文件留下的 JS，避免加载旧测试或重复注册。
  removeDirRecursive(path.join(packDir, "scripts"));
  const args = [tscBin, "-p", tsconfigPath];
  const result = spawnSync(process.execPath, args, { stdio: "inherit", cwd: packDir });
  return result.status === 0;
}

function cleanPack(packDir) {
  removeDirRecursive(path.join(packDir, "scripts"));
  removeDirRecursive(path.join(packDir, "src", "utils"));
  console.log(`[build] cleaned ${path.basename(packDir)}`);
}

function main() {
  let allOk = true;
  if (isVerify) {
    verifyProducts();
    return;
  }
  if (isClean) {
    for (const pack of PACKS) {
      cleanPack(path.join(integratedRoot, pack));
    }
    console.log("[build] clean done");
    return;
  }

  // 确保 node_modules 就绪（typescript + @minecraft 类型包）。CMake 构建期首次跑或
  // clone 后首次跑时 node_modules 缺失，此处自动 npm ci；已装则跳过。
  ensureDeps();

  for (const pack of PACKS) {
    const packDir = path.join(integratedRoot, pack);
    if (!fs.existsSync(packDir)) {
      console.warn(`[build] pack dir missing, skip: ${pack}`);
      continue;
    }
    console.log(`[build] staging utils -> ${pack}`);
    stageUtils(packDir);
    console.log(`[build] staging shared structures -> ${pack}`);
    stageSharedStructures(packDir);
    console.log(`[build] compiling ${pack}`);
    if (!compilePack(packDir)) {
      console.error(`[build] FAILED: ${pack}`);
      allOk = false;
    }
  }
  if (!allOk) {
    process.exit(1);
  }
  console.log("[build] all packs compiled");
}

main();
