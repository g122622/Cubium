#!/usr/bin/env node
// ============================================================
// fix_includes.ts — 全仓库 #include 自动增删脚本（clang-include-cleaner 驱动）
//
// 用 clang-include-cleaner 的 --print=changes 输出，对 .cpp 文件自动增删 #include。
// 与 scripts/iwyu/iwyu.sh 的区别：
//   - iwyu.sh 只打印建议（不落地），是交互式人工 review 工具
//   - 本脚本解析建议、做路径规范化、保护关联头、文本化改写源文件
//
// 为什么不用 iwyu.sh 的 --edit：
//   --edit 是文件级原子改写，无法对单条建议做过滤（如保护 .cpp 的关联同名 .hpp）。
//   本脚本走 --print=changes → 解析 → 过滤 → 文本化改写，可控性更高。
//
// 详见 scripts/iwyu/README.md。
//
// Usage:
//   node --experimental-strip-types scripts/iwyu/fix_includes.ts                 # dry-run 扫 src/server（默认）
//   node --experimental-strip-types scripts/iwyu/fix_includes.ts --write         # 落地改写 src/server
//   node --experimental-strip-types scripts/iwyu/fix_includes.ts src/common/core # 扫指定目录
//   node --experimental-strip-types scripts/iwyu/fix_includes.ts --write src/common/core
//   node --experimental-strip-types scripts/iwyu/fix_includes.ts --summary-only  # 只打印汇总，不打印逐文件 diff
//
// 前置条件：build/compile_commands.json 必须新鲜（先 ./scripts/configure.sh build）。
// 串行执行（用户要求 src/全量串行起步）。--write 前会做"工作树脏则警告"。
// ============================================================

const { execSync, spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

// ---- 配置 ----
const PROJECT_ROOT = path.resolve(__dirname, '..', '..');
const BUILD_DIR = process.env.IWYU_BUILD_DIR
    ? path.resolve(PROJECT_ROOT, process.env.IWYU_BUILD_DIR)
    : path.join(PROJECT_ROOT, 'build');
// 精简 compile_commands.json 输出目录（性能优化：见 buildSlimCompileDb 注释）
const SLIM_BUILD_DIR = path.join(BUILD_DIR, 'iwyu_slim');

// 默认扫 src/server（用户要求先局限该目录看效果）
const DEFAULT_SCAN_TARGET = 'src/server';

// 排除这些 .cpp（风格特殊或风险高）
// - main.cpp：无关联头、含 pragma push_macro 等预处理魔法
const EXCLUDE_FILES = new Set([
    'src/server/main.cpp',
]);

// 默认排除的第三方头噪音（与 iwyu.sh 一致，单一大正则）
const IGNORE_HEADERS_DEFAULT =
    'build/vcpkg_installed|third_party/|perfetto|tracy|_deps/|OffsetAllocator|quickjs';

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
        console.log(`Usage: fix_includes.ts [options] [scan_target...]

  scan_target  目录或文件（相对 PROJECT_ROOT），默认 ${DEFAULT_SCAN_TARGET}
               目录会递归展开为 .cpp 列表（git ls-files）

Options:
  --write         落地改写源文件（默认 dry-run，只打印 diff）
  --summary-only  只打印汇总统计，不打印逐文件 diff
  -h, --help      显示本帮助

Environment:
  IWYU_BUILD_DIR  compile_commands.json 目录（默认 build）
`);
        process.exit(0);
    } else if (!a.startsWith('-')) {
        SCAN_TARGETS.push(a);
    } else {
        console.error(`Unknown option: ${a}`);
        process.exit(2);
    }
}
if (SCAN_TARGETS.length === 0) SCAN_TARGETS = [DEFAULT_SCAN_TARGET];

// ---- 工具：检测 clang-include-cleaner ----
function detectClangIc() {
    if (process.env.CLANG_INCLUDE_CLEANER) return process.env.CLANG_INCLUDE_CLEANER;
    // 1. PATH
    try {
        const r = spawnSync('clang-include-cleaner', ['--version'], { encoding: 'utf8' });
        if (r.status === 0) return 'clang-include-cleaner';
    } catch (_) { /* ignore */ }
    // 2. VS 自带
    const candidate = 'D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-include-cleaner.exe';
    if (fs.existsSync(candidate)) return candidate;
    console.error('ERROR: clang-include-cleaner not found. Set CLANG_INCLUDE_CLEANER.');
    process.exit(1);
}

