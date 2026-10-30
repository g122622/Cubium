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
 * index.ts — 全部差分能力的声明表。
 *
 * 这张表是「能力元数据」的唯一来源：名字、探针域名、oracle 类名、比对口径与已知缺口。
 * 声明是**纯元数据**：加载时不读数据、不启动进程，主进程因此可以在不构建、不跑 Java 的
 * 前提下列出能力、算依赖、决定「今天该跑哪几个」。
 *
 * 两种参照实现：
 *   - `oracle`：原版 Java 类（权威、逐字节可比）；
 *   - `reference`：TS 计算的参照值（没有 Java 侧的纯数据能力，例如 minecraft-data 作参照）。
 *
 * 战利品表族（`loot_table_entity` / `loot_container`）的单元是「表 × 场景」：oracle 枚举表，
 * 能力自己按场景展开，因此 `units` 是 computed，展开逻辑放在 `beforeUnits` 之后的闭包里。
 */

import type { CompareTransform } from "../core/comparison.ts";
import type { HarnessContext, HarnessDeclaration } from "../core/types.ts";
import { KNOWN_MISSING as enchantmentKnownMissing } from "./enchantment_table.ts";
import { KNOWN_MISSING as entityKnownMissing } from "./entity_shape.ts";
import {
    ENTITY_ORACLE_MISSING,
    ENTITY_SCENARIOS,
    lootTableBlockInput,
    lootTableEntityInput,
    lootTableEntityUnit,
    lootTableInput,
} from "./loot_context.ts";
import { KNOWN_MISSING as blockToolsKnownMissing } from "./known_missing_block_tools.ts";
import { KNOWN_GAPS as lootContainerKnownGaps, LOOT_CONTAINER_SCENARIO_NAMES, lootContainerInput, lootContainerUnit } from "./loot_container.ts";
import { KNOWN_MISSING as itemsKnownMissing } from "./known_missing_items.ts";
import {
    dropProjectOnlyCommands,
    KNOWN_MISSING_COMMANDS,
    PROJECT_ONLY_COMMANDS,
} from "./known_missing_commands.ts";
import { itemSummaryReference, itemSummaryUnits } from "./item_summary.ts";
import { recipeDeclaration } from "./recipe.ts";
import { spawnPlacementInput } from "./spawn_placement.ts";
import { spawnRulesInput } from "./spawn_rules.ts";
import { statusEffectInput, statusEffectUnits } from "./status_effect.ts";
import { structureDensityDeclaration } from "./structure_density.ts";
import { structureStartsDeclaration } from "./structure_starts.ts";
import { textEncodeDeclaration } from "./text_encode.ts";
import { KNOWN_MISSING as entityAttributesKnownMissing } from "./known_missing_entity_attributes.ts";
import {
    RUNTIME_DIVERGENT as entityAttributesRuntimeDivergent,
    VANILLA_ENTITY_ATTRIBUTES,
} from "./entity_attributes_reference.ts";

/**
 * biome_spawns 的规范化：每个分类的生成条目按 (type, weight, minCount, maxCount) 排序。
 *
 * 原版把 spawners 存成 WeightedList，`unwrap()` 的顺序与数据包书写顺序无关；集合语义下
 * 条目的先后没有意义，比对前统一排序。浮点字段按 f32 位模式比对：JSON 解析后是 double，
 * 用 Math.fround 还原成两侧共有的值，避免 0.1 与 0.10000000149011612 被当成不一致。
 */
function canonicalSpawnCategories(value: unknown): unknown {
    if (value === null || typeof value !== "object") {
        return value;
    }
    const record = value as Record<string, unknown>;
    if (record["categories"] === undefined || record["categories"] === null) {
        return value;
    }
    const probability = typeof record["creature_spawn_probability"] === "number"
        ? Math.fround(record["creature_spawn_probability"] as number)
        : record["creature_spawn_probability"];
    const categories = record["categories"] as Record<string, unknown>;
    const sortedCategories: Record<string, unknown> = {};
    for (const name of Object.keys(categories).sort()) {
        const category = categories[name] as Record<string, unknown>;
        const spawns = Array.isArray(category["spawns"]) ? [...category["spawns"]] : [];
        spawns.sort((left, right) => compareEntries(left, right));
        sortedCategories[name] = { ...category, spawns };
    }
    return { ...record, categories: sortedCategories, creature_spawn_probability: probability };
}

