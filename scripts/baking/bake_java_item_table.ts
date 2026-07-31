/*
 * bake_java_item_table.ts
 *
 * 离线烘焙脚本:从 assets/data/items_1.21.11.json 生成紧凑的 C++ 静态查找表
 * (java_item_table.gen.cpp / .gen.hpp),编译进 mc_common,供运行时
 * JavaItemIdMap 做 itemName → Java registry id 双向查表。
 *
 * 数据源 items_1.21.11.json 来自 PrismarineJS minecraft-data,其 id 字段即
 * vanilla 1.21.11 BuiltInRegistries.ITEM 注册序(air=0/stone=1/…/ominous_bottle=1504),
 * 与真 Java 客户端 wire 上 ItemStack 的 itemId 一致。已验证 id==index 全成立。
 *
 * 设计目标(与 bake_java_id_tables.ts 同范式):
 *   - 运行时零 JSON 解析。
 *   - 最终常驻结构为排序数组 + 扁平字符串池,C++ 端二分查找。
 *   - 反向(vanilla id → name)用稠密数组(id 连续 0..maxId,直接下标)。
 *
 * 产物不提交 git(.gitignore),每次构建由 CMake add_custom_command 重生成。
 *
 * 运行方式: node scripts/baking/bake_java_item_table.ts
 *   (node >= 22 原生支持 .ts type stripping,无需 tsc/tsx 依赖)
 */

import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, "..", "..");

const inputJson = path.resolve(rootDir, "assets", "data", "items_1.21.11.json");
const outDir = path.resolve(rootDir, "src", "common", "network", "backend", "java", "generated");
const outCpp = path.resolve(outDir, "java_item_table.gen.cpp");
const outHpp = path.resolve(outDir, "java_item_table.gen.hpp");

interface ItemNode {
    id: number;
    name: string;
    displayName?: string;
    stackSize?: number;
}

interface Entry {
    name: string; // 形如 "minecraft:stone"
    vanillaId: number;
}

