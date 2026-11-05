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
 * jigsaw_joint_order.ts — 拼图模板的连接点洗牌顺序。
 *
 * 与 `structure_starts` 互补：那条能力只看装配整体，一处随机数消耗不同就让整棵树从
 * 某个构件开始分叉，报告里只剩「N 处差异」。这条能力把最小的一步单独拉出来——同一模板、
 * 同一旋转、同一随机种子下的连接点最终顺序——于是「洗牌消耗了几次随机数」「同样优先级
 * 的连接点如何排序」这类问题可以逐条比对。
 *
 * 语料直接取自 trial_chambers（试炼密室）用到的模板集合：它是已知最容易暴露
 * 「相邻构件旋转互换」的结构，且连接点数量多、优先级分布有重复（排序无法完全定序），
 * 正好能同时验证洗牌消耗与稳定排序两件事。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/** 覆盖试炼密室装配链上实际出现过的模板。 */
const TEMPLATES = [
    "minecraft:trial_chambers/chamber/end",
    "minecraft:trial_chambers/corridor/end_1",
    "minecraft:trial_chambers/corridor/end_2",
    "minecraft:trial_chambers/corridor/second_plate",
    "minecraft:trial_chambers/chamber/eruption",
    "minecraft:trial_chambers/chamber/eruption/center_1",
    "minecraft:trial_chambers/chamber/eruption/slice_1",
    "minecraft:trial_chambers/chamber/entrance_cap",
    "minecraft:trial_chambers/reward/ominous_vault",
    "minecraft:trial_chambers/reward/vault",
    "minecraft:trial_chambers/decor/disposal",
    "minecraft:trial_chambers/hallway/encounter_3",
    // 村庄与古代城市：跨结构验证同一套洗牌语义。
    "minecraft:village/plains/town_centers/plains_fountain_01",
    "minecraft:village/plains/streets/straight_02",
    "minecraft:ancient_city/city_center/city_center_3",
] as const;

/** 四个旋转都要覆盖：旋转改变连接点坐标，但不应改变洗牌消耗。 */
const ROTATIONS = ["NONE", "CLOCKWISE_90", "CLOCKWISE_180", "COUNTERCLOCKWISE_90"] as const;

/** 每个 (模板, 旋转) 用几个随机种子。 */
const SEEDS = [0, 9248738493917, 123456789, 42] as const;

interface Sample {
    unit: string;
    template: string;
    rotation: string;
    seed: number;
}

const SAMPLES: Sample[] = [];
for (const template of TEMPLATES) {
    for (const rotation of ROTATIONS) {
        for (const seed of SEEDS) {
            SAMPLES.push({ unit: `${template}/${rotation}/${seed}`, template, rotation, seed });
        }
    }
}

export function jigsawJointOrderDeclaration(): HarnessDeclaration {
    return {
        name: "jigsaw_joint_order",
        probe: "jigsaw_joint_order",
        oracle: "JigsawJointOrderOracle",
        key: "unit",
        units: {
            kind: "computed",
            units: (): string[] => SAMPLES.map((sample) => sample.unit),
        },
        input: (unit: string): unknown => {
            const sample = SAMPLES.find((candidate) => candidate.unit === unit);
            if (sample === undefined) {
                throw new Error(`jigsaw_joint_order: unknown unit ${unit}`);
            }
            return { unit, template: sample.template, rotation: sample.rotation, seed: sample.seed };
        },
    } satisfies HarnessDeclaration;
}