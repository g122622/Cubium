#!/usr/bin/env node
// ============================================================
// normalize_includes.ts — 把 #include "..." 中的 ../ 相对路径
// 规范化为项目根的绝对 include 路径（如 common/world/...）。
//
// 动机：CODE_CONVENTIONS.md §8 禁止 #include "../..." 上跳目录，
// 要求用根路径。本脚本扫描 src/ 和 tests/ 下所有 .cpp/.hpp，
// 把形如 #include "../../chunk/IChunkGenerator.hpp" 的相对 include
// 改写为 #include "server/world/gen/chunk/IChunkGenerator.hpp"。
//
// 规则：
//   1. 只处理 quoted include（#include "..."）中含 ../ 的行
//   2. 相对当前文件目录解析 ../，得到目标物理绝对路径
//   3. 物理路径优先用 src/ 根表达（common/...、server/...、client/...）
//      次用 tests/ 根表达（client/...、common/...、...）
//      再用 include/ 根表达
//   4. 解析失败（文件不存在）则保留原样并警告
//   5. 不动 angled include（<...>）和不含 ../ 的 quoted include
//
// Usage:
//   node --experimental-strip-types scripts/format/normalize_includes.ts             # dry-run 扫 src + tests
//   node --experimental-strip-types scripts/format/normalize_includes.ts --write     # 落地改写
//   node --experimental-strip-types scripts/format/normalize_includes.ts src/common  # 扫指定目录
//   node --experimental-strip-types scripts/format/normalize_includes.ts --summary-only
//
//   -h, --help        显示本帮助
//   --write           落地改写源文件（默认 dry-run，只打印 diff）
//   --summary-only    只打印汇总统计，不打印逐文件 diff
//   scan_target       目录或文件（相对 PROJECT_ROOT），默认 src tests
// ============================================================

const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const PROJECT_ROOT = path.resolve(__dirname, '..', '..');

// include 根优先级：
// 先 src/（命中 common/server/client 前缀），再 tests/，再 include/
// src/common 也是 -I 根，但用 src/ 根表达为 common/... 更符合项目规范
const INCLUDE_ROOTS = [
    path.join(PROJECT_ROOT, 'src'),
    path.join(PROJECT_ROOT, 'tests'),
    path.join(PROJECT_ROOT, 'include'),
];

// ---- 命令行参数解析 ----
const argv = process.argv.slice(2);
let WRITE_MODE = false;
let SUMMARY_ONLY = false;
let SCAN_TARGETS = [];
for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--write') WRITE_MODE = true;
    else if (a === '--summary-only') SUMMARY_ONLY = true;
    else if (a === '-h' || a === '--help') {
        console.log(`Usage: normalize_includes.ts [options] [scan_target...]

  scan_target  目录或文件（相对 PROJECT_ROOT），默认 src tests
               目录递归展开为 .cpp + .hpp（git ls-files，排除 .gen.*）

Options:
  --write           落地改写源文件（默认 dry-run，只打印 diff）
  --summary-only    只打印汇总统计，不打印逐文件 diff
  -h, --help        显示本帮助
`);
        process.exit(0);
    } else if (!a.startsWith('-')) {
        SCAN_TARGETS.push(a);
    } else {
        console.error(`Unknown option: ${a}`);
        process.exit(2);
    }
}
if (SCAN_TARGETS.length === 0) SCAN_TARGETS = ['src', 'tests'];

// ---- 把物理绝对路径规范化为项目 include 路径 ----
// 返回 { ok: true, normalized: "common/world/..." } 或 { ok: false }
function normalizePhysPath(phys) {
    const norm = path.resolve(phys).replace(/\\/g, '/');
    for (const root of INCLUDE_ROOTS) {
        const rootNorm = root.replace(/\\/g, '/');
        if (norm.startsWith(rootNorm + '/')) {
            return { ok: true, normalized: norm.slice(rootNorm.length + 1) };
        }
    }
    return { ok: false };
}

// ---- 解析 include 路径为物理绝对路径 ----
// incPath: include 字符串内的路径（如 "../../chunk/IChunkGenerator.hpp"）
// currentFileDir: 当前文件所在目录的绝对路径
// 返回物理绝对路径或 null
function resolveIncludePath(incPath, currentFileDir) {
    // 1. 相对当前文件目录解析（处理 ../X.hpp、subdir/X.hpp 这种相对路径）
    const relToCurrent = path.resolve(currentFileDir, incPath);
    if (fs.existsSync(relToCurrent)) return relToCurrent;
    // 2. 依次拼到各 include 根下试探（模拟编译器 -I 查找）
    for (const root of INCLUDE_ROOTS) {
        const p = path.join(root, incPath);
        if (fs.existsSync(p)) return p;
    }
    return null;
}

// ---- 枚举目标文件（.cpp + .hpp）----
function enumerateFiles(targets) {
    const files = [];
    for (const t of targets) {
        const abs = path.isAbsolute(t) ? t : path.join(PROJECT_ROOT, t);
        const rel = path.relative(PROJECT_ROOT, abs).replace(/\\/g, '/');
        if (fs.existsSync(abs) && fs.statSync(abs).isFile()) {
            if (rel.endsWith('.cpp') || rel.endsWith('.hpp')) files.push(rel);
            continue;
        }
        // 目录：用 git ls-files 枚举
        const prefix = rel.endsWith('/') ? rel : rel + '/';
        const r = spawnSync('git', ['ls-files', '--',
            `${prefix}**/*.cpp`, `${prefix}*.cpp`,
            `${prefix}**/*.hpp`, `${prefix}*.hpp`,
        ], { cwd: PROJECT_ROOT, encoding: 'utf8' });
        if (r.status !== 0) {
            console.error(`git ls-files failed for ${rel}: ${r.stderr}`);
            continue;
        }
        const list = (r.stdout || '').split('\n').filter(x => x.trim());
        for (const f of list) {
            if (f.endsWith('.gen.cpp') || f.endsWith('.gen.hpp')) continue;  // 自动生成
            files.push(f);
        }
    }
    return [...new Set(files)].sort();
}

