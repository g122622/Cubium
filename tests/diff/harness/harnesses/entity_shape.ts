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

/*
 * entity_shape.ts — 实体碰撞箱尺寸。
 *
 * 单元列表取自原版运行时注册表（`EntityShapeOracle list`），比对由 java-oracle.ts
 * 完成：宽度、高度、眼睛高度、固定尺寸、分类全部逐字节比。
 * 不再使用 minecraft-data——它只给宽高两项，参考价值有限。
 */

import type { HarnessSpec } from "../runner.ts";

/**
 * 返回单元提供者（方块/实体等）的 stub。
 *
 * 这里保留 HarnessSpec 形状是为了让 runAll 的统一迭代保持简单；实际的单元枚举
 * 由 Java oracle 的 `list` 子命令给出，见 harnesses/index.ts 的 JAVA_HARNESSES。
 */
/**
 * 原版 1.21.11 注册、Cubium 的实体注册表尚未实现的类型。
 *
 * 这些是**实现缺口**，不是参照数据缺失。清单显式列出、每次运行输出条数；补齐后必须
 * 删掉对应条目，否则原版补齐的类型会被静默跳过。
 */
export const KNOWN_MISSING: readonly string[] = [
    "minecraft:acacia_boat",
    "minecraft:acacia_chest_boat",
    "minecraft:allay",
    "minecraft:bamboo_chest_raft",
    "minecraft:bamboo_raft",
    "minecraft:birch_boat",
    "minecraft:birch_chest_boat",
    "minecraft:block_display",
    "minecraft:breeze_wind_charge",
    "minecraft:cherry_boat",
    "minecraft:cherry_chest_boat",
    "minecraft:command_block_minecart",
    "minecraft:creaking",
    "minecraft:dark_oak_boat",
    "minecraft:dark_oak_chest_boat",
    "minecraft:interaction",
    "minecraft:item_display",
    "minecraft:jungle_boat",
    "minecraft:jungle_chest_boat",
    "minecraft:lingering_potion",
    "minecraft:mangrove_boat",
    "minecraft:mangrove_chest_boat",
    "minecraft:mannequin",
    "minecraft:marker",
    "minecraft:oak_boat",
    "minecraft:oak_chest_boat",
    "minecraft:pale_oak_boat",
    "minecraft:pale_oak_chest_boat",
    "minecraft:splash_potion",
    "minecraft:spruce_boat",
    "minecraft:spruce_chest_boat",
    "minecraft:text_display",
];

export function createEntityShapeHarness(): HarnessSpec {
    return {
        name: "entity_shape",
        domain: "entity_shape",
        key: "entity",
        units: () => [],
        input: (unit) => ({ entity: unit }),
        oracle: (unit) => ({ entity: unit }),
    } satisfies HarnessSpec;
}