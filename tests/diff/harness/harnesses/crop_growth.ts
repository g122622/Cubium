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
 * crop_growth.ts — 作物一次随机刻的结果：生长速率、抽芽、结果、耕地保墒。
 *
 * 单元是固定场景名。地形由两侧各自手工编（同一份字符表），比的是给定地形下算出来的
 * 生长速率，以及随机刻跑完之后作物、邻居、脚下耕地的状态。
 *
 * 随机数两侧同算法同种子，所以「这一次到底长没长」是确定的——不一致就意味着有一侧
 * 的掷骰顺序或分支条件与原版不同，而不只是运气不好。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * 失败侧（未知场景）也要补齐全部字段，否则「原版认这个场景、本侧不认」会因缺键被
 * 静默通过。
 */
export function normaliseCropGrowth(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    const neighbours = record["neighbours"];
    return {
        ok: record["ok"] ?? null,
        growthSpeed: record["growthSpeed"] ?? null,
        after: record["after"] ?? null,
        neighbours: neighbours ?? null,
        ground: record["ground"] ?? null,
    };
}

normaliseCropGrowth.canonicalising = true;

/** 能力声明。 */
export function cropGrowthDeclaration(): HarnessDeclaration {
    return {
        name: "crop_growth",
        probe: "crop_growth",
        oracle: "CropGrowthOracle",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string): { unit: string } => ({ unit }),
        normalise: normaliseCropGrowth,
        // oracle 要在临时世界里铺地形：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}