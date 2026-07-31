/*
 * bake_java_block_state_table.ts
 *
 * 离线烘焙脚本:从 assets/data/blocks_1.21.11.json 生成紧凑的 C++ 静态查找表
 * (java_block_state_table.gen.cpp / .gen.hpp),编译进 mc_common,供运行时
 * JavaBlockStateIdMap 做 (blockName, properties) → Java globalId 查表。
 *
 * 设计目标:
 *   - 运行时零 JSON 解析(消除 6.5MB JSON 的 ~30MB nlohmann::json DOM 峰值)。
 *   - 最终常驻结构为 vector<u32>(stateId 连续稠密),仅 ~230KB。
 *   - 生成表为排序的 (key → globalId) 数组 + 扁平字符串池,C++ 端二分查找。
 *
 * key 格式(与运行时 JavaBlockStateIdMap::buildLookupKey 完全一致):
 *   "minecraft:acacia_button|face=floor,facing=north,powered=true"
 *   properties 按 key 字母序排列(C++ 侧用 std::map,天然字典序;此处显式排序对齐)。
 *
 * 运行方式: node scripts/baking/bake_java_block_state_table.ts
 *   (node >= 22 原生支持 .TS type stripping,无需 tsc/tsx 依赖)
 *
 * 产物不提交 git(.gitignore),每次构建由 CMake add_custom_command 重生成。
 *
 * 归属:本表与 JavaItemIdMap 同属 network/backend/java 协议对齐层,产物置于
 * src/common/network/backend/java/generated/。命名空间 mc::network::backend::java::generated。
 */

import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, "..", "..");

const inputJson = path.resolve(rootDir, "assets", "data", "blocks_1.21.11.json");
const outDir = path.resolve(rootDir, "src", "common", "network", "backend", "java", "generated");
const outCpp = path.resolve(outDir, "java_block_state_table.gen.cpp");
const outHpp = path.resolve(outDir, "java_block_state_table.gen.hpp");

interface StateNode {
    id?: number;
    default?: boolean;
    properties?: Record<string, string | number | boolean>;
}

interface BlockNode {
    states?: StateNode[];
}

/**
 * 构造与 C++ 侧 buildLookupKey 完全一致的查表键。
 * properties 按 key 字典序排列(C++ std::map 语义)。
 */
function buildLookupKey(blockName: string, props: Record<string, string>): string {
    let key = blockName;
    key += "|";
    const sortedKeys = Object.keys(props).sort();
    for (let i = 0; i < sortedKeys.length; i++) {
        if (i > 0) {
            key += ",";
        }
        const k = sortedKeys[i];
        key += k;
        key += "=";
        key += props[k];
    }
    return key;
}

/** 把 JS 值统一转成与 C++ IProperty::valueToString 一致的字符串。 */
function propValueToString(v: string | number | boolean): string {
    return String(v);
}

interface Entry {
    key: string;
    globalId: number;
}