function compareEntries(left: unknown, right: unknown): number {
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
}

/**
 * 浮点比较器：两种写法都被接受。
 *
 * 两侧的浮点协议不同：Gson 写 float 的最短表示（f32 0.485F → `0.485`），nlohmann 写
 * double 的最宽表示（`0.48500001430511475`）。它们是**同一个 f32**，但文本不同；而规范化
 * 又不能改动参照侧的数值（运行层会拒绝非等价变换）。所以按两条规则接受：
 *
 *   1. 被测值等于参照值在某个精度下四舍五入的结果（覆盖两侧都是同一精度、只差 ulp 的情形）；
 *   2. 或者被测值**升高到双精度后再舍入**等于参照值（覆盖「参照是 f32 的短写法、被测是
 *      该 f32 的精确双精度值」的情形）。
 *
 * 规则 1 保持参照侧原样，因此是等价变换；规则 2 对参照侧是恒等映射，同样不破坏等价性。
 * 真正的公式偏差远大于 ulp，不会被这两条规则掩盖。
 */
function floatComparer(): CompareTransform {
    const round = (value: number, precision: number): number => {
        const scale = Math.pow(10, precision);
        return Math.round(value * scale) / scale;
    };
    const closes = (expected: number, actual: number): boolean => {
        // 1) 被测值舍入到参照的精度后相等：两侧只是写法不同（f32 短写法 vs 双精度展开）。
        const short = decimalsOf(expected);
        if (short > 0 && short < 9 && (round(actual, short) === expected || round(Math.fround(actual), short) === expected)) {
            return true;
        }
        // 2) 容差比较：JVM 与 clang 对 f32 中间结果的求值顺序不同，末位会差 1~2 ulp；
        //    floatPrint 的最短表示在「恰好跨过进位边界」的值上会放大成第 8 位小数上的差异。
        //    公式本身是 f32 运算，真实偏差远大于 1e-6 相对误差。
        return Math.abs(actual - expected) <= Math.abs(expected) * 1e-6 + 1e-7;
    };
    const compare = (expected: unknown, actual: unknown): unknown => {
        if (typeof expected === "number" && typeof actual === "number") {
            return closes(expected, actual) ? expected : actual;
        }
        if (Array.isArray(expected) && Array.isArray(actual)) {
            return actual.map((item: unknown, index: number) => compare(expected[index], item));
        }
        if (expected !== null && actual !== null && typeof expected === "object" && typeof actual === "object") {
            const result: Record<string, unknown> = { ...(actual as Record<string, unknown>) };
            for (const [key, value] of Object.entries(expected as Record<string, unknown>)) {
                result[key] = compare(value, (actual as Record<string, unknown>)[key]);
            }
            return result;
        }
        return actual;
    };
    return compare;
}

function decimalPrecisions(value: number): number[] {
    const own = decimalsOf(value);
    const precisions: number[] = [];
    for (let precision = Math.min(17, own); precision <= Math.min(17, Math.max(4, own) + 3); precision++) {
        precisions.push(precision);
    }
    return [...new Set(precisions)];
}

/** 一个数写成十进制时小数点后的位数；指数写法按全精度处理。 */
function decimalsOf(value: number): number {
    const text = String(value);
    if (text.includes("e") || text.includes("E")) {
        return 17;
    }
    const dot = text.indexOf(".");
    return dot < 0 ? 0 : text.length - dot - 1;
}

