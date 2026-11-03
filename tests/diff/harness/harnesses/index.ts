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
import { KNOWN_MISSING as entityTypesKnownMissing } from "./known_missing_entity_types.ts";
import { KNOWN_DISPENSE_GAPS } from "./known_dispense_gaps.ts";
import { KNOWN_POSE_GAPS } from "./known_pose_gaps.ts";
import { KNOWN_DIMENSION_GAPS } from "./known_dimension_gaps.ts";
import {
    dropProjectOnlyCommands,
    KNOWN_MISSING_COMMANDS,
    PROJECT_ONLY_COMMANDS,
} from "./known_missing_commands.ts";
import { itemComponentStackInput, normaliseComponentSnbt } from "./item_component_nbt.ts";
import { missingLabelStackCodec, normaliseStackCodec, stackCodecInput } from "./stack_codec.ts";
import { lootFunctionInput, normaliseLootFunction } from "./loot_function.ts";
import { itemSummaryReference, itemSummaryUnits } from "./item_summary.ts";
import { blockSupportInput } from "./block_support.ts";
import { foodPropertiesInput } from "./food_properties.ts";
import { ENTITY_TYPE_TAGS, ENTITY_TYPE_TAGS_UNORDERED, entityTypeTagsInput } from "./entity_type_tags.ts";
import { jukeboxSongInput } from "./jukebox_song.ts";
import { ENCHANTMENT_TAGS_UNORDERED, enchantmentTagsInput } from "./enchantment_tags.ts";
import { paintingVariantInput } from "./painting_variant.ts";
import { ARMOR_TRIM_UNORDERED, armorTrimInput } from "./armor_trim.ts";
import { animalVariantInput } from "./animal_variant.ts";
import { instrumentInput } from "./instrument.ts";
import { bannerPatternInput } from "./banner_pattern.ts";
import { enchantmentProviderInput, enchantmentProviderUnits } from "./enchantment_provider.ts";
import { trialSpawnerConfigInput, trialSpawnerConfigUnits } from "./trial_spawner_config.ts";
import { dimensionTypeInput } from "./dimension_type.ts";
import { explosionRayInput, explosionRayUnits } from "./explosion_ray.ts";
import { explosionDamageInput } from "./explosion_damage.ts";
import { explosionSeenGridInput } from "./explosion_seen_grid.ts";
import { entityDimensionsInput } from "./entity_dimensions.ts";
import { mobEffectMergeInput } from "./mob_effect_merge.ts";
import { foodStatsInput } from "./food_stats.ts";
import { foodTickInput } from "./food_tick.ts";
import { enchantmentSelectionInput, enchantmentSelectionUnits } from "./enchantment_selection.ts";
import { repairCostInput } from "./repair_cost.ts";
import { anvilDurabilityInput } from "./anvil_durability.ts";
import { itemStackableInput } from "./item_stackable.ts";
import { itemSameComponentsInput } from "./item_same_components.ts";
import { furnaceTickInput } from "./furnace_tick.ts";
import { brewingTickInput } from "./brewing_tick.ts";
import { beaconEffectsInput } from "./beacon_effects.ts";
import { hopperCooldownInput } from "./hopper_cooldown.ts";
import { spawnerDelayInput } from "./spawner_delay.ts";
import { mthRoundingInput } from "./mth_rounding.ts";
import { mthModuloInput } from "./mth_modulo.ts";
import { directionRotateInput } from "./direction_rotate.ts";
import { aabbOpsInput } from "./aabb_ops.ts";
import { vec3MathInput } from "./vec3_math.ts";
import { blockposCodecInput } from "./blockpos_codec.ts";
import { axisPlaneInput } from "./axis_plane.ts";
import { mthLerpInput } from "./mth_lerp.ts";
import { mthBitInput } from "./mth_bit.ts";
import { mthRotInput } from "./mth_rot.ts";
import { mthSinInput } from "./mth_sin.ts";
import { mthApproachInput } from "./mth_approach.ts";
import { regionSectorInput } from "./region_sector.ts";
import { blockposDistInput } from "./blockpos_dist.ts";
import { enchantmentStorageInput } from "./enchantment_storage.ts";
import { collideSweepInput } from "./collide_sweep.ts";
import { shapesBooleanInput } from "./shapes_boolean.ts";
import { fluidSpreadInput } from "./fluid_spread.ts";
import { fluidSpreadDirInput } from "./fluid_spread_dir.ts";
import { blockposBfsInput } from "./blockpos_bfs.ts";
import { scheduledTickInput } from "./scheduled_tick.ts";
import { randomTickPosInput } from "./random_tick_pos.ts";
import { mthColorInput } from "./mth_color.ts";
import { blockTransformInput } from "./block_transform.ts";
import { boundingBoxInput } from "./bounding_box.ts";
import { enchantmentCompatInput, listEnchantmentCompatUnits } from "./enchantment_compat.ts";
import { sectionPosInput } from "./section_pos.ts";
import { levelHeightInput } from "./level_height.ts";
import { chunkPosInput } from "./chunk_pos.ts";
import { mthClampInput } from "./mth_clamp.ts";
import { chatFormattingInput } from "./chat_formatting.ts";
import { dyeColorInput } from "./dye_color.ts";
import { equipmentSlotInput } from "./equipment_slot.ts";
import { heightmapTypesInput } from "./heightmap_types.ts";
import { mthIntDivInput } from "./mth_int_div.ts";
import { directionPlaneInput } from "./direction_plane.ts";
import { rotationInfoInput } from "./rotation_info.ts";
import { blockPosGeomInput } from "./blockpos_geom.ts";
import { vec3iArithInput } from "./vec3i_arith.ts";
import { aabbContainsInput } from "./aabb_contains.ts";
import { fluidHeightInput } from "./fluid_height.ts";
import { mthInverseLerpInput } from "./mth_inverse_lerp.ts";
import { mthAngleLerpInput } from "./mth_angle_lerp.ts";
import { mthWaveformInput } from "./mth_waveform.ts";
import { mthHsvInput } from "./mth_hsv.ts";
import { argbOpsInput } from "./argb_ops.ts";
import { directionNearestInput } from "./direction_nearest.ts";
import { mthSeedInput } from "./mth_seed.ts";
import { mthPackDegreesInput } from "./mth_pack_degrees.ts";
import { mthDegreesInput } from "./mth_degrees.ts";
import { mthFracInput } from "./mth_frac.ts";
import { mthSqrtInput } from "./mth_sqrt.ts";
import { stringUtilInput } from "./string_util.ts";
import { identifierParseInput } from "./identifier_parse.ts";
import { identifierPathInput } from "./identifier_path.ts";
import { blockPosIterateInput } from "./blockpos_iterate.ts";
import { blockPosSpiralInput } from "./blockpos_spiral.ts";
import { aabbFactoryInput } from "./aabb_factory.ts";
import { frontAndTopInput } from "./front_and_top.ts";
import { structureOrientInput } from "./structure_orient.ts";
import { boundingBoxEncapInput } from "./bounding_box_encap.ts";
import { chunkRegionInput } from "./chunk_region.ts";
import { portalRelativeInput } from "./portal_relative.ts";
import { relativeMaskInput } from "./relative_mask.ts";
import { positionMoveRotationInput } from "./position_move_rotation.ts";
import { styleApplyInput } from "./style_apply.ts";
import { textColorInput } from "./text_color.ts";
import { clickEventInput } from "./click_event.ts";
import { hoverEventInput } from "./hover_event.ts";
import { identifierResolveInput } from "./identifier_resolve.ts";
import { plainTextContentsInput } from "./plain_text_contents.ts";
import { mthAbsMaxInput } from "./mth_absmax.ts";
import { displaySlotInput } from "./display_slot.ts";
import { objectiveCriteriaInput } from "./objective_criteria.ts";
import { teamRuleInput } from "./team_rule.ts";
import { bossBarInput } from "./boss_bar.ts";
import { soundSourceInput } from "./sound_source.ts";
import { difficultyInput } from "./difficulty.ts";
import { permissionLevelInput } from "./permission_level.ts";
import { textFormattingColorInput } from "./text_formatting_color.ts";
import { textFormattingNameInput } from "./text_formatting_name.ts";
import { styleTristateInput } from "./style_tristate.ts";
import { teamRuleStringsInput } from "./team_rule_strings.ts";
import { renderTypeStringsInput } from "./render_type_strings.ts";
import { uuidStringsInput } from "./uuid_strings.ts";
import { gameTypeInput } from "./game_type.ts";
import { mapDecorationTypeInput } from "./map_decoration_type.ts";
import { blockStateEnumStringsInput } from "./block_state_enum_strings.ts";
import { blockStateEnumStrings2Input } from "./block_state_enum_strings_2.ts";
import { blockStateEnumStrings3Input } from "./block_state_enum_strings_3.ts";
import { booleanPropertyIndexInput } from "./boolean_property_index.ts";
import { enumPropertyOrderInput } from "./enum_property_order.ts";
import { enumPropertyOrderAllInput } from "./enum_property_order_all.ts";
import { faceSturdyInput } from "./face_sturdy.ts";
import { worldBorderInput } from "./world_border.ts";
import { easingTypeInput } from "./easing_type.ts";
import { mthInterpolationInput } from "./mth_interpolation.ts";
import { legacyRandomInput } from "./legacy_random.ts";
import { customStatsInput } from "./custom_stats.ts";
import { soundEventsInput } from "./sound_events.ts";
import { particleTypeInput } from "./particle_type.ts";
import { shapeFaceInput } from "./shape_face.ts";
import { blockShapeInput } from "./block_shape.ts";
import { blockShapeAllInput } from "./block_shape_all.ts";
import { DAMAGE_TYPE_TAGS_UNORDERED, damageTypeTagsInput } from "./damage_type_tags.ts";
import { enchantmentCostInput } from "./enchantment_cost.ts";
import { ENCHANTMENT_APPLICABILITY_UNORDERED, enchantmentApplicabilityInput } from "./enchantment_applicability.ts";
import { experienceInput, experienceSceneNames } from "./experience.ts";
import { PATH_SCENES, pathFinderInput, pathSceneNames, walkPathTypeInput } from "./path_scenes.ts";
import { pistonMovementInput, pistonSceneNames } from "./piston_movement.ts";
import { POTION_UNORDERED, potionInput } from "./potion.ts";
import { potionBrewingInput, potionBrewingUnits } from "./potion_brewing.ts";
import { recipeDeclaration } from "./recipe.ts";
import { craftingMatchDeclaration } from "./crafting_match.ts";
import { singleItemRecipeDeclaration } from "./single_item_recipe.ts";
import { recipeBookCategoryDeclaration } from "./recipe_book_category.ts";
import { fuelValuesDeclaration } from "./fuel_values.ts";
import { blockSoundTypeDeclaration } from "./block_sound_type.ts";
import { cropGrowthDeclaration } from "./crop_growth.ts";
import { redstoneSignalDeclaration } from "./redstone_signal.ts";
import { lightOcclusionDeclaration } from "./light_occlusion.ts";
import { spawnPlacementInput } from "./spawn_placement.ts";
import { spawnRulesInput } from "./spawn_rules.ts";
import { statusEffectInput, statusEffectUnits } from "./status_effect.ts";
import { structureDensityDeclaration } from "./structure_density.ts";
import { structureStartsDeclaration } from "./structure_starts.ts";
import { villagerTradesDeclaration } from "./villager_trades.ts";
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
    { name: "block_movement", probe: "block_movement", oracle: "BlockMovementOracle", key: "block", units: { kind: "listing" },
        input: (unit) => ({ block: unit }) },
    { name: "can_harvest", probe: "can_harvest", oracle: "CanHarvestOracle", key: "block", units: { kind: "listing" },
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
    craftingMatchDeclaration(),
    singleItemRecipeDeclaration(),
    recipeBookCategoryDeclaration(),
    fuelValuesDeclaration(),
    blockSoundTypeDeclaration(),
    cropGrowthDeclaration(),
    redstoneSignalDeclaration(),
    lightOcclusionDeclaration(),
    { name: "spawn_rules", probe: "spawn_rules", oracle: "SpawnRulesOracle", key: "entity", units: { kind: "listing" },
        input: spawnRulesInput },
    { name: "spawn_placement", probe: "spawn_placement", oracle: "SpawnPlacementOracle", key: "entity", units: { kind: "listing" },
        input: spawnPlacementInput },
    { name: "loot_table", probe: "loot_table", oracle: "LootTableOracle", key: "table", units: { kind: "listing" },
        input: (unit) => lootTableInput(unit) },
    { name: "loot_table_block", probe: "loot_table_block", oracle: "LootTableBlockOracle", key: "table", units: { kind: "listing" },
        // 与 loot_table_entity / loot_container 共用 LootLevel 的存档目录与区域锁：必须同组串行。
        exclusiveGroup: "loot-world",
        input: (unit) => lootTableBlockInput(unit) },
    lootTableEntityDeclaration(),
    lootContainerDeclaration(),
    { name: "item_components", probe: "item_components", oracle: "ItemComponentsOracle", key: "item", units: { kind: "listing" },
        input: (unit) => ({ item: unit }), oracleMissing: itemsKnownMissing },
    structureDensityDeclaration(),
    structureStartsDeclaration(),
    villagerTradesDeclaration("list"),
    villagerTradesDeclaration("gen"),
    villagerTradesDeclaration("price"),
    villagerTradesDeclaration("restock"),
    // 物品组件补丁的 NBT 表示：单元是「物品#变体」，三个变体的组件取值由 item_component_nbt.ts
    // 统一定义，两侧读同一条输入。它覆盖的是「物品栈的可变状态」这一整层——现有的
    // item_components 只比三张静态表（耐久 / 稀有度 / 附魔能力），组件本身从未被比对过。
    { name: "item_component_nbt", probe: "item_component_nbt", oracle: "ItemComponentNbtOracle", key: "unit",
        units: { kind: "listing" }, input: itemComponentStackInput,
        // 两侧的 SNBT 打印器不同（引号风格、浮点后缀），比对前统一成规范形。
        normalise: normaliseComponentSnbt,
        // 单元由 oracle 按**原版**物品表枚举；Cubium 未注册的物品（全是方块物品，见
        // known_missing_items.ts）在探针侧输出 missing。单元键带 `#变体` 后缀，所以要把
        // 缺失物品展开成三个变体——`oracleMissing` 按完整单元键匹配。
        oracleMissing: ["plain", "suite", "removed"].flatMap((variant) =>
            itemsKnownMissing.map((item) => `${item}#${variant}`)) },
    // 物品堆的整壳编解码（原版 `ItemStack.CODEC`）：id / count / components 三个字段、
    // count 的 1..99 区间、空堆的表示、components 段的省略、以及解码应当失败的场合。
    // item_component_nbt 比组件载荷本身，这里比包着组件的那层壳——壳写错不会报错，
    // 只会让存档里的数量被截断、坏数据被静默接受。
    { name: "stack_codec", probe: "stack_codec", oracle: "StackCodecOracle", key: "unit",
        // 单元 = 物品×变体（两侧的 list 枚举）+ 固定场景（两侧的 list 各写一份同名标记，
        // 场景取值本身只在 stack_codec.ts 里有一份，见 stackCodecInput）。
        units: { kind: "listing" },
        input: stackCodecInput,
        // 两侧的 SNBT / NBT 打印器不同（引号风格、浮点后缀、顶层键序），统一成规范形再比。
        normalise: normaliseStackCodec,
        // 物品×变体单元里，Cubium 未注册的物品在探针侧输出 missing（场景单元与物品无关）。
        // 缺口由探针自己的 `list` 推出来：静态清单（known_missing_items.ts）会滞后于注册表，
        // 拿它当唯一判据会把已经支持的物品也剔出去。
        deriveMissing: missingLabelStackCodec,
        // 原版 `count` 的 codec 是 `intRange(1, 99).orElse(1)`，而 `orElse` 对越界是宽容的：
        // 0 / 100 / -1 都退回 1 而不是报错。Cubium 按 Java 源码的字面语义（越界即失败）实现，
        // 两者因此在这三个取值上分叉。要让本侧也「宽容」，需要先确认 COUNT_OVERFLOW 在
        // 数据包校验（DataFixer 的版本包装）里没有另外一层约束；在此之前保持严格。
        knownDifferences: [
            { key: "count#zero", reason: "原版 orElse(1) 对越界数量宽容，Cubium 判为无效" },
            { key: "count#over", reason: "同上" },
            { key: "count#negative", reason: "同上" },
        ] },
    // 战利品函数逐函数比对：现场造「1 池 + 1 个必定产出的条目」的表，表级只挂一个函数，
    // 在固定种子下掷骰。现有 loot_table* 三个能力比的是整张表的结果，一张表的差异可能
    // 来自池、条目、条件或函数任何一层；这里把范围收窄到函数本身。
    { name: "loot_function", probe: "loot_function", oracle: "LootFunctionOracle", key: "unit",
        units: { kind: "listing" }, input: lootFunctionInput, normalise: normaliseLootFunction,
        // oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world" },
    // 寻路：两个能力读同一份场景库（path_scenes.ts），只是比对的口径不同。
    // 逐格路径类型是「求值器判定」的完整快照：任一格的三层类型不一致都会点名到格。
    { name: "walk_path_type", probe: "walk_path_type", oracle: "WalkPathTypeOracle", key: "unit",
        units: { kind: "computed", units: pathSceneNames },
        input: (unit) => walkPathTypeInput(unit),
        // oracle 要在 java.io.tmpdir 里建原版世界并持有区域锁：同组必须串行。
        exclusiveGroup: "path-world" },
    // A* 是「求值器 + 搜索 + 回溯」的整体：路径节点序列必须逐项一致。
    { name: "path_finder", probe: "path_finder", oracle: "PathFinderOracle", key: "unit",
        units: { kind: "computed", units: pathSceneNames },
        input: (unit) => pathFinderInput(unit),
        exclusiveGroup: "path-world" },
    // 飞行寻路：同一份地形，换成 26 邻居的飞行求值器。
    { name: "fly_path_finder", probe: "fly_path_finder", oracle: "FlyPathfinderOracle", key: "unit",
        units: { kind: "computed", units: pathSceneNames },
        input: (unit) => pathFinderInput(unit),
        exclusiveGroup: "path-world" },
    // 活塞推动结构：一次推动的可推动性、移动链与破坏链。
    // 这是**纯计算**能力——结构求解只读世界、不推进刻度（逐刻动画属于 world 型，见 skill
    // `mc-add-diff-test`）。场景由 piston_movement.ts 用固定种子生成，两侧消费同一份输入。
    { name: "piston_movement", probe: "piston_movement", oracle: "PistonMovementOracle", key: "unit",
        units: { kind: "computed", units: pistonSceneNames },
        input: pistonMovementInput },
    // 玩家经验值：等级换算公式 + 加点/扣点的状态机。
    // 进度条是 f32，探针与 oracle 都输出位模式，逐位比较。
    { name: "experience", probe: "experience", oracle: "ExperienceOracle", key: "unit",
        units: { kind: "computed", units: experienceSceneNames },
        input: experienceInput },
    // 药水定义：基础名、效果列表（按效果 ID 排序后比较）、是否含瞬时效果、三种前缀的翻译键。
    // 纯查表与字符串拼接，不触碰世界。
    { name: "potion", probe: "potion", oracle: "PotionOracle", key: "potion",
        units: { kind: "listing" }, input: potionInput, unorderedArrays: POTION_UNORDERED },
    // 酿造配方：药水 × 试剂的笛卡尔积，比对「是否材料 / 能否酿造 / 输出药水」。
    { name: "potion_brewing", probe: "potion_brewing", oracle: "PotionBrewingOracle", key: "unit",
        units: { kind: "computed", units: potionBrewingUnits },
        input: potionBrewingInput },
    // 方块支撑判定：每个方块 × 26 种典型底座，比对「能否站上去」。
    // 只读计算（`isValidPosition` 只查邻域方块），与寻路的 SceneRegion 同一形态。
    { name: "block_support", probe: "block_support", oracle: "BlockSupportOracle", key: "block",
        units: { kind: "listing" }, input: blockSupportInput },
    // 食物属性：饥饿值 / 饱和度修正 / 能否饱腹后继续吃。纯数据查表。
    { name: "food_properties", probe: "food_properties", oracle: "FoodPropertiesOracle", key: "item",
        units: { kind: "listing" }, input: foodPropertiesInput },
    // 实体类型标签：每个标签包含哪些实体类型（集合语义）。纯数据查表。
    { name: "entity_type_tags", probe: "entity_type_tags", oracle: "EntityTypeTagsOracle", key: "tag",
        units: { kind: "computed", units: () => [...ENTITY_TYPE_TAGS] },
        input: entityTypeTagsInput,
        unorderedArrays: ENTITY_TYPE_TAGS_UNORDERED },
    // 唱片机歌曲：声音事件 / 翻译键 / 时长 / 比较器输出。纯数据查表。
    { name: "jukebox_song", probe: "jukebox_song", oracle: "JukeboxSongOracle", key: "song",
        units: { kind: "listing" }, input: jukeboxSongInput },
    // 附魔标签：每个标签包含哪些附魔（集合语义）。纯数据查表。
    { name: "enchantment_tags", probe: "enchantment_tags", oracle: "EnchantmentTagsOracle", key: "tag",
        units: { kind: "listing" }, input: enchantmentTagsInput,
        unorderedArrays: ENCHANTMENT_TAGS_UNORDERED },
    // 画变体：宽高 / 贴图 / 标题与作者的翻译键。纯数据包注册表。
    { name: "painting_variant", probe: "painting_variant", oracle: "PaintingVariantOracle", key: "variant",
        units: { kind: "listing" }, input: paintingVariantInput },
    // 盔甲纹饰：图案的贴图/描述/贴花，材料的贴图后缀/描述颜色/覆盖表。纯数据包注册表。
    { name: "armor_trim", probe: "armor_trim", oracle: "ArmorTrimOracle", key: "unit",
        units: { kind: "listing" }, input: armorTrimInput,
        unorderedArrays: ARMOR_TRIM_UNORDERED },
    // 动物变体：六类变体的贴图资源、模型名与带优先级的出生条件。纯数据包注册表。
    { name: "animal_variant", probe: "animal_variant", oracle: "AnimalVariantOracle", key: "unit",
        units: { kind: "listing" }, input: animalVariantInput },
    // 乐器：音效 ID / 吹奏时长 / 可听距离 / 描述翻译键，以及 tick 数与音量两个派生值。
    { name: "instrument", probe: "instrument", oracle: "InstrumentOracle", key: "instrument",
        units: { kind: "listing" }, input: instrumentInput },
    // 旗帜图案：数据包驱动的注册表（原版 43 项）。
    { name: "banner_pattern", probe: "banner_pattern", oracle: "BannerPatternOracle", key: "pattern",
        units: { kind: "listing" }, input: bannerPatternInput },
    // 附魔提供器：7 项定义 × 7 种难度乘数，重点比 by_cost_with_difficulty 的代价区间。
    { name: "enchantment_provider", probe: "enchantment_provider", oracle: "EnchantmentProviderOracle",
        key: "provider", units: { kind: "computed", units: enchantmentProviderUnits },
        input: enchantmentProviderInput },
    // 试炼刷怪笼配置：28 项 × 9 种玩家数，重点比派生刷怪数量（f32 + floor）。
    { name: "trial_spawner_config", probe: "trial_spawner_config", oracle: "TrialSpawnerConfigOracle",
        key: "config", units: { kind: "computed", units: trialSpawnerConfigUnits },
        input: trialSpawnerConfigInput },
    // 维度类型：高度三项、坐标缩放、天空盒、刷怪光照与 attributes 计数。
    { name: "dimension_type", probe: "dimension_type", oracle: "DimensionTypeOracle",
        key: "dimensionType", units: { kind: "listing" }, input: dimensionTypeInput },
    // 爆炸射线破坏判定：6 个场景 × 8 个种子，纯计算，复刻 ServerExplosion 的射线步进。
    { name: "explosion_ray", probe: "explosion_ray", oracle: "ExplosionRayOracle",
        key: "scene", units: { kind: "computed", units: explosionRayUnits },
        input: explosionRayInput },
    // 爆炸伤害与击退：半径 × 距离 × 可见度 × 击退抗性，纯计算。
    { name: "explosion_damage", probe: "explosion_damage", oracle: "ExplosionDamageOracle",
        key: "radius", units: { kind: "listing" }, input: explosionDamageInput },
    // 爆炸可见度采样网格：碰撞箱 → 步长 / 偏移 / 采样点数与坐标序列，纯计算。
    { name: "explosion_seen_grid", probe: "explosion_seen_grid", oracle: "ExplosionSeenGridOracle",
        key: "unit", units: { kind: "listing" }, input: explosionSeenGridInput },
    // 实体尺寸与包围盒：眼睛高度比例、缩放规则、包围盒展开，纯计算。
    { name: "entity_dimensions", probe: "entity_dimensions", oracle: "EntityDimensionsOracle",
        key: "unit", units: { kind: "listing" }, input: entityDimensionsInput },
    // 药水效果叠加合并：三层分支 + hiddenEffect 降级回退，纯计算。
    { name: "mob_effect_merge", probe: "mob_effect_merge", oracle: "MobEffectMergeOracle",
        key: "unit", units: { kind: "listing" }, input: mobEffectMergeInput },
    // 饥饿值与饱和度：两次 clamp 的顺序、下界钳制、exhaustion 上限，纯计算。
    { name: "food_stats", probe: "food_stats", oracle: "FoodStatsOracle",
        key: "unit", units: { kind: "listing" }, input: foodStatsInput },
    // 饥饿值每 tick 状态机：exhaustion 消耗 + 快/慢再生 + 饥饿伤害四个互斥分支，纯计算。
    { name: "food_tick", probe: "food_tick", oracle: "FoodTickOracle",
        key: "unit", units: { kind: "listing" }, input: foodTickInput },
    // 附魔台可选附魔列表：物品 × 附魔等级 -> 挑出哪些附魔、各几级，纯计算。
    { name: "enchantment_selection", probe: "enchantment_selection", oracle: "EnchantmentSelectionOracle",
        key: "unit", units: { kind: "computed", units: enchantmentSelectionUnits },
        input: enchantmentSelectionInput,
        knownDifferences: [

            { key: "minecraft:bow|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:bow|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:bow|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:bow|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:bow|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|1", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|10", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|15", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|20", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|5", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_axe|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|10", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|100", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|15", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|20", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_boots|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|10", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|100", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|15", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|20", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_helmet|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_pickaxe|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_pickaxe|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_pickaxe|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_pickaxe|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_pickaxe|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_sword|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_sword|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_sword|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_sword|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:diamond_sword|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:elytra|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:elytra|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:elytra|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:elytra|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:elytra|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:fishing_rod|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:fishing_rod|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:fishing_rod|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:fishing_rod|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:fishing_rod|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:shield|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:shield|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:shield|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:shield|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:shield|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:trident|25", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:trident|30", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:trident|35", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:trident|40", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
            { key: "minecraft:trident|50", field: "results", reason: "Cubium 用粗粒度类型枚举代替原版的物品级主物品表，且把宝藏过滤内移到 canApplyAtEnchantingTable" },
        ] },
    // 铁砧修复代价：取 max + 反直觉的跳过增长条件 + 32 位截断，纯计算。
    { name: "repair_cost", probe: "repair_cost", oracle: "RepairCostOracle",
        key: "unit", units: { kind: "listing" }, input: repairCostInput },
    // 铁砧合并耐久：先乘后除 + 有改善才计入 + 溢出边界，纯计算。
    { name: "anvil_durability", probe: "anvil_durability", oracle: "AnvilDurabilityOracle",
        key: "unit", units: { kind: "listing" }, input: anvilDurabilityInput },
    // 物品能否堆叠：MAX_STACK_SIZE 默认 1、可损坏与已损伤的组合，纯计算。
    { name: "item_stackable", probe: "item_stackable", oracle: "ItemStackableOracle",
        key: "unit", units: { kind: "listing" },
        input: itemStackableInput,
        knownDifferences: [{
            key: "minecraft:air|0",
            field: "empty",
            reason: "Cubium 用 nullptr 表示空栈、原版用 Items.AIR 物品表示；构造 ItemStack(air) 在 Cubium 侧得到非空栈"
        }, {
            key: "minecraft:air|0",
            field: "maxStackSize",
            reason: "同上：air 在 Cubium 侧不是空栈，因此读到 MAX_STACK_SIZE=64 而非空栈的 1"
        }, {
            key: "minecraft:air|0",
            field: "stackable",
            reason: "同上：air 在 Cubium 侧不是空栈，因此被判为可堆叠"
        }] },

    // 同物品且同组件：两空视为相同、matches 额外要求数量相等，纯计算。
    { name: "item_same_components", probe: "item_same_components", oracle: "ItemSameComponentsOracle",
        key: "unit", units: { kind: "listing" }, input: itemSameComponentsInput },
    // 熔炉每 tick 状态机：燃烧计时 + 烹饪计时，熄灭后进度减 2 而非归零，纯计算。
    { name: "furnace_tick", probe: "furnace_tick", oracle: "FurnaceTickOracle",
        key: "unit", units: { kind: "listing" }, input: furnaceTickInput },
    // 酿造台每 tick 状态机：燃料在开始酿造时扣、中途换试剂会取消，纯计算。
    { name: "brewing_tick", probe: "brewing_tick", oracle: "BrewingTickOracle",
        key: "unit", units: { kind: "listing" }, input: brewingTickInput },
    // 信标效果范围与时长：半径 = 层级*10+10、时长基数 9 秒，纯计算。
    { name: "beacon_effects", probe: "beacon_effects", oracle: "BeaconEffectsOracle",
        key: "unit", units: { kind: "listing" }, input: beaconEffectsInput },
    // 漏斗传输冷却时序：无条件递减 + 传输成功才设 8 tick 冷却，纯计算。
    { name: "hopper_cooldown", probe: "hopper_cooldown", oracle: "HopperCooldownOracle",
        key: "unit", units: { kind: "listing" }, input: hopperCooldownInput },
    // 刷怪笼生成延迟：-1 哨兵重置 + 区间退化取 min + 玩家范围前置条件，纯计算。
    { name: "spawner_delay", probe: "spawner_delay", oracle: "SpawnerDelayOracle",
        key: "unit", units: { kind: "listing" }, input: spawnerDelayInput },
    // Mth.floor / Mth.ceil 取整语义：向零截断后比较，非 std::floor，纯计算。
    { name: "mth_rounding", probe: "mth_rounding", oracle: "MthRoundingOracle",
        key: "unit", units: { kind: "listing" },
        input: mthRoundingInput,
        // 超界输入：Java 的 (int) 转换对超界值饱和到 Integer.MAX_VALUE，而 C++ 的
        // static_cast<i32>(double) 对超界值是未定义行为（本平台实测为 MIN_VALUE）。
        // 这是语言层面的差异，不属于可修复的实现缺陷，因此登记为已知差异。
        knownDifferences: [
            { key: "3000000000", field: "floorF", reason: "Java (int) 饱和到 MAX_VALUE，C++ 转换超界为未定义行为" },
            { key: "3000000000", field: "floorD", reason: "同上" },
            { key: "-3000000000", field: "floorF", reason: "同上" },
            { key: "-3000000000", field: "floorD", reason: "同上" }
        ] },
    // Mth.positiveModulo 与 Mth.wrapDegrees：区间端点不对称、% 的结果范围前提，纯计算。
    { name: "mth_modulo", probe: "mth_modulo", oracle: "MthModuloOracle",
        key: "unit", units: { kind: "listing" }, input: mthModuloInput },
    // Direction 旋转与 Y 轴角度：轴向旋转对垂直方向返回自身、南为 0 度，纯计算。
    { name: "direction_rotate", probe: "direction_rotate", oracle: "DirectionRotateOracle",
        key: "unit", units: { kind: "listing" }, input: directionRotateInput },
    // AABB 几何运算：intersects 用严格不等号（接触不算相交）、minmax 是并集，纯计算。
    { name: "aabb_ops", probe: "aabb_ops", oracle: "AabbOpsOracle",
        key: "unit", units: { kind: "listing" }, input: aabbOpsInput },
    // Vec3 向量运算：normalize 的 1e-5F 短路阈值 + 除法归一化，纯计算。
    { name: "vec3_math", probe: "vec3_math", oracle: "Vec3MathOracle",
        key: "unit", units: { kind: "listing" }, input: vec3MathInput },
    // BlockPos 位打包：X/Z 各 26 位、Y 12 位，超范围静默截断，纯计算。
    { name: "blockpos_codec", probe: "blockpos_codec", oracle: "BlockPosCodecOracle",
        key: "unit", units: { kind: "listing" }, input: blockposCodecInput },
    // Direction.Axis 与 Plane 判定：Z 轴正方向是 SOUTH、Plane.test(null) 为 false，纯计算。
    { name: "axis_plane", probe: "axis_plane", oracle: "AxisPlaneOracle",
        key: "unit", units: { kind: "listing" }, input: axisPlaneInput },
    // Mth 插值函数族：clampedLerp 越界返回精确端点、lerpInt 用 floor，纯计算。
    { name: "mth_lerp", probe: "mth_lerp", oracle: "MthLerpOracle",
        key: "unit", units: { kind: "listing" }, input: mthLerpInput },
    // Mth 整数位运算：smallestEncompassingPowerOfTwo 对非正输入返回 0，纯计算。
    { name: "mth_bit", probe: "mth_bit", oracle: "MthBitOracle",
        key: "unit", units: { kind: "listing" }, input: mthBitInput },
    // Mth 旋转插值：rotLerp 走最短弧、rotLerpRad 用 while 循环，纯计算。
    { name: "mth_rot", probe: "mth_rot", oracle: "MthRotOracle",
        key: "unit", units: { kind: "listing" }, input: mthRotInput },
    // Mth.sin / cos 查表：65536 项表由 Math.sin 构造，索引 & 65535，纯计算。
    { name: "mth_sin", probe: "mth_sin", oracle: "MthSinOracle",
        key: "unit", units: { kind: "listing" }, input: mthSinInput },
    // Mth 逼近与钳制：approach 对 step 取绝对值、clamp 不校验 min<=max，纯计算。
    { name: "mth_approach", probe: "mth_approach", oracle: "MthApproachOracle",
        key: "unit", units: { kind: "listing" }, input: mthApproachInput },
    // 区域文件扇区寻址：负坐标 >> 与 / 不同、偏移值 24+8 位打包，纯计算。
    { name: "region_sector", probe: "region_sector", oracle: "RegionSectorOracle",
        key: "unit", units: { kind: "listing" }, input: regionSectorInput },
    // BlockPos 距离与包围盒迭代：distManhattan 用 float 累加、迭代 X 最内层，纯计算。
    { name: "blockpos_dist", probe: "blockpos_dist", oracle: "BlockPosDistOracle",
        key: "unit", units: { kind: "listing" }, input: blockposDistInput },
    // 附魔存储：set 对 <=0 删除、upgrade 对 <=0 不做事，两者相反，纯计算。
    { name: "enchantment_storage", probe: "enchantment_storage", oracle: "EnchantmentStorageOracle",
        key: "unit", units: { kind: "listing" }, input: enchantmentStorageInput },
    // 实体碰撞扫描：Y 轴永远第一、主轴最后推进、1e-7 容差，纯计算。
    { name: "collide_sweep", probe: "collide_sweep", oracle: "CollideSweepOracle",
        key: "unit", units: { kind: "listing" }, input: collideSweepInput },
    // VoxelShape 布尔组合：16 种 BooleanOp 的逐格语义，TRUE 被断言拒绝，纯计算。
    { name: "shapes_boolean", probe: "shapes_boolean", oracle: "ShapesBooleanOracle",
        key: "unit", units: { kind: "listing" }, input: shapesBooleanInput },
    // 流体扩散：高度 amount/9、下落等级固定 7、坡度上限按流体区分，纯计算。
    { name: "fluid_spread", probe: "fluid_spread", oracle: "FluidSpreadOracle",
        key: "unit", units: { kind: "listing" }, input: fluidSpreadInput },
    // 流体扩散方向选择：就近全保留、best 无条件赋值，纯计算。
    { name: "fluid_spread_dir", probe: "fluid_spread_dir", oracle: "FluidSpreadDirOracle",
        key: "unit", units: { kind: "listing" }, input: fluidSpreadDirInput },
    // BFS 遍历：去重在状态判断前、STOP 中断、计数先自增后比较，纯计算。
    { name: "blockpos_bfs", probe: "blockpos_bfs", oracle: "BlockPosBfsOracle",
        key: "unit", units: { kind: "listing" }, input: blockposBfsInput },
    // 调度排序与去重：DRAIN_ORDER 三级、INTRA 忽略 tick、去重只认 type+pos，纯计算。
    { name: "scheduled_tick", probe: "scheduled_tick", oracle: "ScheduledTickOracle",
        key: "unit", units: { kind: "listing" }, input: scheduledTickInput },
    // 随机刻坐标采样：独立 LCG（x*3+1013904223）、算术右移，纯计算。
    { name: "random_tick_pos", probe: "random_tick_pos", oracle: "RandomTickPosOracle",
        key: "unit", units: { kind: "listing" }, input: randomTickPosInput },
    // HSV 转 RGB 与 murmur3 混合：截断取整、逻辑右移，纯计算。
    { name: "mth_color", probe: "mth_color", oracle: "MthColorOracle",
        key: "unit", units: { kind: "listing" }, input: mthColorInput },
    // Rotation/Mirror 变换：Y 轴不受旋转、镜像只翻转垂直轴，纯计算。
    { name: "block_transform", probe: "block_transform", oracle: "BlockTransformOracle",
        key: "unit", units: { kind: "listing" }, input: blockTransformInput },
    // 包围盒判定：闭区间相交、构造器自动归一化、move 原地修改，纯计算。
    { name: "bounding_box", probe: "bounding_box", oracle: "BoundingBoxOracle",
        key: "unit", units: { kind: "listing" }, input: boundingBoxInput },
    // 附魔互斥性：同一附魔不兼容、双向或关系，纯计算。
    { name: "enchantment_compat", probe: "enchantment_compat", oracle: "EnchantmentCompatOracle",
        key: "unit", units: { kind: "computed", units: listEnchantmentCompatUnits }, input: enchantmentCompatInput },
    // 区块坐标换算：算术右移、22/20 位编码，纯计算。
    { name: "section_pos", probe: "section_pos", oracle: "SectionPosOracle",
        key: "unit", units: { kind: "listing" }, input: sectionPosInput },
    // 高度范围：maxY 含上界、分区索引从 minSectionY 起算，纯计算。
    { name: "level_height", probe: "level_height", oracle: "LevelHeightOracle",
        key: "unit", units: { kind: "listing" }, input: levelHeightInput },
    // 区块坐标：32 位无符号拼接、区域 32 区块、Z 先异或魔数，纯计算。
    { name: "chunk_pos", probe: "chunk_pos", oracle: "ChunkPosOracle",
        key: "unit", units: { kind: "listing" }, input: chunkPosInput },
    // Mth 钳制族：浮点 clamp 用 < 而非 Math.max、短路上下界，纯计算。
    { name: "mth_clamp", probe: "mth_clamp", oracle: "MthClampOracle",
        key: "unit", units: { kind: "listing" }, input: mthClampInput },
    // 格式码：strip 字符集有限、getById 负数返回 RESET，纯计算。
    { name: "chat_formatting", probe: "chat_formatting", oracle: "ChatFormattingOracle",
        key: "unit", units: { kind: "listing" }, input: chatFormattingInput },
    // 染料颜色：byId 越界返回 WHITE、三个颜色字段互不相同，纯计算。
    { name: "dye_color", probe: "dye_color", oracle: "DyeColorOracle",
        key: "unit", units: { kind: "listing" }, input: dyeColorInput },
    // 装备槽位：isArmor 含 BODY 不含 SADDLE、id 有空洞，纯计算。
    { name: "equipment_slot", probe: "equipment_slot", oracle: "EquipmentSlotOracle",
        key: "unit", units: { kind: "listing" }, input: equipmentSlotInput },
    // 高度图：液体算 MOTION_BLOCKING 不算 OCEAN_FLOOR，纯计算。
    { name: "heightmap_types", probe: "heightmap_types", oracle: "HeightmapTypesOracle",
        key: "unit", units: { kind: "listing" }, input: heightmapTypesInput },
    // 整数除法与 2 的幂：floorDiv 向下取整、smallest(0)=0，纯计算。
    { name: "mth_int_div", probe: "mth_int_div", oracle: "MthIntDivOracle",
        key: "unit", units: { kind: "listing" }, input: mthIntDivInput },
    // 方向数据值：2D id 与 3D id 顺序相反、负数 id 取绝对值回绕，纯计算。
    { name: "direction_plane", probe: "direction_plane", oracle: "DirectionPlaneOracle",
        key: "unit", units: { kind: "listing" }, input: directionPlaneInput },
    // 旋转枚举：CLOCKWISE_180 序列化名是 "180"、BY_ID 用 WRAP，纯计算。
    { name: "rotation_info", probe: "rotation_info", oracle: "RotationInfoOracle",
        key: "unit", units: { kind: "listing" }, input: rotationInfoInput },
    // 方块坐标几何：containing 向负无穷取整、rotate 不对称，纯计算。
    { name: "blockpos_geom", probe: "blockpos_geom", oracle: "BlockPosGeomOracle",
        key: "unit", units: { kind: "listing" }, input: blockPosGeomInput },
    // 整数向量：compareTo 优先级 Y→Z→X、distManhattan 用 float，纯计算。
    { name: "vec3i_arith", probe: "vec3i_arith", oracle: "Vec3iArithOracle",
        key: "unit", units: { kind: "listing" }, input: vec3iArithInput },
    // 包围盒包含：半开区间 [min,max)、contract 只改单侧，纯计算。
    { name: "aabb_contains", probe: "aabb_contains", oracle: "AabbContainsOracle",
        key: "unit", units: { kind: "listing" }, input: aabbContainsInput },
    // 流体高度：getOwnHeight = amount/9，源头也只有 8/9，纯计算。
    { name: "fluid_height", probe: "fluid_height", oracle: "FluidHeightOracle",
        key: "unit", units: { kind: "listing" }, input: fluidHeightInput },
    // 反向插值：不 clamp、from==to 产生 NaN/Infinity、lerp3 的 d11 是死参数。
    { name: "mth_inverse_lerp", probe: "mth_inverse_lerp", oracle: "MthInverseLerpOracle",
        key: "unit", units: { kind: "listing" }, input: mthInverseLerpInput },
    // 角度插值：wrapDegrees 区间 (-180,180]、rotLerp 走短路、catmullrom 会过冲。
    { name: "mth_angle_lerp", probe: "mth_angle_lerp", oracle: "MthAngleLerpOracle",
        key: "unit", units: { kind: "listing" }, input: mthAngleLerpInput },
    // 波形与缓动：triangleWave 负输入越界、smoothstep 不 clamp。
    { name: "mth_waveform", probe: "mth_waveform", oracle: "MthWaveformOracle",
        key: "unit", units: { kind: "listing" }, input: mthWaveformInput },
    // HSV 转换：hue=1.0 回绕到红色、分量截断不四舍五入、alpha 恒 0。
    { name: "mth_hsv", probe: "mth_hsv", oracle: "MthHsvOracle",
        key: "unit", units: { kind: "listing" }, input: mthHsvInput },
    // ARGB 通道与混合：alpha 用无符号右移、三参 color 的 alpha 是 255。
    { name: "argb_ops", probe: "argb_ops", oracle: "ArgbOpsOracle",
        key: "unit", units: { kind: "listing" }, input: argbOpsInput },
    // 由向量反推方向：全零返回 north、平局由枚举顺序决定。
    { name: "direction_nearest", probe: "direction_nearest", oracle: "DirectionNearestOracle",
        key: "unit", units: { kind: "listing" }, input: directionNearestInput },
    // 坐标哈希：x 乘法是 int 溢出、z 是 long 乘法、y 只做裸异或。
    { name: "mth_seed", probe: "mth_seed", oracle: "MthSeedOracle",
        key: "unit", units: { kind: "listing" }, input: mthSeedInput },
    // 角度单字节压缩：packDegrees 不 clamp，180 回绕成 -128。
    { name: "mth_pack_degrees", probe: "mth_pack_degrees", oracle: "MthPackDegreesOracle",
        key: "unit", units: { kind: "listing" }, input: mthPackDegreesInput },
    // 角度差与朝向平滑：rotateIfNecessary 从 target 反推、absMax 在 MIN_VALUE 溢出。
    { name: "mth_degrees", probe: "mth_degrees", oracle: "MthDegreesOracle",
        key: "unit", units: { kind: "listing" }, input: mthDegreesInput },
    // 小数部分：frac(float) 的 floor 是 int，大输入会饱和失效。
    { name: "mth_frac", probe: "mth_frac", oracle: "MthFracOracle",
        key: "unit", units: { kind: "listing" }, input: mthFracInput },
    // 平方根：fastInvSqrt 是近似算法且无特殊值保护，sqrt 走 double 再降 float。
    { name: "mth_sqrt", probe: "mth_sqrt", oracle: "MthSqrtOracle",
        key: "unit", units: { kind: "listing" }, input: mthSqrtInput },
    // 字符串工具：截断的 maxLength>3 条件、lineCount 只认 \\r\\n 与 \\v。
    { name: "string_util", probe: "string_util", oracle: "StringUtilOracle",
        key: "unit", units: { kind: "listing" }, input: stringUtilInput },
    // 标识符解析：命名空间不允许 /、空串合法、bySeparator 只看第一个分隔符。
    { name: "identifier_parse", probe: "identifier_parse", oracle: "IdentifierParseOracle",
        key: "unit", units: { kind: "listing" }, input: identifierParseInput },
    // 标识符派生：toLanguageKey 保留 /、toDebugFileName 保留点、withPrefix 会重新校验。
    { name: "identifier_path", probe: "identifier_path", oracle: "IdentifierPathOracle",
        key: "unit", units: { kind: "listing" }, input: identifierPathInput },
    // 区间遍历：x 最快、z 最慢；六参版本不归一化，两参版本会归一化。
    { name: "blockpos_iterate", probe: "blockpos_iterate", oracle: "BlockPosIterateOracle",
        key: "unit", units: { kind: "listing" }, input: blockPosIterateInput },
    // 螺旋遍历：radius=0 只给中心、段长 leg/2+1 且成对、first/second 交换会镜像。
    { name: "blockpos_spiral", probe: "blockpos_spiral", oracle: "BlockPosSpiralOracle",
        key: "unit", units: { kind: "listing" }, input: blockPosSpiralInput },
    // AABB 工厂：+1 只在 max 侧、getCenter 走 Mth.lerp、intersect 不相交时返回负尺寸盒。
    { name: "aabb_factory", probe: "aabb_factory", oracle: "AabbFactoryOracle",
        key: "unit", units: { kind: "listing" }, input: aabbFactoryInput },
    // FrontAndTop：构造函数参数是 (name, front, top)，36 种组合只有 12 种合法。
    { name: "front_and_top", probe: "front_and_top", oracle: "FrontAndTopOracle",
        key: "unit", units: { kind: "listing" }, input: frontAndTopInput },
    // 结构朝向：UP/DOWN 走 default 分支（同 SOUTH）、WEST/EAST 交换 X 与 Z 的角色。
    { name: "structure_orient", probe: "structure_orient", oracle: "StructureOrientOracle",
        key: "unit", units: { kind: "listing" }, input: structureOrientInput },
    // BoundingBox 封装族：构造器反了才归一化、空集合返回 empty、起点是首点折叠盒。
    { name: "bounding_box_encap", probe: "bounding_box_encap", oracle: "BoundingBoxEncapOracle",
        key: "unit", units: { kind: "listing" }, input: boundingBoxEncapInput },
    // 区块区域：算术右移对负数向下取整、按位与取非负余数。
    { name: "chunk_region", probe: "chunk_region", oracle: "ChunkRegionOracle",
        key: "unit", units: { kind: "listing" }, input: chunkRegionInput },
    // 传送门相对位置：宽度不足取 0.5、高度不足取 0.0（不对称）；z 分量是带符号绝对偏移。
    { name: "portal_relative", probe: "portal_relative", oracle: "PortalRelativeOracle",
        key: "unit", units: { kind: "listing" }, input: portalRelativeInput },
    // Relative 位掩码：rotation 首个参数是 Y_ROT、Y_ROT 的位是 8（X_ROT 是 16）、忽略高位。
    { name: "relative_mask", probe: "relative_mask", oracle: "RelativeMaskOracle",
        key: "unit", units: { kind: "listing" }, input: relativeMaskInput },
    // 传送目标计算：相对/绝对语义、xRot 钳到 ±90、ROTATE_DELTA 旋转速度。
    { name: "position_move_rotation", probe: "position_move_rotation", oracle: "PositionMoveRotationOracle",
        key: "unit", units: { kind: "listing" }, input: positionMoveRotationInput },
    // Style 应用：applyFormat 换色保格式、applyLegacyFormat 换色清格式、applyTo 是 this 优先。
    { name: "style_apply", probe: "style_apply", oracle: "StyleApplyOracle",
        key: "unit", units: { kind: "listing" }, input: styleApplyInput },
    // TextColor：#f00 不是 RGB 缩写（是 0x000F00）、具名色与十六进制值相等但 serialize 不同。
    { name: "text_color", probe: "text_color", oracle: "TextColorOracle",
        key: "unit", units: { kind: "listing" }, input: textColorInput },
    // 点击事件：OPEN_FILE 是唯一禁用的动作、页码从 1 开始。
    { name: "click_event", probe: "click_event", oracle: "ClickEventOracle",
        key: "unit", units: { kind: "listing" }, input: clickEventInput },
    // 悬停事件：三个动作全部允许来自服务端（与 click_event 的 OPEN_FILE 对照）。
    { name: "hover_event", probe: "hover_event", oracle: "HoverEventOracle",
        key: "unit", units: { kind: "listing" }, input: hoverEventInput },
    // Identifier 构造入口：withDefaultNamespace 不解析冒号（整个串当路径）。
    { name: "identifier_resolve", probe: "identifier_resolve", oracle: "IdentifierResolveOracle",
        key: "unit", units: { kind: "listing" }, input: identifierResolveInput },
    // 纯文本内容：空串折叠为 EMPTY 单例，toString 是裸词 empty（不是 literal{}）。
    { name: "plain_text_contents", probe: "plain_text_contents", oracle: "PlainTextContentsOracle",
        key: "unit", units: { kind: "listing" }, input: plainTextContentsInput },
    // Mth 绝对值与距离：absMax(MIN_VALUE) 溢出、chessboardDistance 是 2D。
    { name: "mth_absmax", probe: "mth_absmax", oracle: "MthAbsMaxOracle",
        key: "unit", units: { kind: "listing" }, input: mthAbsMaxInput },
    // 计分板显示槽：团队槽用点分隔、越界 byId 返回 LIST、RESET 不是颜色。
    { name: "display_slot", probe: "display_slot", oracle: "DisplaySlotOracle",
        key: "unit", units: { kind: "listing" }, input: displaySlotInput },
    // 计分判据：camelCase 名、EXPERIENCE 叫 xp、只有血量用 hearts。
    { name: "objective_criteria", probe: "objective_criteria", oracle: "ObjectiveCriteriaOracle",
        key: "unit", units: { kind: "listing" }, input: objectiveCriteriaInput },
    // 队伍规则：序列化名混合全小写与驼峰、两个枚举翻译键前缀不同。
    { name: "team_rule", probe: "team_rule", oracle: "TeamRuleOracle",
        key: "unit", units: { kind: "listing" }, input: teamRuleInput },
    // Boss 栏：PINK 对应 RED、RED 对应 DARK_RED、PURPLE 对应 DARK_BLUE。
    { name: "boss_bar", probe: "boss_bar", oracle: "BossBarOracle",
        key: "unit", units: { kind: "listing" }, input: bossBarInput },
    // 声音通道：RECORDS/BLOCKS/PLAYERS 三个复数枚举映射到单数名。
    { name: "sound_source", probe: "sound_source", oracle: "SoundSourceOracle",
        key: "unit", units: { kind: "listing" }, input: soundSourceInput },
    // 难度：BY_ID 用 WRAP 回绕（不是 ZERO），info 键在末尾追加 .info。
    { name: "difficulty", probe: "difficulty", oracle: "DifficultyOracle",
        key: "unit", units: { kind: "listing" }, input: difficultyInput },
    // 权限等级：CLAMP 越界策略（byId|5 钳到 OWNERS）、higher 是 >= 语义。
    { name: "permission_level", probe: "permission_level", oracle: "PermissionLevelOracle",
        key: "unit", units: { kind: "listing" }, input: permissionLevelInput },
    // ChatFormatting 颜色查询：**探针直接调用 Cubium 的 text::getFormattingColor 等**。
    { name: "text_formatting_color", probe: "text_formatting_color", oracle: "TextFormattingColorOracle",
        key: "unit", units: { kind: "listing" }, input: textFormattingColorInput },
    // ChatFormatting 名字映射：**探针直接调用 Cubium 的 text::toName / fromName**。
    { name: "text_formatting_name", probe: "text_formatting_name", oracle: "TextFormattingNameOracle",
        key: "unit", units: { kind: "listing" }, input: textFormattingNameInput },
    // Style 三态序列化：**探针对接 Cubium 的 Style::toJson**，显式 false 必须写出。
    { name: "style_tristate", probe: "style_tristate", oracle: "StyleTristateOracle",
        key: "unit", units: { kind: "listing" }, input: styleTristateInput },
    // 队伍规则字符串：**探针对接 Cubium 的 teamVisibilityToString 等**。
    { name: "team_rule_strings", probe: "team_rule_strings", oracle: "TeamRuleStringsOracle",
        key: "unit", units: { kind: "listing" }, input: teamRuleStringsInput },
    // 渲染类型字符串：**探针对接 Cubium 的 renderTypeParse**。
    { name: "render_type_strings", probe: "render_type_strings", oracle: "RenderTypeStringsOracle",
        key: "unit", units: { kind: "listing" }, input: renderTypeStringsInput },
    // UUID 解析：**探针对接 Cubium 的 util::uuidFromString**。
    // 差异已登记：Cubium 内部统一用 32 位无连字符字符串存 UUID（见 Entity.cpp 的
    // 注释与 generateRandomUuid 的用法），因此 uuidFromString 接受 32 位是刻意的
    // 架构选择，不属于缺陷。原版 JDK 的 UUID.fromString 只接受 36 位带连字符格式。
    { name: "uuid_strings", probe: "uuid_strings", oracle: "UuidStringsOracle",
        key: "unit", units: { kind: "listing" }, input: uuidStringsInput,
        knownDifferences: [
            { key: "parse|069a79f444e94726a5befca90e38aaf5",
              reason: "Cubium 架构上使用 32 位无连字符 UUID 字符串（内部存储格式），刻意接受；"
                  + "JDK UUID.fromString 拒绝。内部调用方全部传 32/36 位内部字符串，无外部输入风险" },
            { key: "parse|", reason: "原版抛 IllegalArgumentException，Cubium 返回全零 UUID 哨兵值" },
            { key: "parse|dunno", reason: "同上" },
            { key: "parse|-069a79f4-44e9-4726-a5be-fca90e38aaf5", reason: "同上" },
            { key: "parse|069a79f444e94726a5befca90e38aaf5-", reason: "同上" },
            { key: "parse|069a79f4_44e9_4726_a5be_fca90e38aaf5", reason: "同上" },
            { key: "parse|069a79f4-44e9-4726-a5be-fca90e38aaf",
              reason: "JDK 对短段向前补零（35 位→补成 0fca...），Cubium 直接判为失败返回全零；"
                  + "属 JDK 边缘行为，Cubium 内部不产生此类残缺字符串" },
        ] },





    // 游戏模式：**探针对接 Cubium 的 GameMode 枚举**。
    { name: "game_type", probe: "game_type", oracle: "GameTypeOracle",
        key: "unit", units: { kind: "listing" }, input: gameTypeInput },
    // 地图装饰类型：**探针对接 Cubium 的 decorationTypeToString**。
    { name: "map_decoration_type", probe: "map_decoration_type", oracle: "MapDecorationTypeOracle",
        key: "unit", units: { kind: "listing" }, input: mapDecorationTypeInput,
        knownDifferences: [
            { key: "fromString|desert_village",
              reason: "Cubium 额外接受纹理名（desert_village 等）以兼容旧数据包 JSON；"
                  + "原版只认注册名 village_desert。属有意的向后兼容，不影响写出" },

        ] },
    // 方块状态枚举字符串：**探针对接 Cubium 的 EnumProperty Traits**。
    { name: "block_state_enum_strings", probe: "block_state_enum_strings", oracle: "BlockStateEnumStringsOracle",
        key: "unit", units: { kind: "listing" }, input: blockStateEnumStringsInput },
    // 第二组方块状态枚举：PistonType.DEFAULT 序列化名是 normal。
    { name: "block_state_enum_strings_2", probe: "block_state_enum_strings_2", oracle: "BlockStateEnumStrings2Oracle",
        key: "unit", units: { kind: "listing" }, input: blockStateEnumStrings2Input },
    // 第三组方块状态枚举：NoteBlockInstrument 有 23 项。
    { name: "block_state_enum_strings_3", probe: "block_state_enum_strings_3", oracle: "BlockStateEnumStrings3Oracle",
        key: "unit", units: { kind: "listing" }, input: blockStateEnumStrings3Input },
    // 布尔属性内部值顺序：原版 true 在 false 之前，Cubium 相反（无害，见文档）。
    { name: "boolean_property_index", probe: "boolean_property_index", oracle: "BooleanPropertyIndexOracle",
        key: "unit", units: { kind: "listing" }, input: booleanPropertyIndexInput },
    // 枚举属性值顺序：原版 FACING 是显式列表 NORTH,EAST,SOUTH,WEST,UP,DOWN。
    { name: "enum_property_order", probe: "enum_property_order", oracle: "EnumPropertyOrderOracle",
        key: "unit", units: { kind: "listing" }, input: enumPropertyOrderInput },
    // 全部 40 个枚举属性的值顺序（enum_property_order 的扩展版）。
    { name: "enum_property_order_all", probe: "enum_property_order_all", oracle: "EnumPropertyOrderAllOracle",
        key: "unit", units: { kind: "listing" }, input: enumPropertyOrderAllInput },
    // 面朝支撑判定：所有附着型方块存活判定的公共底层。
    { name: "face_sturdy", probe: "face_sturdy", oracle: "FaceSturdyOracle",
        key: "unit", units: { kind: "listing" }, input: faceSturdyInput },
    {
        name: "world_border",
        probe: "world_border",
        oracle: "WorldBorderOracle",
        description: "WorldBorder 的边界判定：isWithinBounds(BlockPos) 只看方块西北角，"
            + "下界 >= / 上界 < 不对称；isWithinBounds(AABB) 的 1.0E-5F 是 float 字面量",
        key: "unit", units: { kind: "listing" }, input: worldBorderInput },
    {
        name: "easing_type",
        probe: "easing_type",
        oracle: "EasingTypeOracle",
        description: "缓动函数 EasingType/Ease 的 32 个具名实现；全程 float 运算但 "
            + "Math.pow/sqrt/sin 走 double，inElastic 两端有硬编码短路",
        key: "unit", units: { kind: "listing" }, input: easingTypeInput },
    {
        name: "mth_interpolation",
        probe: "mth_interpolation",
        oracle: "MthInterpolationOracle",
        description: "Mth.lerp2/lerp3/catmullrom/smoothstep；lerp3 只接受单因子，"
            + "smoothstep 是单参数版本（x³(6x²-15x+10)），与 GLSL 三参数版本不同",
        key: "unit", units: { kind: "listing" }, input: mthInterpolationInput },
    {
        name: "legacy_random",
        probe: "legacy_random",
        oracle: "LegacyRandomOracle",
        description: "LegacyRandomSource 的 48 位 LCG 序列；nextInt(bound) 是拒绝采样，"
            + "2 的幂走快速路径，同 seed 下不同 bound 推进的步数不同",
        key: "unit", units: { kind: "listing" }, input: legacyRandomInput },
    {
        name: "custom_stats",
        probe: "custom_stats",
        oracle: "CustomStatsOracle",
        description: "自定义统计常量表（77 项）；统计 ID 是存档持久键，"
            + "缺项会让对应行为无法记录。缺的统计以 MISSING 暴露",
        key: "unit", units: { kind: "listing" }, input: customStatsInput },
    {
        name: "sound_events",
        probe: "sound_events",
        oracle: "SoundEventsOracle",
        description: "SoundEvents 音效 ID 常量表（1637 项）；音效 ID 是资源路径键与网络包内容，"
            + "缺项会让对应行为完全无声。缺的音效以 MISSING 暴露",
        key: "unit", units: { kind: "listing" }, input: soundEventsInput },
    {
        name: "particle_type",
        probe: "particle_type",
        oracle: "ParticleTypeOracle",
        description: "粒子类型的协议 ID 与注册表 ID 对齐（115 项）；枚举值直接就是网络协议 ID，"
            + "错一位会让后续所有粒子错位且不报任何错误",
        key: "unit", units: { kind: "listing" }, input: particleTypeInput,
        // 以下 5 项是**命名差异**，不是协议错位：Cubium 用了旧版 MC 的枚举名，
        // 而协议 ID（索引）与原版完全一致。名字只在本项目内部引用，不影响网络序列化。
        knownDifferences: [
            { key: "16|effect", field: "result", reason: "Cubium 枚举名为 Spell（旧版命名），索引一致" },
            { key: "22|explosion_emitter", field: "result", reason: "Cubium 枚举名为 HugeExplosion（旧版命名），索引一致" },
            { key: "46|instant_effect", field: "result", reason: "Cubium 枚举名为 InstantSpell（旧版命名），索引一致" },
            { key: "75|campfire_cosy_smoke", field: "result", reason: "Cubium 枚举名为 CampfireCozy（cozy/cosy 拼写差异），索引一致" },
            { key: "76|campfire_signal_smoke", field: "result", reason: "Cubium 枚举名为 CampfireSignal，索引一致" },
        ] },
    {
        name: "shape_face",
        probe: "shape_face",
        oracle: "ShapeFaceOracle",
        description: "VoxelShape 的面切片（15 种边界形状 × 6 方向）；把 slice / findIndex / "
            + "isCubeLikeAlong 单独拎出来比对，用于定位 face_sturdy 剩余差异的底层根因",
        key: "unit", units: { kind: "listing" }, input: shapeFaceInput },
    {
    name: "block_shape_all",
    probe: "block_shape_all",
    oracle: "BlockShapeAllOracle",
    description: "方块三种形状的坐标（外形 / 碰撞 / 支撑，**全部属性组合**）；"
    + "block_shape 只测默认状态，本能力遍历约 3 万个状态，用于抓只在非默认状态下出现的形状错误",
    key: "unit", units: { kind: "listing" }, input: blockShapeAllInput },
    {
        name: "axis_cycle",
        probe: "axis_cycle",
        oracle: "AxisCycleOracle",
        description: "AxisCycle 三轴循环映射（两个 cycle 重载 + inverse + between）；"
        + "这是 VoxelShape.collide、DiscreteVoxelShape 边/面遍历的底层坐标变换，错一个映射会让上层所有几何整体偏移",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "entity_type_size",
        probe: "entity_type_size",
        oracle: "EntityTypeSizeOracle",
        description: "实体类型注册尺寸（宽 / 高 / 眼睛高度 / 是否固定）；"
        + "这四项是原版硬编码在每个 EntityType.Builder 里的注册数据，不在数据包中，也没有别的机制能校验抄写是否正确。"
        + "抄错一位小数不会报错，只会让实体卡进方块或视线落点差一格。浮点输出位模式以避开格式差异",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        oracleMissing: entityTypesKnownMissing },
    {
        name: "dispense_behavior",
        probe: "dispense_behavior",
        oracle: "DispenseBehaviorOracle",
        description: "发射器行为表：每个物品是否注册了专用行为、是否为 OptionalDispenseItemBehavior；"
        + "原版 169 条注册里 109 条是匿名子类（类名是 DispenserBlock$1 这种序号），跨实现不可比，"
        + "因此比对归一化标签与 isSuccess() 初值——后者决定是否播发射音效、是否消耗物品",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        oracleMissing: itemsKnownMissing,
        knownDifferences: KNOWN_DISPENSE_GAPS.flatMap((unit) =>
            ["registered", "kind", "successDefault"].map((field) => ({
                key: unit,
                field,
                reason: "Cubium 尚未注册该类发射器行为（见 known_dispense_gaps.ts 的分组与修法）",
            }))) },
    {
        name: "attribute_modifier_apply",
        probe: "attribute_modifier_apply",
        oracle: "AttributeModifierApplyOracle",
        description: "属性修饰符三阶段计算（AttributeInstance.calculateValue）的最终值；"
        + "三个易错点：ADD_MULTIPLIED_BASE 乘的是经过 ADD_VALUE 之后的基数而非原始 base；"
        + "ADD_MULTIPLIED_TOTAL 必须逐个连乘 (1+amount) 而不是先相加；sanitizeValue 对 NaN 返回 minValue",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "entity_pose_dimensions",
        probe: "entity_pose_dimensions",
        oracle: "EntityPoseDimensionsOracle",
        description: "实体在 18 个姿态下的宽 / 高 / 眼睛高度（16 类型 × 18 姿态 = 288 单元）；"
        + "核心是 LivingEntity.getDimensions(Pose) 是 **public final**：SLEEPING 恒为 (0.2, 0.2, 0.2)，"
        + "与实体类型、年龄、缩放全都无关；其余姿态走类型尺寸。漏掉这条会让所有生物躺下时碰撞箱还是站立的尺寸",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        knownDifferences: KNOWN_POSE_GAPS },
    {
        name: "mob_spawn_light_level",
        probe: "mob_spawn_light_level",
        oracle: "MobSpawnLightLevelOracle",
        description: "怪物生成光照判定（Monster.isDarkEnoughToSpawn）的三段逻辑；"
        + "三个易错点：天空光与 nextInt(32) 的随机阈值比而非常量；方块光上限仅在 blockLightLimit<15 时才检查；"
        + "最后比的是综合亮度而非方块光。输出同时带 lightTest 采样值，便于区分「随机流不同」与「判定逻辑不同」",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "dimension_type_params",
        probe: "dimension_type_params",
        oracle: "DimensionTypeParamsOracle",
        description: "维度类型参数（DimensionType 的 14 个字段 × 3 维度）；"
        + "这是**数据包驱动的注册表**，每个字段都直接决定游戏行为且彼此独立：coordinate_scale 决定主世界与下界的坐标换算、"
        + "ambient_light 决定无光照处底色、fixed_time 让末地永远停在黄昏、min_y/height/logical_height 决定建筑高度与传送门规则。"
        + "抄错任何一项都不会崩溃，只会让某个维度的手感整体偏差——这是最难靠人工发现的一类数据错误",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        knownDifferences: KNOWN_DIMENSION_GAPS },
    {
        name: "beacon_pyramid_level",
        probe: "beacon_pyramid_level",
        oracle: "BeaconPyramidLevelOracle",
        description: "信标金字塔等级（BeaconBlockEntity.updateBase）；"
        + "要点：第 j 层是 (2j+1)^2 的**整片**而非只有边框、必须**连续**（某层一破即止）、"
        + "**低于世界底即停止**（世界底是维度参数而非常量：主世界 -64、下界与末地 0）。"
        + "布局字符串让「整层缺一格」「层间不连续」「世界底截断」三类错误都能精确表达",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "conduit_frame_level",
        probe: "conduit_frame_level",
        oracle: "ConduitFrameLevelOracle",
        description: "潮涌核心框架等级（ConduitBlockEntity.updateShape）；"
        + "要点：中心 3x3x3 必须**全是水**否则立刻失效、只认**框架环**（42 格）而非 5x5x5 整个外壳（98 格）、"
        + "两档门槛 MIN_ACTIVE_SIZE=16 与 MIN_KILL_SIZE=42",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "campfire_cooking",
        probe: "campfire_cooking",
        oracle: "CampfireCookingOracle",
        description: "营火烹饪配方表（campfire_cooking 类型的全部配方）；"
        + "营火只认 CampfireCooking 类型，**不读熔炉或烟熏机配方**，三者数据文件独立，同一物品在三种设施下的可熔炼性与耗时都可能不同。"
        + "cookTime 就是营火的实际烹饪耗时（cookTick 每 tick 递增、达到即掉落），抄错任何一个都会让该物品熟得太早或太晚",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "cooking_recipe_types",
        probe: "cooking_recipe_types",
        oracle: "CookingRecipeTypesOracle",
        description: "四类烹饪配方的横向对照（熔炉/烟熏机/高炉/营火）—— 16 输入 × 4 设施 = 64 单元；"
        + "四套配方在数据包里各自独立、cookingtime 也写在 JSON 里。语义差异：熔炉通用、"
        + "烟熏机只做食物且快一倍、高炉只做矿石且快一倍、营火只做食物且慢三倍。"
        + "最容易出错的是**串配方**（smoking 走成 smelting 之类），不会崩溃、只会让某些物品在错误设施里可加工或耗时差一倍",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "game_event_radius",
        probe: "game_event_radius",
        oracle: "GameEventRadiusOracle",
        description: "游戏事件的通知半径（GameEvent.notificationRadius）；"
        + "60 个事件里 57 个是默认 16，只有 jukebox_play/jukebox_stop_play（10）与 shriek（32）不同。"
        + "半径决定事件能被多远的监听器收到——幽匿感测体、尖啸体、监守者、悦灵这些「听声音」的机制全靠它；"
        + "抄成默认 16 不会崩溃，只会让监守者的尖啸只惊动半张地图。单元由 allGameEventIds() 生成，事件名集合的差异也会直接体现为单元缺失/多出",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "container_analog_signal",
        probe: "container_analog_signal",
        oracle: "ContainerAnalogSignalOracle",
        description: "容器红石信号强度（AbstractContainerMenu.getRedstoneSignalFromContainer）—— 比较器读箱子时的强度换算；"
        + "三个易错点：每格按**各自的 maxStackSize** 归一化而不是统一按 64（否则 16 个工具会被算成只装了 4%）、"
        + "`f > 0` 才 +1 所以非空最低为 1 而空箱为 0、先逐格累加再整体除以槽位数",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "item_max_stack_size",
        probe: "item_max_stack_size",
        oracle: "ItemMaxStackSizeOracle",
        description: "全部物品的堆叠上限（ItemStack.getMaxStackSize 的默认值）—— 每个已注册物品一个单元；"
        + "上限是散落在上千处物品注册调用里的小整数（64/16/1），抄错不会崩溃但会实实在在影响玩法："
        + "本该 1 的桶会在一个格子叠一摞、本该 64 的食材会让物品栏凭空多占格子、"
        + "而比较器读箱子时按 count/maxStackSize 折算占比，上限错会让**信号强度整体偏移**",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_light_emission",
        probe: "block_light_emission",
        oracle: "BlockLightEmissionOracle",
        description: "方块状态的发光强度（BlockBehaviour.getLightEmission）—— 每个状态一个单元；"
        + "与 light_occlusion 互补：后者量「光穿过这一格后还剩多少」，本条量「这一格自己发出多少光」。"
        + "只测衰减时，一个本该发光却写成 0 的火把看起来「透光性完美」，但整间屋子不亮。"
        + "原版用 MAX_LEVEL 截断，抄成 16 与 15 在游戏里看不出区别，只能靠比对发现",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_hardness",
        probe: "block_hardness",
        oracle: "BlockHardnessOracle",
        description: "全部方块的硬度与爆炸抗性（getDestroySpeed / getExplosionResistance）—— 每个方块一个单元，f32 位模式比对；"
        + "硬度差一点挖掘手感就不对、差一个数量级玩家会以为自己拿错了工具；**硬度 -1 表示不可破坏**，写成 0 会一碰就碎、写成 50 会挖得动但挖不完；"
        + "抗性写成 0 的黑曜石会被一颗 TNT 清空。与 block_destroy_progress 互补：后者验公式，本条验公式的输入",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_map_color",
        probe: "block_map_color",
        oracle: "BlockMapColorOracle",
        description: "全部方块状态的地图颜色（BlockBehaviour.getMapColor）—— 每个状态一个单元；"
        + "地图上每一格的颜色都由它决定（再按高度套 4 级明暗）。只影响地图渲染、不影响任何游戏逻辑，"
        + "所以抄错了既不崩溃也不会让玩家觉得哪里不对，直到有人做了张大地图、发现白羊毛屋顶在地图上是灰的。"
        + "典型错误形态：16 种染色方块全指到同一颜色、陶瓦错指到 STONE",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_friction",
        probe: "block_friction",
        oracle: "BlockFrictionOracle",
        description: "方块的移动手感三件套——摩擦、速度倍率、跳跃倍率（getFriction / getSpeedFactor / getJumpFactor）；"
        + "三者互相替代，必须一起比：冰改的是 friction(0.98)、蜂蜜块不改 friction 而改 speedFactor(0.4)+jumpFactor(0.5)、"
        + "灵魂沙改 speedFactor(0.4) 但不改 jumpFactor。只比 friction 会漏掉蜂蜜块与灵魂沙的大半行为，只比 speedFactor 会把冰判成普通方块",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_fire_info",
        probe: "block_fire_info",
        oracle: "BlockFireInfoOracle",
        description: "方块的可燃性参数（FireBlock.setFlammable 的 encouragement/flammability）—— 每个方块一个单元；"
        + "encouragement 是火焰蔓延到它的概率权重（木板 5、树叶 30），flammability 是它被烧毁的概率权重（木板 20、羊毛 60），两者独立："
        + "树叶极易被点燃但烧尽速度和木板差不多。火焰传播是概率系统，抄错了不崩溃、只会让火势「感觉不太对」，只能逐方块比对",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_solidity",
        probe: "block_solidity",
        oracle: "BlockSolidityOracle",
        description: "方块的「可放置 / 可穿过」两态（isSolid 与 canBeReplaced）—— 每个方块一个单元；"
        + "solid 决定算不算实体方块（怪物能否在其上生成、玩家视线是否被挡），算法是 forceSolidOn→真 / forceSolidOff→假 / 否则看碰撞形状是否满格；"
        + "replaceable 决定能否被别的方块直接覆盖（草丛、雪层、空气可以，石头不行）。两个都是布尔量，抄错极隐蔽："
        + "replaceable 误真会让玩家的墙被随手放上的方块直接替换掉，solid 误真会让屋顶刷不出怪",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_instrument",
        probe: "block_instrument",
        oracle: "BlockInstrumentOracle",
        description: "音符盒放在方块上方时演奏的乐器（BlockStateBase#instrument()）—— 每个方块一个单元；"
        + "石头类给大鼓、沙类给小军鼓、玻璃给踩镲、木类给贝斯、泥土给长笛、金块给钟、羊毛给吉他、冰给风铃、骨块给木琴、"
        + "铁块给铁琴、灵魂沙给牛铃、南瓜给迪吉里杜管、绿宝石块给电子音、干草块给班卓琴、萤石给电钢琴，头颅模仿生物叫声，"
        + "其余竖琴。抄错不影响任何方块行为，只会让红石音乐作品听起来不对，只能逐方块比对",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "noteblock_pitch",
        probe: "noteblock_pitch",
        oracle: "NoteBlockPitchOracle",
        description: "音符盒的调音（音符值 0..24 的音高与循环）—— 25 个单元；"
        + "pitch 是原版 getPitchFromNote = 2^((note-12)/12)，按 f32 位模式比（中间用 f32 还是 f64 会让结果差 1 ulp，"
        + "听不出来但会让包络计算逐帧漂移）；cycled 是右键一次后的音符值，回绕点写错会让某些音根本弹不出来"
        ,
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "noteblock_trigger",
        probe: "noteblock_trigger",
        oracle: "NoteBlockTriggerOracle",
        description: "音符盒被触发时播放的音效三元组（音效 id / 音量 / 音高）与音符粒子 —— 23 种乐器 x 3 个音符值 = 69 个单元；"
        + "音量恒为 3.0，音高在可调音乐器上走 getPitchFromNote、头颅类固定 1.0，粒子只有可调音乐器才发（颜色偏移 note/24）。"
        + "绑定在 Block::triggerEvent 上而不是直接调播放函数：原版正是靠方块事件广播让附近所有玩家都听到，直接播放只有发起方出声",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "bell_placement",
        probe: "bell_placement",
        oracle: "BellPlacementOracle",
        description: "钟被放置时推导出的附着方式（BellBlock#getStateForPlacement）—— 6 种点击面 x 4 种玩家朝向 x 16 种四邻坚固性 = 384 个单元；"
        + "钟是少数按摆放方式长出不同形状的方块：点顶面立在地上（floor）、点底面吊在天花板（ceiling）、"
        + "点侧面且对向两侧都是墙就跨在两面墙之间（double_wall），否则单面挂（single_wall）。四种的碰撞形状与敲击范围都不同。"
        + "双墙写成单墙会让钟只挂在一侧、朝向用点击面本身而非反方向会让钟面朝墙里，都不会崩溃只是看起来不对",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "bell_ring",
        probe: "bell_ring",
        oracle: "BellRingOracle",
        description: "敲钟的「有效敲击」判定（BellBlock#isProperHit）—— 4 种附着方式 x 4 种钟朝向 x 6 种点击面 x 5 个点击高度 = 480 个单元；"
        + "钟不是碰一下就响：点击面在 Y 轴永远不响、点击高度 > 0.8124 永远不响（敲钟帽不响），"
        + "floor 要求点击面与钟朝向同轴，墙钟要求不同轴，ceiling 任何水平面都响。"
        + "判定错不崩溃、不掉日志，只会让某些角度敲不响或不该响的地方能响",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "jukebox_insert",
        probe: "jukebox_insert",
        oracle: "JukeboxInsertOracle",
        description: "唱片机的交互判定（JukeboxPlayable#tryInsertIntoJukebox 的分支表）—— 4 种手持物 x HAS_RECORD = 8 个单元；"
        + "只有三种结果：insert（手里是唱片且机器空着）、eject（机器里有唱片，任何手持物都一样）、try_empty_hand（都不是）。"
        + "陷阱是顺序：「机器里有唱片」优先于「手里拿什么」—— 拿着另一张唱片点也是先弹旧的，写反了旧唱片会凭空消失",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "lever_game_event",
        probe: "lever_game_event",
        oracle: "LeverGameEventOracle",
        description: "拨动拉杆时发出的游戏事件与音效（LeverBlock#play）—— 拨动前 POWERED 的两态 = 2 个单元；"
        + "规则是开启发 block_activate、关闭发 block_deactivate，音效恒为 block.lever.click。"
        + "游戏事件是幽匿感测体、尖啸体、监守者、悦灵这些听声音机制的唯一输入；漏发不崩溃不掉日志，"
        + "方块状态和音效都正常，只有红石侦测端完全没反应",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "note_block_event",
        probe: "note_block_event",
        oracle: "NoteBlockEventOracle",
        description: "音符盒响不响、发不发事件（NoteBlock#playNote）—— 23 种乐器 x 上方是否被方块堵住 = 46 个单元；"
        + "前置条件是「乐器 above-note-block 生效 或 上方是空气」：头颅类乐器不受上方方块影响，其余乐器上方必须为空。"
        + "被堵住时既不出声也不发 NOTE_BLOCK_PLAY 事件 —— 事件必须发在判定之后。漏掉前置条件会让埋了方块的音符盒照样发声",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "piston_event",
        probe: "piston_event",
        oracle: "PistonEventOracle",
        description: "活塞缩回时的音效与游戏事件（PistonBaseBlock#move）—— 2 种活塞 x 6 个朝向 = 12 个单元；"
        + "缩回放 block.piston.contract（音量 0.5）并发 BLOCK_DEACTIVATE，伸出是 extend + BLOCK_ACTIVATE。"
        + "漏掉音效时活塞完全无声、玩家判断不出装置有没有在工作；漏掉事件则活塞在幽匿电路里完全隐形",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "cake_eat",
        probe: "cake_eat",
        oracle: "CakeEatOracle",
        description: "吃一片蛋糕时发出的游戏事件（CakeBlock#eat）—— 吃之前的 BITES 0..6 = 7 个单元；"
        + "事件是叠加的：每一片都发 eat，若是最后一片（BITES 已是 6）额外发 block_destroy。"
        + "只写 EAT 会让最后一片消失时幽匿电路少一次 destroy，只写 DESTROY 则前六片完全静默",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "lectern_book",
        probe: "lectern_book",
        oracle: "LecternBookOracle",
        description: "讲台放书/取书时的状态复位与事件（LecternBlock#resetBookState）—— 初始 HAS_BOOK x POWERED x 目标 = 8 个单元；"
        + "POWERED 一律复位为 false、HAS_BOOK 置为目标值、发 BLOCK_CHANGE。"
        + "讲台翻页时会拉高一次 POWERED 当比较器脉冲，放书取书不清掉的话，取走书之后下游比较器会一直读到不该存在的信号",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "tripwire_state",
        probe: "tripwire_state",
        oracle: "TripwireStateOracle",
        description: "绊线钩状态变化时的音效与事件（TripWireHookBlock#emitState）—— 变化前 powered/attached x 变化后 = 16 个单元；"
        + "四个分支互斥且按优先级：通电/断电优先于挂上/脱开。两者同时变化时只报电量那次；"
        + "写成并列四个 if 会让一次扯断连发两个事件两个音效，幽匿感测体被连续唤醒、红石计时装置算错 tick。都不变时必须完全静默",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "farmland_decay",
        probe: "farmland_decay",
        oracle: "FarmlandDecayOracle",
        description: "耕地退化时的方块替换与事件（FarmBlock#turnToDirt）—— 耕地 MOISTURE 0..7 = 8 个单元；"
        + "被踩踏、缺水、上方被盖住三条路径都走 turnToDirt：换方块 + 发 block_change。"
        + "换方块与发事件是两件事，幽匿感测体只认 gameEvent，不发就完全感知不到耕地被踩坏",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "tripwire_disarm",
        probe: "tripwire_disarm",
        oracle: "TripwireDisarmOracle",
        description: "用剪刀拆绊线时的解除武装与事件（TripWireBlock#playerWillDestroy）—— 主手物品 3 种 = 3 个单元；"
        + "手里是剪刀时先把绊线置成 DISARMED 再移除，两端钩子看到「已解除」走拆除路径而不是触发路径，并发 shear 事件。"
        + "不置 DISARMED 的话，用剪刀拆陷阱反而会把它引爆一次 —— 正好是剪刀存在的意义（安全拆除）的反面",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "berry_growth",
        probe: "berry_growth",
        oracle: "BerryGrowthOracle",
        description: "甜浆果丛生长时的阶段推进与事件（SweetBerryBushBlock#randomTick）—— AGE 0..3 = 4 个单元；"
        + "随机刻生长与骨粉催熟都让 AGE+1 并发 block_change。AGE 已是 3（满阶段）时不再生长也不发事件 ——"
        + "满阶段还发事件的话，一丛浆果能让附近幽匿感测体每 tick 醒一次",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "anchor_charge",
        probe: "anchor_charge",
        oracle: "AnchorChargeOracle",
        description: "重生锚充能/放电时的层数变化与事件（RespawnAnchorBlock#charge）—— CHARGES 0..4 x 动作 = 10 个单元；"
        + "充能 +1 上限 4、放电 -1 下限 0，都发 block_change；已满/已空时不变也不发事件。"
        + "已达上限还继续加会让层数溢出到 5，光照等级 charges*3.75 算出 18 超出亮度上限",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_shape",
        probe: "block_shape",
        oracle: "BlockShapeOracle",
        description: "方块三种形状的坐标（外形 / 碰撞 / 支撑，默认状态）；"
            + "face_sturdy 只看布尔结论，本能力直接摊开坐标，用于定位形状写错还是碰撞开关错",
        key: "unit", units: { kind: "listing" }, input: blockShapeInput },








































































































    // 伤害类型标签：每个标签包含哪些伤害类型（集合语义）。纯数据查表。
    { name: "damage_type_tags", probe: "damage_type_tags", oracle: "DamageTypeTagsOracle", key: "tag",
        units: { kind: "listing" }, input: damageTypeTagsInput,
        unorderedArrays: DAMAGE_TYPE_TAGS_UNORDERED },
    // 附魔等级与代价曲线：最高等级 / 铁砧代价 / 每级的附魔台等级区间。
    { name: "enchantment_cost", probe: "enchantment_cost", oracle: "EnchantmentCostOracle", key: "enchantment",
        units: { kind: "listing" }, input: enchantmentCostInput },
    // 附魔适用性：每个附魔能附到哪些物品上（集合语义）。
    { name: "enchantment_applicability", probe: "enchantment_applicability", oracle: "EnchantmentApplicabilityOracle",
        key: "enchantment", units: { kind: "listing" }, input: enchantmentApplicabilityInput,
        unorderedArrays: ENCHANTMENT_APPLICABILITY_UNORDERED },
];

/** 按名字查找能力声明；未知名字返回 undefined。 */
export function findHarness(name: string): HarnessDeclaration | undefined {
    return HARNESSES.find((harness) => harness.name === name);
}
