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
 * experience.ts — 玩家经验值能力的场景库与输入构造。
 *
 * 一个单元是一串**操作序列**：两侧从「0 级、0 进度、0 累计」出发，依次施加同一串操作，
 * 比较最终的等级、进度条（f32 位模式）与累计经验。
 *
 * 场景由固定种子生成，保证单元集跨进程稳定（增量缓存与报告都依赖这一点）。
 */

/** 一步操作。只收两侧**语义相同**的入口：加点（`giveExperiencePoints`）与加等级（`giveExperienceLevels`）。 */
export type ExperienceOp =
    | { readonly op: "points"; readonly n: number }
    | { readonly op: "levels"; readonly n: number };

/** 一个场景。 */
export interface ExperienceScene {
    readonly name: string;
    readonly ops: readonly ExperienceOp[];
}

/**
 * 确定性伪随机（mulberry32）。
 *
 * 不用 Math.random：单元集必须可复现，否则缓存与报告都不可信。
 */
function createRandom(seed: number): () => number {
    let state = seed >>> 0;
    return (): number => {
        state = (state + 0x6d2b79f5) >>> 0;
        let t = state;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

/**
 * 生成圈定的场景集。
 *
 * 分五组，分别对准不同分支：
 *   - `formula/*`：从 0 级加等级，检查「每级所需经验」阶梯及其三段分区边界（14/15/29/30/31）；
 *   - `points/*`：先升到某级再加点，覆盖恰好升级、跨多级、不足一级；
 *   - `drain/*`：先补满进度再扣点，覆盖降到 0 级时的归零夹取与负数进度回绕；
 *   - `levels/*`：加/减等级，覆盖 `saturatedAdd` 与降到 0 级的归零；
 *   - `random/*`：固定种子的随机操作序列，覆盖任意组合。
 *
 * 只使用两侧语义相同的入口：原版的 `Player#giveExperiencePoints` / `#giveExperienceLevels`
 * 对应 Cubium 的 `addExperience` / `addLevels`。Cubium 另有 `setLevel`，但原版没有对应方法
 * （`experienceLevel` 是裸字段，赋值不改累计经验），两者语义不同，**不纳入本能力**。
 */
export function experienceScenes(): ExperienceScene[] {
    const scenes: ExperienceScene[] = [];

    // ---- 组 1：公式阶梯。只设等级，直接检验 getXpNeededForNextLevel 的分段 ----
    // 注意：加点会逐级上升（每次 1 级），因此单次加点量必须落在「能升的级数有限」的范围内。
    // 常驻玩家的单次加点最多几千点（击杀经验），这里上限取 10000；超大的 int 极值会让
    // 两侧都陷入数千万次循环——那是输入不合理，不是行为差异。
    for (const level of [0, 1, 7, 14, 15, 16, 29, 30, 31, 40, 100, 21862]) {
        scenes.push({ name: `formula/${level}`, ops: [{ op: "levels", n: level }] });
    }

    // ---- 组 2：单次加点。覆盖恰好填满一级、跨多级、不足一级 ----
    const addValues = [0, 1, 7, 8, 15, 16, 17, 100, 1395, 1500, 10000];
    for (const level of [0, 5, 14, 15, 29, 30, 100]) {
        for (const amount of addValues) {
            scenes.push({
                name: `points/${level}/${amount}`,
                ops: [{ op: "levels", n: level }, { op: "points", n: amount }],
            });
        }
    }

    // ---- 组 3：扣点。覆盖进度回绕、跨级下降、降到 0 级的归零夹取 ----
    const drainValues = [-1, -5, -16, -100, -1395, -10000];
    for (const level of [0, 1, 5, 16, 30, 50]) {
        for (const amount of drainValues) {
            scenes.push({
                name: `drain/${level}/${Math.abs(amount)}`,
                ops: [
                    { op: "levels", n: level },
                    // 先补满当前进度，再扣，才能触发进度回绕分支。
                    { op: "points", n: Math.max(1, 7 + level * 2) },
                    { op: "points", n: amount },
                ],
            });
        }
    }

    // ---- 组 4：加等级。覆盖 saturatedAdd 与降到 0 级的归零 ----
    for (const level of [0, 3, 30]) {
        for (const amount of [-5, -1, 0, 1, 5]) {
            scenes.push({
                name: `levels/${level}/${amount}`,
                ops: [{ op: "levels", n: level }, { op: "levels", n: amount }],
            });
        }
    }

    // ---- 组 5：随机操作序列。固定种子，覆盖任意组合 ----
    const random = createRandom(0xe4b1);
    for (let index = 0; index < 300; index++) {
        const ops: ExperienceOp[] = [];
        const steps = 1 + Math.floor(random() * 6);
        for (let step = 0; step < steps; step++) {
            const roll = random();
            if (roll < 0.15) {
                ops.push({ op: "levels", n: Math.floor(random() * 60) });
            } else if (roll < 0.75) {
                ops.push({ op: "points", n: Math.floor(random() * 2000) - 500 });
            } else {
                ops.push({ op: "levels", n: Math.floor(random() * 7) - 3 });
            }
        }
        scenes.push({ name: `random/${index}`, ops });
    }

    return scenes;
}

/** 场景名列表（能力声明用它做 `units: computed`）。 */
export function experienceSceneNames(): string[] {
    return experienceScenes().map((scene) => scene.name);
}

/** 场景名 → 探针与 oracle 共用的一行输入。 */
export function experienceInput(unit: string): Record<string, unknown> {
    const scene = experienceScenes().find((candidate) => candidate.name === unit);
    if (scene === undefined) {
        throw new Error(`experience：未知场景 ${unit}`);
    }
    return { unit: scene.name, ops: scene.ops.map((op) => ({ ...op })) };
}