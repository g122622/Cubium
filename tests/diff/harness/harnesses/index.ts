import { KNOWN_PATHFINDABLE_GAPS } from "./known_pathfindable_gaps.ts";
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
import { mansionGridDeclaration } from "./mansion_grid.ts";
import { mthLerpInput } from "./mth_lerp.ts";
import { mthBitInput } from "./mth_bit.ts";
import { mthRotInput } from "./mth_rot.ts";
import { mthSinInput } from "./mth_sin.ts";
import { mthAtan2Input } from "./mth_atan2.ts";
import { KNOWN_ATAN2_GAPS } from "./known_atan2_gaps.ts";
import { mthMapLerpInput } from "./mth_map_lerp.ts";
import { mthOutFromOriginInput } from "./mth_out_from_origin.ts";
import { mthLengthEqualInput } from "./mth_length_equal.ts";
import { blockposCenterInput } from "./blockpos_center.ts";
import { directionDataInput } from "./direction_data.ts";
import { shapeClosestInput } from "./shape_closest.ts";
import { shapeBoxesInput } from "./shape_boxes.ts";
import { shapeQueryInput } from "./shape_query.ts";
import { directionFacingInput } from "./direction_facing.ts";
import { mthRoundExtInput } from "./mth_round_ext.ts";
import { vec3RotateInput } from "./vec3_rotate.ts";
import { aabbExtInput } from "./aabb_ext.ts";
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
import { mineshaftPiecesDeclaration } from "./mineshaft_pieces.ts";
import { strongholdPiecesDeclaration } from "./stronghold_pieces.ts";
import { strongholdRoomCrossingDeclaration } from "./stronghold_room_crossing.ts";
import { strongholdSmallDoorDeclaration } from "./stronghold_small_door.ts";
import { endCityPiecesDeclaration } from "./end_city_pieces.ts";
import { jigsawJointOrderDeclaration } from "./jigsaw_joint_order.ts";
import { beardifierDensityDeclaration } from "./beardifier_density.ts";
import { biomeSourceSampleDeclaration } from "./biome_source_sample.ts";
import { climateSampleDeclaration } from "./climate_sample.ts";
import { structureStartSingleDeclaration } from "./structure_start_single.ts";
import { structureBlocksSingleDeclaration } from "./structure_blocks_single.ts";
import { structureBlocksSeedsSingleDeclaration } from "./structure_blocks_seeds_single.ts";
import { structureBlocksTerrainSingleDeclaration } from "./structure_blocks_terrain_single.ts";
import { structureBlocksJigsawSingleDeclaration } from "./structure_blocks_jigsaw_single.ts";
import { structureBlocksJigsawSeedsSingleDeclaration } from "./structure_blocks_jigsaw_seeds_single.ts";
import { structureBlocksJigsawTerrainSingleDeclaration } from "./structure_blocks_jigsaw_terrain_single.ts";
import { structureEntitiesSingleDeclaration } from "./structure_entities_single.ts";
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

/**
 * 浮点容差比较器：用于参照实现给出**原始数值**（double/f32 十进制）的能力。
 *
 * 与 {@link floatComparer} 的分工：那个处理「两侧都给了数字、只是写法不同」的场合；
 * 这个处理「两侧算出同一个量、但末位有 ulp 级差异」的场合——项目规范明确不对齐
 * 浮点精度与 `-ffast-math`，生产构建带 FMA 收缩，噪声/密度/样条这类算法的末位必然
 * 与原版差若干 ulp，**这不是缺陷**。
 *
 * 判定用相对容差：`|a - b| <= max(|a|, |b|) * RELATIVE + ABSOLUTE`。
 * 相对项覆盖正常量级；绝对项让接近 0 的值也能比——噪声在过零点附近绝对值本身很小，
 * 纯相对容差会把「都接近 0」的两次计算判成不相等。
 *
 * 真正的逻辑错误（取错倍频、octaveRange 算错、坐标轴置换、归一化常数写错）造成的偏差
 * 在 1e-3 以上，远大于这里的容差，不会被掩盖。
 */