// ---- 工具：检测 MSVC / Windows SDK / clang builtin 头路径 ----
function detectSystemHeaders(clangIc) {
    const headers = { msvc: '', sdkUcrt: '', sdkShared: '', sdkUm: '', clangBuiltin: '' };

    // MSVC
    const msvcRoot = process.env.MSVC_ROOT;
    let msvc = msvcRoot;
    if (!msvc) {
        const toolDir = 'D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC';
        if (fs.existsSync(toolDir)) {
            const versions = fs.readdirSync(toolDir).filter(v => /^\d+\.\d+\.\d+(\.\d+)?$/.test(v));
            versions.sort((a, b) => {
                const pa = a.split('.').map(Number);
                const pb = b.split('.').map(Number);
                for (let i = 0; i < pa.length; i++) if (pa[i] !== pb[i]) return pa[i] - pb[i];
                return 0;
            });
            if (versions.length) msvc = path.join(toolDir, versions[versions.length - 1]);
        }
    }
    if (msvc && fs.existsSync(path.join(msvc, 'include'))) headers.msvc = path.join(msvc, 'include');
    else { console.error('ERROR: MSVC include not found. Set MSVC_ROOT.'); process.exit(1); }

    // Windows SDK
    const sdkRoot = process.env.WIN_SDK_ROOT || 'D:/Windows Kits';
    let sdkVer = process.env.WIN_SDK_VERSION;
    if (!sdkVer) {
        const incDir = path.join(sdkRoot, '10/Include');
        if (fs.existsSync(incDir)) {
            const vers = fs.readdirSync(incDir).filter(v => /^\d+\.\d+\.\d+(\.\d+)?$/.test(v));
            vers.sort((a, b) => {
                const pa = a.split('.').map(Number);
                const pb = b.split('.').map(Number);
                for (let i = 0; i < pa.length; i++) if (pa[i] !== pb[i]) return pa[i] - pb[i];
                return 0;
            });
            if (vers.length) sdkVer = vers[vers.length - 1];
        }
    }
    if (sdkVer) {
        const sdkInc = path.join(sdkRoot, '10/Include', sdkVer);
        headers.sdkUcrt = path.join(sdkInc, 'ucrt');
        headers.sdkShared = path.join(sdkInc, 'shared');
        headers.sdkUm = path.join(sdkInc, 'um');
    } else { console.error('ERROR: Windows SDK not found. Set WIN_SDK_ROOT/WIN_SDK_VERSION.'); process.exit(1); }

    // clang builtin（从 --version 提取 LLVM 主版本号）
    try {
        const v = spawnSync(clangIc, ['--version'], { encoding: 'utf8' });
        const m = (v.stdout || '').match(/LLVM version (\d+)/);
        if (m) {
            const binDir = path.dirname(clangIc);
            const cand = path.join(binDir, '..', 'lib', 'clang', m[1], 'include');
            if (fs.existsSync(cand)) headers.clangBuiltin = cand;
        }
    } catch (_) { /* ignore */ }

    return headers;
}

// ---- 工具：构建 extra-arg 列表（只补系统头） ----
function buildExtraArgs(hdrs) {
    const args = [
        '--extra-arg=-DNOMINMAX',
        '--extra-arg=-DWIN32_LEAN_AND_MEAN',
        '--extra-arg=-D_CRT_SECURE_NO_WARNINGS',
        `--extra-arg=-I${hdrs.msvc}`,
        `--extra-arg=-I${hdrs.sdkUcrt}`,
        `--extra-arg=-I${hdrs.sdkShared}`,
        `--extra-arg=-I${hdrs.sdkUm}`,
    ];
    if (hdrs.clangBuiltin) args.push(`--extra-arg=-I${hdrs.clangBuiltin}`);
    return args;
}

