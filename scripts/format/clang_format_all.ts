#!/usr/bin/env node
// ============================================================
// clang_format_all.ts — 对 src/ 和 tests/ 下所有 .cpp/.hpp 文件
// 执行 clang-format -i（原地格式化）。
//
// 遵循 CLAUDE.md 规范：
//   - 只格式化 .cpp 和 .hpp 文件（其他文件严禁格式化）
//   - .gen.cpp/.gen.hpp 是自动生成的，跳过
//   - 使用项目根的 .clang-format 配置
//
// Usage:
//   node --experimental-strip-types scripts/format/clang_format_all.ts            # 格式化 src + tests
//   node --experimental-strip-types scripts/format/clang_format_all.ts --check   # 只检查不改写（CI 模式）
//   node --experimental-strip-types scripts/format/clang_format_all.ts src/common # 格式化指定目录
//
//   -h, --help     显示本帮助
//   --check        只检查格式是否正确，不落地改写（返回非 0 表示有文件需格式化）
//   scan_target    目录或文件（相对 PROJECT_ROOT），默认 src tests
// ============================================================

const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const PROJECT_ROOT = path.resolve(__dirname, '..', '..');

// ---- 命令行参数解析 ----
const argv = process.argv.slice(2);
let CHECK_MODE = false;
let SCAN_TARGETS = [];
for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--check') CHECK_MODE = true;
    else if (a === '-h' || a === '--help') {
        console.log(`Usage: clang_format_all.ts [options] [scan_target...]

  scan_target  目录或文件（相对 PROJECT_ROOT），默认 src tests
               目录递归展开为 .cpp + .hpp（git ls-files，排除 .gen.*）

Options:
  --check       只检查格式是否正确，不落地改写（CI 模式）
  -h, --help    显示本帮助
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

// ---- 定位 clang-format ----
function findClangFormat() {
    // 1. 环境变量
    if (process.env.CLANG_FORMAT) return process.env.CLANG_FORMAT;
    // 2. macOS homebrew llvm 路径
    const macPaths = [
        '/opt/homebrew/opt/llvm/bin/clang-format',
        '/opt/homebrew/opt/llvm@22/bin/clang-format',
        '/opt/homebrew/opt/llvm@21/bin/clang-format',
        '/opt/homebrew/opt/llvm@20/bin/clang-format',
        '/opt/homebrew/bin/clang-format',
        '/usr/local/opt/llvm/bin/clang-format',
        '/usr/local/bin/clang-format',
    ];
    for (const p of macPaths) {
        if (fs.existsSync(p)) return p;
    }
    // 3. Windows VS 自带路径
    const winPath = 'D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-format.exe';
    if (process.platform === 'win32' && fs.existsSync(winPath)) return winPath;
    // 4. PATH 查找
    const r = spawnSync('clang-format', ['--version'], { encoding: 'utf8' });
    if (r.status === 0) return 'clang-format';
    console.error('ERROR: clang-format not found. Set CLANG_FORMAT env variable.');
    process.exit(1);
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
            if (f.endsWith('.gen.cpp') || f.endsWith('.gen.hpp')) continue;  // 自动生成，跳过
            files.push(f);
        }
    }
    return [...new Set(files)].sort();
}

// ---- 主流程 ----
function main() {
    const clangFormat = findClangFormat();
    const version = spawnSync(clangFormat, ['--version'], { encoding: 'utf8' });
    console.log(`clang-format: ${clangFormat}`);
    console.log((version.stdout || '').trim());
    console.log(`Mode: ${CHECK_MODE ? 'CHECK (只检查)' : 'FORMAT (原地格式化)'}`);
    console.log('');

    const files = enumerateFiles(SCAN_TARGETS);
    if (files.length === 0) {
        console.error('ERROR: No .cpp/.hpp files found.');
        process.exit(1);
    }
    console.log(`Found ${files.length} file(s) to ${CHECK_MODE ? 'check' : 'format'}`);

    let totalChanged = 0;
    let totalProcessed = 0;
    let totalErrors = 0;
    const changedFiles = [];

    const BATCH_SIZE = 100;  // 分批避免命令行过长（Windows ~32k 限制）

    for (let i = 0; i < files.length; i += BATCH_SIZE) {
        const batch = files.slice(i, i + BATCH_SIZE);

        if (CHECK_MODE) {
            // --check 模式：逐文件用 --output-replacements-xml 判断
            // 输出含 <replacement> 标签 = 需要格式化
            for (const rel of batch) {
                const abs = path.join(PROJECT_ROOT, rel);
                const r = spawnSync(clangFormat, ['--output-replacements-xml', '--style=file', abs], {
                    encoding: 'utf8',
                    cwd: PROJECT_ROOT,
                });
                if ((r.stdout || '').includes('<replacement ')) {
                    changedFiles.push(rel);
                    totalChanged++;
                }
                totalProcessed++;
                if (totalProcessed % 50 === 0 || totalProcessed === files.length) {
                    process.stderr.write(`\r\x1b[2K[ Progress ] ${totalProcessed}/${files.length}`);
                }
            }
        } else {
            // 格式化模式：batch 跑 -i（原地改写）
            const absBatch = batch.map(f => path.join(PROJECT_ROOT, f));
            const r = spawnSync(clangFormat, ['-i', '--style=file', ...absBatch], {
                encoding: 'utf8',
                cwd: PROJECT_ROOT,
                maxBuffer: 50 * 1024 * 1024,
            });
            if (r.status !== 0) {
                totalErrors++;
                console.error(`\nBatch error: ${(r.stderr || '').slice(0, 200)}`);
            }
            totalProcessed += batch.length;
            process.stderr.write(`\r\x1b[2K[ Progress ] ${totalProcessed}/${files.length}`);
        }
    }

    process.stderr.write('\r\x1b[2K');
    console.log('');
    console.log('==========================================');
    console.log(`  Total: ${files.length}  Processed: ${totalProcessed}  Changed: ${totalChanged}  Errors: ${totalErrors}`);
    console.log('==========================================');

    if (CHECK_MODE && changedFiles.length > 0) {
        console.log('\nFiles needing format:');
        changedFiles.forEach(f => console.log(`  - ${f}`));
        process.exit(1);  // CI 模式：有文件需格式化则返回非 0
    }

    if (!CHECK_MODE) {
        console.log('\n格式化完成。建议：');
        console.log('  1. git diff 复核');
        console.log('  2. cmake --build 验证编译');
    }
}

main();