/**
 * 「表 × 场景」类能力的单元展开：oracle 枚举表，能力按场景展开，两侧按「表#场景」对齐。
 *
 * 场景集合在声明期固定（不读数据），因此主进程拿到的是「这个能力有几种场景」的元数据，
 * 实际表名由执行进程在 `beforeUnits` 里问 oracle。为了让扩容后方便，主进程只看 `probe`。
 */
function scenarioUnits(
    oracle: string,
    scenarios: readonly string[],
    unit: (table: string, scenario: string) => string,
    allowed: (unit: string) => boolean = () => true,
) {
    let tables: string[] = [];
    return {
        beforeUnits: (context: { listOracleUnits: (name: string, domain: string) => string[] }): void => {
            tables = context.listOracleUnits(oracle, domainOf(oracle));
        },
        units: (): string[] => {
            const units: string[] = [];
            for (const table of tables) {
                for (const scenario of scenarios) {
                    const candidate = unit(table, scenario);
                    if (allowed(candidate)) {
                        units.push(candidate);
                    }
                }
            }
            return units;
        },
    };
}

/** oracle 类名 → 产物目录名（与能力名一致的只用于产物路径，不参与比对）。 */
function domainOf(oracle: string): string {
    return oracle.replace(/Oracle$/, "").toLowerCase();
}

/**
 * 实体战利品的已知缺口：这些生物在 `#player` 场景（击杀者是玩家、looting 3）下的掉落。
 *
 * Cubium 多出了「按击杀者附魔等级加投」的那部分（蜘蛛眼、幻翼膜、吹风机碎片、湿海绵……），
 * 而同一生物按 `#default`（击杀者是僵尸）比对反而一致。也就是说差异只出现在玩家击杀的
 * 上下文里，具体是 looting 修饰器的求值还是 `killed_by_player` 条件，尚未定位。
 * 先登记，不阻塞其余 5000 多单元的比对。
 *
 * TODO: 【待排查】核对玩家击杀上下文的 looting 修饰器倍率与条件求值；修好后整段删除本登记。
 */
const ENTITY_LOOT_KNOWN_GAPS: readonly string[] = [
    "entities/blaze",
    "entities/bogged",
    "entities/breeze",
    "entities/cave_spider",
    "entities/elder_guardian",
    "entities/evoker",
    "entities/guardian",
    "entities/parched",
    "entities/phantom",
    "entities/rabbit",
    "entities/spider",
    "entities/stray",
    "entities/vindicator",
    "entities/zombie_nautilus",
];

/** 实体战利品：单元键是「表#场景」。 */
function lootTableEntityDeclaration(): HarnessDeclaration {
    const scenarioOf = (unit: string): { table: string; scenario: string } => {
        const hash = unit.lastIndexOf("#");
        return { table: unit.slice(0, hash), scenario: unit.slice(hash + 1) };
    };
    const expanded = scenarioUnits(
        "LootTableEntityOracle",
        ENTITY_SCENARIOS.map((scenario) => scenario.name),
        lootTableEntityUnit,
        // 原版建不出这些表的被击杀实体（creaking / mannequin）：整张表不参与比较。
        (unit) => !ENTITY_ORACLE_MISSING.includes(scenarioOf(unit).table),
    );
    return {
        name: "loot_table_entity",
        probe: "loot_table_entity",
        oracle: "LootTableEntityOracle",
        key: "table",
        units: { kind: "computed", units: expanded.units },
        // oracle 要在 java.io.tmpdir 里建原版世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
        input: (unit: string) => {
            const { table, scenario } = scenarioOf(unit);
            return lootTableEntityInput(table, scenario);
        },
        beforeUnits: expanded.beforeUnits,
        knownDifferences: ENTITY_LOOT_KNOWN_GAPS.map((table) => ({
            key: `minecraft:${table}#player`,
            field: "samples",
            reason: "玩家击杀上下文下的掉落差异（见 ENTITY_LOOT_KNOWN_GAPS 的说明）",
        })),
    };
}

