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
 * entity_shape.ts — 实体碰撞箱尺寸（Cubium vs minecraft-data 的 entities）。
 *
 * 只比 minecraft-data 陈述的字段：宽度、高度。眼睛高度与追踪距离/更新间隔
 * 参照数据没有，因此**不比**——宁可比的少而准，也不要为了让数字好看去猜。
 */

import { loadMinecraftData } from "../vanilla.ts";
import type { HarnessSpec } from "../runner.ts";
import { f32Bits } from "./shared.ts";

interface RawEntity {
    name: string;
    width: number;
    height: number;
}

export function createEntityShapeHarness(): HarnessSpec {
    const md = loadMinecraftData()("1.21.11") as unknown as { entitiesArray: RawEntity[] };
    const entities = md.entitiesArray;
    const byName = new Map(entities.map((entity) => [entity.name, entity]));

    return {
        name: "entity_shape",
        domain: "entity_shape",
        key: "entity",
        units: () => entities.map((entity) => `minecraft:${entity.name}`).sort(),
        input: (unit) => ({ entity: unit }),
        oracle: (unit) => {
            const entity = byName.get(unit.replace(/^minecraft:/, ""));
            if (entity === undefined) {
                return { entity: unit, missing: true };
            }
            return { entity: unit, width: f32Bits(entity.width), height: f32Bits(entity.height) };
        },
    } satisfies HarnessSpec;
}