// ---- 生成简单 diff ----
function makeDiff(original, modified) {
    if (original === modified) return '';
    const a = original.split(/\r?\n/);
    const b = modified.split(/\r?\n/);
    const n = a.length, m = b.length;
    const dp = Array.from({ length: n + 1 }, () => new Array(m + 1).fill(0));
    for (let i = n - 1; i >= 0; i--) {
        for (let j = m - 1; j >= 0; j--) {
            if (a[i] === b[j]) dp[i][j] = dp[i + 1][j + 1] + 1;
            else dp[i][j] = Math.max(dp[i + 1][j], dp[i][j + 1]);
        }
    }
    const out = [];
    let i = 0, j = 0;
    while (i < n && j < m) {
        if (a[i] === b[j]) { i++; j++; }
        else if (dp[i + 1][j] >= dp[i][j + 1]) { out.push(`  - ${a[i]}`); i++; }
        else { out.push(`  + ${b[j]}`); j++; }
    }
    while (i < n) { out.push(`  - ${a[i]}`); i++; }
    while (j < m) { out.push(`  + ${b[j]}`); j++; }
    return out.join('\n');
}

// ---- 处理单个文件 ----
// 返回 { changed, original, newContent, replaced, unresolved, warnings } 或 null（无变化）
function processFile(relPath) {
    const abs = path.join(PROJECT_ROOT, relPath);
    const original = fs.readFileSync(abs, 'utf8');
    const lines = original.split(/\r?\n/);
    let replaced = 0;
    let unresolved = 0;
    const warnings = [];

    for (let i = 0; i < lines.length; i++) {
        // 匹配 #include "..." 且路径含 ../
        const m = lines[i].match(/^(\s*#\s*include\s+")([^"]+)(")(.*)$/);
        if (!m) continue;
        const prefix = m[1];
        const incPath = m[2];
        const closing = m[3];
        const trailing = m[4];
        // 只处理含 ../ 的相对路径
        if (!incPath.includes('../')) continue;

        // 解析相对路径
        const currentFileDir = path.dirname(abs);
        const phys = resolveIncludePath(incPath, currentFileDir);
        if (!phys) {
            unresolved++;
            warnings.push(`  L${i + 1}: 无法解析 ${incPath}`);
            continue;
        }
        // 规范化为项目 include 路径
        const { ok, normalized } = normalizePhysPath(phys);
        if (!ok) {
            unresolved++;
            warnings.push(`  L${i + 1}: ${incPath} → ${phys} 不在 include 根下`);
            continue;
        }
        // 替换
        lines[i] = `${prefix}${normalized}${closing}${trailing}`;
        replaced++;
    }

    if (replaced === 0 && unresolved === 0) return null;
    const newContent = lines.join('\n');
    return { changed: newContent !== original, original, newContent, replaced, unresolved, warnings };
}

// ---- 主流程 ----
function main() {
    const files = enumerateFiles(SCAN_TARGETS);
    if (files.length === 0) {
        console.error('ERROR: No .cpp/.hpp files found.');
        process.exit(1);
    }
    const cppCount = files.filter(f => f.endsWith('.cpp')).length;
    const hppCount = files.length - cppCount;
    console.log(`Scanning ${files.length} file(s) from: ${SCAN_TARGETS.join(', ')}  (.cpp: ${cppCount}, .hpp: ${hppCount})`);
    console.log(`Mode: ${WRITE_MODE ? 'WRITE (落地改写)' : 'DRY-RUN (只打印)'}`);
    console.log('');

    let totalChanged = 0;
    let totalReplaced = 0;
    let totalUnresolved = 0;
    const allWarnings = [];

    for (const rel of files) {
        const result = processFile(rel);
        if (!result) continue;
        totalReplaced += result.replaced;
        totalUnresolved += result.unresolved;
        if (result.warnings.length) allWarnings.push(...result.warnings);

        if (result.changed) {
            totalChanged++;
            if (!SUMMARY_ONLY) {
                console.log(`\x1b[33mCHANGE: ${rel}\x1b[0m  (-${result.replaced} replaced, ${result.unresolved} unresolved)`);
                const diff = makeDiff(result.original, result.newContent);
                if (diff) console.log(diff);
            }
            if (WRITE_MODE) {
                fs.writeFileSync(path.join(PROJECT_ROOT, rel), result.newContent);
            }
        } else if (result.replaced === 0 && result.unresolved > 0) {
            // 无替换但有未解析
            if (!SUMMARY_ONLY) {
                console.log(`\x1b[31mWARN: ${rel}  (${result.unresolved} unresolved)\x1b[0m`);
                result.warnings.forEach(w => console.log(w));
            }
        }
    }

    console.log('');
    console.log('==========================================');
    console.log(`  Total: ${files.length}  Changed: ${totalChanged}  Replaced: ${totalReplaced}  Unresolved: ${totalUnresolved}`);
    console.log('==========================================');

    if (allWarnings.length) {
        console.log('\nWarnings (unresolved includes):');
        allWarnings.forEach(w => console.log(w));
    }

    if (WRITE_MODE && totalChanged) {
        console.log(`\n已落地改写 ${totalChanged} 个文件。建议：`);
        console.log('  1. git diff 复核');
        console.log('  2. clang-format -i 格式化改动的 .cpp/.hpp');
        console.log('  3. cmake --build 验证编译');
    }
}

main();
