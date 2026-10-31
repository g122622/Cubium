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
 * explosion_ray.ts — 爆炸射线破坏判定能力的输入构造。
 *
 * 单元是「场景 × 随机种子」。场景决定半径与逐层抗性，种子决定每条射线的初始强度。
 * 两个维度都要扫：抗性决定衰减速度，种子决定 1352 条射线各自的起点强度，任何一处
 * 常量偏差都会让边界上的方块判定翻转。
 */

/**
 * 覆盖用的随机种子。
 *
 * 0 与小的正整数覆盖「随机数落在区间边界」的情形；负数与大数覆盖 Java 的
 * `RandomSource.create(long)` 对高位的处理。
 */
const SEEDS = ["0", "1", "2", "42", "-1", "123456789", "-987654321", "9223372036854775807"] as const;

/** 爆炸射线能力的一个单元：场景名与随机种子。 */
export interface ExplosionRayUnit {
    readonly scene: string;
    readonly seed: string;
}

/**
 * 场景名。
 *
 * 必须与 probes/explosion_ray.cpp 的 `SCENES` 以及 ExplosionRayOracle 的同名表
 * 逐项一致。`computed` 单元拿不到 `list` 的输出，只能在这里列一遍；三处不一致时
 * 单元会因为找不到场景而报 `missing`，不会静默通过。
 */
const SCENES = ["tnt", "tnt_air", "creeper", "obsidian", "mixed", "charged"] as const;

/** 把场景名展开成「场景 × 种子」的单元列表。 */
export function explosionRayUnits(): string[] {
    const units: string[] = [];
    for (const scene of SCENES) {
        for (const seed of SEEDS) {
            units.push(`${scene}@${seed}`);
        }
    }
    return units;
}

/** 单元字符串 → 探针与 oracle 共用的一行输入。 */
export function explosionRayInput(unit: string): ExplosionRayUnit {
    const at = unit.lastIndexOf("@");
    if (at < 0) {
        throw new Error(`explosion_ray: 单元缺少 '@种子' 后缀：${unit}`);
    }
    return {
        scene: unit.slice(0, at),
        // 种子保持字符串形式：i64 的最小/最大值超出 JS 的安全整数范围。
        seed: unit.slice(at + 1),
    };
}
