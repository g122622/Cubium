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
#include "common/core/Types.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/item/loot/conditions/BlockStateCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/conditions/LootConditions.hpp"
#include "common/item/loot/conditions/TableBonusCondition.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/item/loot/functions/LootFunctions.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
     * @brief 序列化 TableBonusCondition 到 JSON
     */
    static nlohmann::json toJson(const TableBonusCondition& condition);

    /**
     * @brief 序列化 StatePropertiesPredicate 到 JSON
     */
    static nlohmann::json toJson(const StatePropertiesPredicate& predicate);

    // ========================================================================
    // LootFunction 解析
    // ========================================================================

    /**
     * @brief 从 JSON 解析掉落函数
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

    static Result<std::unique_ptr<LootCondition>> _parseSilkTouchCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseFortuneCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseRandomChanceCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseRandomChanceWithLootingCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseInvertedCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseAlternativeCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseBlockStatePropertyCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseMatchToolCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseKilledByPlayerCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseEntityPropertiesCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseSurvivesExplosionCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseEntityScoresCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseLocationCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseWeatherCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseTimeCheckCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseDamageSourcePropertiesCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseReferenceCondition(const nlohmann::json& json);
    static Result<std::unique_ptr<LootCondition>> _parseTableBonusCondition(const nlohmann::json& json);

    // ========================================================================
    // 函数解析辅助方法
    // ========================================================================

    static Result<std::unique_ptr<LootFunction>> _parseSetCountFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseApplyBonusFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseLootingEnchantFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetDamageFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetNameFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetLoreFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseLimitCountFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseFurnaceSmeltFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseEnchantWithLevelsFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseEnchantRandomlyFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseExplosionDecayFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetNbtFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseCopyNameFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseCopyBlockStateFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseCopyNbtFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseFillPlayerHeadFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetAttributesFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetContentsFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetLootTableFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseExplorationMapFunction(const nlohmann::json& json);
    static Result<std::unique_ptr<LootFunction>> _parseSetStewEffectFunction(const nlohmann::json& json);

    // ========================================================================
    // 条目解析辅助方法
    // ========================================================================

    static Result<std::unique_ptr<LootEntry>> _parseEmptyEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseItemEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseTableEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseTagEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseDynamicEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseAlternativesEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseSequenceEntry(const nlohmann::json& json);
    static Result<std::unique_ptr<LootEntry>> _parseGroupEntry(const nlohmann::json& json);

    /**
     * @brief 解析条目的公共属性（weight, quality, conditions, functions）
     */
    static void _parseEntryBase(LootEntry& entry, const nlohmann::json& json);
};

} // namespace loot
} // namespace mc
