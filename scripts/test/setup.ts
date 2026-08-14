/*
 * setup.ts — 跨服务端 GameTest 对比工具的环境一次性准备脚本。
 *
 * 职责（仅运行一次或环境变更后重跑，幂等）：
 *   1. 构建 tests/integrated 行为包产物（node build.mjs → 各 pack scripts/*.js）。
 *   2. 对 4 个 pack 建 Windows junction，链 development_behavior_packs/<pack> → tests/integrated/<pack>。
 *      junction 让基岩服务端直接读仓库内产物，改用例后重跑 build.mjs 即生效，无需 cp。
 *   3. 改基岩 server.properties：level-name=gametest-diff（专用世界，基岩首次启动自动生成空世界）、
 *      allow-cheats=true（/gametest 要求）、allow-list=false（避免空 allowlist 阻塞）、
 *      content-log-file-enabled=true（开文件日志供离线解析）。
 *
 * 不启动基岩（setup 与 run 分离）。完成后输出运行指导。
 *
 * 运行方式：node scripts/test/setup.ts（node >= 22 原生 type stripping，无需 tsc/tsx）。
 */

import { spawnSync } from "node:child_process";
import fs from "node:fs/promises";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const cubiumRoot = path.resolve(__dirname, "..", "..");

// ============================================================================
// 配置常量
// ============================================================================

const BEDROCK_DIR = "D:/Minecraft/bedrock-server-1.26.43.1";
const BEDROCK_DEV_PACKS = path.join(BEDROCK_DIR, "development_behavior_packs");
const BEDROCK_PROPERTIES = path.join(BEDROCK_DIR, "server.properties");

const PACKS_SRC = path.join(cubiumRoot, "tests", "integrated");
const BUILD_MJS = path.join(PACKS_SRC, "build.mjs");

// 4 个对比行为包（与 tests/integrated 子目录一致）。
const PACKS = ["starter", "mob_behavior", "command", "challenge"];

// server.properties 需覆写的键值对（key → 目标值）。
const PROPERTIES_OVERRIDES: Record<string, string> = {
    "level-name": "gametest-diff",
    "allow-cheats": "true",
    "allow-list": "false",
    "content-log-file-enabled": "true",
    "content-log-console-output-enabled": "true",
    "content-log-level": "info",
};

// ============================================================================
// 工具函数
// ============================================================================

function log(msg: string): void {
    console.log(`[setup] ${msg}`);
}

function warn(msg: string): void {
    console.warn(`[setup] WARN: ${msg}`);
}

/** 以 UTF-8 读文本文件。 */
async function readText(p: string): Promise<string> {
    return fs.readFile(p, "utf-8");
}

/** 原子写文本文件（先写临时文件再 rename，避免半写状态）。 */
async function writeText(p: string, content: string): Promise<void> {
    await fs.mkdir(path.dirname(p), { recursive: true });
    const tmp = `${p}.tmp.${process.pid}`;
    await fs.writeFile(tmp, content, "utf-8");
    await fs.rename(tmp, p);
}

/**
 * 判断路径是否为 junction（Windows 目录重解析点）。
 * fs.lstat().isSymbolicLink() 对 junction 返回 true（junction 是 reparse point 的一种）。
 */
async function isJunction(p: string): Promise<boolean> {
    try {
        const st = await fs.lstat(p);
        return st.isSymbolicLink();
    } catch {
        return false;
    }
}

// ============================================================================
// 步骤 1：构建 tests/integrated 产物
// ============================================================================

async function buildPacks(): Promise<void> {
    log(`Building behavior packs: node ${path.relative(cubiumRoot, BUILD_MJS)}`);
    const result = spawnSync("node", [BUILD_MJS], {
        cwd: PACKS_SRC,
        stdio: "inherit",
        shell: false,
    });
    if (result.status !== 0) {
        throw new Error(`build.mjs failed with exit code ${result.status}`);
    }
    // 校验产物存在（每个 pack 应有 scripts/ 目录）。
    for (const pack of PACKS) {
        const scriptsDir = path.join(PACKS_SRC, pack, "scripts");
        if (!existsSync(scriptsDir)) {
            warn(`Pack '${pack}' scripts/ not found after build: ${scriptsDir}`);
        }
    }
    log("Behavior packs built.");
}

// ============================================================================
// 步骤 2：建 junction 链接
// ============================================================================

/**
 * 对单个 pack 建 junction：development_behavior_packs/<pack> → tests/integrated/<pack>。
 * 已存在则先 rmdir（junction 的 rmdir 安全，只删链接不删源）再重建。
 * mklink /J 不需管理员权限（与符号链接 /D 不同）。
 */