/** 容器战利品：单元键是「表#场景」，场景集合由 loot_container.ts 定义。 */
function lootContainerDeclaration(): HarnessDeclaration {
    const expanded = scenarioUnits("LootContainerOracle", LOOT_CONTAINER_SCENARIO_NAMES, lootContainerUnit);
    return {
        name: "loot_container",
        probe: "loot_container",
        oracle: "LootContainerOracle",
        key: "table",
        units: { kind: "computed", units: expanded.units },
        // oracle 要在 java.io.tmpdir 里建原版世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
        input: (unit: string) => lootContainerInput(unit),
        // 掉落物里的物品顺序来自掷骰后的合并顺序，不承载语义；按规范序对齐后再比。
        unorderedArrays: ["drop/items"],
        beforeUnits: expanded.beforeUnits,
        // 已知缺口按单元展开：表和场景在声明期不可知，只能对「所有单元」放行这两段字段。
        knownDifferences: containerKnownDifferences(),
    };
}

/**
 * 把 loot_container 的两条已知缺口（slots / drop）展开成「对全部单元放行对应字段」。
 *
 * 表名要问 oracle 才知道，声明期不可知，因此用通配键 `*` 表达「所有单元」。
 */
function containerKnownDifferences(): NonNullable<HarnessDeclaration["knownDifferences"]> {
    return lootContainerKnownGaps.map((gap) => ({ key: "*", field: gap.field, reason: gap.reason }));
}

/**
 * 物品摘要：参照实现是 minecraft-data（离线数据包，不需要 JDK 与 jar）。
 *
 * minecraft-data 的加载推迟到 `beforeUnits`：主进程加载声明表时只读元数据，不能为了
 * 「列一下能力」去解析一个几十 MB 的 npm 数据包；执行进程第一次用到时才读，并记入
 * 运行期依赖（模块追踪会看到 data/vanilla.ts 被加载）。
 */
function itemSummaryDeclaration(): HarnessDeclaration {
    let data: Parameters<typeof itemSummaryUnits>[0] | undefined;
    const requireData = (): Parameters<typeof itemSummaryUnits>[0] => {
        if (data === undefined) {
            throw new Error("item_summary：参照数据尚未加载");
        }
        return data;
    };
    return {
        name: "item_summary",
        probe: "item_summary",
        key: "item",
        units: { kind: "computed", units: () => itemSummaryUnits(requireData()) },
        input: (unit: string) => ({ item: unit }),
        reference: (unit: string) => itemSummaryReference(requireData(), unit),
        beforeUnits: (context: HarnessContext): void => {
            const loader = context.minecraftData as (() => unknown) | undefined;
            if (loader === undefined) {
                throw new Error("item_summary：运行上下文没有提供 minecraft-data 加载器");
            }
            data = loader() as Parameters<typeof itemSummaryUnits>[0];
        },
        oracleMissing: itemsKnownMissing,
    } satisfies HarnessDeclaration;
}

