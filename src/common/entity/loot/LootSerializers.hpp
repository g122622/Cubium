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
 * @file LootSerializers.hpp
 * @brief 掉落表 JSON 序列化器
 *
 * 提供从 JSON 解析掉落表的功能，兼容 MC 1.16.5 数据包格式。
 *
 * 支持的掉落表结构：
 * - pools: 掉落池数组
 * - functions: 掉落函数数组
 * - type: 参数集类型（可选）
 *
 * 参考: net.minecraft.loot.LootSerializers
 */

#pragma once

#include "common/core/Result.hpp"
#include "common/entity/loot/LootConditions.hpp"
#include "common/entity/loot/LootEntry.hpp"
#include "common/entity/loot/LootFunctions.hpp"
#include "common/entity/loot/LootPool.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace mc {
namespace loot {

/**
 * @brief 掉落表序列化器
 *
 * 提供从 JSON 解析掉落表各组件的功能。
 * 完全兼容 Minecraft 1.16.5 数据包格式。
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:block",
 *   "pools": [
 *     {
 *       "rolls": 1,
 *       "bonus_rolls": {
 *         "min": 0,
 *         "max": 1
 *       },
 *       "entries": [
 *         {
 *           "type": "minecraft:item",
 *           "name": "minecraft:diamond",
 *           "weight": 10,
 *           "functions": [
 *             {
 *               "function": "minecraft:set_count",
 *               "count": {
 *                 "min": 1,
 *                 "max": 3
 *               }
 *             }
 *           ],
 *           "conditions": [
 *             {
 *               "condition": "minecraft:random_chance",
 *               "chance": 0.5
 *             }
 *           ]
 *         }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 */
class LootSerializers {
public:
    // ========================================================================
    // RandomValueRange 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析随机值范围
     *
     * 支持格式：
     * - 固定值: 5 或 {"min": 5, "max": 5}
     * - 范围: {"min": 1, "max": 3} 或 {"min": 1.0, "max": 3.0}
     * - 带 type: {"type": "minecraft:uniform", "min": 1, "max": 3}
     *
     * @param json JSON 数据
     * @return 解析的随机值范围
     */
    static Result<RandomValueRange> parseRandomValueRange(const nlohmann::json& json);

    /**
     * @brief 序列化随机值范围到 JSON
     */
    static nlohmann::json toJson(const RandomValueRange& range);

    /**
     * @brief 从 JSON 解析二项分布范围
     *
     * 格式: {"type": "minecraft:binomial", "n": 5, "p": 0.5}
     */
    static Result<BinomialRange> parseBinomialRange(const nlohmann::json& json);

    /**
     * @brief 序列化二项分布范围到 JSON
     */
    static nlohmann::json toJson(const BinomialRange& range);

    /**
     * @brief 从 JSON 解析常量范围
     *
     * 格式: 纯数字或 {"type": "minecraft:constant", "value": 5}
     */
    static Result<ConstantRange> parseConstantRange(const nlohmann::json& json);

    /**
     * @brief 序列化常量范围到 JSON
     */
    static nlohmann::json toJson(const ConstantRange& range);

    /**
     * @brief 从 JSON 解析 IRandomRange（自动识别类型）
     *
     * 根据格式自动识别类型：
     * - 纯数字 -> ConstantRange
     * - {"type": "minecraft:uniform", ...} -> RandomValueRange
     * - {"type": "minecraft:binomial", ...} -> BinomialRange
     * - {"type": "minecraft:constant", ...} -> ConstantRange
     * - 无 type 的对象 -> RandomValueRange（默认）
     */
    static Result<std::unique_ptr<math::IRandomRange>> parseRandomRange(const nlohmann::json& json);

    // ========================================================================
    // LootCondition 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落条件
     *
     * 根据 condition 字段自动识别类型：
     * - minecraft:silk_touch -> SilkTouchCondition
     * - minecraft:table_bonus -> FortuneCondition
     * - minecraft:random_chance -> RandomChanceCondition
     * - minecraft:random_chance_with_looting -> RandomChanceWithLuckCondition
     * - minecraft:inverted -> NotCondition
     * - minecraft:alternative -> AndCondition
     * - minecraft:block_state_property -> BlockStateCondition
     * - minecraft:match_tool -> ToolTypeCondition
     */
    static Result<std::unique_ptr<LootCondition>> parseCondition(const nlohmann::json& json);

    /**
     * @brief 从 JSON 数组解析条件列表
     */
    static Result<std::vector<std::unique_ptr<LootCondition>>> parseConditions(const nlohmann::json& json);

    /**
     * @brief 序列化条件到 JSON
     */
    static nlohmann::json toJson(const LootCondition& condition);

    /**
     * @brief 序列化 BlockStateCondition 到 JSON
     */
    static nlohmann::json toJson(const BlockStateCondition& condition);

    /**
     * @brief 序列化 StatePropertiesPredicate 到 JSON
     */
    static nlohmann::json toJson(const StatePropertiesPredicate& predicate);