async function linkPack(pack: string): Promise<void> {
    const target = path.join(PACKS_SRC, pack); // junction 指向的源（仓库内 pack 目录）
    const link = path.join(BEDROCK_DEV_PACKS, pack); // 基岩侧的链接路径

    if (!existsSync(target)) {
        throw new Error(`Pack source not found: ${target}`);
    }

    // 清理已存在的链接或目录。
    if (existsSync(link)) {
        const isLink = await isJunction(link);
        if (!isLink) {
            warn(`'${link}' exists but is NOT a junction — skipping (refuse to delete real directory).`);
            return;
        }
        // junction 用 rmdir 删除安全（只删重解析点，不递归删源）。
        await fs.rmdir(link);
        log(`Removed existing junction: ${pack}`);
    }

    // mklink /J 必须经 cmd /c 调用（mklink 是 cmd 内部命令，非独立 exe）。
    // shell:true 让 cmd 解析引号（路径可能含空格），windowsVerbatimArguments 避免引号被 Node 二次处理。
    const linkWin = link.replace(/\//g, "\\");
    const targetWin = target.replace(/\//g, "\\");
    const result = spawnSync(`mklink /J "${linkWin}" "${targetWin}"`, {
        stdio: "pipe",
        shell: "cmd.exe",
    });
    const out = (result.stdout?.toString() ?? "") + (result.stderr?.toString() ?? "");
    if (result.status !== 0) {
        throw new Error(`mklink /J failed for '${pack}' (exit ${result.status}): ${out.trim()}`);
    }
    log(`Linked: ${pack} → ${path.relative(cubiumRoot, target)}`);
}

async function linkAllPacks(): Promise<void> {
    log(`Ensuring dev packs dir: ${BEDROCK_DEV_PACKS}`);
    await fs.mkdir(BEDROCK_DEV_PACKS, { recursive: true });
    for (const pack of PACKS) {
        await linkPack(pack);
    }
    log(`All ${PACKS.length} junctions ready.`);
}

// ============================================================================
// 步骤 3：改 server.properties
// ============================================================================

/**
 * 改写 server.properties：保留注释与未覆写项，仅替换 PROPERTIES_OVERRIDES 中的键。
 * 基岩 properties 格式：key=value，# 开头为注释。逐行处理，匹配 key 前缀则替换值，
 * 不存在则追加。幂等：重复跑只替换值不产生重复行。
 */
async function patchProperties(): Promise<void> {
    if (!existsSync(BEDROCK_PROPERTIES)) {
        throw new Error(`server.properties not found: ${BEDROCK_PROPERTIES}`);
    }
    const original = await readText(BEDROCK_PROPERTIES);
    const lines = original.split(/\r?\n/);
    const seen = new Set<string>();
    const out: string[] = [];

    for (const line of lines) {
        const trimmed = line.trim();
        if (trimmed === "" || trimmed.startsWith("#")) {
            out.push(line);
            continue;
        }
        const eq = trimmed.indexOf("=");
        if (eq < 0) {
            out.push(line);
            continue;
        }
        const key = trimmed.slice(0, eq).trim();
        if (key in PROPERTIES_OVERRIDES) {
            out.push(`${key}=${PROPERTIES_OVERRIDES[key]}`);
            seen.add(key);
        } else {
            out.push(line);
        }
    }

    // 追加未在文件中出现的覆写项。
    for (const [key, value] of Object.entries(PROPERTIES_OVERRIDES)) {
        if (!seen.has(key)) {
            out.push(`${key}=${value}`);
        }
    }

    const updated = out.join("\r\n");
    if (updated !== original) {
        await writeText(BEDROCK_PROPERTIES, updated);
        log(`Patched server.properties: ${[...Object.keys(PROPERTIES_OVERRIDES)].join(", ")}`);
    } else {
        log("server.properties already up to date.");
    }
}

// ============================================================================
// 主流程
// ============================================================================

async function main(): Promise<void> {
    log("=== GameTest cross-server diff: environment setup ===");
    log(`Bedrock dir: ${BEDROCK_DIR}`);
    log(`Cubium root: ${cubiumRoot}`);

    await buildPacks();
    await linkAllPacks();
    await patchProperties();

    console.log("");
    log("=== Setup complete ===");
    console.log("Next steps:");
    console.log("  跑对比: node scripts/test/run_diff.ts");
    console.log("    run_diff.ts 首次跑会自动生成 gametest-diff 空世界 + 注入 Beta APIs 实验开关到 level.dat。");
    console.log("");
    console.log("  单步调试: node scripts/test/run_diff.ts --step cubium|bedrock");
}

main().catch((err: unknown) => {
    console.error(`[setup] FAILED: ${err instanceof Error ? err.message : String(err)}`);
    process.exit(1);
});
