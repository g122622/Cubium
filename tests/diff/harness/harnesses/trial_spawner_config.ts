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
 * trial_spawner_config.ts — 试炼刷怪笼配置能力的输入构造。
 *
 * 单元是「配置 ID + 玩家数」。只比配置本身不够：真正的语义在
 * `calculateTargetTotalMobs(n)` 与 `calculateTargetSimultaneousMobs(n)` 里，而它们对
 * 玩家数敏感——四个数值字段在原版是 f32，玩家数乘以 0.5 之后取 floor，边界上
 * （例如 1 + 0.5*3 = 2.5）差一个整数就会少刷一只怪。
 */

import { existsSync, readdirSync } from "node:fs";
import path from "node:path";

import { vanillaDataPackDir } from "../oracle/environment.ts";

/**
 * 覆盖用的玩家数。
 *
 * 从 0 到 8 覆盖单人、小队、满编三种规模。原版的每玩家加成多为 0.5 或 1，玩家数为
 * 奇数时最容易踩到浮点 floor 的边界。
 */
const PLAYER_COUNTS = [0, 1, 2, 3, 4, 5, 6, 7, 8] as const;

/**
 * 从数据包目录枚举全部配置 ID。
 *
 * `computed` 单元拿不到 `list` 的输出，所以这里直接扫目录。规则与探针的 `list`
 * 一致：`data/<ns>/trial_spawner/**<name>.json` → `<ns>:<name>`，递归包含子目录
 * （原版有 `trial_chamber/breeze/` 这类多层目录）。
 */
function listConfigIds(): string[] {
    // 用 oracle 侧的 `vanillaDataPackDir()`：`computed` 单元在 runner 注入
    // `MC_DIFF_VANILLA_DATAPACK` 之前就会求值，直接读环境变量拿不到。
    const root = vanillaDataPackDir();
    const dataDir = path.join(root, "data");
    const ids: string[] = [];
    for (const ns of readdirSync(dataDir, { withFileTypes: true })) {
        if (!ns.isDirectory()) {
            continue;
        }
        const configDir = path.join(dataDir, ns.name, "trial_spawner");
        if (!existsSync(configDir)) {
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
        walk(configDir, "");
    }
    ids.sort();
    return ids;
}

/**
 * 把配置 ID 展开成「ID × 玩家数」的单元列表。
 *
 * 原版 28 个配置 × 9 种玩家数 = 252 个单元。
 */
export function trialSpawnerConfigUnits(): string[] {
    const units: string[] = [];
    for (const id of listConfigIds()) {
        for (const playerCount of PLAYER_COUNTS) {
            units.push(`${id}@${playerCount}`);
        }
    }
    return units;
}

/** 试炼刷怪笼配置能力的一个单元：配置 ID 与玩家数。 */
export interface TrialSpawnerConfigUnit {
    readonly config: string;
    readonly playerCount: number;
}

/**
 * 单元字符串 → 探针与 oracle 共用的一行输入。
 *
 * 单元形如 `minecraft:trial_chamber/breeze/normal@2`。配置 ID 不含 `@`，按最后一个
 * `@` 切分是安全的。
 */
export function trialSpawnerConfigInput(unit: string): TrialSpawnerConfigUnit {
    const at = unit.lastIndexOf("@");
    if (at < 0) {
        throw new Error(`trial_spawner_config: 单元缺少 '@玩家数' 后缀：${unit}`);
    }
    return {
        config: unit.slice(0, at),
        playerCount: Number(unit.slice(at + 1)),
    };
}