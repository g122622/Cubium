#!/usr/bin/env node
// ============================================================
// check-keys.ts — 审计代码中使用的翻译键是否存在于原版语言文件。
//
// 背景：项目要求「数据驱动」，语言键必须能在原版 en_us.json 中找到；
// 自造键（如 commands.setblock.failed.noWorld）在玩家侧会原样显示成键名。
// 本脚本扫描 src/ 下的 C++ 源码，提取形如 "commands.xxx" 的字符串字面量，
// 与 en_us.json 对比，列出缺失键及其出现位置。
//
// Usage:
//   node --experimental-strip-types scripts/i18n/check-keys.ts
//   node --experimental-strip-types scripts/i18n/check-keys.ts --lang <en_us.json> --src <dir>
//   node --experimental-strip-types scripts/i18n/check-keys.ts --fail-on-missing
//
// Options:
//   --lang <path>        原版语言文件路径（默认探测 Windows/macOS/WSL 常见位置）
//   --src <dir>          要扫描的源码目录（默认 <repo>/src）
//   --fail-on-missing    存在缺失键时以退出码 1 结束（CI 用）
//   -h, --help           显示帮助
// ============================================================

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const PROJECT_ROOT = path.resolve(__dirname, '..', '..');

// 只识别「命令反馈 / UI / 系统消息」相关前缀。
// block./item./entity. 等前缀与声音事件 ID 共用命名空间（如 block.vault.eject_item），
// 且其显示名通常由注册表动态生成，不属于本脚本的审计目标。
const KEY_PREFIXES = [
    'commands', 'parsing', 'permissions', 'argument', 'arguments', 'connect', 'disconnect',
    'death', 'chat', 'multiplayer', 'advancements', 'stat', 'gui', 'gamerule', 'menu',
    'container', 'options', 'resourcepack', 'build', 'advMode', 'book', 'mount', 'sleep',
    'narration', 'selectWorld',
];

const KEY_PATTERN = new RegExp(`"((?:${KEY_PREFIXES.join('|')})\\.[A-Za-z0-9_./-]+)"`, 'g');

const argv = process.argv.slice(2);
let langPath = null;
let srcRoot = path.join(PROJECT_ROOT, 'src');
let failOnMissing = false;

for (let i = 0; i < argv.length; ++i) {
    const arg = argv[i];
    if (arg === '--lang') {
        langPath = argv[++i];
    } else if (arg === '--src') {
        srcRoot = argv[++i];
    } else if (arg === '--fail-on-missing') {
        failOnMissing = true;
    } else if (arg === '-h' || arg === '--help') {
        console.log(`Usage: check-keys.ts [options]

Options:
  --lang <path>       原版语言文件路径（默认自动探测）
  --src <dir>         要扫描的源码目录（默认 <repo>/src）
  --fail-on-missing   存在缺失键时以退出码 1 结束
  -h, --help          显示帮助`);
        process.exit(0);
    } else {
        console.error(`Unknown argument: ${arg}`);
        process.exit(2);
    }
}

function detectLanguageFile() {
    const home = os.homedir();
    const candidates = [
        'C:/Users/Administrator/minecraft_reborn/resourcepacks/Vanilla/assets/minecraft/lang/en_us.json',
        path.join(home, 'minecraft_reborn/resourcepacks/Vanilla/assets/minecraft/lang/en_us.json'),
        '/mnt/c/Users/Administrator/minecraft_reborn/resourcepacks/Vanilla/assets/minecraft/lang/en_us.json',
        path.join(PROJECT_ROOT, 'third_party/minecraft/lang/en_us.json'),
    ];
    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }
    return null;
}

function collectSourceFiles(dir, out) {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            // 第三方与客户端目录不参与：客户端已停止维护，且不产生服务端语言键。
            // sound/ 目录存放声音事件 ID（与翻译键共用命名空间），不是翻译键。
            if (entry.name === 'third_party' || entry.name === 'client' || entry.name === 'sound') {
                continue;
            }
            collectSourceFiles(fullPath, out);
        } else if (/\.(cpp|hpp|cc|h)$/.test(entry.name)) {
            out.push(fullPath);
        }
    }
}

