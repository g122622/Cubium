/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * enchantment_compat.ts — 附魔互斥性判定能力的输入构造。
 *
 * 单元形如 `<a>|<b>|<aExcl>|<bExcl>`：
 *
 * - `a`、`b` 是附魔 id（如 `minecraft:sharpness`）
 * - `aExcl`、`bExcl` 是各自 `exclusive_set` 展开后的附魔 id，逗号分隔；空串表示空集
 *
 * 互斥表从数据包读取并展开 `#tag` 引用，保证两侧看到同一份数据。
 */

import fs from "node:fs";
import path from "node:path";

import { vanillaDataPackDir } from "../oracle/environment.ts";

/** 互斥性能力的一个单元。 */
export interface EnchantmentCompatUnit {
    readonly unit: string;
}

/**
 * 从数据包里读取所有附魔的 `exclusive_set` 并展开标签。
 *
 * 返回值是 `附魔 id -> 互斥附魔 id 列表`。展开规则与原版 `HolderSet` 一致：
 * `"#namespace:path"` 形态引用标签，其余视为单元素列表。
 */
function loadExclusiveSets(): Map<string, string[]> {
    const root = vanillaDataPackDir();
    const enchantDir = path.join(root, "data", "minecraft", "enchantment");
    const tagDir = path.join(root, "data", "minecraft", "tags", "enchantment");
    if (!fs.existsSync(enchantDir)) {
        return new Map();
    }

    /** 读一个标签文件，返回它直接列出的附魔 id（不递归，递归在下方统一处理）。 */
    const readTag = (tagId: string): string[] => {
        const bare = tagId.startsWith("#") ? tagId.slice(1) : tagId;
        const [namespace, name] = bare.includes(":")
            ? (bare.split(":", 2) as [string, string])
            : (["minecraft", bare] as [string, string]);
        const file = path.join(root, "data", namespace, "tags", "enchantment", `${name}.json`);
        if (!fs.existsSync(file)) {
            return [];
        }
        const parsed = JSON.parse(fs.readFileSync(file, "utf8")) as { values?: unknown[] };
        const values = Array.isArray(parsed.values) ? parsed.values : [];
        const result: string[] = [];
        for (const entry of values) {
            if (typeof entry === "string") {
                result.push(entry.includes(":") ? entry : `minecraft:${entry}`);
            } else if (entry && typeof entry === "object" && "id" in entry) {
                const id = (entry as { id: unknown }).id;
                if (typeof id === "string") {
                    result.push(id.includes(":") ? id : `minecraft:${id}`);
                }
            }
        }
        return result;
    };

    /** 递归展开，带环保护。 */
    const expand = (entries: string[], seen: Set<string>): string[] => {
        const result: string[] = [];
        for (const entry of entries) {
            if (entry.startsWith("#")) {
                if (seen.has(entry)) {
                    continue;
                }
                seen.add(entry);
                result.push(...expand(readTag(entry), seen));
            } else {
                result.push(entry);
            }
        }
        return result;
    };

    const sets = new Map<string, string[]>();
    for (const file of fs.readdirSync(enchantDir)) {
        if (!file.endsWith(".json")) {
            continue;
        }
        const name = file.slice(0, -".json".length);
        const enchantmentId = `minecraft:${name}`;
        const parsed = JSON.parse(fs.readFileSync(path.join(enchantDir, file), "utf8")) as {
            exclusive_set?: unknown;
        };
        const raw = parsed.exclusive_set;
        let entries: string[] = [];
        if (typeof raw === "string") {
            entries = [raw];
        } else if (Array.isArray(raw)) {
            entries = raw.filter((item): item is string => typeof item === "string");
        }
        const expanded = expand(entries, new Set<string>());
        // 去重，排序保证两侧一致。
        sets.set(enchantmentId, [...new Set(expanded)].sort());
    }
    return sets;
}

/**
 * 生成互斥性判定用的单元列表。
 *
 * 覆盖三类配对：
 *
 * 1. 每个附魔与自身（`areCompatible(x, x)` 恒为 `false`）
 * 2. 每个确有互斥关系的组合
 * 3. 一批无互斥关系的随机搭配
 */
export function listEnchantmentCompatUnits(): string[] {
    const sets = loadExclusiveSets();
    const ids = [...sets.keys()].sort();
    const units: string[] = [];

    const encode = (list: string[]): string => list.join(",");

    // 1. 自反。
    for (const id of ids) {
        units.push(`${id}|${id}|${encode(sets.get(id) ?? [])}|${encode(sets.get(id) ?? [])}`);
    }

    // 2. 真实互斥对：只取 a < b 的无序对，避免重复。
    for (const a of ids) {
        const aExcl = sets.get(a) ?? [];
        for (const b of aExcl) {
            if (!sets.has(b)) {
                continue;
            }
            if (a >= b) {
                continue;
            }
            const bExcl = sets.get(b) ?? [];
            units.push(`${a}|${b}|${encode(aExcl)}|${encode(bExcl)}`);
            // 反向单独出一条，确认判定对称。
            units.push(`${b}|${a}|${encode(bExcl)}|${encode(aExcl)}`);
        }
    }

    // 3. 无互斥的搭配：取前若干个附魔两两配对（不含互斥的）。
    const sample = ids.slice(0, 12);
    for (let i = 0; i < sample.length; i += 1) {
        for (let j = i + 1; j < sample.length; j += 1) {
            const a = sample[i];
            const b = sample[j];
            const aExcl = sets.get(a) ?? [];
            const bExcl = sets.get(b) ?? [];
            if (aExcl.includes(b) || bExcl.includes(a)) {
                continue;
            }
            units.push(`${a}|${b}|${encode(aExcl)}|${encode(bExcl)}`);
        }
    }

    return units;
}

/**
 * 单元字符串原样透传。
 *
 * 互斥表已在 harness 侧展开完毕，两侧共用同一段解析逻辑。
 */
export function enchantmentCompatInput(unit: string): EnchantmentCompatUnit {
    return { unit };
}