async function main(): Promise<void> {
    const raw = await fs.readFile(inputJson, "utf8");
    const root = JSON.parse(raw) as Record<string, BlockNode>;

    const entries: Entry[] = [];
    let stateCount = 0;

    for (const [blockName, blockNode] of Object.entries(root)) {
        const states = blockNode.states;
        if (!Array.isArray(states)) {
            continue;
        }
        for (const state of states) {
            if (state.id === undefined || typeof state.id !== "number") {
                continue;
            }
            const props: Record<string, string> = {};
            if (state.properties && typeof state.properties === "object") {
                for (const [pk, pv] of Object.entries(state.properties)) {
                    props[pk] = propValueToString(pv);
                }
            }
            const key = buildLookupKey(blockName, props);
            entries.push({ key, globalId: state.id });
            ++stateCount;
        }
    }

    // 按 key 字典序排序,C++ 端用二分查找。
    entries.sort((a, b) => (a.key < b.key ? -1 : a.key > b.key ? 1 : 0));

    // 构建扁平字符串池:所有 key 连续存放,\0 分隔,便于 C++ strcmp 二分。
    // 同时记录每条 entry 的 keyOffset。
    let pool = "";
    const offsets: number[] = new Array(entries.length);
    for (let i = 0; i < entries.length; i++) {
        offsets[i] = pool.length;
        pool += entries[i].key;
        pool += "\0";
    }

    await fs.mkdir(outDir, { recursive: true });

    // ------------------------------------------------------------------
    // 生成 .gen.hpp(声明)
    // ------------------------------------------------------------------
    const hpp = `/*
 * 自动生成,勿手动编辑。
 * 由 scripts/baking/bake_java_block_state_table.ts 从 assets/data/blocks_1.21.11.json 生成。
 * 构建时由 CMake add_custom_command 重生成,产物不入 git(.gitignore)。
 */

#pragma once

#include "common/core/Types.hpp"

#include <cstddef>
#include <cstdint>

namespace mc::network::backend::java::generated {

/// 单条 (blockName|properties) → Java globalId 表项。数组按 key 字典序排序。
struct BlockStateIdEntry {
    u32 globalId;
    u32 keyOffset; ///< 在 kBlockStateKeyPool 中的字节偏移(key 以 \\0 结尾)。
};

/// 排序后的表项数组(二分查找用)。
extern const BlockStateIdEntry* blockStateIdEntries();
/// 表项数量。
extern size_t blockStateIdEntriesCount();
/// 扁平 key 字符串池(所有 key 连续存放,各以 \\0 结尾)。
extern const char* blockStateKeyPool();

} // namespace mc::network::backend::java::generated
`;

    // ------------------------------------------------------------------
    // 生成 .gen.cpp(定义)
    // ------------------------------------------------------------------
    // 字符串池转义为 C 字符串字面量。\0 用 \\0 表示,其他特殊字符转义。
    const poolLiteral = cStringLiteral(pool);
    const entryLines = entries.map((e, i) => `    { ${e.globalId}u, ${offsets[i]}u }`);

    const cpp = `/*
 * 自动生成,勿手动编辑。
 * 由 scripts/baking/bake_java_block_state_table.ts 从 assets/data/blocks_1.21.11.json 生成。
 * 构建时由 CMake add_custom_command 重生成,产物不入 git(.gitignore)。
 *
 * 本文件含 1.21.11 全部 ${stateCount} 个 block state 的 (name|properties) → globalId 查找表,
 * 按 key 字典序排序,供 JavaBlockStateIdMap 运行时二分查找。编译后进只读数据段,
 * 运行时零 JSON 解析、零堆分配。
 */

#include "common/network/backend/java/generated/java_block_state_table.gen.hpp"

namespace mc::network::backend::java::generated {

// 扁平 key 字符串池:所有 key 连续存放,各以 '\\0' 结尾。
// 总长 ${pool.length} 字节(含分隔 \\0)。
static const char kBlockStateKeyPool[] =
${poolLiteral};

// 排序表项数组(按 key 字典序)。共 ${entries.length} 条。
static const BlockStateIdEntry kBlockStateIdEntries[] = {
${entryLines.join(",\n")}
};

const BlockStateIdEntry* blockStateIdEntries()
{
    return kBlockStateIdEntries;
}

size_t blockStateIdEntriesCount()
{
    return sizeof(kBlockStateIdEntries) / sizeof(kBlockStateIdEntries[0]);
}

const char* blockStateKeyPool()
{
    return kBlockStateKeyPool;
}

} // namespace mc::network::backend::java::generated
`;

    await fs.writeFile(outHpp, hpp, "utf8");
    await fs.writeFile(outCpp, cpp, "utf8");

    process.stdout.write(
        `[bake_java_block_state_table] parsed ${stateCount} states, wrote ${entries.length} entries ` +
            `(pool ${pool.length} bytes) ->\n  ${path.relative(rootDir, outCpp)}\n  ${path.relative(
                rootDir,
                outHpp,
            )}\n`,
    );
}

/**
 * 把任意字符串(含 \\0 与不可见字符)转成合法的 C 字符串字面量拼接。
 * 采用相邻字符串字面量自动拼接,每行一段,可读且避免单行过长。
 */
function cStringLiteral(s: string): string {
    const chunkSize = 64; // 每行字符数,控制行宽
    const lines: string[] = [];
    for (let i = 0; i < s.length; i += chunkSize) {
        let line = '"';
        for (let j = i; j < Math.min(i + chunkSize, s.length); j++) {
            line += cEscape(s.charCodeAt(j));
        }
        line += '"';
        lines.push(line);
    }
    return lines.join("\n");
}

/** 单字符 C 转义。 */
function cEscape(c: number): string {
    switch (c) {
        case 0x00:
            return "\\0";
        case 0x07:
            return "\\a";
        case 0x08:
            return "\\b";
        case 0x09:
            return "\\t";
        case 0x0a:
            return "\\n";
        case 0x0b:
            return "\\v";
        case 0x0c:
            return "\\f";
        case 0x0d:
            return "\\r";
        case 0x22: // "
            return '\\"';
        case 0x5c: // \\
            return "\\\\";
        default:
            if (c < 0x20 || c === 0x7f) {
                return "\\x" + c.toString(16).padStart(2, "0");
            }
            return String.fromCharCode(c);
    }
}

main().catch((err) => {
    process.stderr.write(`[bake_java_block_state_table] FAILED: ${err}\n`);
    process.exit(1);
});