// ---- 头文件物理路径反查表（用于路径规范化） ----
// -I 根优先级：先 src/common（短前缀在此根下命中），再 src，再 include，再 addon
// 注意：src/common 和 src 都是 -I 根，同一个 common 头在两根下都能找到。
// 规范化目标：除 perfetto 外，统一从 src 根表达（common/...、server/...、client/...）。
const IROOTS = [
    path.join(PROJECT_ROOT, 'src/common'),
    path.join(PROJECT_ROOT, 'src'),
    path.join(PROJECT_ROOT, 'include'),
    path.join(PROJECT_ROOT, 'src/common/mod/bedrock/addon'),
    path.join(PROJECT_ROOT, 'third_party/perfetto'),
];

// 把物理绝对路径规范化为 quoted 字符串（项目风格）
function normalizePhysPath(phys) {
    const norm = path.resolve(phys).replace(/\\/g, '/');
    const srcRoot = path.join(PROJECT_ROOT, 'src').replace(/\\/g, '/');
    const incRoot = path.join(PROJECT_ROOT, 'include').replace(/\\/g, '/');
    const perfettoRoot = path.join(PROJECT_ROOT, 'third_party/perfetto').replace(/\\/g, '/');

    if (norm.startsWith(perfettoRoot + '/')) {
        const rel = norm.slice(perfettoRoot.length + 1);
        return '<' + rel + '>';  // perfetto 三方库用尖括号
    }
    if (norm.startsWith(incRoot + '/')) {
        return '"' + norm.slice(incRoot.length + 1) + '"';
    }
    if (norm.startsWith(srcRoot + '/')) {
        return '"' + norm.slice(srcRoot.length + 1) + '"';  // common/... server/... client/...
    }
    return null;
}

// 把工具建议的 quoted/angled 头路径解析为物理路径，再规范化
// 返回规范化后的 quoted 字符串；若解析失败返回原始建议字符串
function resolveAndNormalize(headerSpec, currentFileDir) {
    // headerSpec 形如 '"command/ICommandSource.hpp"' 或 '"<memory>"' 或 '<memory>'
    // 拆出引号样式与路径
    let isAngle = false;
    let raw = headerSpec;
    if (raw.startsWith('<') && raw.endsWith('>')) { isAngle = true; raw = raw.slice(1, -1); }
    else if (raw.startsWith('"') && raw.endsWith('"')) { raw = raw.slice(1, -1); }
    else { return headerSpec; }

    if (isAngle) return headerSpec;  // 系统头/三方库尖括号不规范化

    // 1. 相对于当前文件目录解析（处理 ../X.hpp、subdir/X.hpp 这种相对路径）
    const relToCurrent = path.resolve(currentFileDir, raw);
    if (fs.existsSync(relToCurrent)) {
        const n = normalizePhysPath(relToCurrent);
        if (n) return n;
    }

    // 2. 依次拼到各 -I 根下试探
    for (const root of IROOTS) {
        const p = path.join(root, raw);
        if (fs.existsSync(p)) {
            const n = normalizePhysPath(p);
            if (n) return n;
        }
    }
    // 解析失败：保留原始建议（保守，不破坏）
    return headerSpec;
}

// ---- 枚举 .cpp 文件 ----
function enumerateCppFiles(targets) {
    const files = [];
    for (const t of targets) {
        const abs = path.isAbsolute(t) ? t : path.join(PROJECT_ROOT, t);
        const rel = path.relative(PROJECT_ROOT, abs).replace(/\\/g, '/');
        if (fs.existsSync(abs) && fs.statSync(abs).isFile()) {
            if (rel.endsWith('.cpp')) files.push(rel);
            continue;
        }
        // 目录：用 git ls-files 枚举（自然排除 .gitignore）
        // git ls-files 'src/server/*.cpp' 不递归；要递归用 'src/server/**/*.cpp'（git pathspec）
        const prefix = rel.endsWith('/') ? rel : rel + '/';
        const r = spawnSync('git', ['ls-files', '--', `${prefix}**/*.cpp`, `${prefix}*.cpp`], {
            cwd: PROJECT_ROOT, encoding: 'utf8',
        });
        if (r.status !== 0) {
            console.error(`git ls-files failed for ${rel}: ${r.stderr}`);
            continue;
        }
        const list = (r.stdout || '').split('\n').filter(x => x.trim());
        for (const f of list) {
            if (f.endsWith('.gen.cpp')) continue;        // 自动生成
            if (EXCLUDE_FILES.has(f)) continue;
            files.push(f);
        }
    }
    // 去重 + 排序
    return [...new Set(files)].sort();
}