async function main(): Promise<void> {
    const raw = await fs.readFile(inputJson, "utf8");
    const items = JSON.parse(raw) as ItemNode[];

    const entries: Entry[] = [];
    for (const it of items) {
        if (typeof it.id !== "number" || typeof it.name !== "string") {
            continue;
        }
        // 统一带 minecraft: 前缀(PrismarineJS name 为裸 path 如 "stone",
        // 与项目 Item::itemLocation().toString() 的 "minecraft:stone" 对齐)。
        const fullName = it.name.startsWith("minecraft:") ? it.name : `minecraft:${it.name}`;
        entries.push({ name: fullName, vanillaId: it.id });
    }

    // 按 name 字典序排序,C++ 端用二分查找。
    entries.sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));

    // 构建扁平字符串池:所有 name 连续存放,\0 分隔,便于 C++ strcmp 二分。
    let pool = "";
    const offsets: number[] = new Array(entries.length);
    for (let i = 0; i < entries.length; i++) {
        offsets[i] = pool.length;
        pool += entries[i].name;
        pool += "\0";
    }

    // 反向表:vanilla id → nameOffset。id 连续稠密(0..maxId),直接作下标。
    let maxId = -1;
    for (const e of entries) {
        if (e.vanillaId > maxId) {
            maxId = e.vanillaId;
        }
    }
    const reverseTable = new Array<number>(maxId + 1).fill(0);
    for (let i = 0; i < entries.length; i++) {
        reverseTable[entries[i].vanillaId] = offsets[i];
    }

    await fs.mkdir(outDir, { recursive: true });

    // ------------------------------------------------------------------
    // 生成 .gen.hpp(声明)
    // ------------------------------------------------------------------
    const hpp = `/*
 * 自动生成,勿手动编辑。
 * 由 scripts/baking/bake_java_item_table.ts 从 assets/data/items_1.21.11.json 生成。
 * 构建时由 CMake add_custom_command 重生成,产物不入 git(.gitignore)。
 */

#pragma once

#include "common/core/Types.hpp"

#include <cstddef>
#include <cstdint>

namespace mc::network::backend::java::generated {

/// 单条 itemName → Java registry id 表项。数组按 name 字典序排序。
struct ItemIdEntry {
    u32 vanillaId;
    u32 nameOffset; ///< 在 kItemNamePool 中的字节偏移(name 以 \\0 结尾)。
};

/// 排序后的表项数组(二分查找用)。
extern const ItemIdEntry* itemIdEntries();
/// 表项数量。
extern size_t itemIdEntriesCount();
/// 扁平 name 字符串池(所有 name 连续存放,各以 \\0 结尾)。
extern const char* itemNamePool();

/// 反向表:vanilla id → nameOffset。id 连续稠密(0..maxId),下标 = vanilla id。
extern const u32* itemNameOffsetByVanillaId();
/// 反向表条目数(= maxId + 1)。
extern size_t itemNameOffsetByVanillaIdCount();

} // namespace mc::network::backend::java::generated
`;

    // ------------------------------------------------------------------
    // 生成 .gen.cpp(定义)
    // ------------------------------------------------------------------
    const poolLiteral = cStringLiteral(pool);
    const entryLines = entries.map((e, i) => `    { ${e.vanillaId}u, ${offsets[i]}u }`);
    const reverseLines = reverseTable.map((off) => `    ${off}u`);

    const cpp = `/*
 * 自动生成,勿手动编辑。
 * 由 scripts/baking/bake_java_item_table.ts 从 assets/data/items_1.21.11.json 生成。
 * 构建时由 CMake add_custom_command 重生成,产物不入 git(.gitignore)。
 *
 * 本文件含 1.21.11 全部 ${entries.length} 个 item 的 name → Java registry id 查找表,
 * 按 name 字典序排序,供 JavaItemIdMap 运行时二分查找。编译后进只读数据段,
 * 运行时零 JSON 解析、零堆分配。反向表(vanilla id → nameOffset)稠密下标访问。
 */

#include "common/network/backend/java/generated/java_item_table.gen.hpp"

namespace mc::network::backend::java::generated {

// 扁平 name 字符串池:所有 name 连续存放,各以 '\\0' 结尾。
// 总长 ${pool.length} 字节(含分隔 \\0)。
static const char kItemNamePool[] =
${poolLiteral};

// 排序表项数组(按 name 字典序)。共 ${entries.length} 条。
static const ItemIdEntry kItemIdEntries[] = {
${entryLines.join(",\n")}
};

// 反向表:vanilla id(0..${maxId}) → nameOffset。下标 = vanilla id。
static const u32 kItemNameOffsetByVanillaId[] = {
${reverseLines.join(",\n")}
};

const ItemIdEntry* itemIdEntries()
{
    return kItemIdEntries;
}

size_t itemIdEntriesCount()
{
    return sizeof(kItemIdEntries) / sizeof(kItemIdEntries[0]);
}

const char* itemNamePool()
{
    return kItemNamePool;
}

const u32* itemNameOffsetByVanillaId()
{
    return kItemNameOffsetByVanillaId;
}

size_t itemNameOffsetByVanillaIdCount()
{
    return sizeof(kItemNameOffsetByVanillaId) / sizeof(kItemNameOffsetByVanillaId[0]);
}

} // namespace mc::network::backend::java::generated
`;

    await fs.writeFile(outHpp, hpp, "utf8");
    await fs.writeFile(outCpp, cpp, "utf8");

    process.stdout.write(
        `[bake_java_item_table] parsed ${entries.length} items (maxId=${maxId}), ` +
            `pool ${pool.length} bytes ->\n  ${path.relative(rootDir, outCpp)}\n  ${path.relative(
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
    process.stderr.write(`[bake_java_item_table] FAILED: ${err}\n`);
    process.exit(1);
});
