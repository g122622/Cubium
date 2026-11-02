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
 * light_occlusion.ts — 方块状态的光照阻挡属性。
 *
 * 单元是方块 ID；每个方块输出它全部状态的四个值：lightBlock（每格衰减量）、
 * propagatesSkylightDown（天空光直通）、useShapeForLightOcclusion（按形状遮挡）、
 * canOcclude（遮挡总结开关）。它们决定光照引擎如何让一束光走过这一格。
 *
 * 这四个值是纯数据，抄错没有编译期提示：多衰减一级屋子里就差一格看不清，树叶判成
 * 不透光树冠下就长不出草。只能靠逐条比对发现。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * 两侧输出都是「状态名 → 四个值」的 map，但键的书写顺序不同（Cubium 走 map 插入序、
 * 原版走 TreeMap 之外的声明序），JSON 对象的键序本身没有语义。这里按名字重排一遍，
 * 让比较只关心内容。
 *
 * 失败侧（方块缺失）也要补齐字段：只带 `missing: true` 而另一边带着整张状态表时，
 * 差异必须体现出来，不能被"缺键"糊过去。
 */
export function normaliseLightOcclusion(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    const values = record["values"];
    if (values === null || typeof values !== "object") {
        return { missing: record["missing"] ?? false, values: null };
    }
    const entries = Object.entries(values as Record<string, unknown>)
        .sort(([left], [right]) => (left < right ? -1 : left > right ? 1 : 0));
    const normalised: Record<string, unknown> = {};
    for (const [name, value] of entries) {
        const fields = (value ?? {}) as Record<string, unknown>;
        normalised[name] = {
            lightBlock: fields["lightBlock"] ?? null,
            propagatesSkylightDown: fields["propagatesSkylightDown"] ?? null,
            useShapeForLightOcclusion: fields["useShapeForLightOcclusion"] ?? null,
            canOcclude: fields["canOcclude"] ?? null,
        };
    }
    return { missing: record["missing"] ?? false, values: normalised };
}

normaliseLightOcclusion.canonicalising = true;

/** 能力声明。 */
export function lightOcclusionDeclaration(): HarnessDeclaration {
    return {
        name: "light_occlusion",
        probe: "light_occlusion",
        oracle: "LightOcclusionOracle",
        // 单元是方块（一次返回它全部状态），不是单个状态。
        key: "block",
        units: { kind: "listing" },
        input: (unit: string): { block: string } => ({ block: unit }),
        normalise: normaliseLightOcclusion,
    };
}
