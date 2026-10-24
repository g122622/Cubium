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
 * enchantment_table.ts — 附魔等级与附魔台代价（Cubium vs 原版 jar）。
 *
 * 参照实现是 `EnchantmentExport`：它按服务端启动的方式加载原版数据包（1.21 起附魔是
 * 数据包注册表，Bootstrap 拿不到），导出每个附魔的 minLevel / maxLevel 以及两端等级
 * 上的附魔台代价。附魔台每格显示什么等级、能给哪个附魔，全由这几个数决定。
 *
 * 单元由 oracle 自己的 `list` 给出（见 harnesses/index.ts）：两侧必须对「有哪些附魔」
 * 达成一致，缺失的一侧会以 `<absent in probe>` 出现在报告里。
 */

import type { HarnessSpec } from "../runner.ts";

/**
 * 原版 1.21.11 有、Cubium 的 EnchantmentRegistry 尚未注册的附魔。
 *
 * 这是**实现缺口**，不是参照数据缺失：清单显式列出、每次运行输出条数；补齐实现后
 * 必须删掉对应条目，否则会出现「豁免了原版也有的项」的假一致。
 */
export const KNOWN_MISSING: readonly string[] = [
    "minecraft:lunge",
    "minecraft:sweeping_edge",
    "minecraft:swift_sneak",
];

export function createEnchantmentTableHarness(): HarnessSpec {
    return {
        name: "enchantment_table",
        domain: "enchantment_table",
        key: "enchantment",
        // 单元由 oracle 枚举；这里的列表不参与（见 index.ts 的 JavaHarness.units 缺省）。
        units: () => [],
        // 两端：oracle 只给 minLevel 与 maxLevel 的代价（公式线性，端点一致即全等级一致）。
        input: (unit) => ({ enchantment: unit, levels: [1, 1] }),
        oracle: (unit) => ({ enchantment: unit }),
        oracleMissing: KNOWN_MISSING,
    } satisfies HarnessSpec;
}