// ---- 性能优化：为目标文件生成精简 compile_commands.json ----
// clang-include-cleaner 在 -p 模式下加载整个 compile_commands.json，开销与 database 大小成正比。
// 全量 database（Ninja Multi-Config 三配置 × 全部源文件）约 20MB / 1.2 万条，单文件冷启动 ~21 秒。
// 只保留本次目标文件、每文件一条 RelWithDebInfo 条目后，database 缩到 <1MB，单文件降至 ~1.5 秒。
// 返回精简 db 目录路径。失败时回退到全量 BUILD_DIR。
function buildSlimCompileDb(targetCppRels) {
    const fullDb = path.join(BUILD_DIR, 'compile_commands.json');
    if (!fs.existsSync(fullDb)) return BUILD_DIR;  // 回退（外层会报错）

    let db;
    try {
        db = JSON.parse(fs.readFileSync(fullDb, 'utf8'));
    } catch (e) {
        console.error(`WARN: parse compile_commands.json failed: ${e.message}; use full db`);
        return BUILD_DIR;
    }

    // 目标文件绝对路径集合（正斜杠）
    const targetSet = new Set(targetCppRels.map(f =>
        path.resolve(PROJECT_ROOT, f).replace(/\\/g, '/')));

    // 每文件优先取 RelWithDebInfo 配置；找不到则取任意一条
    const picked = new Map();  // file -> entry
    for (const e of db) {
        const f = (e.file || '').replace(/\\/g, '/');
        if (!targetSet.has(f)) continue;
        if (picked.has(f)) continue;  // 已取（保留首次，JSON 顺序即配置顺序）
        picked.set(f, e);
    }
    // 第二轮：优先替换为 RelWithDebInfo 那条（更贴近日常构建配置）
    for (const e of db) {
        const f = (e.file || '').replace(/\\/g, '/');
        if (!targetSet.has(f)) continue;
        const cmd = e.command || '';
        if (cmd.includes('RelWithDebInfo')) {
            picked.set(f, e);  // 后续命中覆盖，最终保留最后一条 RelWithDebInfo
        }
    }

    const missing = [...targetSet].filter(f => !picked.has(f));
    if (missing.length) {
        console.error(`WARN: ${missing.length} target file(s) not in compile_commands.json (stale db?):`);
        missing.slice(0, 5).forEach(f => console.error('  - ' + f));
        console.error('  Run ./scripts/configure.sh build to refresh. Falling back to full db.');
        return BUILD_DIR;
    }

    try {
        fs.mkdirSync(SLIM_BUILD_DIR, { recursive: true });
        fs.writeFileSync(
            path.join(SLIM_BUILD_DIR, 'compile_commands.json'),
            JSON.stringify([...picked.values()], null, 2));
    } catch (e) {
        console.error(`WARN: write slim db failed: ${e.message}; use full db`);
        return BUILD_DIR;
    }
    return SLIM_BUILD_DIR;
}

// ---- 跑 clang-include-cleaner 拿建议 ----
function runIncludeCleaner(clangIc, extraArgs, cppRel, dbDir) {
    const abs = path.join(PROJECT_ROOT, cppRel);
    const args = [
        '-p', dbDir,
        '--print=changes',
        `--ignore-headers=${IGNORE_HEADERS_DEFAULT}`,
        ...extraArgs,
        abs,
    ];
    const r = spawnSync(clangIc, args, { encoding: 'utf8', cwd: PROJECT_ROOT });
    // exit 0 + 有输出 = SUGGEST；0 + 无输出 = CLEAN；139/<0 = CRASH；其他 = ERROR
    const stdout = r.stdout || '';
    const stderr = r.stderr || '';
    const output = stdout + stderr;
    return { exitCode: r.status ?? -1, output };
}

