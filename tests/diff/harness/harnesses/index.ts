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
 * index.ts — 差分 harness 注册表与批量入口。
 *
 *   node tests/diff/harness/runner.ts run all          # 全部
 *   node tests/diff/harness/runner.ts run item_summary # 单个
 *
 * 每个 harness 自带参照实现，互不依赖：任何一个失败都不影响其余 harness 运行。
 * 参照实现有两类：
 *   - 原版 Java oracle（优先）：直接调用原版 jar 的 API，权威、逐字节可比；
 *   - minecraft-data（回退）：原版 jar 不可用时的离线数据源，覆盖字段更少。
 */

import { runJavaOracle } from "../java-oracle.ts";
import type { HarnessSpec } from "../runner.ts";
import { runSpec } from "../runner.ts";
import { KNOWN_MISSING as enchantmentKnownMissing } from "./enchantment_table.ts";
import { KNOWN_MISSING as entityKnownMissing, createEntityShapeHarness } from "./entity_shape.ts";
import { createItemSummaryHarness } from "./item_summary.ts";
import {
    ENTITY_ORACLE_MISSING,
    lootTableBlockInput,
    lootTableEntityInput,
    lootTableInput,
} from "./loot_context.ts";
import { KNOWN_MISSING as itemsKnownMissing } from "./known_missing_items.ts";
import { KNOWN_MISSING_COMMANDS } from "./known_missing_commands.ts";
import { spawnRulesInput } from "./spawn_rules.ts";
import { createRecipeHarness } from "./recipe.ts";
import { spawnPlacementInput } from "./spawn_placement.ts";
import { statusEffectInput, statusEffectUnits } from "./status_effect.ts";
import { KNOWN_MISSING as entityAttributesKnownMissing } from "./known_missing_entity_attributes.ts";
import {
    RUNTIME_DIVERGENT as entityAttributesRuntimeDivergent,
    VANILLA_ENTITY_ATTRIBUTES,
} from "./entity_attributes_reference.ts";

/**
 * 已登记、使用 minecraft-data 的声明式 harness。
 *
 * 目前为空：曾走 minecraft-data 的两条能力（附魔、食物饱和度）都已改接原版 jar。
 * 保留这个数组是因为它仍是「无 Java oracle 时」的回退通道。
 */
/**
 * biome_spawns 的规范化：每个分类的生成条目按 (type, weight, minCount, maxCount) 排序。
 *
 * 原版把 spawners 存成 WeightedList，`unwrap()` 的顺序与数据包书写顺序无关；集合语义下
 * 条目的先后没有意义，比对前统一排序。其余字段原样保留。
 */
function canonicalSpawnCategories(value: unknown): unknown {
    if (value === null || typeof value !== "object") {
        return value;
    }
    const record = value as Record<string, unknown>;
    if (record["categories"] === undefined || record["categories"] === null) {
        return value;
    }
    // float 字段按「f32 位模式」比对：JSON 解析后是 double，用 Math.fround 还原成两侧共有的值，
    // 避免 0.1 与 0.10000000149011612 这种写法差异被当成不一致。
    const probability = typeof record["creature_spawn_probability"] === "number"
        ? Math.fround(record["creature_spawn_probability"] as number)
        : record["creature_spawn_probability"];
    const categories = record["categories"] as Record<string, unknown>;
    const sortedCategories: Record<string, unknown> = {};
    for (const name of Object.keys(categories).sort()) {
        const category = categories[name] as Record<string, unknown>;
        const spawns = Array.isArray(category["spawns"]) ? [...category["spawns"]] : [];
        spawns.sort((left, right) => {
            const fields = (entry: unknown): unknown[] => {
                const spawn = entry as Record<string, unknown>;
                return [spawn["type"], spawn["weight"], spawn["minCount"], spawn["maxCount"]];
            };
            const a = fields(left);
            const b = fields(right);
            for (let index = 0; index < a.length; index++) {
                if (a[index] === b[index]) {
                    continue;
                }
                return (a[index] as never) < (b[index] as never) ? -1 : 1;
            }
            return 0;
        });
        sortedCategories[name] = { ...category, spawns };
    }
    return { ...record, categories: sortedCategories, creature_spawn_probability: probability };
}

