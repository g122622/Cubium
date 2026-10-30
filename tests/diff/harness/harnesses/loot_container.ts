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
 * loot_container.ts — 容器战利品表的单元与输入。
 *
 * 一张表在多个场景下各开一次箱子，因为「掷出来什么」和「怎么摆进 27 个格子」
 * 是两段独立的逻辑：
 *
 *   - plain       ：空箱子，只比「掷 + 摆」；
 *   - prefilled   ：箱子底上已经有东西，原版只会往剩下的空格里摆（槽位集合因此不同）；
 *   - named       ：箱子带自定义名，掉落物的 `copy_components(source: block_entity)`
 *                   要把名字抄过去；
 *   - locked      ：箱子上了锁，掉落物要带上 lock 组件；
 *   - unopened    ：箱子还没被打开过就掉出来（潜影盒就是这种），掉落物要带
 *                   container_loot 组件，并且 container 组件里不能有东西。
 *
 * 种子取固定值而不是随机：两侧读同一份输入，任何随机都会让比对失去意义。
 */
export interface LootContainerScenario {
    readonly name: string;
    readonly seed: number;
    readonly name_?: string;
    readonly lock?: string;
    /** 预置物品：结构生成时箱子里本来就摆着的东西。 */
    readonly items?: readonly { readonly slot: number; readonly id: string; readonly count?: number }[];
}

export const LOOT_CONTAINER_SCENARIOS: readonly LootContainerScenario[] = [
    // seed 为 0 表示「结构里没有记种子」，原版这时直接用服务器的随机序列
    // （`getRandomSequence`）。两侧都拿不到同一条序列，所以这一档只比结构差异，
    // 见文件头 TODO。
    { name: "plain", seed: 0, name_: "Chest" },
    { name: "prefilled", seed: 0, name_: "Chest", items: [{ slot: 3, id: "minecraft:cobblestone", count: 7 }] },
    { name: "named", seed: 0, name_: "Dungeon Chest" },
    { name: "locked", seed: 0, name_: "Chest", lock: "minecraft:gold_ingot" },
];

export const LOOT_CONTAINER_SCENARIO_NAMES: readonly string[] = LOOT_CONTAINER_SCENARIOS.map(
    (scenario) => scenario.name,
);

/**
 * 单元键：`表 ID#场景名`。
 *
 * 两侧都用同一个键对齐；表名从 `#` 之前的部分取。
 */
export function lootContainerUnit(table: string, scenarioName: string): string {
    return `${table}#${scenarioName}`;
}

/** 单元键 → 探针/oracle 的输入行。 */
export function lootContainerInput(unit: string): unknown {
    const hash = unit.lastIndexOf("#");
    const table = unit.slice(0, hash);
    const scenarioName = unit.slice(hash + 1);
    const scenario = LOOT_CONTAINER_SCENARIOS.find((candidate) => candidate.name === scenarioName);
    if (scenario === undefined) {
        throw new Error(`Unknown loot container scenario: ${scenarioName}`);
    }
    return {
        table: lootContainerUnit(table, scenario.name),
        block: "minecraft:chest",
        seed: scenario.seed,
        ...(scenario.name_ === undefined ? {} : { name: scenario.name_ }),
        ...(scenario.lock === undefined ? {} : { lock: scenario.lock }),
        ...(scenario.items === undefined ? {} : { items: scenario.items }),
    };
}