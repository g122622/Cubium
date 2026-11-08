#!/usr/bin/env node
// ============================================================
// check_encoding.ts — 校验仓库内所有文本文件的编码是否统一为 UTF-8。
//
// 检查项：
//   1. 能否按 UTF-8 严格解码（失败时提示是否可用 GB18030 解码，便于定位存量 GBK 文件）；
//   2. 是否含 U+FFFD 替换字符（有损转码或乱码修复不彻底留下的痕迹）；
//   3. 是否带 UTF-8 BOM（*.ps1 例外：Windows PowerShell 5.1 需要 BOM 才能正确解码中文）。
//
// 与 .editorconfig / .gitattributes 配合使用：前者约束编辑器写入编码与换行，
// 后者约束 git 检出换行；本脚本用于在提交前发现问题。
//
// Usage:
//   node --experimental-strip-types scripts/format/check_encoding.ts
//   node --experimental-strip-types scripts/format/check_encoding.ts --fix-bom
//
//   -h, --help   显示本帮助
//   --fix-bom    自动移除非 *.ps1 文件的 UTF-8 BOM
// ============================================================

const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const PROJECT_ROOT = path.resolve(__dirname, '..', '..');

// 子模块与第三方内容不归本仓库管理；二进制文件不参与编码检查。
const SKIP_PREFIXES = ['third_party/', 'vcpkg_installed/', 'docs/minecraft-wiki-source/'];
const BINARY_EXTENSIONS = new Set([
    '.png', '.jpg', '.jpeg', '.gif', '.ico', '.bmp', '.tif', '.tiff', '.webp',
    '.pdf', '.pyc', '.pyd', '.so', '.dylib', '.dll', '.exe', '.lib', '.a', '.o', '.obj',
    '.jar', '.zip', '.gz', '.bz2', '.xz', '.7z', '.tar',
    '.ttf', '.otf', '.woff', '.woff2', '.ogg', '.mp3', '.wav',
    '.mca', '.mcr', '.mcstructure', '.ldb', '.dat', '.dat_old', '.db', '.sqlite', '.bin',
]);

const argv = process.argv.slice(2);
let FIX_BOM = false;
for (const a of argv) {
    if (a === '--fix-bom') {
        FIX_BOM = true;
    } else if (a === '-h' || a === '--help') {
        console.log(`Usage: check_encoding.ts [options]

Options:
  --fix-bom   自动移除非 *.ps1 文件的 UTF-8 BOM
  -h, --help  显示帮助`);
        process.exit(0);
    } else {
        console.error(`Unknown argument: ${a}`);
        process.exit(2);
    }
}

function gitFiles() {
    const res = spawnSync('git', ['ls-files', '-z'], { cwd: PROJECT_ROOT, encoding: 'utf8' });
    if (res.status !== 0) {
        console.error('Failed to run git ls-files.');
        process.exit(2);
    }
    return res.stdout.split('\0').filter(Boolean);
}

function isBinary(filePath) {
    const ext = path.extname(filePath).toLowerCase();
    if (BINARY_EXTENSIONS.has(ext)) {
        return true;
    }
    // 内容里出现 NUL 也按二进制处理（防止漏配扩展名）。
    const fd = fs.openSync(filePath, 'r');
    try {
        const head = Buffer.alloc(4096);
        const n = fs.readSync(fd, head, 0, head.length, 0);
        return head.subarray(0, n).includes(0);
    } finally {
        fs.closeSync(fd);
    }
}

const errors = [];
const fixed = [];
let checked = 0;

function isSkipped(relativePath) {
    return SKIP_PREFIXES.some((prefix) => {
        const base = prefix.replace(/\/$/, '');
        return relativePath === base || relativePath.startsWith(`${base}/`);
    });
}

for (const relativePath of gitFiles()) {
    if (isSkipped(relativePath)) {
        continue;
    }

    const absolutePath = path.join(PROJECT_ROOT, relativePath);
    if (!fs.existsSync(absolutePath) || isBinary(absolutePath)) {
        continue;
    }
    checked += 1;

    let bytes = fs.readFileSync(absolutePath);
    const hasBom = bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf;

    // *.ps1 依赖 BOM 让 Windows PowerShell 5.1 按 UTF-8 解码。
    if (hasBom && !relativePath.endsWith('.ps1')) {
        if (FIX_BOM) {
            fs.writeFileSync(absolutePath, bytes.subarray(3));
            bytes = bytes.subarray(3);
            fixed.push(relativePath);
        } else {
            errors.push(`${relativePath}: unexpected UTF-8 BOM (only *.ps1 may keep it)`);
        }
    }

    let text;
    try {
        // Node 的 toString('utf8') 会静默替换非法字节，这里必须用严格解码。
        text = new TextDecoder('utf-8', { fatal: true }).decode(bytes);
    } catch {
        let hint = '';
        try {
            new TextDecoder('gb18030', { fatal: true }).decode(bytes);
            hint = ' — file is valid GB18030/GBK, convert it to UTF-8';
        } catch {
            hint = '';
        }
        errors.push(`${relativePath}: not valid UTF-8${hint}`);
        continue;
    }

    if (text.includes('\uFFFD')) {
        errors.push(`${relativePath}: contains U+FFFD replacement characters (lossy transcoding or mojibake)`);
    }
}

for (const filePath of fixed) {
    console.log(`Removed BOM: ${filePath}`);
}
for (const message of errors) {
    console.error(message);
}

if (errors.length > 0) {
    console.error(`\nEncoding check failed: ${errors.length} problem(s) in ${checked} text file(s).`);
    process.exit(1);
}

console.log(`Encoding check passed: ${checked} text file(s) are valid UTF-8.`);