const DATA_SPECS: ReadonlyArray<() => HarnessSpec> = [];

/** 已登记、使用原版 Java oracle 的能力：oracle 类名 + 单元来源。 */
interface JavaHarness {
    readonly name: string;
    readonly oracle: string;
    /** probe 输入单元；缺省时由 oracle 的 list 子命令枚举。 */
    readonly units?: () => string[];
    readonly input: (unit: string) => unknown;
    readonly key: string;
    /** 原版有、本项目尚未实现的单元（显式缺口，非参照数据缺失）。 */
    readonly oracleMissing?: readonly string[];
}

const JAVA_HARNESSES: ReadonlyArray<() => JavaHarness> = [
    () => ({
        name: "status_effect",
        oracle: "StatusEffectOracle",
        units: statusEffectUnits,
        input: statusEffectInput,
        key: "unit",
    }),
    () => {
        const spec = createItemSummaryHarness();
        return {
            name: spec.name,
            oracle: "ItemSummaryOracle",
            units: spec.units,
            input: spec.input,
            key: spec.key,
            oracleMissing: itemsKnownMissing,
        };
    },
    () => {
        const spec = createEntityShapeHarness();
        return {
            name: spec.name,
            oracle: "EntityShapeOracle",
            input: spec.input,
            key: spec.key,
            oracleMissing: entityKnownMissing,
        };
    },
    () => ({
        name: "entity_attributes",
        oracle: "EntityAttributesOracle",
        // 单元取参考表：Cubium 有、但 oracle 没列的生物（原版没有的）也照样测。
        units: () => [...VANILLA_ENTITY_ATTRIBUTES.keys()],
        input: (unit: string) => ({ entity: unit }),
        key: "entity",
        // 非生物实体（展示实体、船、投射物……）在 Cubium 尚未注册；
        // 运行时改写属性的生物（史莱姆按尺寸、马按随机数）oracle 不给默认值，以参考表为准。
        oracleMissing: [...entityAttributesKnownMissing, ...entityAttributesRuntimeDivergent],
    }),
    () => ({
        name: "mob_category",
        oracle: "MobCategoryOracle",
        input: (unit: string) => ({ category: unit }),
        key: "category",
    }),
    () => ({
        name: "block_properties",
        oracle: "BlockPropertiesOracle",
        input: (unit: string) => ({ block: unit }),
        key: "block",
    }),
    () => ({
        name: "enchantment_table",
        oracle: "EnchantmentExport",
        input: (unit: string) => ({ enchantment: unit, levels: [1, 1] }),
        key: "enchantment",
        oracleMissing: enchantmentKnownMissing,
    }),
    () => ({
        name: "block_state_names",
        oracle: "BlockStateNamesOracle",
        input: (unit: string) => ({ block: unit }),
        key: "block",
    }),
    () => ({
        name: "block_state_ids",
        oracle: "BlockStateIdsOracle",
        input: (unit: string) => ({ block: unit }),
        key: "block",
    }),
    () => ({
        name: "random_sequence",
        oracle: "RandomSequenceOracle",
        input: (unit: string) => ({ unit }),
        key: "unit",
    }),
    () => ({
        name: "command_tree",
        oracle: "CommandTreeOracle",
        input: (unit: string) => ({ unit }),
        key: "unit",
        // 原版有、Cubium 尚未注册的根命令（见 known_missing_commands.ts）。
        oracleMissing: KNOWN_MISSING_COMMANDS,
        // 这些命令不注册会让同一批节点（如 /give 的 item 参数）在两侧处于同一状态。
        oracleArgs: [KNOWN_MISSING_COMMANDS.join(",")],
    }),
    () => ({
        name: "command_argument",
        oracle: "CommandArgumentOracle",
        input: (unit: string) => ({ unit }),
        key: "unit",
    }),
    () => ({
        name: "block_tags",
        oracle: "BlockTagOracle",
        input: (unit: string) => ({ tag: unit }),
        key: "tag",
    }),
    () => ({
        name: "item_tags",
        oracle: "ItemTagOracle",
        input: (unit: string) => ({ tag: unit }),
        key: "tag",
        // 标签成员里的未实现物品：与 item_summary 共用同一份缺口清单。
        memberGaps: { field: "values", missing: itemsKnownMissing },
    }),
    () => ({
        name: "damage_type",
        oracle: "DamageTypeOracle",
        input: (unit: string) => ({ damageType: unit }),
        key: "damageType",
    }),
    () => ({
        name: "biome_climate",
        oracle: "BiomeClimateOracle",
        input: (unit: string) => ({ biome: unit }),
        key: "biome",
    }),
    () => ({
        name: "advancement",
        oracle: "AdvancementOracle",
        input: (unit: string) => ({ advancement: unit }),
        key: "advancement",
    }),
    () => ({
        name: "biome_spawns",
        oracle: "BiomeSpawnsOracle",
        input: (unit: string) => ({ biome: unit }),
        key: "biome",
        // 生成条目与分类都是集合，原版 Java 侧的权重顺序与数据包书写顺序不同，比对前按
        // (type, weight, minCount, maxCount) 排序；分类名同样排序。
        normalise: (value: unknown) => canonicalSpawnCategories(value),
    }),
    () => createRecipeHarness(),
    () => ({
        name: "spawn_rules",
        oracle: "SpawnRulesOracle",
        input: spawnRulesInput,
        key: "entity",
    }),
    () => ({
        name: "spawn_placement",
        oracle: "SpawnPlacementOracle",
        input: spawnPlacementInput,
        key: "entity",
    }),
    () => ({
        name: "loot_table",
        oracle: "LootTableOracle",
        input: (unit: string) => lootTableInput(unit),
        key: "table",
    }),
    () => ({
        name: "loot_table_block",
        oracle: "LootTableBlockOracle",
        input: (unit: string) => lootTableBlockInput(unit),
        key: "table",
    }),
    () => ({
        name: "loot_table_entity",
        oracle: "LootTableEntityOracle",
        input: (unit: string) => lootTableEntityInput(unit),
        key: "table",
        oracleMissing: ENTITY_ORACLE_MISSING,
    }),
    () => ({
        name: "item_components",
        oracle: "ItemComponentsOracle",
        input: (unit: string) => ({ item: unit }),
        key: "item",
        oracleMissing: itemsKnownMissing,
    }),
];