    // ========================================================================
    // LootFunction 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落函数
     *
     * 根据 function 字段自动识别类型：
     * - minecraft:set_count -> SetCountFunction
     * - minecraft:apply_bonus -> ApplyBonusFunction
     * - minecraft:looting_enchant -> LootingEnchantBonusFunction
     * - minecraft:set_damage -> SetDamageFunction
     * - minecraft:set_name -> SetNameFunction
     * - minecraft:set_lore -> SetLoreFunction
     * - minecraft:limit_count -> LimitCountFunction
     * - minecraft:furnace_smelt -> FurnaceSmeltFunction
     * - minecraft:enchant_with_levels -> EnchantWithLevelsFunction
     * - minecraft:enchant_randomly -> EnchantRandomlyFunction
     * - minecraft:explosion_decay -> ExplosionDecayFunction
     * - minecraft:set_nbt -> SetNbtFunction
     * - minecraft:copy_name -> CopyNameFunction
     * - minecraft:copy_block_state -> CopyBlockStateFunction
     * - minecraft:copy_nbt -> CopyNbtFunction
     * - minecraft:fill_player_head -> FillPlayerHeadFunction
     * - minecraft:set_attributes -> SetAttributesFunction
     * - minecraft:set_contents -> SetContentsFunction
     * - minecraft:set_loot_table -> SetLootTableFunction
     * - minecraft:exploration_map -> ExplorationMapFunction
     * - minecraft:set_stew_effect -> SetStewEffectFunction
     */
    static Result<std::unique_ptr<LootFunction>> parseFunction(const nlohmann::json& json);

    /**
     * @brief 从 JSON 数组解析函数列表
     */
    static Result<std::vector<std::unique_ptr<LootFunction>>> parseFunctions(const nlohmann::json& json);

    /**
     * @brief 序列化函数到 JSON
     */
    static nlohmann::json toJson(const LootFunction& function);

    // ========================================================================
    // LootEntry 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落条目
     *
     * 根据 type 字段自动识别类型：
     * - minecraft:empty -> EmptyLootEntry
     * - minecraft:item -> ItemLootEntry
     * - minecraft:loot_table -> TableLootEntry
     * - minecraft:alternatives -> AlternativesLootEntry
     * - minecraft:sequence -> SequenceLootEntry
     * - minecraft:group -> GroupLootEntry
     */
    static Result<std::unique_ptr<LootEntry>> parseEntry(const nlohmann::json& json);

    /**
     * @brief 从 JSON 数组解析条目列表
     */
    static Result<std::vector<std::unique_ptr<LootEntry>>> parseEntries(const nlohmann::json& json);

    /**
     * @brief 序列化条目到 JSON
     */
    static nlohmann::json toJson(const LootEntry& entry);

    // ========================================================================
    // LootPool 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落池
     *
     * JSON 字段：
     * - name: 池名称（可选）
     * - rolls: 掷骰次数（必需）
     * - bonus_rolls: 额外掷骰次数（可选）
     * - entries: 条目数组（必需）
     * - functions: 函数数组（可选）
     * - conditions: 条件数组（可选）
     */
    static Result<std::unique_ptr<LootPool>> parsePool(const nlohmann::json& json);

    /**
     * @brief 从 JSON 数组解析池列表
     */
    static Result<std::vector<std::unique_ptr<LootPool>>> parsePools(const nlohmann::json& json);

    /**
     * @brief 序列化池到 JSON
     */
    static nlohmann::json toJson(const LootPool& pool);

    // ========================================================================
    // LootTable 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落表
     *
     * JSON 字段：
     * - type: 参数集类型（可选，默认 generic）
     * - pools: 掉落池数组（可选）
     * - functions: 函数数组（可选）
     *
     * @param json JSON 数据
     * @return 解析的掉落表
     */
    static Result<std::unique_ptr<LootTable>> parseLootTable(const nlohmann::json& json);

    /**
     * @brief 从 JSON 字符串解析掉落表
     * @param jsonStr JSON 字符串
     * @return 解析的掉落表
     */
    static Result<std::unique_ptr<LootTable>> parseLootTable(const std::string& jsonStr);

    /**
     * @brief 序列化掉落表到 JSON
     */
    static nlohmann::json toJson(const LootTable& table);

    /**
     * @brief 序列化掉落表到 JSON 字符串
     */
    static std::string toJsonString(const LootTable& table, i32 indent = -1);

private:
    // ========================================================================
    // 条件解析辅助方法
    // ========================================================================

    static Result<std::unique_ptr<LootCondition>> parseSilkTouchCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseFortuneCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseRandomChanceCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseRandomChanceWithLootingCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseInvertedCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseAlternativeCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseBlockStatePropertyCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseMatchToolCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseKilledByPlayerCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseEntityPropertiesCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseSurvivesExplosionCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseEntityScoresCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseLocationCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseWeatherCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseTimeCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseDamageSourcePropertiesCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseReferenceCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> parseTableBonusCondition(const nlohmann::json& json);

    // ========================================================================
    // 函数解析辅助方法
    // ========================================================================

    static Result<std::unique_ptr<LootFunction>> parseSetCountFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseApplyBonusFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseLootingEnchantFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetDamageFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetNameFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetLoreFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseLimitCountFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseFurnaceSmeltFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseEnchantWithLevelsFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseEnchantRandomlyFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseExplosionDecayFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetNbtFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseCopyNameFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseCopyBlockStateFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseCopyNbtFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseFillPlayerHeadFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetAttributesFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetContentsFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetLootTableFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseExplorationMapFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> parseSetStewEffectFunction(const nlohmann::json& json);

    // ========================================================================
    // 条目解析辅助方法
    // ========================================================================

    static Result<std::unique_ptr<LootEntry>> parseEmptyEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseItemEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseTableEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseTagEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseDynamicEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseAlternativesEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseSequenceEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> parseGroupEntry(const nlohmann::json& json);

    /**
     * @brief 解析条目的公共属性（weight, quality, conditions, functions）
     *
     * 注意：functions 在当前实现中暂不支持（Entry 级别的函数），
     * 因为当前 LootEntry 类没有函数列表。
     */
    static void parseEntryBase(LootEntry& entry, const nlohmann::json& json);
};

} // namespace loot
} // namespace mc