// ---- 去重 --print=changes 的 N 段重复（等价 awk '!seen[$0]++'） ----
function dedupeChanges(text) {
    const lines = text.split(/\r?\n/);
    const seen = new Set();
    const out = [];
    for (const ln of lines) {
        if (!seen.has(ln)) { seen.add(ln); out.push(ln); }
    }
    return out.join('\n');
}

// ---- 解析 --print=changes 输出为建议列表 ----
// 行格式：
//   移除：`- "Result.hpp" @Line:24` 或 `- <algorithm> @Line:27`
//   插入：`+ "command/ICommandSource.hpp"` 或 `+ <memory>`
function parseChanges(text, currentFileDir) {
    const removals = [];  // { line: 1-based, header: 原始, normalized: 规范化, isAngle }
    const insertions = []; // { header: 原始, normalized, isAngle }
    const lines = text.split(/\r?\n/);
    for (const ln of lines) {
        // 移除：- <...> @Line:N 或 - "..." @Line:N
        let m = ln.match(/^\s*-\s+(<[^>]+>|"[^"]+")\s+@Line:(\d+)\s*$/);
        if (m) {
            const header = m[1];
            const line = parseInt(m[2], 10);
            const isAngle = header.startsWith('<');
            removals.push({
                line, header,
                normalized: isAngle ? header : resolveAndNormalize(header, currentFileDir),
                isAngle,
            });
            continue;
        }
        // 插入：+ <...> 或 + "..."
        m = ln.match(/^\s*\+\s+(<[^>]+>|"[^"]+")\s*$/);
        if (m) {
            const header = m[1];
            const isAngle = header.startsWith('<');
            insertions.push({
                header,
                normalized: isAngle ? header : resolveAndNormalize(header, currentFileDir),
                isAngle,
            });
            continue;
        }
    }
    return { removals, insertions };
}

// ---- 判断某 quoted include 是否是 .cpp 的关联头 ----
// 关联头 = 同名 .hpp（同目录，裸名 include）。规范化后的 quoted 路径若指向同目录同名 .hpp 视为关联头。
function isAssociatedHeader(quotedNormalized, cppRel) {
    const cppAbs = path.join(PROJECT_ROOT, cppRel);
    const cppBase = path.basename(cppAbs, '.cpp');
    const dir = path.dirname(cppAbs);
    // normalized 形如 "common/command/Foo.hpp" 或 "Foo.hpp"
    const inner = quotedNormalized.replace(/^"|"$/g, '');
    // 解析为物理路径（先相对当前目录，再各 -I 根）
    let phys = null;
    const localP = path.resolve(dir, inner);
    if (fs.existsSync(localP)) phys = localP;
    else {
        for (const root of IROOTS) {
            const p = path.join(root, inner);
            if (fs.existsSync(p)) { phys = p; break; }
        }
    }
    if (!phys) return false;
    const hBase = path.basename(phys, path.extname(phys));
    // 关联头：basename 相同，且物理在同目录或同目录子树（处理 gameevent 跨目录关联头）
    // 严格判定：物理头与 .cpp 同目录
    return hBase === cppBase && path.dirname(phys) === dir;
}

// ---- 文本化应用增删到一个文件内容 ----
// 返回 { newContent, removed, inserted, skippedAssociated }
function applyChanges(content, removals, insertions, cppRel) {
    const lines = content.split(/\r?\n/);
    let skippedAssociated = 0;

    // ---- 1. 移除（按行号从后往前删，避免行号偏移） ----
    // 同一行可能有多个移除建议（罕见），去重后按行号降序排序
    const removeByLine = new Map();  // line -> Set(normalized header)
    const filteredRemovals = [];
    for (const r of removals) {
        if (!r.isAngle) {
            // 保护关联头：跳过同名 .hpp 的移除
            if (isAssociatedHeader(r.normalized, cppRel)) {
                skippedAssociated++;
                continue;
            }
        }
        if (!removeByLine.has(r.line)) removeByLine.set(r.line, new Set());
        removeByLine.get(r.line).add(r.normalized);
        filteredRemovals.push(r);
    }

    const removedCount = filteredRemovals.length;
    // 按行号降序删
    const sortedLines = [...removeByLine.keys()].sort((a, b) => b - a);
    for (const ln of sortedLines) {
        const idx = ln - 1;  // 1-based -> 0-based
        if (idx < 0 || idx >= lines.length) continue;
        const headersToRemove = removeByLine.get(ln);
        const cur = lines[idx];
        // 当前行是否就是这些头的 include？精确匹配 #include "X" 或 #include <X>
        const incMatch = cur.match(/^(\s*#\s*include\s+)(<[^>]+>|"[^"]+")\s*$/);
        if (incMatch) {
            const hdr = incMatch[2];
            // 头是否在移除集合中（用规范化比较）
            let inSet = headersToRemove.has(hdr);
            if (!inSet) {
                // 尝试规范化当前行头再比较
                const isAngle = hdr.startsWith('<');
                if (!isAngle) {
                    const dir = path.dirname(path.join(PROJECT_ROOT, cppRel));
                    const norm = resolveAndNormalize(hdr, dir);
                    inSet = headersToRemove.has(norm) || headersToRemove.has(hdr);
                }
            }
            if (inSet) {
                lines.splice(idx, 1);
                continue;
            }
        }
        // 行号对不上（可能文件已被其他改过，或工具行号失准）：跳过该移除，保守不动
    }

    // ---- 2. 插入 ----
    // include 块约定：关联头(裸名) → 项目头("...") → 系统头(<...>) → 空行 → namespace
    // 插入策略（按分组，防重复）：
    //   项目头(quoted) → 插到最后一个 quoted include 之后；若无 quoted include，插到首个 include 之前
    //   系统头(angled) → 插到最后一个 angled include 之后；若无 angled include，紧跟 quoted 批之后
    //   组内按字母序，保持整洁

    // 重新扫描 include 结构（移除已改变行索引）
    const dir = path.dirname(path.join(PROJECT_ROOT, cppRel));
    const existingIncludes = new Set();
    let firstIncludeIdx = -1;
    let lastQuotedIdx = -1;
    let lastAngledIdx = -1;
    for (let i = 0; i < lines.length; i++) {
        const m = lines[i].match(/^\s*#\s*include\s+(<[^>]+>|"[^"]+")\s*$/);
        if (!m) continue;
        const hdr = m[1];
        const isAngle = hdr.startsWith('<');
        if (firstIncludeIdx === -1) firstIncludeIdx = i;
        if (isAngle) lastAngledIdx = i;
        else lastQuotedIdx = i;
        existingIncludes.add(isAngle ? hdr : resolveAndNormalize(hdr, dir));
    }

    // 去重后的插入列表
    const inserts = [];
    let skippedDup = 0;
    for (const ins of insertions) {
        if (existingIncludes.has(ins.normalized)) { skippedDup++; continue; }
        if (inserts.some(x => x.normalized === ins.normalized)) { skippedDup++; continue; }
        inserts.push(ins);
    }
    const quotedInserts = inserts.filter(x => !x.isAngle).map(x => `#include ${x.normalized}`).sort();
    const angledInserts = inserts.filter(x => x.isAngle).map(x => `#include ${x.normalized}`).sort();

    if (quotedInserts.length === 0 && angledInserts.length === 0) {
        return { newContent: lines.join('\n'), removed: removedCount, inserted: 0, skippedAssociated, skippedDup };
    }

    // 计算两批插入锚点（"在某行索引之后插入"）。无任何 include 时特殊处理。
    if (firstIncludeIdx === -1) {
        // 文件无任何 include：插到第一个非注释非空行之前
        let insertAt = 0;
        for (let i = 0; i < lines.length; i++) {
            const t = lines[i].trim();
            if (t === '' || t.startsWith('/*') || t.startsWith('*') || t.startsWith('//')) continue;
            insertAt = i;
            break;
        }
        const batch = [...quotedInserts, ...angledInserts];
        lines.splice(insertAt, 0, ...batch, '');
    } else {
        // 构造插入操作列表：{ after: 行索引(在此行后插), lines: [...] }
        // 按锚点降序执行避免索引漂移。
        const ops = [];

        // 系统头批（A）
        if (angledInserts.length) {
            if (lastAngledIdx !== -1) {
                // 紧跟现有最后一个 angled include 之后
                ops.push({ after: lastAngledIdx, lines: angledInserts });
            }
            // else：无 angled include，A 批合并到 quoted 批之后（见下方）
        }

        // 项目头批（Q）
        if (quotedInserts.length) {
            if (lastQuotedIdx !== -1) {
                // 有现有 quoted include：Q 批在其后
                // 若 A 批无锚点，则 A 批紧跟 Q 批尾部
                let qLines = quotedInserts;
                if (angledInserts.length && lastAngledIdx === -1) {
                    qLines = [...quotedInserts, ...angledInserts];
                }
                ops.push({ after: lastQuotedIdx, lines: qLines });
            } else {
                // 无现有 quoted include（但有 angled include）：Q 批插在首个 include 之前
                ops.push({ after: firstIncludeIdx - 1, lines: quotedInserts, atFileStart: firstIncludeIdx === 0 });
            }
        } else if (angledInserts.length && lastAngledIdx === -1) {
            // 无 Q 批，但 A 批无锚点（文件只有 quoted include 的情况已由 Q 分支处理）
            // 走到这里说明：无 Q 批插入 + A 批无 angled 锚点 + 有 quoted include
            // A 批插在最后一个 quoted include 之后
            if (lastQuotedIdx !== -1) ops.push({ after: lastQuotedIdx, lines: angledInserts });
        }

        // 按锚点降序执行
        ops.sort((a, b) => b.after - a.after);
        for (const op of ops) {
            if (op.atFileStart && op.after < 0) {
                lines.splice(0, 0, ...op.lines);
            } else {
                lines.splice(op.after + 1, 0, ...op.lines);
            }
        }
    }

    const inserted = quotedInserts.length + angledInserts.length;
    return { newContent: lines.join('\n'), removed: removedCount, inserted, skippedAssociated, skippedDup };
}

// ---- 生成统一 diff（用于 dry-run 展示） ----
function makeDiff(original, modified, cppRel) {
    if (original === modified) return '';
    const a = original.split(/\r?\n/);
    const b = modified.split(/\r?\n/);
    // 简单 LCS diff
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

// ============================================================
// 主流程
// ============================================================
function main() {
    const clangIc = detectClangIc();
    console.log(`Using clang-include-cleaner: ${clangIc}`);
    const v = spawnSync(clangIc, ['--version'], { encoding: 'utf8' });
    console.log((v.stdout || '').split('\n')[0]);
    const hdrs = detectSystemHeaders(clangIc);
    console.log(`MSVC: ${hdrs.msvc}`);
    console.log(`Windows SDK ucrt: ${hdrs.sdkUcrt}`);
    if (hdrs.clangBuiltin) console.log(`Clang builtin: ${hdrs.clangBuiltin}`);
    const extraArgs = buildExtraArgs(hdrs);

    // compile_commands.json 存在性
    const compileDb = path.join(BUILD_DIR, 'compile_commands.json');
    if (!fs.existsSync(compileDb)) {
        console.error(`ERROR: ${compileDb} not found. Run './scripts/configure.sh build' first.`);
        process.exit(1);
    }

    // 枚举
    const files = enumerateCppFiles(SCAN_TARGETS);
    if (files.length === 0) {
        console.error('ERROR: No .cpp files found.');
        process.exit(1);
    }
    console.log(`\nScanning ${files.length} file(s) from: ${SCAN_TARGETS.join(', ')}`);
    console.log(`Mode: ${WRITE_MODE ? 'WRITE (落地改写)' : 'DRY-RUN (只打印)'}`);
    console.log('');

    // --write 安全闸
    if (WRITE_MODE) {
        const gitStatus = spawnSync('git', ['status', '--porcelain'], { cwd: PROJECT_ROOT, encoding: 'utf8' });
        if (gitStatus.stdout && gitStatus.stdout.trim()) {
            console.error('WARNING: Working tree has uncommitted changes. --write will modify source files.');
            console.error('Aborting. Commit/stash first, or run dry-run.');
            process.exit(1);
        }
    }

    // 性能优化：为目标文件生成精简 compile_commands.json（单文件 21s→1.5s）
    const t0 = Date.now();
    const dbDir = buildSlimCompileDb(files);
    console.log(`Compile db: ${path.relative(PROJECT_ROOT, dbDir)} (built in ${Date.now() - t0}ms)`);
    console.log('');

    let totalClean = 0, totalSuggest = 0, totalSkip = 0, totalFail = 0;
    let totalRemoved = 0, totalInserted = 0, totalSkippedAssoc = 0, totalSkippedDup = 0;
    const changedFiles = [];  // { file, removed, inserted, skippedAssoc, skippedDup }
    const crashedFiles = [];
    const failedFiles = [];

    for (const cppRel of files) {
        const { exitCode, output } = runIncludeCleaner(clangIc, extraArgs, cppRel, dbDir);

        if (exitCode === 139 || exitCode < 0) {
            console.log(`CRASH: ${cppRel} (exit ${exitCode})`);
            totalSkip++;
            crashedFiles.push(cppRel);
            continue;
        }
        if (exitCode !== 0) {
            console.log(`ERROR: ${cppRel} (exit ${exitCode})`);
            (output || '').split('\n').slice(0, 10).forEach(l => console.log('  ' + l));
            totalFail++;
            failedFiles.push(cppRel);
            continue;
        }

        // exit 0
        const deduped = dedupeChanges(output);
        if (!deduped.trim()) {
            totalClean++;
            continue;
        }

        // 解析建议
        const cppAbs = path.join(PROJECT_ROOT, cppRel);
        const dir = path.dirname(cppAbs);
        const { removals, insertions } = parseChanges(deduped, dir);

        if (removals.length === 0 && insertions.length === 0) {
            totalClean++;
            continue;
        }

        // 读取文件、应用改写
        const original = fs.readFileSync(cppAbs, 'utf8');
        const { newContent, removed, inserted, skippedAssociated, skippedDup } =
            applyChanges(original, removals, insertions, cppRel);

        totalSuggest++;
        totalRemoved += removed;
        totalInserted += inserted;
        totalSkippedAssoc += skippedAssociated;
        totalSkippedDup += skippedDup;

        if (newContent !== original) {
            changedFiles.push({ file: cppRel, removed, inserted, skippedAssociated, skippedDup });
            if (!SUMMARY_ONLY) {
                console.log(`\x1b[33mCHANGE: ${cppRel}\x1b[0m`);
                console.log(`  (-${removed} +${inserted} skipAssoc=${skippedAssociated} skipDup=${skippedDup})`);
                const diff = makeDiff(original, newContent, cppRel);
                if (diff) console.log(diff);
            }
            if (WRITE_MODE) {
                fs.writeFileSync(cppAbs, newContent);
            }
        } else {
            // 有建议但应用后无变化（全是被跳过的关联头/重复）
            if (!SUMMARY_ONLY) {
                console.log(`\x1b[32m  NO-OP: ${cppRel} (建议全被跳过: assoc=${skippedAssociated} dup=${skippedDup})\x1b[0m`);
            }
            // NO-OP 不计入 changedFiles，但仍算 suggest（有建议产生）
        }
    }

    // ---- 汇总 ----
    console.log('');
    console.log('==========================================');
    console.log(`  Total: ${files.length}  Clean: ${totalClean}  Suggest: ${totalSuggest}  Fail: ${totalFail}  Skip(Crash): ${totalSkip}`);
    console.log(`  Removed: ${totalRemoved}  Inserted: ${totalInserted}  SkippedAssoc: ${totalSkippedAssoc}  SkippedDup: ${totalSkippedDup}`);
    console.log(`  Changed files: ${changedFiles.length}`);
    console.log('==========================================');

    if (crashedFiles.length) {
        console.log('\nCrashed (segfault, skipped):');
        crashedFiles.forEach(f => console.log('  - ' + f));
    }
    if (failedFiles.length) {
        console.log('\nFailed (parse error):');
        failedFiles.forEach(f => console.log('  - ' + f));
    }
    if (WRITE_MODE && changedFiles.length) {
        console.log(`\n已落地改写 ${changedFiles.length} 个文件。建议：`);
        console.log('  1. git diff 复核');
        console.log('  2. clang-format -i 格式化改动的 .cpp');
        console.log('  3. ./scripts/configure.sh build 验证编译');
    }
}

main();