function allDataSpecs(): HarnessSpec[] {
    return DATA_SPECS.map((factory) => factory());
}

/** 跑全部 harness；任一失败即返回非零。 */
export function runAll(): number {
    let failures = 0;
    let total = 0;

    for (const factory of JAVA_HARNESSES) {
        const harness = factory();
        total++;
        try {
            if (runJavaOracle({ ...harness, domain: harness.name }).exitCode !== 0) {
                failures++;
            }
        } catch (error) {
            console.error(`环境问题（${harness.name}）：${(error as Error).message}`);
            failures++;
        }
    }

    for (const spec of allDataSpecs()) {
        total++;
        try {
            if (runSpec(spec).exitCode !== 0) {
                failures++;
            }
        } catch (error) {
            console.error(`环境问题（${spec.name}）：${(error as Error).message}`);
            failures++;
        }
    }

    console.log(failures === 0 ? `\n全部 ${total} 个 harness 通过` : `\n${failures} / ${total} 个 harness 未通过`);
    return failures === 0 ? 0 : 1;
}

function findJavaHarness(name: string): JavaHarness | undefined {
    return JAVA_HARNESSES.map((factory) => factory()).find((harness) => harness.name === name);
}

/** 跑指定名字的 harness。 */
export async function runNamed(name: string): Promise<number> {
    if (name === "text_encode") {
        // text_encode 是手写 harness（比状态全集与摘要两件事），单独入口。
        const { main } = await import("../test_text_encode_parity.ts");
        return main(["--no-build"]);
    }

    const javaHarness = findJavaHarness(name);
    if (javaHarness !== undefined) {
        try {
            return runJavaOracle({ ...javaHarness, domain: javaHarness.name }).exitCode;
        } catch (error) {
            console.error(`环境问题（${name}）：${(error as Error).message}`);
            return 2;
        }
    }

    const spec = allDataSpecs().find((candidate) => candidate.name === name);
    if (spec === undefined) {
        console.error(`未知 harness：${name}`);
        return 2;
    }
    try {
        return runSpec(spec).exitCode;
    } catch (error) {
        console.error(`环境问题（${name}）：${(error as Error).message}`);
        return 2;
    }
}
