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
 * loot_context.ts — 战利品表三类上下文的输入模板。
 *
 * 三个探针（loot_table / loot_table_block / loot_table_entity）共用同一批单元
 * （数据包里的每张战利品表），区别仅在构造上下文的参数：
 *
 *   - loot_table        ：只给 origin，什么都不多给；
 *   - loot_table_block  ：补上 block_state 与工具，按「挖掘」方式取；
 *   - loot_table_entity ：补上 this_entity / attacking_entity，按「击杀」方式取。
 *
 * 这些输入**必须与 Java oracle 逐字段一致**——两侧读的是同一个 inputs.jsonl，
 * 所以模板只写一遍，由 index.ts 分发给两侧。
 */

/** 方块探针使用的固定工具与时运等级。 */
export const BLOCK_CONTEXT = {
    tool: "minecraft:diamond_pickaxe",
    fortune: 3,
    silkTouch: false,
} as const;

/**
 * 实体探针的参照缺口：数据包里有这些表，但原版侧无法构造对应的被击杀实体
 * （`creaking` 与 `mannequin` 在服务端建不出可用实例），双方都没有样本可比。
 *
 * 探针侧遇到「实体类型未知」时按 `{"missing": true}` 处理，因此这里只登记两侧
 * 口径不同、需要豁免的单元。
 */
export const ENTITY_ORACLE_MISSING: readonly string[] = [
    "minecraft:entities/creaking",
    "minecraft:entities/mannequin",
];

/**
 * 实体探针的输入场景。
 *
 * 同一张表在多组场景下各掷一次：只掷一组时，「条件被误判为真」与「条件被误判为假」
 * 最多只能暴露一半。而「掠食者是队长」「被击杀者着火」「击杀者是玩家」这些判据在
 * 实体刚建出来时全部为假——不构造正向场景，谓词即使永远返回 false 也看不出来。
 *
 *   - default  ：击杀者是僵尸、被击杀者未着火（最初的形态）；
 *   - player   ：击杀者是玩家（`killed_by_player`、装备类条件走玩家分支）；
 *   - on_fire  ：被击杀者着火（`flags.is_on_fire`、燃烧掉落）；
 *   - captain  ：击杀者是袭击队长（`type_specific: raider.is_captain`）。
 *
 * 队长的判定依据是「头戴不祥旗帜」：两侧都在实体创建后把一份真正的不祥旗帜写进
 * 头盔槽，而不是伪造一个布尔标志。
 */
export interface EntityScenario {
    readonly name: string;
    readonly attacker?: string;
    readonly looting?: number;
    readonly onFire?: boolean;
    readonly attackerCaptain?: boolean;
}

export const ENTITY_SCENARIOS: readonly EntityScenario[] = [
    { name: "default", attacker: "minecraft:zombie", looting: 3, onFire: false },
    { name: "player", attacker: "minecraft:player", looting: 3, onFire: false },
    { name: "on_fire", attacker: "minecraft:zombie", looting: 3, onFire: true },
    { name: "captain", attacker: "minecraft:pillager", looting: 3, onFire: false, attackerCaptain: true },
];

/** 单元 → 该表的全部场景名。 */
export const ENTITY_SCENARIO_NAMES: readonly string[] = ENTITY_SCENARIOS.map((scenario) => scenario.name);

/** 单元 → `loot_table` 探针输入。 */
export function lootTableInput(unit: string): unknown {
    return { table: unit };
}

/**
 * 单元 → 方块探针输入。
 *
 * 方块名取自战利品表 ID 的末段（`minecraft:blocks/stone` → `minecraft:stone`），
 * 两侧都按同一个规则解析，因此即使某张表的方块名与表名不一致，差异也会如实暴露。
 */
export function lootTableBlockInput(unit: string): unknown {
    const path = unit.startsWith("minecraft:") ? unit.slice("minecraft:".length) : unit;
    const blockName = path.startsWith("blocks/") ? path.slice("blocks/".length) : path;
    return {
        table: unit,
        block: `minecraft:${blockName}`,
        ...BLOCK_CONTEXT,
    };
}

/**
 * 单元 → 实体探针输入。
 *
 * 每张表按 {@link ENTITY_SCENARIOS} 展开成多行；场景名进 `scene` 字段，用来把同一张表
 * 的多组结果区分开（比对按 `table` 对齐，因此每一行都是独立的单元键）。
 */
export function lootTableEntityInput(unit: string, scenarioName: string): unknown {
    const scenario = ENTITY_SCENARIOS.find((candidate) => candidate.name === scenarioName);
    if (scenario === undefined) {
        throw new Error(`Unknown loot entity scenario: ${scenarioName}`);
    }
    const path = unit.startsWith("minecraft:") ? unit.slice("minecraft:".length) : unit;
    const entityName = path.startsWith("entities/") ? path.slice("entities/".length) : path;
    // 显式展开字段而不是整体展开 scenario：`name` 只在场景内有意义，不能混进输入。
    return {
        table: lootTableEntityUnit(unit, scenario.name),
        entity: `minecraft:${entityName}`,
        ...(scenario.attacker === undefined ? {} : { attacker: scenario.attacker }),
        ...(scenario.looting === undefined ? {} : { looting: scenario.looting }),
        ...(scenario.onFire === undefined ? {} : { onFire: scenario.onFire }),
        ...(scenario.attackerCaptain === undefined ? {} : { attackerCaptain: scenario.attackerCaptain }),
    };
}

/** 单元键：表 ID + 场景名（两侧按同一个键对齐）。 */
export function lootTableEntityUnit(unit: string, scenarioName: string): string {
    return `${unit}#${scenarioName}`;
}