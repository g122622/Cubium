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
 * redstone_signal.ts — 红石信号：谁向哪个方向输出多少强度。
 *
 * 单元是固定场景名（字幕 + 主语方块 + 周围的 5×5×3 地形）。两侧各自按同一张字符表
 * 铺地形，再逐方向提问。比的是「同一片地形下，这块方块朝六个方向各输出多少」——
 * 红石电路失效的地方几乎都落在这几个数字上，而它既不崩溃也不报错。
 *
 * 字段口径见探针文件头「字段与原版的对应」：`signal` / `weak` / `strong` 对应原版的
 * 三条查询入口，`bestNeighbor` 对应 getBestNeighborSignal，`quiet*` 是关掉红石线自我
 * 屏蔽之后的重问。
 */

import type { HarnessDeclaration } from "../core/types.ts";

const DIRECTIONS = ["down", "up", "north", "south", "west", "east"] as const;

/**
 * 六个方向的功率表统一成「方向 → 数字」的固定形状。
 *
 * 两侧序列化时键的顺序不同（Cubium 走 Direction 枚举，原版走 Direction.values()），
 * 而 JSON 对象的键序本身没有语义。这里按固定顺序重建一遍，让比较只关心数值。
 * 缺方向时补 null 而不是省略——省略会让「少了一个方向」看起来和原版一样。
 */
function normaliseDirectionMap(value: unknown): Record<string, unknown> | null {
    if (value === null || typeof value !== "object") {
        return null;
    }
    const record = value as Record<string, unknown>;
    const result: Record<string, unknown> = {};
    for (const direction of DIRECTIONS) {
        result[direction] = record[direction] ?? null;
    }
    return result;
}

/**
 * 比对前的规范化。
 *
 * 失败侧（未知场景）也要补齐全部字段，否则「原版认这个场景、本侧不认」会因缺键被
 * 静默通过。
 */
export function normaliseRedstoneSignal(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    const wire = record["wire"];
    return {
        ok: record["ok"] ?? null,
        signal: normaliseDirectionMap(record["signal"]),
        weak: normaliseDirectionMap(record["weak"]),
        strong: normaliseDirectionMap(record["strong"]),
        bestNeighbor: record["bestNeighbor"] ?? null,
        quietSignal: normaliseDirectionMap(record["quietSignal"]),
        quietBestNeighbor: record["quietBestNeighbor"] ?? null,
        wire: wire === undefined || wire === null
            ? null
            : {
                input: (wire as Record<string, unknown>)["input"] ?? null,
                neighborPower: (wire as Record<string, unknown>)["neighborPower"] ?? null,
                // 连接表只有四个水平方向；上下两格恒为 null，两侧同样是 null，不造成误判。
                links: normaliseDirectionMap((wire as Record<string, unknown>)["links"]),
            },
    };
}

normaliseRedstoneSignal.canonicalising = true;

/** 能力声明。 */
export function redstoneSignalDeclaration(): HarnessDeclaration {
    return {
        name: "redstone_signal",
        probe: "redstone_signal",
        oracle: "RedstoneSignalOracle",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string): { unit: string } => ({ unit }),
        normalise: normaliseRedstoneSignal,
        // oracle 要在临时世界里铺地形：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}