/**
 * 收集 SoundEvents.hpp 中声明的声音事件 ID。
 *
 * 声音事件与翻译键共用 `block.*` / `entity.*` 命名空间（例如
 * `block.fire.extinguish`），但只有 `subtitles.*` 是语言文件里的键。
 * 直接按字符串前缀无法区分，故从声音事件声明表构建忽略集合。
 */
function collectSoundEventIds(srcDir) {
    const result = new Set();
    const soundEventsFile = path.join(srcDir, 'common/sound/SoundEvents.hpp');
    if (!fs.existsSync(soundEventsFile)) {
        return result;
    }
    const text = fs.readFileSync(soundEventsFile, 'utf8');
    for (const match of text.matchAll(/"([A-Za-z0-9_./-]+)"/g)) {
        result.add(match[1]);
    }
    return result;
}

/** 判断候选键是否属于拼接前缀/误报，而不是一个完整翻译键。 */
function isNoiseKey(key) {
    if (key.endsWith('.')) {
        return true; // 代码里是 "prefix." + name 的拼接，不是完整键
    }
    if (/^container\.(\d+|[A-Z])$/.test(key)) {
        return true; // 物品栏槽位索引（含注释里的 container.N 占位写法）
    }
    if (/\.(spv|json|png|ogg|nbt)$/.test(key)) {
        return true; // 文件名
    }
    if (key === 'key.xxx') {
        return true; // 占位示例
    }
    return false;
}

const languageFile = langPath ?? detectLanguageFile();
if (languageFile === null || !fs.existsSync(languageFile)) {
    console.error('Cannot locate vanilla en_us.json. Pass --lang <path>.');
    process.exit(2);
}

const translations = JSON.parse(fs.readFileSync(languageFile, 'utf8'));
const knownKeys = new Set(Object.keys(translations));

const sourceFiles = [];
collectSourceFiles(srcRoot, sourceFiles);
const soundEventIds = collectSoundEventIds(srcRoot);

/** @type {Map<string, string[]>} key -> 出现位置列表 */
const usedKeys = new Map();
for (const file of sourceFiles) {
    const text = fs.readFileSync(file, 'utf8');
    const relative = path.relative(PROJECT_ROOT, file).replaceAll('\\', '/');
    for (const match of text.matchAll(KEY_PATTERN)) {
        const key = match[1];
        if (isNoiseKey(key) || soundEventIds.has(key)) {
            continue;
        }
        const locations = usedKeys.get(key) ?? [];
        locations.push(relative);
        usedKeys.set(key, locations);
    }
}

const missing = [];
for (const [key, locations] of usedKeys) {
    if (!knownKeys.has(key)) {
        missing.push({ key, locations });
    }
}

missing.sort((a, b) => a.key.localeCompare(b.key));

console.log(`Language file: ${languageFile}`);
console.log(`Scanned: ${sourceFiles.length} source files, ${usedKeys.size} distinct keys`);
console.log(`Missing: ${missing.length}`);

if (missing.length > 0) {
    const byPrefix = new Map();
    for (const { key } of missing) {
        const prefix = key.split('.').slice(0, 2).join('.');
        byPrefix.set(prefix, (byPrefix.get(prefix) ?? 0) + 1);
    }
    console.log('\nBy prefix:');
    for (const [prefix, count] of [...byPrefix.entries()].sort((a, b) => b[1] - a[1])) {
        console.log(`  ${prefix}: ${count}`);
    }

    console.log('\nMissing keys:');
    for (const { key, locations } of missing) {
        const unique = [...new Set(locations)];
        console.log(`  ${key}  <- ${unique.slice(0, 3).join(', ')}${unique.length > 3 ? ', ...' : ''}`);
    }
}

process.exit(missing.length > 0 && failOnMissing ? 1 : 0);