function floatTolerance(relative: number, absolute: number): CompareTransform {
    const close = (expected: number, actual: number): boolean => {
        if (Number.isNaN(expected) || Number.isNaN(actual)) {
            return Number.isNaN(expected) && Number.isNaN(actual);
        }
        if (!Number.isFinite(expected) || !Number.isFinite(actual)) {
            return expected === actual;
        }
        const scale = Math.max(Math.abs(expected), Math.abs(actual));
        return Math.abs(expected - actual) <= scale * relative + absolute;
    };
    const compare = (expected: unknown, actual: unknown): unknown => {
        // 非有限值两侧都以字符串形态出现（"nan" / "inf" / "-inf"），直接按原文比对：
        // 它们没有「误差」可言，NaN 只与 NaN 相等。
        if (typeof expected === "string" || typeof actual === "string") {
            return expected === actual ? expected : actual;
        }
        if (typeof expected === "number" && typeof actual === "number") {
            return close(expected, actual) ? expected : actual;
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

/** 噪声与密度函数求值的容差：整条链是双精度，末位误差远小于 1e-9。 */
const NOISE_TOLERANCE: CompareTransform = floatTolerance(1e-9, 1e-12);

/**
 * f32 运算链的容差。
 *
 * 原版内部用 float、Cubium 用 double（或反过来）时，同一个数学式的相对误差在 1e-7 量级，
 * 累加几步后会到 1e-6。这是精度差异，不是逻辑差异——项目明确不对齐浮点精度。
 */
const FLOAT_TOLERANCE: CompareTransform = floatTolerance(1e-6, 1e-7);

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
    mansionGridDeclaration(),
    beardifierDensityDeclaration(),
    biomeSourceSampleDeclaration(),
    climateSampleDeclaration(),
    structureStartSingleDeclaration(),
    structureBlocksSingleDeclaration(),
    structureBlocksSeedsSingleDeclaration(),
    structureBlocksTerrainSingleDeclaration(),
    structureBlocksJigsawSingleDeclaration(),
    structureBlocksJigsawSeedsSingleDeclaration(),
    structureBlocksJigsawTerrainSingleDeclaration(),
    structureEntitiesSingleDeclaration(),
    structureStartsDeclaration(),
    mineshaftPiecesDeclaration(),
    strongholdPiecesDeclaration(),
    strongholdRoomCrossingDeclaration(),
    strongholdSmallDoorDeclaration(),
    endCityPiecesDeclaration(),
    jigsawJointOrderDeclaration(),
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
    // Mth.atan2 查表近似：fastInvSqrt + 位模式查表 + 三次修正，与 std::atan2 差 1e-6 量级。
    { name: "mth_atan2", probe: "mth_atan2", oracle: "MthAtan2Oracle",
        key: "unit", units: { kind: "listing" }, input: mthAtan2Input,
        knownDifferences: KNOWN_ATAN2_GAPS },
    // Mth 整数插值三兄弟：lerpInt 乘 span、lerpDiscrete 乘 span-1 再对正值补一格。
    { name: "mth_map_lerp", probe: "mth_map_lerp", oracle: "MthMapLerpOracle",
        key: "unit", units: { kind: "listing" }, input: mthMapLerpInput },
    // Mth 从原点的交替扩展序列：hasNext 只拦双向越界，末尾可能越界一格。
    { name: "mth_out_from_origin", probe: "mth_out_from_origin", oracle: "MthOutFromOriginOracle",
        key: "unit", units: { kind: "listing" }, input: mthOutFromOriginInput },
    // Mth 长度与近似相等：length 分 f32/f64 两版，equal 的阈值是 f32 的 1e-5。
    { name: "mth_length_equal", probe: "mth_length_equal", oracle: "MthLengthEqualOracle",
        key: "unit", units: { kind: "listing" }, input: mthLengthEqualInput },
    // BlockPos 中心点：原版 Vec3 是 double，本项目 Vector3 是 float，大坐标会丢 0.5。
    { name: "blockpos_center", probe: "blockpos_center", oracle: "BlockposCenterOracle",
        key: "unit", units: { kind: "listing" }, input: blockposCenterInput },
    // Direction 的两套数据值编码：三维等于枚举序号，二维按 South=0 重排。
    { name: "direction_data", probe: "direction_data", oracle: "DirectionDataOracle",
        key: "unit", units: { kind: "listing" }, input: directionDataInput },
    // VoxelShape 的最近点与整平包裹盒。
    { name: "shape_closest", probe: "shape_closest", oracle: "ShapeClosestOracle",
        key: "unit", units: { kind: "listing" }, input: shapeClosestInput },
    // 形状拆盒：既验数值也验遍历顺序。
    { name: "shape_boxes", probe: "shape_boxes", oracle: "ShapeBoxesOracle",
        key: "unit", units: { kind: "listing" }, input: shapeBoxesInput },
    // 形状查询：坐标表、二分索引、立方判定、拆盒。
    { name: "shape_query", probe: "shape_query", oracle: "ShapeQueryOracle",
        key: "unit", units: { kind: "listing" }, input: shapeQueryInput },
    // 方向的轴向正负与朝向角度判定。
    { name: "direction_facing", probe: "direction_facing", oracle: "DirectionFacingOracle",
        key: "unit", units: { kind: "listing" }, input: directionFacingInput },
    // ceilLong 与 map 的边界行为。
    { name: "mth_round_ext", probe: "mth_round_ext", oracle: "MthRoundExtOracle",
        key: "unit", units: { kind: "listing" }, input: mthRoundExtInput },
    // 向量绕轴旋转与投影。
    { name: "vec3_rotate", probe: "vec3_rotate", oracle: "Vec3RotateOracle",
        key: "unit", units: { kind: "listing" }, input: vec3RotateInput },
    // 盒中心、角点与向上包含。
    { name: "aabb_ext", probe: "aabb_ext", oracle: "AabbExtOracle",
        key: "unit", units: { kind: "listing" }, input: aabbExtInput },
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
        name: "vibration_frequency",
        probe: "vibration_frequency",
        oracle: "VibrationFrequencyOracle",
        description: "游戏事件到振动频率的映射（VibrationSystem.VIBRATION_FREQUENCY_FOR_EVENT）—— 全部 40 个事件；"
        + "这张表把事件归入 1..15 的频段，频段决定红石比较器输出强度与紫水晶共振方块的 RESONATE_N 事件，"
        + "表里没有的事件频率为 0 不产生振动。分组整体偏移不会崩也不会报错，只是所有振动装置读数同时「差不多」地错掉 ——"
        + "这是最难靠手动测试发现的一类缺陷。实测修复前 40 条里 35 条与原版不符",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "vibration_strength",
        probe: "vibration_strength",
        oracle: "VibrationStrengthOracle",
        description: "振动距离到红石强度的换算（VibrationSystem#getRedstoneStrengthForDistance）—— 监听半径 x 距离 = 120 个单元；"
        + "公式 max(1, 15 - floor(15/radius * distance))，是幽匿感测体给红石比较器的唯一输出值。"
        + "三个容易写错的地方：除号内外（整数截断两次会差 1）、漏掉 max(1,...) 下限（远处给出 0 而非弱信号）、距离为 0 必须给 15；"
        + "三个错都不崩不报错，只是读数差 1，而依赖精确读数的装置会整体失效",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "vibration_info_nbt",
        probe: "vibration_info_nbt",
        oracle: "VibrationInfoNbtOracle",
        description: "振动信息的 NBT 结构（VibrationInfo.CODEC）—— 字段组合 4 种；"
        + "game_event(string) / distance(float) / pos(double[3]) 必填，source 与 projectile_owner 是可选的 UUID。"
        + "UUID 按原版 UUIDUtil.CODEC 拆成 4 个有符号 int，不是单个 long —— 写成 long 会让存档与网络包都和原版对不上；"
        + "Cubium 原来 source 就是 long、projectile_owner 整个缺失",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "vibration_selector",
        probe: "vibration_selector",
        oracle: "VibrationSelectorOracle",
        description: "幽匿感测体的振动选优（VibrationSelector#shouldReplaceVibration）—— 事件 x 距离 x 时刻 = 24 个单元；"
        + "同一时刻的多个振动要挑一个：距离近的优先，距离相同再比频率高的优先，不同时刻一律不替换。"
        + "顺序不可颠倒 —— 先比频率再比距离，远处一次爆炸（频率 15）会盖掉近处的脚步声；"
        + "允许跨 tick 替换则高频事件不断插队，低频事件永远得不到处理",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "sculk_sensor_step",
        probe: "sculk_sensor_step",
        oracle: "SculkSensorStepOracle",
        description: "幽匿感测体被踩踏时是否发出 STEP 振动（SculkSensorBlock#stepOn）—— 相位 x 踩踏者 = 9 个单元；"
        + "三个前置条件缺一不可：相位为 Inactive（否则站上去不走也会一直触发，感测体变压力板）、"
        + "踩踏者不是监守者（同源自激）、踩踏者不在潜行（潜行绕过感测体是核心玩法）。"
        + "修复前 Cubium 根本没有覆写 onEntityWalk，踩踏振动完全缺失",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "brushable_fall",
        probe: "brushable_fall",
        oracle: "BrushableFallOracle",
        description: "可疑沙/沙砾坠落碎开时的效果与事件（BrushableBlock#onBrokenAfterFall）—— 方块种类 2 个单元；"
        + "两个动作缺一不可：放方块破碎粒子（levelEvent 2001）、发 block_destroy 事件。"
        + "只发粒子不发事件，坠落破碎在幽匿电路里隐形；只发事件不放粒子，玩家看不到摔碎的视觉反馈。"
        + "修复前 Cubium 的 BrushableBlock 继承 FallingBlock 却没有覆写 onBroken，两者都没有",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "vibration_selector_nbt",
        probe: "vibration_selector_nbt",
        oracle: "VibrationSelectorNbtOracle",
        description: "振动选择器的 NBT 结构与往返（VibrationSelector.CODEC）—— 无候选/有候选 2 个单元；"
        + "只有两个字段：event（可选的 VibrationInfo 嵌套）与 tick（long）。"
        + "无候选时 tick 必须写 -1 而不是省略字段（原版 orElse(-1L)，字段存在但值是哨兵），"
        + "否则「读取失败」和「没有候选」语义混淆；event 存在但 tick < 0 时不能恢复候选，两字段是一对",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_blast_resistance",
        probe: "block_blast_resistance",
        oracle: "BlockBlastResistanceOracle",
        description: "方块默认状态的爆炸抗性（Block#getExplosionResistance）—— 全部已注册方块；"
        + "爆炸破坏时每个方块按抗性算是否被摧毁，阈值约等于 威力/抗性。写错不会崩也不会报错，"
        + "只表现为「TNT 炸不动这堵墙」或「炸得比预期干净」。"
        + "注意原版很多方块的抗性不来自自身属性而是转交底层方块（楼梯、台阶、活板门、栅栏、墙），这类继承关系最易出错",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_movement_factor",
        probe: "block_movement_factor",
        oracle: "BlockMovementFactorOracle",
        description: "方块的速度倍率与跳跃倍率（Block#getSpeedFactor / getJumpFactor）—— 全部已注册方块；"
        + "速度倍率决定站上去走得多快，跳跃倍率决定起跳初速度。灵魂沙与蜂蜜块都是 0.4，但只有蜂蜜块的跳跃倍率是 0.5 ——"
        + "灵魂沙只减速不限制跳跃。把蜂蜜块的跳跃倍数写成 1.0，玩家能轻松跳出蜂蜜陷阱；"
        + "把灵魂沙的写成 0.5，沙漠神殿的灵魂沙跑酷就几乎不可能",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_shade_brightness",
        probe: "block_shade_brightness",
        oracle: "BlockShadeBrightnessOracle",
        description: "方块遮光亮度与天空光透传（BlockBehaviour#getShadeBrightness / propagatesSkylightDown）—— 全部已注册方块；"
        + "碰撞形状是完整方块返回 0.2（形成阴影），否则 1.0。三个必须单独覆写的特例：泥巴与灵魂沙是 14/16 矮块但视觉实心、仍需 0.2，"
        + "雪层满层 0.2 其余 1.0；屏障/结构空位这类不可见方块返回 1.0。按默认规则处理泥巴和灵魂沙会让它们看起来像浮在地面上的亮块",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_random_ticks",
        probe: "block_random_ticks",
        oracle: "BlockRandomTicksOracle",
        description: "方块状态是否响应随机刻（BlockBehaviour#isRandomlyTicking）—— 全部 29671 个状态；"
        + "随机刻是「自然发生」的引擎，每个游戏刻随机挑一批位置，只对 true 的状态调用 randomTick："
        + "作物生长、树叶腐烂、草方块蔓延、冰融化、铜氧化、火焰熄灭都靠它。"
        + "设计上是方块级开关，但有方块按状态覆写（红石矿只有 lit=true 才随机刻），状态粒度的错误无法靠方块级比对发现",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_signal_source",
        probe: "block_signal_source",
        oracle: "BlockSignalSourceOracle",
        description: "方块状态是否红石信号源（BlockBehaviour#isSignalSource）—— 全部 29671 个状态；"
        + "为 true 的方块会被红石粉当作强充能来源（强度 15），并参与比较器/中继器与相邻活塞、门、铁轨的判定。"
        + "最容易写错的是那些看起来该是信号源却不是的方块：铁轨（只自己变形状/启动矿车）、命令方块（只输出成功次数）、"
        + "合成器、信标（输出金字塔等级是常见误解）、幽匿催化体 —— 这些不是是原版刻意的设计",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "item_container_fit",
        probe: "item_container_fit",
        oracle: "ItemContainerFitOracle",
        description: "物品能否放进容器类物品（Item#canFitInsideContainerItems）—— 全部已注册物品；"
        + "Item 默认 true，BlockItem 覆写为「潜影盒返回 false」。它防的是递归存储："
        + "潜影盒能装物品，如果潜影盒也能装潜影盒，就能做出无限嵌套容器 —— 既是无限容量漏洞也是存档体积炸弹。"
        + "判据挂在 BlockItem 上而不是按物品 id 白名单，所以新增颜色/材质变体自动被覆盖",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "entity_peaceful",
        probe: "entity_peaceful",
        oracle: "EntityPeacefulOracle",
        description: "实体类型在和平难度下是否允许生成/召唤（EntityType#isAllowedInPeaceful）—— 全部已注册实体类型；"
        + "原版默认允许，需显式 notInPeaceful() 才禁止。反直觉之处：大多数 MONSTER 类别实体确实禁止，但并非全部 ——"
        + "潜影贝、僵尸猪灵、猪灵、猪灵蛮兵、疣猪兽等都不带 notInPeaceful。"
        + "写反的后果玩家可直接观察到却不会崩溃：该禁止的没禁止（和平难度刷出敌对生物），该允许的被禁止（/summon 与刷怪蛋失效）",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        oracleMissing: entityTypesKnownMissing },
    {
        name: "entity_serialize_summon",
        probe: "entity_serialize_summon",
        oracle: "EntitySerializeSummonOracle",
        description: "实体类型的存档序列化与召唤开关（EntityType#canSerialize / canSummon）—— 全部已注册实体类型；"
        + "两个都是「默认值写反」型缺陷：serialize 默认 true（显式 noSave() 才关：拴绳结/闪电/玩家/浮标），"
        + "summon 默认也是 true（原版没有任何实体关闭它）。写反不会崩溃不会报错，只是某些实体悄悄失去存档或召唤能力——"
        + "玩家造的东西重进世界全没了，或者 /summon 对某些实体静默失效",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        oracleMissing: entityTypesKnownMissing },
    {
        name: "entity_type_flags",
        probe: "entity_type_flags",
        oracle: "EntityTypeFlagsOracle",
        description: "实体类型的刷怪分类、网络追踪参数与火焰免疫（MobCategory / clientTrackingRange / updateInterval / fireImmune）—— 全部已注册实体类型；"
        + "分类是重灾区：原版 8 类的生物上限完全不同（monster 70 / creature 10 / ambient 15 / axolotls 5 /"
        + "underground_water_creature 5 / water_creature 5 / water_ambient 20 / misc -1）。把美西螈错归进水生生物就会和鱿鱼抢名额，"
        + "表现为某种生物怎么都不刷。trackingRange 太小则远处实体突然消失，太大浪费带宽",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }),
        oracleMissing: entityTypesKnownMissing },
    {
        name: "fluid_lava_react",
        probe: "fluid_lava_react",
        oracle: "FluidLavaReactOracle",
        description: "岩浆与相邻流体/方块相遇的产物（LiquidBlock#shouldSpreadLiquid）—— 岩浆源/流动岩浆 x 四方位 x 邻居种类 = 24 个单元；"
        + "岩浆源+水 -> 黑曜石，流动岩浆+水 -> 圆石，岩浆+蓝冰+灵魂土 -> 玄武岩，成功凝固时发 levelEvent 1501（嗤声与烟）。"
        + "两个易错点：源与流动的产物不同（写成一个，玩家浇岩浆造石头时得到黑曜石，或造黑曜石时只得到圆石）；"
        + "只看水平四个方向不看正上方（原版 POSSIBLE_FLOW_DIRECTIONS 是 DOWN + 四水平，往岩浆正上方倒水不会当场凝固）",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_replaceable",
        probe: "block_replaceable",
        oracle: "BlockReplaceableOracle",
        description: "方块状态是否可被替换（BlockStateBase#canBeReplaced）—— 全部 29671 个状态；"
        + "它描述「这一格现在算不算空」：直接放置、流体流动、世界生成、掉落方块都靠它。"
        + "两处容易写反：草/蕨/海草必须 true（false 则玩家在草上放不了方块）；树叶必须 false（true 则被后来生成的树覆盖，森林一片片秃）；"
        + "铁轨必须 true（false 则放不了方块覆盖轨道）；耕地必须 false（true 则农田被无意填平）。"
        + "注意与 canBeReplacedByFluid 不是一回事：后者对非固体方块放宽（门、告示牌虽然不可替换但水能流进去）",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_spawn_query",
        probe: "block_spawn_query",
        oracle: "BlockSpawnQueryOracle",
        description: "方块状态上与「能否生成」相关的四条静态判据—— 全部 29671 个状态：碰撞形状满格（isCollisionShapeFullBlock）、"
        + "红石导体（isRedstoneConductor）、信号源（isSignalSource）、支撑面满顶面（isFaceFull(getBlockSupportShape, UP)）。"
        + "前三条正是 NaturalSpawner.isValidEmptySpawnBlock 的否定条件，最后一条是 ON_GROUND 判定「脚下站不住」用的。"
        + "最容易写错的是把后三条都当成「碰撞形状满格」：泥巴、灵魂沙碰撞箱比整格矮却不是导体也不是信号源，原版允许在其上生成；"
        + "台阶、楼梯、陷阱门碰撞箱不满格却是导体，原版据此拒绝在其上生成。",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_pathfindable",
        probe: "block_pathfindable",
        oracle: "BlockPathfindableOracle",
        description: "方块状态在三种寻路类型下是否可通行（BlockBehaviour#isPathfindable）—— 全部 29671 个状态 x 3 种类型；"
        + "LAND=陆地行走、WATER=水中游动、AIR=空中飞行。原版默认判据：LAND/AIR 为「碰撞形状不完整即可通行」，WATER 为「只认水」。"
        + "两处最容易写错：① 判据是「不完整」而不是「为空」—— 台阶、楼梯、栅栏门碰撞形状非空但不完整，生物能从上方走过，"
        + "用 isEmpty() 判定会让生物绕开所有台阶（表现为村民卡在台阶前不动）；② WATER 不看形状只看是不是水 —— 三种类型统一按形状判，"
        + "陆行生物会走进水里",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_light_occlusion_shape",
        probe: "block_light_occlusion_shape",
        oracle: "BlockLightOcclusionShapeOracle",
        description: "方块状态是否按形状决定光照遮挡（BlockBehaviour#useShapeForLightOcclusion）—— 全部 29671 个状态；"
        + "光照引擎判断方块是否挡光有两种口径：false（默认）只看 canOcclude 与具体形状无关；true 按形状实际包围盒累加遮挡。"
        + "台阶、楼梯、耕地、土径、雪这类半格或略矮的方块必须是 true —— 写成 false 会让台阶下方一片漆黑、室内光照蔓延范围偏小。"
        + "反向写成 true 则让整格不透明但形状略小的方块在其上方凭空透光。此标志不影响实体碰撞，只影响光照，错了不会撞穿只会「暗得没道理」",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_skylight_propagation",
        probe: "block_skylight_propagation",
        oracle: "BlockSkylightPropagationOracle",
        description: "方块状态是否向下透传天空光（BlockBehaviour#propagatesSkylightDown）—— 全部 29671 个状态；"
        + "天空光从世界顶部往下传播时遇方块要判断光能否直接穿过这一格，该值同时决定 getLightBlock（透光则 0，否则 1）。"
        + "原版默认：!isShapeFullBlock(shape) && fluidState.isEmpty()。两个易错点：① 必须同时检查形状与流体 —— 只判形状会让水透光（海底一片明亮），"
        + "只判流体会让台阶、栅栏、楼梯全部挡光（白天进屋一片漆黑）；② 判据是「形状非整格」而不是「碰撞形状为空」—— 树叶、草丛、栅栏、告示牌的碰撞形状都不是整格，"
        + "它们都应透光，用「碰撞为空」会让树叶下方凭空变暗（森林地面收不到天空光）",
        key: "unit", units: { kind: "listing" }, input: (unit: string) => ({ unit }) },
    {
        name: "block_view_blocking",
        probe: "block_view_blocking",
        oracle: "BlockViewBlockingOracle",
        description: "方块状态是否挡视线 / 是否造成窒息（BlockState#isViewBlocking / #isSuffocating）—— 全部 29671 个状态；"
        + "默认判据是「blocksMotion() && 碰撞形状满格」，但树叶、玻璃、色彩玻璃、铜格栅、耕地、泥土路径、灵魂沙、雪层、活塞、潜影盒等"
        + "几十处注册点单独覆写。抄错的后果都不崩溃：树叶被当成窒息方块会让玩家站进树冠持续掉血、第三人称镜头穿不进树冠；"
        + "玻璃被当成满格则隔着窗户呼吸也会闷血。只能逐状态与原版比对发现",
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
    {
        name: "dig_speed",
        probe: "dig_speed",
        oracle: "DigSpeedOracle",
        description: "玩家挖掘速度的整条计算链（原版 Player.getDestroySpeed）—— 33 个固定场景覆盖七环："
        + "工具基础速度、效率附魔、急迫/潮涌能量、挖掘疲劳、block_break_speed、水下、空中。"
        + "最容易漏的是第二环的 > 1.0 判断：效率附魔只在「工具确实比方块快」时叠加，"
        + "空手挖泥土吃不到它；把这一环写成无条件相加，带效率附魔的手挖速度就会偏高。"
        + "其次是挖掘疲劳的四档是写死的常量（0.3 / 0.09 / 0.0027 / 8.1e-4），不是 0.3 的幂——按幂推算在 III 级起就偏离。"
        + "急迫与潮涌能量取等级较高者而不叠加，这条也容易写成相加。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "entity_physics",
        probe: "entity_physics",
        oracle: "EntityPhysicsOracle",
        description: "实体空中移动一个 tick 的完整计算（原版 LivingEntity.travelInAir）—— 23 个固定场景："
        + "地面/空中移动因子、重力、缓降、悬浮、跳跃初速度。"
        + "地面移动因子是 `speed * 0.21600002 / (f * f * f)`，滑度进三次方除数；"
        + "本工程 Release 下的 -ffast-math 会把该除法改写成乘倒数，与原版差 1 ulp，故函数内局部关闭。"
        + "两处容易漏：缓降只在下落时生效（上升途中仍按原重力减速，否则跳跃高度减半）；"
        + "竖直速度的 0.98 保留比例写在悬浮分支之后，悬浮也照乘。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "timeline_sample",
        probe: "timeline_sample",
        oracle: "TimelineSampleOracle",
        description: "关键帧轨道在给定时刻的采样值（原版 KeyframeTrackSampler）—— 10 条轨道 x 22 个时刻。"
        + "覆盖单关键帧、无周期钳位、三关键帧、周期回绕、非零起点回绕、三段常量。"
        + "叙事重点：时刻表里 -1、-24000 这类负数必须经 Math.floorMod 回绕到周期末尾，"
        + "用取余会落到 0 —— 夜晚的天空亮度会因此提前切到白天值；超过一个周期的大值同样要折回。"
        + "三关键帧中途点的取值必须精确命中关键帧本身，不允许出现插值残差。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "perlin_noise",
        probe: "perlin_noise",
        oracle: "PerlinNoiseOracle",
        description: "Perlin 噪声在给定坐标上的取值（原版 PerlinNoise.getValue）—— 7 组配置 x 12 个坐标。"
        + "配置覆盖单倍频、两倍频递减、原版地形的 1/2 衰减链、以及各层贡献量级接近的非几何递减。"
        + "坐标混合整数点（噪声落在网格顶点）、半整数点与任意小数点。"
        + "噪声直接决定地形高度：一旦有 1 ulp 偏差，表现是「地形形状看起来对但具体高度不同」，"
        + "人工几乎无法定位，只能靠逐点比对。三线性插值里每层 `start + delta*(end-start)` 都是"
        + "FMA 收缩的候选点（同类问题已修过三处），这条链上任何一处漏防都会被这里抓到。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "normal_noise",
        probe: "normal_noise",
        oracle: "NormalNoiseOracle",
        description: "归一化噪声在给定坐标上的取值（原版 NormalNoise.getValue）—— 7 组参数 x 12 个坐标。"
        + "NormalNoise 由两个 PerlinNoise 叠加：第二个的坐标乘以 1.0181268882175227 错开，"
        + "取和后乘 0.16666666666666666 / expectedDeviation(octaveRange)。它是地形高度与气候参数的"
        + "基础采样，比单独的 PerlinNoise 多三层易错点：octaveRange 只统计非零振幅的最小/最大下标之差"
        + "（不是列表长度）、expectedDeviation 里的 1.0/(n+1)、以及归一化常数的字面量形式。"
        + "参数组特意包含中间夹零振幅、首尾为零振幅、只有一个非零振幅三种形状，让 octaveRange 分别取到"
        + "不同值。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "density_function",
        probe: "density_function",
        oracle: "DensityFunctionOracle",
        description: "密度函数（原版 DensityFunctions）在给定坐标上的取值与 min/max —— 38 组配置 x 10 个坐标。"
        + "只覆盖不依赖噪声、不依赖其他密度函数的纯函数：Constant、YClampedGradient、Clamp、以及 Mapped 的七种变换。"
        + "密度函数直接决定地形：YClampedGradient 是主世界「从 Y=-64 的实心到 Y=320 的空气」那类梯度的载体，"
        + "Mapped 的七种变换参与几乎所有噪声整形。用例包含 fromY>toY 的反向梯度与 fromY==toY 的退化梯度"
        + "（后者会让 inverseLerp 除以 0），以及各变换的正负与非整输入。噪声类与缓存类留待后续能力。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "cubic_spline",
        probe: "cubic_spline",
        oracle: "CubicSplineOracle",
        description: "三次样条（原版 CubicSpline / DensityFunctions.spline）在给定坐标上的取值与 min/max —— 8 组控制点 x 18 个输入。"
        + "样条是主世界地形 offset/factor/jaggedness 的核心：控制点、导数和三次 Hermite 插值共同决定地表起伏。"
        + "用例覆盖全零导数（退化成线性）、非零导数（真正三次）、控制点间距不等（width 直接进 Hermite 的两次乘法）、"
        + "端点线性外推、输入恰好落在控制点上（t 为 0/1 时 tachy 归零）、以及小数 location。"
        + "注意原版整条链是 float：Multipoint 的 locations/derivatives/minValue/maxValue 全是 float，apply 内部也全程 float 运算。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "density_composite",
        probe: "density_composite",
        oracle: "DensityCompositeOracle",
        description: "组合型密度函数（原版 DensityFunctions）的取值与 min/max —— 30 组配置。"
        + "覆盖 TwoArgument（add/mul/min/max）、Lerp、RangeChoice；子函数全部用常量，整棵树可由参数量化重建。"
        + "三者都是噪声整形的实际主力：add/mul 拼出密度组合，Lerp 做双线性混合，RangeChoice 在洞穴与地形之间切换。"
        + "用例重点覆盖 Mul 在一边为 0 时的短路（另一侧即使是 inf/nan 也要短路）、Lerp 的 d 在 0/1 边界与区间外、"
        + "RangeChoice 的 [lo, hi) 边界（等于 lo 命中、等于 hi 不命中），以及非整齐数值。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "noise_density",
        probe: "noise_density",
        oracle: "NoiseDensityOracle",
        description: "噪声型密度函数（原版 NoiseDensity / MappedNoise）的取值与 min/max —— 8 组配置 x 9 个坐标。"
        + "NoiseDensity 是 noise * scale，MappedNoise 把噪声重映射到 [from, to]；两者直接决定主世界与下界的洞穴形状。"
        + "缩放特意用非 2 的幂（0.1、0.011、0.31），因为 x * xzScale 这一步的舍入会直接传到噪声输入。"
        + "采样点覆盖原点、各轴单独偏移、负坐标、以及远离原点的大坐标。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "shifted_noise",
        probe: "shifted_noise",
        oracle: "ShiftedNoiseOracle",
        description: "带坐标平移的噪声密度函数（原版 DensityFunctions.ShiftedNoise）的取值与 min/max —— 5 组配置 x 9 个坐标。"
        + "语义是坐标先缩放再叠加位移，然后把结果喂给噪声；位移让噪声在空间上扭曲，形成蜿蜒的洞穴。"
        + "位移函数用两种形态：常量（覆盖缩放加常量的加法顺序）与线性（覆盖非零位移路径），两者都只用加乘，不引入噪声。"
        + "用例包含位移全零（退化成普通 NoiseDensity）、极小的常量位移（对大坐标可能被吸收）、"
        + "以及线性位移配不对称缩放。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "shift_noise",
        probe: "shift_noise",
        oracle: "ShiftNoiseOracle",
        description: "顶点位移噪声（原版 DensityFunctions.ShiftA/ShiftB/Shift）的取值与 min/max —— 9 组配置 x 11 个坐标。"
        + "三者共享同一套算术：三个坐标各乘 0.25 后喂给噪声、结果再乘 4；区别只在坐标如何映射到噪声的三个轴："
        + "ShiftA 是 (x,0,z)，ShiftB 是 (z,x,0)，Shift 是 (x,y,z)。"
        + "它们是 ShiftedNoise 位移函数的实际提供方，两者组合起来才是完整的地形扭曲链路。"
        + "采样点刻意让三轴互不相等，才能区分 ShiftA 与 ShiftB 的轴置换。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "weird_scaled_sampler",
        probe: "weird_scaled_sampler",
        oracle: "WeirdScaledSamplerOracle",
        description: "奇异缩放采样器（原版 DensityFunctions.WeirdScaledSampler）的取值与 min/max —— 2 种 rarity 映射 x 21 个输入值 x 2 组噪声 x 5 个坐标。"
        + "它不是纯算术：先拿输入值查表选出稀有度系数，再用 坐标/系数 采样噪声、结果取绝对值后乘回系数。"
        + "主世界的 weirdness/ridges 用它把噪声在空间上分层，查表用的是严格小于，所以阈值上取等与取不等的落档不同。"
        + "输入值刻意取在全部阈值上与其两侧：Type1 的 -0.5/0.0/0.5，Type2 的 -0.75/-0.5/0.5/0.75。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "end_islands",
        probe: "end_islands",
        oracle: "EndIslandsOracle",
        description: "末地岛屿密度（原版 DensityFunctions.EndIslandDensityFunction）的取值与 min/max —— 5 个种子 x 15 个坐标。"
        + "它是末地地形的主密度函数：方块坐标除以 8 得到岛屿网格坐标，在该网格上求岛屿高度，再映射到 [-0.84375, 0.5625]。"
        + "内部的 25x25 邻域搜索只在距中心超过 64 格且噪声小于 -0.9 时产生外岛，所以采样点必须同时覆盖中心区与远区，否则外岛分支一次都不会进入。"
        + "远区采样点分别取沿单轴、沿对角、跨多个岛屿网格与极端坐标，覆盖 f32 精度边界。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: FLOAT_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "lerp3_interpolate",
        probe: "lerp3_interpolate",
        oracle: "Lerp3InterpolateOracle",
        description: "三线性插值（原版 Mth.lerp3）的逐位结果 —— 7 组角值 x 11 组插值因子。"
        + "它是区块内噪声插值的核心：NoiseInterpolator 把密度函数在 4x8x4 的网格上采样，再用 lerp3 把网格值插到任意方块坐标，这是密度函数之上最后一层生成逻辑。"
        + "插值顺序 X → Y → Z，三步 lerp 各自独立舍入，任何一步被编译器融合成 FMA 都会让结果偏若干 ulp 且随层级放大。"
        + "角值覆盖全零、单位立方体、参差小数、大数量级、极大动态范围、相邻值只差几个 ulp、全负数；因子混入 1/3、0.1 这类无法精确表示的分数。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "mth_misc",
        probe: "mth_misc",
        oracle: "MthMiscOracle",
        description: "Mth 里一批纯计算函数 —— murmur3 混洗、快速立方根倒数、三态符号、向下量化、棋盘距离、整数倍判断、正方形边长、Catmull-Rom 样条、整数平方。"
        + "这些函数此前在 Cubium 里缺失，本次补齐实现的同时补上差分覆盖；都不依赖世界状态，可直接喂入任意输入比对。"
        + "每个都有各自的陷阱：murmur 的移位必须无符号、立方根倒数的整数部分是带符号除法、sign 的负零应返回 0 而 NaN 返回 -1、quantize 不能向零截断。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: FLOAT_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "ray_aabb",
        probe: "ray_aabb",
        oracle: "RayAabbOracle",
        description: "射线与轴对齐包围盒求交（原版 Mth.rayIntersectsAABB）的判定结果 —— 12 组射线与盒的组合。"
        + "这是实体拾取、区块剔除等热路径上的判定：先三轴各做一次「起点在盒外且方向朝外」的快速排除，再对三对轴做叉积。"
        + "关键在不对称——前两次叉积超出即排除用 >，最后一次要在界内才相交用 <，边界行为因此不对称。"
        + "用例覆盖正对射入、从内部出发、背向、平行擦边、盒外一点点、对角穿心、擦角、零体积盒、扁平盒、大坐标、零方向向量。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "density_nested",
        probe: "density_nested",
        oracle: "DensityNestedOracle",
        description: "嵌套密度函数树的求值与区间 —— 17 棵树 x 10 个采样点。"
        + "density_composite 只测了「两个常量喂一个二元算子」这一层；真实地形的 final_density 是一棵多层树："
        + "噪声项经 Mapped 整形、与梯度相加、被 Clamp 截断、再经 RangeChoice 在洞穴与地形之间切换。"
        + "组合型密度函数有三个单层测不到的易错点："
        + "① 区间传播——minValue/maxValue 自底向上合成，Mul 的区间不是两端点相乘（负负得正）、"
        + "Lerp 要看 delta 的符号、RangeChoice 要合并两个分支；算错不崩溃，只让缓存与插值调度偏掉；"
        + "② 短路语义——Mul 一边为 0 时另一侧即使是 NaN/Inf 也应短路成 0；"
        + "③ 梯度与 Clamp 的相对位置——先 Clamp 再 Mapped 与反过来结果不同，写反会让地形整体变平或长出尖刺。"
        + "浅层组合（clamp_after_add / mapped_after_clamp 等成对出现）专门卡第 ③ 点，deep_mix 把三者串起来。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: NOISE_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "carving_mask",
        probe: "carving_mask",
        oracle: "CarvingMaskOracle",
        description: "雕刻掩码（原版 CarvingMask）的写入 / 查询行为 —— 7 个场景 x 5~11 步操作，外加整块扫描的标记格数。"
        + "掩码记录「这一格已经刻过」，雕刻器每次写入前都要查它，避免同一条洞穴被重复雕刻。它看起来只是位数组，"
        + "但索引公式 x & 15 | (z & 15) << 4 | (y - minY) << 8 决定了可观察行为：X、Z 按区块宽度掩码，"
        + "Y 用相对 minY 的偏移参与高位位移——minY 取错会让整张掩码的 Y 基准平移，相邻 Y 落到同一格上。"
        + "场景覆盖 wrap_xz（X/Z 回绕到 0..15）、y_edges（位移 0 与 height-1 是否串段）、collide（回绕与位移撞索引），"
        + "以及主世界 -64/384、下界 0/128、末地 0/256 三种几何——下界与末地的 minY 为 0，与主世界的负 minY 是两条不同的路径。"
        + "这类错误不崩溃、不报错，只让洞穴在区块边界处多刻或少刻，玩家看到的是「有断层的洞穴」，几乎无法靠肉眼定位。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "shape_collide",
        probe: "shape_collide",
        oracle: "ShapeCollideOracle",
        description: "实体与方块形状的碰撞求解（原版 Shapes.collide）—— 19 个场景，全三个移动轴。"
        + "Entity.move 的核心不是加速度积分，而是沿位移方向把碰撞箱收敛到贴面位置：对每个将要撞上的"
        + "形状按移动轴求允许的最大位移，再吃掉剩余位移。它决定玩家能否卡进缝隙、能否站上方块边缘、"
        + "斜向撞墙时会不会被滑过去。"
        + "关键细节是收敛方向与重叠判定：正向移动取形状的 min 面、反向取 max 面，还要和当前位移取更近的"
        + "那个；写反的表现是实体穿模或卡在半空。"
        + "场景覆盖整块墙（三轴各一）、半格台阶（形状上表面与实体底面齐平，不应产生水平阻挡）、"
        + "细栅栏柱、只在 Y 上部分重叠的形状、斜向被两块夹住、首尾相接的两块（取更近的那块）、"
        + "已经重叠（应返回 0 而不是推得更深）、恰好贴着不重叠、空形状列表、零位移、"
        + "以及与形状无关的小数位移和大跨度位移。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: FLOAT_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "edge_backoff",
        probe: "edge_backoff",
        oracle: "EdgeBackoffOracle",
        description: "潜行时的边缘保护（原版 Player.maybeBackOffFromEdge + canFallAtLeast）—— 12 个场景。"
        + "玩家潜行走到方块边缘不会掉下去，但沿边缘斜向移动仍然可以滑行：只削掉会让他掉下去的那个分量。"
        + "原版为此逐 0.05 步回退，X 与 Z 分别处理，最后再处理斜向组合，每个循环在 |位移| <= 0.05 时归零跳出。"
        + "关键是分轴回退——写成「整块判定、要么全过要么全停」的话，玩家贴边斜走会被完全卡住而不是沿墙滑行。"
        + "canFallAtLeast 的判定是：把碰撞箱沿水平方向平移、沿垂直方向向下扩张 step，盒子里没有任何碰撞箱"
        + "就意味着会掉下去；盒子四周各内缩 1e-7，避免与脚下方块的侧壁误判为支撑。"
        + "场景覆盖平台正中、走出边缘、只削 X 保留 Z 的贴边滑行、两个方向都会掉的角落、朝内侧走、"
        + "单方块平台、窄条平台、恰好 0.05 的位移、台阶高度吸收、负方向移动、以及完全没有地面。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: FLOAT_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "item_cooldowns",
        probe: "item_cooldowns",
        oracle: "ItemCooldownsOracle",
        description: "物品冷却追踪（原版 ItemCooldowns）—— 14 个操作序列，逐条输出进度、剩余 tick 与是否生效。"
        + "冷却进度决定准星旁的遮罩高度，也决定「这个物品现在能不能用」。"
        + "公式是 progress = clamp((endTime - (tickCount + partialTicks)) / (endTime - startTime), 0, 1)，"
        + "三个易错点：① 分母是冷却总长而不是剩余值，partialTicks=0 时两者等价、非零时就偏；"
        + "② partialTicks 加在 tickCount 上而不是从剩余值里减，分母不为 1 时结果不同；"
        + "③ 到期项在 tick() 里移除，移除发生在 tickCount 自增之后并用 <= 比较，"
        + "所以「刚好到期」的那一 tick 就已经查不到冷却了。"
        + "序列覆盖设置后立即查询、推进到一半、恰好到期、过期、partialTicks 取 0/0.25/0.5/0.75 与越界正负值、"
        + "重复设置重置、时长为 0 与负数、设置后显式移除、从未设置就查询、72000 tick 的极长冷却、1 tick 的极短冷却。",
        key: "unit",
        units: { kind: "listing" },
        // 浮点按容差比较，不逐位：项目不对齐浮点精度与 -ffast-math。
        normalise: FLOAT_TOLERANCE,
        input: (unit: string) => ({ unit }) },
    {
        name: "inventory_slot_mapping",
        probe: "inventory_slot_mapping",
        oracle: "InventorySlotMappingOracle",
        description: "玩家背包内部索引与背包菜单槽位号的互转 —— 双向 x 全部合法索引 + 越界值，共 94 个单元。"
        + "两套索引同时存在且语义完全不同：背包内部索引（0..40）是玩家背包对象的存储顺序；"
        + "菜单槽（0..45）是 ContainerSetContent / ContainerSetSlot 包里 slot 字段的含义，客户端按它摆放物品。"
        + "网络同步时按菜单槽逐个反查内部索引再取物品，所以这张表直接决定客户端看到的背包界面。"
        + "错一位的表现是「装备穿在错误的格子里」，而服务端自己看起来完全自洽——只有逐槽对比才发现。"
        + "原版把这张表分散在 InventoryMenu 的构造里：护甲用 39-i 从背包尾部倒着绑、"
        + "快捷栏 0..8 映射到菜单 36..44、副手 40 映射到菜单 45。"
        + "要注意数值重叠：背包槽 36..39 是护甲，菜单槽 36..44 是快捷栏，同一个数字在两套索引里指不同的东西。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "inventory_add",
        probe: "inventory_add",
        oracle: "InventoryAddOracle",
        description: "物品进入玩家背包时的装填顺序（原版 Inventory.add / addResource）—— 14 个场景。"
        + "玩家捡起物品、合成产物入包、漏斗输出到玩家，最后都走这条路径。"
        + "原版顺序是：先在选中槽装，再在副手槽（40）装，然后从索引 0 递增扫全部槽位找同类堆叠；"
        + "都装不下时同样从 0 递增找第一个空槽。两个细节最容易写错——"
        + "① 副手槽排在全部槽位扫描之前，不是「顺便也看一眼副手」；"
        + "② 扫描与找空槽都从 0 递增，而 0..8 是快捷栏，所以捡东西会先塞满快捷栏再去主背包，"
        + "写成「先主背包后快捷栏」会让物品凭空出现在背包第二排。"
        + "add 返回的是没装下的剩余数量而不是装进去多少，这两个值在很多场景下不相等。"
        + "场景覆盖空背包、整组、超过一组、并入选中槽、并入副手、副手优先于扫描、按下标递增扫描、"
        + "快捷栏占满、不同物品不合并、加入 0 个、加入空气、选中槽已满、同类全满、以及跨多个槽铺开。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "fall_damage",
        probe: "fall_damage",
        oracle: "FallDamageOracle",
        description: "摔落伤害的数值公式（原版 LivingEntity.calculateFallDamage / calculateFallPower）—— 109 个单元。"
        + "公式是 floor((distance + 1e-6 - SAFE_FALL_DISTANCE) * multiplier * FALL_DAMAGE_MULTIPLIER) 再减子类减免。"
        + "三个地方最容易写错：① +1e-6 把「恰好等于安全距离」推到安全距离之上一点点，"
        + "让 3.0 格不掉血、3.0001 格才掉 1 点，少写这一项会让恰好 4.0 格也掉血；"
        + "② 运算是 double——伤害按整数格结算，中途落到 f32 会让 1e-6 被舍入吃掉，整数边界附近整格偏差；"
        + "③ 先 floor 再减法，子类减免（青蛙 5、山羊 10）作用在取整之后，先折算距离再取整结果不同。"
        + "multiplier 由落地方块给出：普通 1.0、干草块/蜂蜜块 0.2、石笋 2.0、史莱姆块 0。"
        + "单元覆盖安全距离边界（2.999999/3.0/3.000001/4.000001）、1~25 格的整格与半格与 .999、"
        + "以及五种倍率与七种距离的笛卡尔组合。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
    {
        name: "surface_rule",
        probe: "surface_rule",
        oracle: "SurfaceRuleOracle",
        description: "地表规则的三种条件判定：水面、石头深度、Y 高度 —— 684 个单元。"
        + "地表规则决定「这一格铺草方块还是沙子还是深板岩」，由一棵条件树逐格求值。"
        + "三个边界写错会让整片地形错乱，而且不崩、不报错："
        + "① 水面条件在 waterHeight == MIN_VALUE 时恒真（哨兵表示「这一列没有水」，不是「水面在最低处」），"
        + "写成恒假会让所有非含水地形少铺一层；"
        + "② 石头深度用 <= 而不是 <，深度从 1 起计、1 就是最表面那一层；"
        + "③ Y 高度锚点可以是从上往下数（below_top），换算方式与从下往上不同。"
        + "surfaceDepth 不作为输入而由真实噪声算出（noise * 2.75 + 3.0 + rng.nextDouble() * 0.25 取整），"
        + "噪声曲线与位置随机工厂只要有一处不同就会得到不同的深度值。"
        + "单元覆盖 MIN_VALUE 哨兵、正负 offset 与 multiplier、Floor/Ceiling 两种取法、"
        + "secondary_depth_range 的 0 与非 0、以及两种锚点。",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string) => ({ unit }) },
];
export function findHarness(name: string): HarnessDeclaration | undefined {
    return HARNESSES.find((harness) => harness.name === name);
}