/** 全部能力声明（顺序只影响展示；实际调度按上次耗时排序）。 */
export const HARNESSES: readonly HarnessDeclaration[] = [
    textEncodeDeclaration(),
    { name: "status_effect", probe: "status_effect", oracle: "StatusEffectOracle", key: "unit",
        units: { kind: "computed", units: statusEffectUnits }, input: statusEffectInput },
    itemSummaryDeclaration(),
    { name: "entity_shape", probe: "entity_shape", oracle: "EntityShapeOracle", key: "entity", units: { kind: "listing" },
        input: (unit) => ({ entity: unit }), oracleMissing: entityKnownMissing },
    { name: "entity_attributes", probe: "entity_attributes", oracle: "EntityAttributesOracle", key: "entity",
        units: { kind: "computed", units: () => [...VANILLA_ENTITY_ATTRIBUTES.keys()] },
        input: (unit) => ({ entity: unit }),
        // 非生物实体（展示实体、船、投射物……）在 Cubium 尚未注册；运行时改写属性的生物
        // （史莱姆按尺寸、马按随机数）oracle 不给默认值，以参考表为准。
        oracleMissing: [...entityAttributesKnownMissing, ...entityAttributesRuntimeDivergent] },
    { name: "mob_category", probe: "mob_category", oracle: "MobCategoryOracle", key: "category", units: { kind: "listing" },
        input: (unit) => ({ category: unit }) },
    { name: "block_properties", probe: "block_properties", oracle: "BlockPropertiesOracle", key: "block", units: { kind: "listing" },
        input: (unit) => ({ block: unit }) },
    { name: "block_experience", probe: "block_experience", oracle: "BlockExperienceOracle", key: "block", units: { kind: "listing" },
        input: (unit) => ({ block: unit }) },
    { name: "item_attribute_modifiers", probe: "item_attribute_modifiers", oracle: "ItemAttributeModifiersOracle", key: "item",
        units: { kind: "listing" }, input: (unit) => ({ item: unit }), oracleMissing: itemsKnownMissing },
    { name: "damage_reduction", probe: "damage_reduction", oracle: "DamageReductionOracle", key: "unit", units: { kind: "listing" },
        input: (unit) => JSON.parse(unit) as Record<string, unknown>,
        // 两侧末位相差 1 ulp（JVM 与 clang 对 f32 中间结果的求值顺序不同），按固定小数位比对。
        normalise: floatComparer() },
    { name: "gamerule", probe: "gamerule", oracle: "GameRuleOracle", key: "rule", units: { kind: "listing" },
        input: (unit) => ({ rule: unit }) },
    { name: "armor_material", probe: "armor_material", oracle: "ArmorMaterialOracle", key: "material", units: { kind: "listing" },
        input: (unit) => ({ material: unit }), normalise: floatComparer() },
    { name: "tool_material", probe: "tool_material", oracle: "ToolMaterialOracle", key: "material", units: { kind: "listing" },
        input: (unit) => ({ material: unit }), normalise: floatComparer() },
    { name: "enchantment_effects", probe: "enchantment_effects", oracle: "EnchantmentEffectsOracle", key: "enchantment",
        units: { kind: "listing" }, input: (unit) => ({ enchantment: unit }), oracleMissing: enchantmentKnownMissing,
        normalise: floatComparer(), unorderedArrays: ["levels/attributes"] },
    { name: "attribute_range", probe: "attribute_range", oracle: "AttributeRangeOracle", key: "attribute", units: { kind: "listing" },
        input: (unit) => ({ attribute: unit }),
        // 项目自研的非原版属性：原版注册表里没有，取值范围无从比对，必须显式登记。
        projectOnly: {
            field: "attribute",
            extra: ["horse.jump_strength"],
            drop: (record, matches) => (matches(String(record["attribute"])) ? {} : record),
        } },
    { name: "mob_experience", probe: "mob_experience", oracle: "MobExperienceOracle", key: "entity", units: { kind: "listing" },
        input: (unit) => ({ entity: unit }), oracleMissing: entityKnownMissing },
    { name: "block_destroy_progress", probe: "block_destroy_progress", oracle: "BlockDestroyProgressOracle", key: "block",
        units: { kind: "listing" }, input: (unit) => ({ block: unit }), oracleMissing: blockToolsKnownMissing },
    { name: "enchantment_table", probe: "enchantment_table", oracle: "EnchantmentExport", key: "enchantment", units: { kind: "listing" },
        input: (unit) => ({ enchantment: unit, levels: [1, 1] }), oracleMissing: enchantmentKnownMissing },
    { name: "block_state_names", probe: "block_state_names", oracle: "BlockStateNamesOracle", key: "block", units: { kind: "listing" },
        input: (unit) => ({ block: unit }) },
    { name: "block_state_ids", probe: "block_state_ids", oracle: "BlockStateIdsOracle", key: "block", units: { kind: "listing" },
        input: (unit) => ({ block: unit }) },
    { name: "random_sequence", probe: "random_sequence", oracle: "RandomSequenceOracle", key: "unit", units: { kind: "listing" },
        input: (unit) => ({ unit }) },
    { name: "command_tree", probe: "command_tree", oracle: "CommandTreeOracle", key: "unit", units: { kind: "listing" },
        input: (unit) => ({ unit }),
        // 原版有、Cubium 尚未注册的根命令（见 known_missing_commands.ts）。
        oracleMissing: KNOWN_MISSING_COMMANDS,
        // 同时让 oracle 在注册阶段就跳过它们：这些命令会带来参数类型注册等副作用，留在
        // 参照侧会让其余命令的同一批节点处于不同状态（比对结果变成上千条伪差异）。
        oracleArgs: [KNOWN_MISSING_COMMANDS.join(",")],
        // 本项目自研的调试命令（原版没有对应项），不属于「未对齐」。
        projectOnly: {
            field: "tree",
            extra: PROJECT_ONLY_COMMANDS,
            drop: (record, matches) => dropProjectOnlyCommands(record, matches),
        },
        // 自研命令 /panel 会改变整棵树的编码字节（wire），使两侧逐字节比对无意义。
        // TODO: 把自研命令从命令树的「可编码」集合里摘掉，让 wire 恢复可比。
        knownDifferences: [{
            key: "tree",
            field: "wire",
            reason: "wire 是整棵树的编码字节：含自研命令 /panel，两侧节点表不同，逐字节必然不同",
        }] },
    { name: "command_argument", probe: "command_argument", oracle: "CommandArgumentOracle", key: "unit", units: { kind: "listing" },
        input: (unit) => ({ unit }) },
    { name: "block_tags", probe: "block_tags", oracle: "BlockTagOracle", key: "tag", units: { kind: "listing" },
        input: (unit) => ({ tag: unit }) },
    { name: "item_tags", probe: "item_tags", oracle: "ItemTagOracle", key: "tag", units: { kind: "listing" },
        input: (unit) => ({ tag: unit }), memberGaps: { field: "values", missing: itemsKnownMissing } },
    { name: "damage_type", probe: "damage_type", oracle: "DamageTypeOracle", key: "damageType", units: { kind: "listing" },
        input: (unit) => ({ damageType: unit }) },
    { name: "biome_climate", probe: "biome_climate", oracle: "BiomeClimateOracle", key: "biome", units: { kind: "listing" },
        input: (unit) => ({ biome: unit }) },
    { name: "advancement", probe: "advancement", oracle: "AdvancementOracle", key: "advancement", units: { kind: "listing" },
        input: (unit) => ({ advancement: unit }) },
    { name: "biome_spawns", probe: "biome_spawns", oracle: "BiomeSpawnsOracle", key: "biome", units: { kind: "listing" },
        input: (unit) => ({ biome: unit }), normalise: canonicalSpawnCategories },
    recipeDeclaration(),
    { name: "spawn_rules", probe: "spawn_rules", oracle: "SpawnRulesOracle", key: "entity", units: { kind: "listing" },
        input: spawnRulesInput },
    { name: "spawn_placement", probe: "spawn_placement", oracle: "SpawnPlacementOracle", key: "entity", units: { kind: "listing" },
        input: spawnPlacementInput },
    { name: "loot_table", probe: "loot_table", oracle: "LootTableOracle", key: "table", units: { kind: "listing" },
        input: (unit) => lootTableInput(unit) },
    { name: "loot_table_block", probe: "loot_table_block", oracle: "LootTableBlockOracle", key: "table", units: { kind: "listing" },
        input: (unit) => lootTableBlockInput(unit) },
    lootTableEntityDeclaration(),
    lootContainerDeclaration(),
    { name: "item_components", probe: "item_components", oracle: "ItemComponentsOracle", key: "item", units: { kind: "listing" },
        input: (unit) => ({ item: unit }), oracleMissing: itemsKnownMissing },
    structureDensityDeclaration(),
    structureStartsDeclaration(),
];

/** 按名字查找能力声明；未知名字返回 undefined。 */
export function findHarness(name: string): HarnessDeclaration | undefined {
    return HARNESSES.find((harness) => harness.name === name);
}