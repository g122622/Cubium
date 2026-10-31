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
 * enchantment_provider.ts — 附魔提供器能力的输入构造。
 *
 * 单元是「提供器 ID + 难度乘数」。只比提供器 ID 是不够的：`by_cost_with_difficulty`
 * 的代价上界 `minCost + (int)(specialMultiplier * maxCostSpan)` 依赖乘数，而 `(int)`
 * 是截断操作——乘数取整数、半整、以及能触发截断差异的小数时，两侧的实现差异才会
 * 暴露出来。因此每个提供器都跑一组固定的乘数。
 */

import { existsSync, readdirSync } from "node:fs";
import path from "node:path";

import { vanillaDataPackDir } from "../oracle/environment.ts";

/**
 * 覆盖用的难度乘数。
 *
 * `DifficultyInstance.getSpecialMultiplier()` 的实际取值范围是 0 ~ 1.5（随难度、
 * 世界时间、区块驻留时间、月相变化）。这里额外加入 0.25 / 0.75 这类会与 `maxCostSpan`
 * 相乘产生小数的值，专门用来暴露「截断 vs 四舍五入」的差异。
 */
const SPECIAL_MULTIPLIERS = ["0", "0.25", "0.5", "0.75", "1", "1.25", "1.5"] as const;

/** 提供器能力的一个单元：提供器 ID 与其中一个乘数的组合。 */
export interface EnchantmentProviderUnit {
    readonly provider: string;
    readonly specialMultiplier: string;
}

/**
 * 从数据包目录枚举全部提供器 ID。
 *
 * `computed` 单元拿不到 `list` 的输出，所以这里直接扫目录。扫描规则与探针的
 * `list` 一致：`data/<ns>/enchantment_provider/**<name>.json` → `<ns>:<name>`，
 * 递归包含子目录（原版有 `raid/` 一层）。
 */
function listProviderIds(): string[] {
    // 用 oracle 侧的 `vanillaDataPackDir()` 而不是直接读环境变量：`computed` 单元
    // 在 runner 注入 `MC_DIFF_VANILLA_DATAPACK` 之前就会求值，那里还拿不到该变量。
    // 这个辅助函数带「仓库内置副本 → 用户目录」的自动探测，两条路径都能工作。
    const root = vanillaDataPackDir();
    const dataDir = path.join(root, "data");
    const ids: string[] = [];
    for (const ns of readdirSync(dataDir, { withFileTypes: true })) {
        if (!ns.isDirectory()) {
            continue;
        }
        const providerDir = path.join(dataDir, ns.name, "enchantment_provider");
        if (!existsSync(providerDir)) {
            continue;
        }
        const walk = (dir: string, prefix: string): void => {
            for (const entry of readdirSync(dir, { withFileTypes: true })) {
                if (entry.isDirectory()) {
                    walk(path.join(dir, entry.name), `${prefix}${entry.name}/`);
                } else if (entry.name.endsWith(".json")) {
                    const name = entry.name.slice(0, -".json".length);
                    ids.push(`${ns.name}:${prefix}${name}`);
                }
            }
        };
        walk(providerDir, "");
    }
    ids.sort();
    return ids;
}

/**
 * 把 `list` 输出的提供器 ID 展开成「ID × 乘数」的单元列表。
 *
 * 与其它能力不同，这里的单元数不等于 `list` 的行数，因此声明里用
 * `kind: "computed"` 由这个函数生成。
 */
export function enchantmentProviderUnits(): string[] {
    const units: string[] = [];
    for (const id of listProviderIds()) {
        for (const multiplier of SPECIAL_MULTIPLIERS) {
            units.push(`${id}@${multiplier}`);
        }
    }
    return units;
}

/**
 * 单元字符串 → 探针与 oracle 共用的一行输入。
 *
 * 单元形如 `minecraft:mob_spawn_equipment@0.75`。提供器 ID 本身不含 `@`，所以按
 * 最后一个 `@` 切分是安全的。
 */
export function enchantmentProviderInput(unit: string): EnchantmentProviderUnit {
    const at = unit.lastIndexOf("@");
    if (at < 0) {
        throw new Error(`enchantment_provider: 单元缺少 '@乘数' 后缀：${unit}`);
    }
    return {
        provider: unit.slice(0, at),
        specialMultiplier: unit.slice(at + 1),
    };
}