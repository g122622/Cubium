// tests/integrated 构建脚本：把共享 utils 复制到各行为包 src/ 下，再用 tsc 编译每个包。
//
// 设计：行为包之间是独立模块（不能跨包 import），但多个包需要共享 utils 工具函数。
// 为兼顾"共享源码"与"行为包隔离"，构建期把 tests/integrated/utils/ 复制到每个包的
// src/utils/ 下，再由各包 tsconfig 独立编译。这样每个包产出自包含的 scripts/（含 utils）。
//
// 运行：node build.mjs         编译所有包
//       node build.mjs --clean 清理所有包的 scripts/ 与复制的 src/utils/
//
// 由 CMake 在构建期调用（src/server/test/facade 的 add_custom_command），也支持开发者手动运行。

import * as fs from "node:fs";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const integratedRoot = __dirname;
const utilsDir = path.join(integratedRoot, "utils");
const tscBin = path.join(integratedRoot, "node_modules", "typescript", "bin", "tsc");
const nodeModulesDir = path.join(integratedRoot, "node_modules");

// 行为包目录名（每个目录是一个独立行为包，含 manifest.json + src/ + tsconfig.json）
const PACKS = ["challenge", "command", "mob_behavior", "block_behavior", "lighting", "starter", "teleport"];

const isClean = process.argv.includes("--clean");

/**
 * 确保 node_modules 已安装（typescript + @minecraft 类型包）。
 * node_modules 不入 git（.gitignore），CMake 构建期首次跑或 clone 后首次跑时需先 npm install。
 * 检测 node_modules/typescript 存在性，缺失则 spawn npm install（在线/离线均可，npm 自行解析）。
 * 幂等：已装则直接返回，不重复安装。
 */
function ensureDeps() {
  if (fs.existsSync(tscBin)) {
    return; // typescript 已就位（@minecraft 包同批安装，假定齐备）
  }
  console.log("[build] node_modules missing, running npm install...");
  const result = spawnSync("npm", ["install", "--no-audit", "--no-fund"], {
    stdio: "inherit",
    cwd: integratedRoot,
    shell: process.platform === "win32",
  });
  if (result.status !== 0) {
    console.error("[build] npm install failed (exit code {})".replace("{}", result.status));
    process.exit(1);
  }
  if (!fs.existsSync(tscBin)) {
    console.error("[build] npm install completed but tsc still missing at " + tscBin);
    process.exit(1);
  }
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
  if (isClean) {
    for (const pack of PACKS) {
      cleanPack(path.join(integratedRoot, pack));
    }
    console.log("[build] clean done");
    return;
  }

  // 确保 node_modules 就绪（typescript + @minecraft 类型包）。CMake 构建期首次跑或
  // clone 后首次跑时 node_modules 缺失，此处自动 npm install；已装则跳过。
  ensureDeps();

  for (const pack of PACKS) {
    const packDir = path.join(integratedRoot, pack);
    if (!fs.existsSync(packDir)) {
      console.warn(`[build] pack dir missing, skip: ${pack}`);
      continue;
    }
    console.log(`[build] staging utils -> ${pack}`);
    stageUtils(packDir);
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
