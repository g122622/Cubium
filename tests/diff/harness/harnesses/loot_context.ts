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

/** 实体探针使用的固定击杀者与掠夺等级。 */
export const ENTITY_CONTEXT = {
    attacker: "minecraft:zombie",
    looting: 3,
    onFire: false,
} as const;

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

/** 单元 → 实体探针输入。 */
export function lootTableEntityInput(unit: string): unknown {
    const path = unit.startsWith("minecraft:") ? unit.slice("minecraft:".length) : unit;
    const entityName = path.startsWith("entities/") ? path.slice("entities/".length) : path;
    return {
        table: unit,
        entity: `minecraft:${entityName}`,
        ...ENTITY_CONTEXT,
    };
}