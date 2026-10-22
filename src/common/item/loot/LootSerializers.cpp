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

#include "LootSerializers.hpp"
#include "StatePropertiesPredicate.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/component/Fireworks.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/conditions/BlockStateCondition.hpp"
#include "common/item/loot/conditions/DamageSourcePropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityScoresCondition.hpp"
#include "common/item/loot/conditions/FishingOpenWaterCondition.hpp"
#include "common/item/loot/conditions/FortuneCondition.hpp"
#include "common/item/loot/conditions/KilledByPlayerCondition.hpp"
#include "common/item/loot/conditions/LocationCheckCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/conditions/MatchToolCondition.hpp"
#include "common/item/loot/conditions/NotCondition.hpp"
#include "common/item/loot/conditions/OrCondition.hpp"
#include "common/item/loot/conditions/RandomChanceCondition.hpp"
#include "common/item/loot/conditions/RandomChanceWithLuckCondition.hpp"
#include "common/item/loot/conditions/ReferenceCondition.hpp"
#include "common/item/loot/conditions/SilkTouchCondition.hpp"
#include "common/item/loot/conditions/SurvivesExplosionCondition.hpp"
#include "common/item/loot/conditions/TableBonusCondition.hpp"
#include "common/item/loot/conditions/TimeCheckCondition.hpp"
#include "common/item/loot/conditions/WeatherCheckCondition.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/entries/AlternativesLootEntry.hpp"
#include "common/item/loot/entries/DynamicLootEntry.hpp"
#include "common/item/loot/entries/EmptyLootEntry.hpp"
#include "common/item/loot/entries/GroupLootEntry.hpp"
#include "common/item/loot/entries/ItemLootEntry.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/loot/entries/SequenceLootEntry.hpp"
#include "common/item/loot/entries/TableLootEntry.hpp"
#include "common/item/loot/entries/TagLootEntry.hpp"
#include "common/item/loot/functions/ApplyBonusFunction.hpp"
#include "common/item/loot/functions/CopyComponentsFunction.hpp"
#include "common/item/loot/functions/CopyCustomDataFunction.hpp"
#include "common/item/loot/functions/CopyNameFunction.hpp"
#include "common/item/loot/functions/CopyStateFunction.hpp"
#include "common/item/loot/functions/DiscardItemFunction.hpp"
#include "common/item/loot/functions/EnchantRandomlyFunction.hpp"
#include "common/item/loot/functions/EnchantWithLevelsFunction.hpp"
#include "common/item/loot/functions/EnchantedCountIncreaseFunction.hpp"
#include "common/item/loot/functions/ExplorationMapFunction.hpp"
#include "common/item/loot/functions/ExplosionDecayFunction.hpp"
#include "common/item/loot/functions/FillPlayerHeadFunction.hpp"
#include "common/item/loot/functions/FilteredFunction.hpp"
#include "common/item/loot/functions/FunctionReferenceFunction.hpp"
#include "common/item/loot/functions/FurnaceSmeltFunction.hpp"
#include "common/item/loot/functions/LimitCountFunction.hpp"
#include "common/item/loot/functions/ListOperation.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/item/loot/functions/LootFunctionManager.hpp"
#include "common/item/loot/functions/ModifyContentsFunction.hpp"
#include "common/item/loot/functions/SequenceFunction.hpp"
#include "common/item/loot/functions/SetAttributesFunction.hpp"
#include "common/item/loot/functions/SetBannerPatternFunction.hpp"
#include "common/item/loot/functions/SetBookCoverFunction.hpp"
#include "common/item/loot/functions/SetComponentsFunction.hpp"
#include "common/item/loot/functions/SetContentsFunction.hpp"
#include "common/item/loot/functions/SetCountFunction.hpp"
#include "common/item/loot/functions/SetCustomDataFunction.hpp"
#include "common/item/loot/functions/SetCustomModelDataFunction.hpp"
#include "common/item/loot/functions/SetDamageFunction.hpp"
#include "common/item/loot/functions/SetEnchantmentsFunction.hpp"
#include "common/item/loot/functions/SetFireworkExplosionFunction.hpp"
#include "common/item/loot/functions/SetFireworksFunction.hpp"
#include "common/item/loot/functions/SetInstrumentFunction.hpp"
#include "common/item/loot/functions/SetItemFunction.hpp"
#include "common/item/loot/functions/SetLootTableFunction.hpp"
#include "common/item/loot/functions/SetLoreFunction.hpp"
#include "common/item/loot/functions/SetNameFunction.hpp"
#include "common/item/loot/functions/SetOminousBottleAmplifierFunction.hpp"
#include "common/item/loot/functions/SetPotionFunction.hpp"
#include "common/item/loot/functions/SetStewEffectFunction.hpp"
#include "common/item/loot/functions/SetWritableBookPagesFunction.hpp"
#include "common/item/loot/functions/SetWrittenBookPagesFunction.hpp"
#include "common/item/loot/functions/ToggleTooltipsFunction.hpp"
#include "common/item/potion/PotionRegistry.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include "common/world/map/MapDecoration.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

// ============================================================================
// 辅助函数：将派生类 unique_ptr 转换为基类 unique_ptr
// ============================================================================

namespace {

/**
 * @brief 将派生类 unique_ptr 转换为基类 unique_ptr
 *
 * 必须经由 unique_ptr 的隐式派生→基转换构造函数（即隐式 upcast），不能直接
 * `std::unique_ptr<Base>(ptr.release())`：后者由裸指针构造，ncv 下多继承或含虚基类的
 * 派生对象会得到错位的 this 指针（实测 dynamic_cast 返回非空但字段全错）。
 */
template <typename Derived, typename Base>
std::unique_ptr<Base> castToBase(std::unique_ptr<Derived> ptr)
{
    return ptr;
}

} // anonymous namespace

// ============================================================================
// RandomValueRange 解析
// ============================================================================

Result<RandomValueRange> LootSerializers::parseRandomValueRange(const nlohmann::json& json)
{
    // 纯数字 -> 固定值
    if (json.is_number_integer()) {
        i32 value = json.get<i32>();
        return RandomValueRange(static_cast<f32>(value), static_cast<f32>(value));
    }
    if (json.is_number_float()) {
        f32 value = json.get<f32>();
        return RandomValueRange(value, value);
    }

    // 对象格式
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "RandomValueRange must be a number or object");
    }

    // 检查是否指定了 type
    std::string type = "minecraft:uniform"; // 默认类型
    if (json.contains("type") && json["type"].is_string()) {
        type = json["type"].get<std::string>();
    }

    // 常量范围：值固定，可直接收窄为等值随机范围
    if (type == "minecraft:constant" || type == "constant") {
        auto result = parseConstantRange(json);
        if (!result.success()) {
            return result.error();
        }
        return RandomValueRange(
            static_cast<f32>(result.value().getValue()), static_cast<f32>(result.value().getValue()));
    }

    if (type == "minecraft:uniform" || type == "uniform") {
        // 统一分布
        f32 min = 0.0f;
        f32 max = 0.0f;

        if (json.contains("min")) {
            if (json["min"].is_number_integer()) {
                min = static_cast<f32>(json["min"].get<i32>());
            } else if (json["min"].is_number_float()) {
                min = json["min"].get<f32>();
            }
        }

        if (json.contains("max")) {
            if (json["max"].is_number_integer()) {
                max = static_cast<f32>(json["max"].get<i32>());
            } else if (json["max"].is_number_float()) {
                max = json["max"].get<f32>();
            }
        }

        return RandomValueRange(min, max);
    }

    return Error(ErrorCode::InvalidData, "Unknown RandomValueRange type: " + type);
}

nlohmann::json LootSerializers::toJson(const RandomValueRange& range)
{
    if (range.isFixed()) {
        return static_cast<i32>(range.getMin());
    }
    return {{"min", range.getMin()}, {"max", range.getMax()}};
}

Result<BinomialRange> LootSerializers::parseBinomialRange(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "BinomialRange must be an object");
    }

    // n 与 p 在原版 JSON 中是 double（数据包常见写法 "n": 3.0、"p": 0.53...），
    // 试验次数取整后使用
    if (!json.contains("n") || !json["n"].is_number()) {
        return Error(ErrorCode::InvalidData, "BinomialRange missing numeric 'n' field");
    }

    if (!json.contains("p") || !json["p"].is_number()) {
        return Error(ErrorCode::InvalidData, "BinomialRange missing numeric 'p' field");
    }

    const f64 nValue = json["n"].get<f64>();
    if (nValue < 0.0 || nValue > static_cast<f64>(std::numeric_limits<i32>::max())) {
        return Error(ErrorCode::InvalidData, "BinomialRange 'n' out of range");
    }
    i32 n = static_cast<i32>(std::lround(nValue));
    f32 p = json["p"].get<f32>();

    return BinomialRange(n, p);
}

nlohmann::json LootSerializers::toJson(const BinomialRange& range)
{
    return {{"type", "minecraft:binomial"}, {"n", range.getN()}, {"p", range.getP()}};
}

Result<ConstantRange> LootSerializers::parseConstantRange(const nlohmann::json& json)
{
    // 纯数字
    if (json.is_number_integer()) {
        return ConstantRange(json.get<i32>());
    }

    // 对象格式
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "ConstantRange must be a number or object");
    }

    if (!json.contains("value")) {
        return Error(ErrorCode::InvalidData, "ConstantRange missing 'value' field");
    }

    i32 value = 0;
    if (json["value"].is_number_integer()) {
        value = json["value"].get<i32>();
    } else if (json["value"].is_number_float()) {
        value = static_cast<i32>(json["value"].get<f32>());
    }

    return ConstantRange(value);
}

nlohmann::json LootSerializers::toJson(const ConstantRange& range)
{
    return range.getValue();
}

Result<std::unique_ptr<math::IRandomRange>> LootSerializers::parseRandomRange(const nlohmann::json& json)
{
    // 纯数字 -> 常量
    if (json.is_number()) {
        i32 value = json.is_number_integer() ? json.get<i32>() : static_cast<i32>(json.get<f32>());
        return std::unique_ptr<math::IRandomRange>(std::make_unique<ConstantRange>(value));
    }

    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "RandomRange must be a number or object");
    }

    // 获取类型
    std::string type = "minecraft:uniform"; // 默认类型
    if (json.contains("type") && json["type"].is_string()) {
        type = json["type"].get<std::string>();
    }

    // 根据类型分发
    if (type == "minecraft:uniform" || type == "uniform") {
        auto result = parseRandomValueRange(json);
        if (!result.success()) {
            return result.error();
        }
        return std::unique_ptr<math::IRandomRange>(std::make_unique<RandomValueRange>(result.value()));
    } else if (type == "minecraft:binomial" || type == "binomial") {
        auto result = parseBinomialRange(json);
        if (!result.success()) {
            return result.error();
        }
        return std::unique_ptr<math::IRandomRange>(std::make_unique<BinomialRange>(result.value()));
    } else if (type == "minecraft:constant" || type == "constant") {
        auto result = parseConstantRange(json);
        if (!result.success()) {
            return result.error();
        }
        return std::unique_ptr<math::IRandomRange>(std::make_unique<ConstantRange>(result.value()));
    }

    return Error(ErrorCode::InvalidData, "Unknown RandomRange type: " + type);
}

// ============================================================================
// LootCondition 解析
// ============================================================================

Result<std::unique_ptr<LootCondition>> LootSerializers::parseCondition(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Condition must be an object");
    }

    if (!json.contains("condition") || !json["condition"].is_string()) {
        return Error(ErrorCode::InvalidData, "Condition missing 'condition' field");
    }

    std::string conditionType = json["condition"].get<std::string>();

    // 根据 condition 类型分发
    if (conditionType == "minecraft:silk_touch" || conditionType == "silk_touch") {
        return _parseSilkTouchCondition(json);
    } else if (conditionType == "minecraft:table_bonus" || conditionType == "table_bonus") {
        return _parseTableBonusCondition(json);
    } else if (conditionType == "minecraft:fortune" || conditionType == "fortune") {
        return _parseFortuneCondition(json);
    } else if (conditionType == "minecraft:random_chance" || conditionType == "random_chance") {
        return _parseRandomChanceCondition(json);
    } else if (conditionType == "minecraft:random_chance_with_looting" ||
        conditionType == "random_chance_with_looting") {
        return _parseRandomChanceWithLootingCondition(json);
    } else if (conditionType == "minecraft:inverted" || conditionType == "inverted") {
        return _parseInvertedCondition(json);
    } else if (conditionType == "minecraft:alternative" || conditionType == "alternative") {
        return _parseAlternativeCondition(json);
    } else if (conditionType == "minecraft:block_state_property" || conditionType == "block_state_property") {
        return _parseBlockStatePropertyCondition(json);
    } else if (conditionType == "minecraft:match_tool" || conditionType == "match_tool") {
        return _parseMatchToolCondition(json);
    } else if (conditionType == "minecraft:killed_by_player" || conditionType == "killed_by_player") {
        return _parseKilledByPlayerCondition(json);
    } else if (conditionType == "minecraft:entity_properties" || conditionType == "entity_properties") {
        return _parseEntityPropertiesCondition(json);
    } else if (conditionType == "minecraft:survives_explosion" || conditionType == "survives_explosion") {
        return _parseSurvivesExplosionCondition(json);
    } else if (conditionType == "minecraft:entity_scores" || conditionType == "entity_scores") {
        return _parseEntityScoresCondition(json);
    } else if (conditionType == "minecraft:location_check" || conditionType == "location_check") {
        return _parseLocationCheckCondition(json);
    } else if (conditionType == "minecraft:weather_check" || conditionType == "weather_check") {
        return _parseWeatherCheckCondition(json);
    } else if (conditionType == "minecraft:time_check" || conditionType == "time_check") {
        return _parseTimeCheckCondition(json);
    } else if (conditionType == "minecraft:damage_source_properties" || conditionType == "damage_source_properties") {
        return _parseDamageSourcePropertiesCondition(json);
    } else if (conditionType == "minecraft:reference" || conditionType == "reference") {
        return _parseReferenceCondition(json);
    } else if (conditionType == "minecraft:fishing_hook_in_open_water" ||
        conditionType == "fishing_hook_in_open_water") {
        return castToBase<FishingOpenWaterCondition, LootCondition>(std::make_unique<FishingOpenWaterCondition>(true));
    } else {
        return Error(ErrorCode::InvalidData, "Unknown condition type: " + conditionType);
    }
}

Result<std::vector<std::unique_ptr<LootCondition>>> LootSerializers::parseConditions(const nlohmann::json& json)
{
    std::vector<std::unique_ptr<LootCondition>> conditions;

    if (json.is_null() || !json.is_array()) {
        return conditions; // 空条件列表
    }

    for (const auto& condJson : json) {
        auto result = parseCondition(condJson);
        if (!result.success()) {
            return result.error();
        }
        conditions.push_back(result.value());
    }

    return conditions;
}

nlohmann::json LootSerializers::toJson(const LootCondition& condition)
{
    nlohmann::json json;
    json["condition"] = "minecraft:" + condition.getType();

    // 特殊处理 BlockStateCondition
    if (condition.getType() == "block_state_property") {
        const auto* blockStateCond = dynamic_cast<const BlockStateCondition*>(&condition);
        if (blockStateCond) {
            return toJson(*blockStateCond);
        }
    }

    // 特殊处理 TableBonusCondition
    if (condition.getType() == "table_bonus") {
        const auto* tableBonusCond = dynamic_cast<const TableBonusCondition*>(&condition);
        if (tableBonusCond) {
            return toJson(*tableBonusCond);
        }
    }

    return json;
}

nlohmann::json LootSerializers::toJson(const BlockStateCondition& condition)
{
    nlohmann::json json;
    json["condition"] = "minecraft:block_state_property";
    json["block"] = condition.getBlockId();

    const auto& properties = condition.getProperties();
    if (!properties.isEmpty()) {
        nlohmann::json propsJson = toJson(properties);
        json["properties"] = propsJson;
    }

    return json;
}

nlohmann::json LootSerializers::toJson(const TableBonusCondition& condition)
{
    nlohmann::json json;
    json["condition"] = "minecraft:table_bonus";
    json["enchantment"] = condition.getEnchantmentId();

    nlohmann::json chancesArray = nlohmann::json::array();
    for (f32 chance : condition.getChances()) {
        chancesArray.push_back(chance);
    }
    json["chances"] = chancesArray;

    return json;
}

nlohmann::json LootSerializers::toJson(const StatePropertiesPredicate& predicate)
{
    nlohmann::json json = nlohmann::json::object();

    for (const auto& matcher : predicate.matchers()) {
        const std::string& propName = matcher->propertyName();

        // 使用 dynamic_cast 来判断匹配器类型
        if (const auto* exactMatcher = dynamic_cast<const StatePropertiesPredicate::ExactMatcher*>(matcher.get())) {
            json[propName] = exactMatcher->value();
        } else if (const auto* rangedMatcher =
                       dynamic_cast<const StatePropertiesPredicate::RangedMatcher*>(matcher.get())) {
            nlohmann::json rangeJson = nlohmann::json::object();
            if (rangedMatcher->minValue().has_value()) {
                rangeJson["min"] = *rangedMatcher->minValue();
            }
            if (rangedMatcher->maxValue().has_value()) {
                rangeJson["max"] = *rangedMatcher->maxValue();
            }
            json[propName] = rangeJson;
        }
    }

    return json;
}

// ============================================================================
// 条件解析辅助方法
// ============================================================================

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseSilkTouchCondition(const nlohmann::json& /*json*/)
{
    // 精准采集条件没有额外参数
    return castToBase<SilkTouchCondition, LootCondition>(std::make_unique<SilkTouchCondition>());
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseFortuneCondition(const nlohmann::json& json)
{
    i32 minLevel = 0;
    if (json.contains("min_level") && json["min_level"].is_number_integer()) {
        minLevel = json["min_level"].get<i32>();
    }
    return castToBase<FortuneCondition, LootCondition>(std::make_unique<FortuneCondition>(minLevel));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseTableBonusCondition(const nlohmann::json& json)
{
    // 解析附魔ID
    if (!json.contains("enchantment") || !json["enchantment"].is_string()) {
        return Error(ErrorCode::InvalidData, "table_bonus condition missing 'enchantment' field");
    }
    std::string enchantmentId = json["enchantment"].get<std::string>();

    // 解析概率表
    if (!json.contains("chances") || !json["chances"].is_array()) {
        return Error(ErrorCode::InvalidData, "table_bonus condition missing 'chances' array");
    }
    const auto& chancesArray = json["chances"];
    if (chancesArray.empty()) {
        return Error(ErrorCode::InvalidData, "table_bonus condition 'chances' array must not be empty");
    }

    std::vector<f32> chances;
    chances.reserve(chancesArray.size());
    for (const auto& chanceVal : chancesArray) {
        if (chanceVal.is_number_float()) {
            chances.push_back(chanceVal.get<f32>());
        } else if (chanceVal.is_number_integer()) {
            chances.push_back(static_cast<f32>(chanceVal.get<i32>()));
        } else {
            return Error(ErrorCode::InvalidData, "table_bonus condition 'chances' array contains non-numeric value");
        }
    }

    return castToBase<TableBonusCondition, LootCondition>(
        std::make_unique<TableBonusCondition>(std::move(enchantmentId), std::move(chances)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseRandomChanceCondition(const nlohmann::json& json)
{
    if (!json.contains("chance")) {
        return Error(ErrorCode::InvalidData, "random_chance condition missing 'chance' field");
    }

    f32 chance = 0.0f;
    if (json["chance"].is_number_float()) {
        chance = json["chance"].get<f32>();
    } else if (json["chance"].is_number_integer()) {
        chance = static_cast<f32>(json["chance"].get<i32>());
    }

    return castToBase<RandomChanceCondition, LootCondition>(std::make_unique<RandomChanceCondition>(chance));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseRandomChanceWithLootingCondition(
    const nlohmann::json& json)
{
    if (!json.contains("chance")) {
        return Error(ErrorCode::InvalidData, "random_chance_with_looting condition missing 'chance' field");
    }

    f32 baseChance = 0.0f;
    if (json["chance"].is_number_float()) {
        baseChance = json["chance"].get<f32>();
    } else if (json["chance"].is_number_integer()) {
        baseChance = static_cast<f32>(json["chance"].get<i32>());
    }

    f32 lootingMultiplier = 0.0f;
    if (json.contains("looting_multiplier")) {
        if (json["looting_multiplier"].is_number_float()) {
            lootingMultiplier = json["looting_multiplier"].get<f32>();
        } else if (json["looting_multiplier"].is_number_integer()) {
            lootingMultiplier = static_cast<f32>(json["looting_multiplier"].get<i32>());
        }
    }

    return castToBase<RandomChanceWithLuckCondition, LootCondition>(
        std::make_unique<RandomChanceWithLuckCondition>(baseChance, lootingMultiplier));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseInvertedCondition(const nlohmann::json& json)
{
    if (!json.contains("term")) {
        return Error(ErrorCode::InvalidData, "inverted condition missing 'term' field");
    }

    auto innerResult = parseCondition(json["term"]);
    if (!innerResult.success()) {
        return innerResult.error();
    }

    return castToBase<NotCondition, LootCondition>(std::make_unique<NotCondition>(innerResult.value()));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseAlternativeCondition(const nlohmann::json& json)
{
    if (!json.contains("terms") || !json["terms"].is_array()) {
        return Error(ErrorCode::InvalidData, "alternative condition missing 'terms' array");
    }

    std::vector<std::unique_ptr<LootCondition>> conditions;
    for (const auto& termJson : json["terms"]) {
        auto result = parseCondition(termJson);
        if (!result.success()) {
            return result.error();
        }
        conditions.push_back(result.value());
    }

    // alternative 是 OR 逻辑
    return castToBase<OrCondition, LootCondition>(std::make_unique<OrCondition>(std::move(conditions)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseBlockStatePropertyCondition(const nlohmann::json& json)
{
    if (!json.contains("block") || !json["block"].is_string()) {
        return Error(ErrorCode::InvalidData, "block_state_property condition missing 'block' field");
    }

    std::string blockId = json["block"].get<std::string>();

    // 解析 properties 字段
    StatePropertiesPredicate properties;

    if (json.contains("properties") && json["properties"].is_object()) {
        const auto& propsJson = json["properties"];

        for (auto it = propsJson.begin(); it != propsJson.end(); ++it) {
            const std::string& propName = it.key();
            const auto& propValue = it.value();

            if (propValue.is_string()) {
                // 精确匹配：{ "age": "3" }
                properties.addExactMatch(propName, propValue.get<std::string>());
            } else if (propValue.is_object()) {
                // 范围匹配：{ "age": { "min": "5", "max": "7" } }
                std::optional<std::string> min;
                std::optional<std::string> max;

                if (propValue.contains("min") && propValue["min"].is_string()) {
                    min = propValue["min"].get<std::string>();
                }
                if (propValue.contains("max") && propValue["max"].is_string()) {
                    max = propValue["max"].get<std::string>();
                }

                properties.addRangeMatch(propName, std::move(min), std::move(max));
            }
        }
    }

    return castToBase<BlockStateCondition, LootCondition>(
        std::make_unique<BlockStateCondition>(blockId, std::move(properties)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseMatchToolCondition(const nlohmann::json& json)
{
    // match_tool 条件：使用 ItemPredicate 对工具物品进行完整匹配
    // 对应 MC Java: MatchTool，支持 predicate 中的物品ID、标签、数量、耐久、附魔、NBT等匹配
    if (!json.contains("predicate")) {
        // 无谓词时，仅检查上下文中是否存在非空工具（ItemStack）
        return castToBase<MatchToolCondition, LootCondition>(std::make_unique<MatchToolCondition>());
    }

    const auto& predicate = json["predicate"];
    auto predResult = advancement::ItemPredicate::fromJson(predicate);
    if (!predResult.success()) {
        // 解析失败时回退到匹配任意工具的条件
        return castToBase<MatchToolCondition, LootCondition>(std::make_unique<MatchToolCondition>());
    }

    return castToBase<MatchToolCondition, LootCondition>(
        std::make_unique<MatchToolCondition>(std::move(predResult.value())));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseKilledByPlayerCondition(const nlohmann::json& /*json*/)
{
    // killed_by_player 条件检查 KILLER_PLAYER 参数是否存在
    return castToBase<KilledByPlayerCondition, LootCondition>(std::make_unique<KilledByPlayerCondition>());
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseEntityPropertiesCondition(const nlohmann::json& json)
{
    // 解析 entity 目标
    EntityPropertiesCondition::EntityTarget target = EntityPropertiesCondition::EntityTarget::This;
    if (json.contains("entity") && json["entity"].is_string()) {
        target = EntityPropertiesCondition::parseEntityTarget(json["entity"].get<std::string>());
    }

    // 解析 predicate
    advancement::EntityPredicate predicate;
    if (json.contains("predicate") && json["predicate"].is_object()) {
        auto predResult = advancement::EntityPredicate::fromJson(json["predicate"]);
        if (predResult.success()) {
            predicate = predResult.value();
        }
    }

    return castToBase<EntityPropertiesCondition, LootCondition>(
        std::make_unique<EntityPropertiesCondition>(target, std::move(predicate)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseSurvivesExplosionCondition(const nlohmann::json& /*json*/)
{
    // survives_explosion 条件不需要参数
    return castToBase<SurvivesExplosionCondition, LootCondition>(std::make_unique<SurvivesExplosionCondition>());
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseEntityScoresCondition(const nlohmann::json& json)
{
    // 解析 entity 目标
    EntityPropertiesCondition::EntityTarget target = EntityPropertiesCondition::EntityTarget::This;
    if (json.contains("entity") && json["entity"].is_string()) {
        target = EntityPropertiesCondition::parseEntityTarget(json["entity"].get<std::string>());
    }

    // 解析 scores 映射
    std::unordered_map<std::string, RandomValueRange> scores;
    if (json.contains("scores") && json["scores"].is_object()) {
        for (auto it = json["scores"].begin(); it != json["scores"].end(); ++it) {
            const std::string& objectiveName = it.key();
            const auto& scoreValue = it.value();

            auto rangeResult = parseRandomValueRange(scoreValue);
            if (rangeResult.success()) {
                scores[objectiveName] = rangeResult.value();
            }
        }
    }

    return castToBase<EntityScoresCondition, LootCondition>(
        std::make_unique<EntityScoresCondition>(target, std::move(scores)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseLocationCheckCondition(const nlohmann::json& json)
{
    // 解析偏移量
    i32 offsetX = 0, offsetY = 0, offsetZ = 0;
    if (json.contains("offsetX") && json["offsetX"].is_number_integer()) {
        offsetX = json["offsetX"].get<i32>();
    }
    if (json.contains("offsetY") && json["offsetY"].is_number_integer()) {
        offsetY = json["offsetY"].get<i32>();
    }
    if (json.contains("offsetZ") && json["offsetZ"].is_number_integer()) {
        offsetZ = json["offsetZ"].get<i32>();
    }

    // 解析 predicate
    advancement::LocationPredicate predicate;
    if (json.contains("predicate") && json["predicate"].is_object()) {
        auto predResult = advancement::LocationPredicate::fromJson(json["predicate"]);
        if (predResult.success()) {
            predicate = predResult.value();
        }
    }

    return castToBase<LocationCheckCondition, LootCondition>(
        std::make_unique<LocationCheckCondition>(std::move(predicate), offsetX, offsetY, offsetZ));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseWeatherCheckCondition(const nlohmann::json& json)
{
    std::optional<bool> raining;
    std::optional<bool> thundering;

    if (json.contains("raining") && json["raining"].is_boolean()) {
        raining = json["raining"].get<bool>();
    }
    if (json.contains("thundering") && json["thundering"].is_boolean()) {
        thundering = json["thundering"].get<bool>();
    }

    return castToBase<WeatherCheckCondition, LootCondition>(
        std::make_unique<WeatherCheckCondition>(std::move(raining), std::move(thundering)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseTimeCheckCondition(const nlohmann::json& json)
{
    i64 period = 0;
    if (json.contains("period") && json["period"].is_number_integer()) {
        period = json["period"].get<i64>();
    }

    if (!json.contains("value")) {
        return Error(ErrorCode::InvalidData, "time_check condition missing 'value' field");
    }

    auto valueResult = parseRandomValueRange(json["value"]);
    if (!valueResult.success()) {
        return valueResult.error();
    }

    return castToBase<TimeCheckCondition, LootCondition>(
        std::make_unique<TimeCheckCondition>(period, valueResult.value()));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseDamageSourcePropertiesCondition(
    const nlohmann::json& json)
{
    advancement::DamageSourcePredicate predicate;
    if (json.contains("predicate") && json["predicate"].is_object()) {
        auto predResult = advancement::DamageSourcePredicate::fromJson(json["predicate"]);
        if (predResult.success()) {
            predicate = predResult.value();
        }
    }

    return castToBase<DamageSourcePropertiesCondition, LootCondition>(
        std::make_unique<DamageSourcePropertiesCondition>(std::move(predicate)));
}

Result<std::unique_ptr<LootCondition>> LootSerializers::_parseReferenceCondition(const nlohmann::json& json)
{
    // minecraft:reference 条件引用一个命名谓词
    // JSON 格式: { "condition": "minecraft:reference", "name": "minecraft:gameplay/raid" }
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Reference condition requires a 'name' string field");
    }

    std::string name = json["name"].get<std::string>();
    return Result<std::unique_ptr<LootCondition>>(std::make_unique<ReferenceCondition>(name));
}

// ============================================================================
// LootFunction 解析
// ============================================================================

Result<std::unique_ptr<LootFunction>> LootSerializers::parseFunction(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Function must be an object");
    }

    if (!json.contains("function") || !json["function"].is_string()) {
        return Error(ErrorCode::InvalidData, "Function missing 'function' field");
    }

    std::string functionType = json["function"].get<std::string>();

    // 根据 function 类型分发（函数名对齐原版 1.21.11 LootItemFunctions 注册名）
    if (functionType == "minecraft:set_count" || functionType == "set_count") {
        return _parseSetCountFunction(json);
    } else if (functionType == "minecraft:apply_bonus" || functionType == "apply_bonus") {
        return _parseApplyBonusFunction(json);
    } else if (functionType == "minecraft:enchanted_count_increase" || functionType == "enchanted_count_increase") {
        return _parseEnchantedCountIncreaseFunction(json);
    } else if (functionType == "minecraft:set_damage" || functionType == "set_damage") {
        return _parseSetDamageFunction(json);
    } else if (functionType == "minecraft:set_name" || functionType == "set_name") {
        return _parseSetNameFunction(json);
    } else if (functionType == "minecraft:set_lore" || functionType == "set_lore") {
        return _parseSetLoreFunction(json);
    } else if (functionType == "minecraft:limit_count" || functionType == "limit_count") {
        return _parseLimitCountFunction(json);
    } else if (functionType == "minecraft:furnace_smelt" || functionType == "furnace_smelt") {
        return _parseFurnaceSmeltFunction(json);
    } else if (functionType == "minecraft:enchant_with_levels" || functionType == "enchant_with_levels") {
        return _parseEnchantWithLevelsFunction(json);
    } else if (functionType == "minecraft:enchant_randomly" || functionType == "enchant_randomly") {
        return _parseEnchantRandomlyFunction(json);
    } else if (functionType == "minecraft:explosion_decay" || functionType == "explosion_decay") {
        return _parseExplosionDecayFunction(json);
    } else if (functionType == "minecraft:set_custom_data" || functionType == "set_custom_data") {
        return _parseSetCustomDataFunction(json);
    } else if (functionType == "minecraft:copy_name" || functionType == "copy_name") {
        return _parseCopyNameFunction(json);
    } else if (functionType == "minecraft:copy_state" || functionType == "copy_state") {
        return _parseCopyStateFunction(json);
    } else if (functionType == "minecraft:copy_custom_data" || functionType == "copy_custom_data") {
        return _parseCopyCustomDataFunction(json);
    } else if (functionType == "minecraft:fill_player_head" || functionType == "fill_player_head") {
        return _parseFillPlayerHeadFunction(json);
    } else if (functionType == "minecraft:set_attributes" || functionType == "set_attributes") {
        return _parseSetAttributesFunction(json);
    } else if (functionType == "minecraft:set_contents" || functionType == "set_contents") {
        return _parseSetContentsFunction(json);
    } else if (functionType == "minecraft:set_loot_table" || functionType == "set_loot_table") {
        return _parseSetLootTableFunction(json);
    } else if (functionType == "minecraft:exploration_map" || functionType == "exploration_map") {
        return _parseExplorationMapFunction(json);
    } else if (functionType == "minecraft:set_stew_effect" || functionType == "set_stew_effect") {
        return _parseSetStewEffectFunction(json);
    } else if (functionType == "minecraft:set_enchantments" || functionType == "set_enchantments") {
        return _parseSetEnchantmentsFunction(json);
    } else if (functionType == "minecraft:set_potion" || functionType == "set_potion") {
        return _parseSetPotionFunction(json);
    } else if (functionType == "minecraft:set_instrument" || functionType == "set_instrument") {
        return _parseSetInstrumentFunction(json);
    } else if (functionType == "minecraft:copy_components" || functionType == "copy_components") {
        return _parseCopyComponentsFunction(json);
    } else if (functionType == "minecraft:set_components" || functionType == "set_components") {
        return _parseSetComponentsFunction(json);
    } else if (functionType == "minecraft:set_ominous_bottle_amplifier" ||
        functionType == "set_ominous_bottle_amplifier") {
        return _parseSetOminousBottleAmplifierFunction(json);
    } else if (functionType == "minecraft:set_item" || functionType == "set_item") {
        return _parseSetItemFunction(json);
    } else if (functionType == "minecraft:filtered" || functionType == "filtered") {
        return _parseFilteredFunction(json);
    } else if (functionType == "minecraft:reference" || functionType == "reference") {
        return _parseFunctionReferenceFunction(json);
    } else if (functionType == "minecraft:sequence" || functionType == "sequence") {
        return _parseSequenceFunction(json);
    } else if (functionType == "minecraft:discard" || functionType == "discard") {
        return castToBase<DiscardItemFunction, LootFunction>(std::make_unique<DiscardItemFunction>());
    } else if (functionType == "minecraft:set_fireworks" || functionType == "set_fireworks") {
        return _parseSetFireworksFunction(json);
    } else if (functionType == "minecraft:set_firework_explosion" || functionType == "set_firework_explosion") {
        return _parseSetFireworkExplosionFunction(json);
    } else if (functionType == "minecraft:toggle_tooltips" || functionType == "toggle_tooltips") {
        return _parseToggleTooltipsFunction(json);
    } else if (functionType == "minecraft:set_custom_model_data" || functionType == "set_custom_model_data") {
        return _parseSetCustomModelDataFunction(json);
    } else if (functionType == "minecraft:modify_contents" || functionType == "modify_contents") {
        return _parseModifyContentsFunction(json);
    } else if (functionType == "minecraft:set_written_book_pages" || functionType == "set_written_book_pages") {
        return _parseSetBookPagesFunction(json, true);
    } else if (functionType == "minecraft:set_writable_book_pages" || functionType == "set_writable_book_pages") {
        return _parseSetBookPagesFunction(json, false);
    } else if (functionType == "minecraft:set_book_cover" || functionType == "set_book_cover") {
        return _parseSetBookCoverFunction(json);
    } else if (functionType == "minecraft:set_banner_pattern" || functionType == "set_banner_pattern") {
        return _parseSetBannerPatternFunction(json);
    } else {
        return Error(ErrorCode::InvalidData, "Unknown function type: " + functionType);
    }
}

Result<std::vector<std::unique_ptr<LootFunction>>> LootSerializers::parseFunctions(const nlohmann::json& json)
{
    std::vector<std::unique_ptr<LootFunction>> functions;

    if (json.is_null() || !json.is_array()) {
        return functions; // 空函数列表
    }

    for (const auto& funcJson : json) {
        auto result = parseFunction(funcJson);
        if (!result.success()) {
            return result.error();
        }
        functions.push_back(result.value());
    }

    return functions;
}

nlohmann::json LootSerializers::toJson(const LootFunction& function)
{
    nlohmann::json json;
    json["function"] = "minecraft:" + function.getType();
    // 各子类可以扩展此方法
    return json;
}

// ============================================================================
// 函数解析辅助方法
// ============================================================================

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetCountFunction(const nlohmann::json& json)
{
    if (!json.contains("count")) {
        return Error(ErrorCode::InvalidData, "set_count function missing 'count' field");
    }

    // 数量提供器（原版 NumberProvider）：uniform/纯数字、binomial、constant。
    // parseRandomValueRange 无法表达 binomial 的随机语义，故先走 parseRandomRange；
    // 纯数字/无 type 的 uniform 也由其解析为 Constant/RandomValueRange。
    auto providerResult = parseRandomRange(json["count"]);
    RandomValueRange countRange(0.0f, 0.0f);
    std::unique_ptr<math::IRandomRange> provider;
    if (providerResult.success()) {
        provider = providerResult.value();
        countRange = RandomValueRange(provider->getMin(), provider->getMax());
    } else {
        // 对象形式但 parseRandomRange 无法识别（如缺少 min/max 字段）时才回退
        auto countResult = parseRandomValueRange(json["count"]);
        if (!countResult.success()) {
            return countResult.error();
        }
        countRange = countResult.value();
    }

    bool add = false;
    if (json.contains("add") && json["add"].is_boolean()) {
        add = json["add"].get<bool>();
    }

    if (provider != nullptr) {
        return castToBase<SetCountFunction, LootFunction>(
            std::make_unique<SetCountFunction>(countRange, std::move(provider), add));
    }
    return castToBase<SetCountFunction, LootFunction>(std::make_unique<SetCountFunction>(countRange, add));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseApplyBonusFunction(const nlohmann::json& json)
{
    ApplyBonusFunction::BonusType bonusType = ApplyBonusFunction::BonusType::OreDrops;
    i32 bonusMultiplier = 1;
    i32 extra = 1;
    f32 probability = 1.0f;

    // 解析公式类型
    if (json.contains("formula") && json["formula"].is_string()) {
        std::string formula = json["formula"].get<std::string>();

        if (formula == "minecraft:uniform" || formula == "uniform") {
            bonusType = ApplyBonusFunction::BonusType::Uniform;
        } else if (formula == "minecraft:binomial_with_bonus_count" || formula == "binomial_with_bonus_count") {
            bonusType = ApplyBonusFunction::BonusType::Binomial;
        } else if (formula == "minecraft:ore_drops" || formula == "ore_drops") {
            bonusType = ApplyBonusFunction::BonusType::OreDrops;
        }
    }

    // 解析参数
    if (json.contains("parameters") && json["parameters"].is_object()) {
        const auto& params = json["parameters"];

        if (params.contains("bonusMultiplier")) {
            bonusMultiplier = params["bonusMultiplier"].is_number_integer()
                ? params["bonusMultiplier"].get<i32>()
                : static_cast<i32>(params["bonusMultiplier"].get<f32>());
        }
        if (params.contains("extra")) {
            extra = params["extra"].is_number_integer() ? params["extra"].get<i32>()
                                                        : static_cast<i32>(params["extra"].get<f32>());
        }
        if (params.contains("probability")) {
            probability = params["probability"].is_number_float() ? params["probability"].get<f32>()
                                                                  : static_cast<f32>(params["probability"].get<i32>());
        }
    }

    return castToBase<ApplyBonusFunction, LootFunction>(
        std::make_unique<ApplyBonusFunction>(bonusType, bonusMultiplier, extra, probability));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseEnchantedCountIncreaseFunction(const nlohmann::json& json)
{
    RandomValueRange count(0.0f, 1.0f);
    i32 limit = 0;

    if (json.contains("count")) {
        auto countResult = parseRandomValueRange(json["count"]);
        if (!countResult.success()) {
            return countResult.error();
        }
        count = countResult.value();
    }

    if (json.contains("limit") && json["limit"].is_number_integer()) {
        limit = json["limit"].get<i32>();
    }

    return castToBase<EnchantedCountIncreaseFunction, LootFunction>(
        std::make_unique<EnchantedCountIncreaseFunction>(count, limit));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetDamageFunction(const nlohmann::json& json)
{
    if (!json.contains("damage")) {
        return Error(ErrorCode::InvalidData, "set_damage function missing 'damage' field");
    }

    auto damageResult = parseRandomValueRange(json["damage"]);
    if (!damageResult.success()) {
        return damageResult.error();
    }

    bool add = false;
    if (json.contains("add") && json["add"].is_boolean()) {
        add = json["add"].get<bool>();
    }

    return castToBase<SetDamageFunction, LootFunction>(std::make_unique<SetDamageFunction>(damageResult.value(), add));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetNameFunction(const nlohmann::json& json)
{
    std::string name;
    bool replace = true;

    if (json.contains("name")) {
        if (json["name"].is_string()) {
            name = json["name"].get<std::string>();
        } else if (json["name"].is_object()) {
            // JSON 文本组件，简化为取 text 字段
            if (json["name"].contains("text") && json["name"]["text"].is_string()) {
                name = json["name"]["text"].get<std::string>();
            }
        }
    }

    if (json.contains("replace") && json["replace"].is_boolean()) {
        replace = json["replace"].get<bool>();
    }

    return castToBase<SetNameFunction, LootFunction>(std::make_unique<SetNameFunction>(name, replace));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetLoreFunction(const nlohmann::json& json)
{
    std::vector<std::string> lore;
    bool replace = true;

    if (json.contains("lore") && json["lore"].is_array()) {
        for (const auto& line : json["lore"]) {
            if (line.is_string()) {
                lore.push_back(line.get<std::string>());
            } else if (line.is_object() && line.contains("text") && line["text"].is_string()) {
                lore.push_back(line["text"].get<std::string>());
            }
        }
    }

    if (json.contains("replace") && json["replace"].is_boolean()) {
        replace = json["replace"].get<bool>();
    }

    return castToBase<SetLoreFunction, LootFunction>(std::make_unique<SetLoreFunction>(lore, replace));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseLimitCountFunction(const nlohmann::json& json)
{
    i32 min = -1;
    i32 max = -1;

    if (json.contains("limit")) {
        const auto& limit = json["limit"];

        if (limit.is_number_integer()) {
            // 固定值
            i32 value = limit.get<i32>();
            min = value;
            max = value;
        } else if (limit.is_object()) {
            if (limit.contains("min") && limit["min"].is_number_integer()) {
                min = limit["min"].get<i32>();
            }
            if (limit.contains("max") && limit["max"].is_number_integer()) {
                max = limit["max"].get<i32>();
            }
        }
    }

    return castToBase<LimitCountFunction, LootFunction>(std::make_unique<LimitCountFunction>(min, max));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseFurnaceSmeltFunction(const nlohmann::json& /*json*/)
{
    return castToBase<FurnaceSmeltFunction, LootFunction>(std::make_unique<FurnaceSmeltFunction>());
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseEnchantWithLevelsFunction(const nlohmann::json& json)
{
    RandomValueRange levels(1.0f, 30.0f);
    bool treasure = false;

    if (json.contains("levels")) {
        auto levelsResult = parseRandomValueRange(json["levels"]);
        if (!levelsResult.success()) {
            return levelsResult.error();
        }
        levels = levelsResult.value();
    }

    if (json.contains("treasure") && json["treasure"].is_boolean()) {
        treasure = json["treasure"].get<bool>();
    }

    return castToBase<EnchantWithLevelsFunction, LootFunction>(
        std::make_unique<EnchantWithLevelsFunction>(levels, treasure));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseEnchantRandomlyFunction(const nlohmann::json& json)
{
    std::vector<std::string> enchantments;
    bool treasure = false;

    if (json.contains("enchantments") && json["enchantments"].is_array()) {
        for (const auto& ench : json["enchantments"]) {
            if (ench.is_string()) {
                enchantments.push_back(ench.get<std::string>());
            }
        }
    }

    if (json.contains("treasure") && json["treasure"].is_boolean()) {
        treasure = json["treasure"].get<bool>();
    }

    return castToBase<EnchantRandomlyFunction, LootFunction>(
        std::make_unique<EnchantRandomlyFunction>(enchantments, treasure));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseExplosionDecayFunction(const nlohmann::json& /*json*/)
{
    return castToBase<ExplosionDecayFunction, LootFunction>(std::make_unique<ExplosionDecayFunction>());
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetCustomDataFunction(const nlohmann::json& json)
{
    if (!json.contains("tag") || !json["tag"].is_string()) {
        return Error(ErrorCode::InvalidData, "set_custom_data function missing 'tag' field");
    }

    std::string nbtString = json["tag"].get<std::string>();
    // 加载期即解析自定义数据：非法 NBT 文本使整张战利品表加载失败，不在掉落时静默丢弃。
    auto created = SetCustomDataFunction::create(nbtString);
    if (!created.success()) {
        return created.error();
    }
    return castToBase<SetCustomDataFunction, LootFunction>(created.value());
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseCopyNameFunction(const nlohmann::json& json)
{
    CopyNameFunction::Source source = CopyNameFunction::Source::This;

    if (json.contains("source") && json["source"].is_string()) {
        std::string sourceStr = json["source"].get<std::string>();
        if (sourceStr == "this") {
            source = CopyNameFunction::Source::This;
        } else if (sourceStr == "killer") {
            source = CopyNameFunction::Source::Killer;
        } else if (sourceStr == "killer_player") {
            source = CopyNameFunction::Source::KillerPlayer;
        } else if (sourceStr == "block_entity") {
            source = CopyNameFunction::Source::BlockEntity;
        }
    }

    return castToBase<CopyNameFunction, LootFunction>(std::make_unique<CopyNameFunction>(source));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseCopyStateFunction(const nlohmann::json& json)
{
    if (!json.contains("block") || !json["block"].is_string()) {
        return Error(ErrorCode::InvalidData, "copy_state function missing 'block' field");
    }

    std::string blockId = json["block"].get<std::string>();
    std::vector<std::string> properties;

    if (json.contains("properties") && json["properties"].is_array()) {
        for (const auto& prop : json["properties"]) {
            if (prop.is_string()) {
                properties.push_back(prop.get<std::string>());
            }
        }
    }

    return castToBase<CopyStateFunction, LootFunction>(std::make_unique<CopyStateFunction>(blockId, properties));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseCopyCustomDataFunction(const nlohmann::json& json)
{
    // 解析 source 字段
    CopyCustomDataFunction::Source source = CopyCustomDataFunction::Source::This;
    if (json.contains("source") && json["source"].is_string()) {
        std::string sourceStr = json["source"].get<std::string>();
        if (sourceStr == "this") {
            source = CopyCustomDataFunction::Source::This;
        } else if (sourceStr == "killer") {
            source = CopyCustomDataFunction::Source::Killer;
        } else if (sourceStr == "killer_player") {
            source = CopyCustomDataFunction::Source::KillerPlayer;
        } else if (sourceStr == "block_entity") {
            source = CopyCustomDataFunction::Source::BlockEntity;
        }
    }

    auto func = std::make_unique<CopyCustomDataFunction>(source);

    // 解析 ops 数组
    if (json.contains("ops") && json["ops"].is_array()) {
        for (const auto& op : json["ops"]) {
            if (!op.contains("source") || !op["source"].is_string()) {
                continue;
            }
            if (!op.contains("target") || !op["target"].is_string()) {
                continue;
            }

            std::string sourcePath = op["source"].get<std::string>();
            std::string targetPath = op["target"].get<std::string>();

            CopyCustomDataFunction::Operation operation = CopyCustomDataFunction::Operation::Replace;
            if (op.contains("op") && op["op"].is_string()) {
                std::string opStr = op["op"].get<std::string>();
                if (opStr == "replace") {
                    operation = CopyCustomDataFunction::Operation::Replace;
                } else if (opStr == "append") {
                    operation = CopyCustomDataFunction::Operation::Append;
                } else if (opStr == "merge") {
                    operation = CopyCustomDataFunction::Operation::Merge;
                }
            }

            func->addOperation(sourcePath, targetPath, operation);
        }
    }

    return castToBase<CopyCustomDataFunction, LootFunction>(std::move(func));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseFillPlayerHeadFunction(const nlohmann::json& json)
{
    CopyNameFunction::Source source = CopyNameFunction::Source::This;

    if (json.contains("entity") && json["entity"].is_string()) {
        std::string entityStr = json["entity"].get<std::string>();
        if (entityStr == "this") {
            source = CopyNameFunction::Source::This;
        } else if (entityStr == "killer") {
            source = CopyNameFunction::Source::Killer;
        } else if (entityStr == "killer_player") {
            source = CopyNameFunction::Source::KillerPlayer;
        }
    }

    return castToBase<FillPlayerHeadFunction, LootFunction>(std::make_unique<FillPlayerHeadFunction>(source));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetAttributesFunction(const nlohmann::json& json)
{
    auto function = std::make_unique<SetAttributesFunction>();

    if (json.contains("modifiers") && json["modifiers"].is_array()) {
        for (const auto& mod : json["modifiers"]) {
            if (!mod.contains("name") || !mod.contains("attribute") || !mod.contains("amount")) {
                continue;
            }

            std::string name = mod["name"].get<std::string>();
            std::string attributeId = mod["attribute"].get<std::string>();

            // 解析 amount（支持固定值或范围）
            math::RandomValueRange amount(0.0f, 0.0f);
            auto amountResult = parseRandomValueRange(mod["amount"]);
            if (amountResult.success()) {
                amount = amountResult.value();
            } else if (mod["amount"].is_number_float()) {
                f32 val = mod["amount"].get<f32>();
                amount = math::RandomValueRange(val, val);
            } else if (mod["amount"].is_number_integer()) {
                f32 val = static_cast<f32>(mod["amount"].get<i32>());
                amount = math::RandomValueRange(val, val);
            }

            // 解析操作类型
            u8 operation = 0;
            if (mod.contains("operation") && mod["operation"].is_string()) {
                std::string opStr = mod["operation"].get<std::string>();
                if (opStr == "addition" || opStr == "minecraft:addition") {
                    operation = 0;
                } else if (opStr == "multiply_base" || opStr == "minecraft:multiply_base") {
                    operation = 1;
                } else if (opStr == "multiply_total" || opStr == "minecraft:multiply_total") {
                    operation = 2;
                }
            }

            // 解析槽位（支持单个槽位或多个槽位）
            std::vector<std::string> slots;
            if (mod.contains("slot")) {
                if (mod["slot"].is_string()) {
                    slots.push_back(mod["slot"].get<std::string>());
                } else if (mod["slot"].is_array()) {
                    for (const auto& s : mod["slot"]) {
                        if (s.is_string()) {
                            slots.push_back(s.get<std::string>());
                        }
                    }
                }
            } else if (mod.contains("slots")) {
                if (mod["slots"].is_string()) {
                    slots.push_back(mod["slots"].get<std::string>());
                } else if (mod["slots"].is_array()) {
                    for (const auto& s : mod["slots"]) {
                        if (s.is_string()) {
                            slots.push_back(s.get<std::string>());
                        }
                    }
                }
            }
            if (slots.empty()) {
                slots.push_back("mainhand"); // 默认主手
            }

            // 解析 UUID（可选）
            std::string uuid;
            if (mod.contains("id") && mod["id"].is_string()) {
                uuid = mod["id"].get<std::string>();
            }

            SetAttributesFunction::Modifier modifier(name, attributeId, amount, operation, slots, uuid);
            function->addModifier(modifier);
        }
    }

    return castToBase<SetAttributesFunction, LootFunction>(std::move(function));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetContentsFunction(const nlohmann::json& json)
{
    auto function = std::make_unique<SetContentsFunction>();

    // 解析 entries 数组
    if (json.contains("entries") && json["entries"].is_array()) {
        auto entriesResult = parseEntries(json["entries"]);
        if (!entriesResult.success()) {
            return entriesResult.error();
        }
        for (auto& entry : entriesResult.value()) {
            function->addEntry(std::move(entry));
        }
    }

    return castToBase<SetContentsFunction, LootFunction>(std::move(function));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetLootTableFunction(const nlohmann::json& json)
{
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::InvalidData, "set_loot_table function missing 'name' field");
    }

    std::string lootTableId = json["name"].get<std::string>();
    u64 seed = 0;

    if (json.contains("seed") && json["seed"].is_number_integer()) {
        seed = static_cast<u64>(json["seed"].get<i64>());
    }

    return castToBase<SetLootTableFunction, LootFunction>(std::make_unique<SetLootTableFunction>(lootTableId, seed));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseExplorationMapFunction(const nlohmann::json& json)
{
    using world::map::DecorationType;

    // 解析 destination（可选，默认 minecraft:on_treasure_maps）：结构标签资源位置，
    // 标签是否存在由世界侧按维度判定，此处只校验资源位置语法，语法非法即整张战利品表加载失败。
    ResourceLocation destination = ExplorationMapFunction::defaultDestination();
    if (json.contains("destination")) {
        const auto& destinationJson = json["destination"];
        if (!destinationJson.is_string()) {
            return Error(ErrorCode::InvalidData, "exploration_map: 'destination' must be a string");
        }
        auto parsed = ExplorationMapFunction::parseDestination(destinationJson.get<std::string>());
        if (!parsed.has_value()) {
            return Error(ErrorCode::InvalidData,
                "exploration_map: invalid destination '" + destinationJson.get<std::string>() + "'");
        }
        destination = parsed.value();
    }

    // 解析 decoration（可选，默认 woodland_mansion）：地图装饰注册表项，与 destination 无关；
    // 取值无法识别即整张战利品表加载失败（原版按注册表解析，未注册的键直接报错）。
    DecorationType decoration = ExplorationMapFunction::DEFAULT_DECORATION;
    if (json.contains("decoration")) {
        const auto& decorationJson = json["decoration"];
        if (!decorationJson.is_string()) {
            return Error(ErrorCode::InvalidData, "exploration_map: 'decoration' must be a string");
        }
        auto parsed = world::map::decorationTypeFromString(decorationJson.get<std::string>());
        if (!parsed.has_value()) {
            return Error(ErrorCode::InvalidData,
                "exploration_map: unknown decoration '" + decorationJson.get<std::string>() + "'");
        }
        decoration = parsed.value();
    }

    // 解析 zoom（可选，默认 2）：字节编码，超出 i8 范围即解析失败
    i8 zoom = ExplorationMapFunction::DEFAULT_ZOOM;
    if (json.contains("zoom")) {
        const auto& zoomJson = json["zoom"];
        if (!zoomJson.is_number_integer()) {
            return Error(ErrorCode::InvalidData, "exploration_map: 'zoom' must be an integer");
        }
        const i64 zoomValue = zoomJson.get<i64>();
        if (zoomValue < std::numeric_limits<i8>::min() || zoomValue > std::numeric_limits<i8>::max()) {
            return Error(ErrorCode::InvalidData,
                "exploration_map: 'zoom' out of byte range (" + std::to_string(zoomValue) + ")");
        }
        zoom = static_cast<i8>(zoomValue);
    }

    // 解析 search_radius（可选，默认 50 区块）：负数即解析失败
    i32 searchRadius = ExplorationMapFunction::DEFAULT_SEARCH_RADIUS;
    if (json.contains("search_radius")) {
        const auto& radiusJson = json["search_radius"];
        if (!radiusJson.is_number_integer()) {
            return Error(ErrorCode::InvalidData, "exploration_map: 'search_radius' must be an integer");
        }
        const i64 radiusValue = radiusJson.get<i64>();
        if (radiusValue < 0 || radiusValue > std::numeric_limits<i32>::max()) {
            return Error(ErrorCode::InvalidData,
                "exploration_map: 'search_radius' out of range (" + std::to_string(radiusValue) + ")");
        }
        searchRadius = static_cast<i32>(radiusValue);
    }

    // 解析 skip_existing_chunks（可选，默认 true）
    bool searchSkipsLoadedChunks = true;
    if (json.contains("skip_existing_chunks")) {
        const auto& skipJson = json["skip_existing_chunks"];
        if (!skipJson.is_boolean()) {
            return Error(ErrorCode::InvalidData, "exploration_map: 'skip_existing_chunks' must be a boolean");
        }
        searchSkipsLoadedChunks = skipJson.get<bool>();
    }

    return castToBase<ExplorationMapFunction, LootFunction>(
        std::make_unique<ExplorationMapFunction>(destination, decoration, zoom, searchRadius, searchSkipsLoadedChunks));
}

// ============================================================================
// 数据包组件 / 注册表引用解析辅助
// ============================================================================

namespace {

/// 解析单个烟花爆炸对象（FireworkExplosion.CODEC：shape/colors/fade_colors/trail/twinkle）
std::optional<item::component::FireworkExplosion> _parseFireworkExplosionObject(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return std::nullopt;
    }
    item::component::FireworkExplosion explosion{};
    if (json.contains("shape")) {
        if (!json["shape"].is_string()) {
            return std::nullopt;
        }
        auto shape = item::component::fireworkShapeFromName(json["shape"].get<std::string>());
        if (!shape.has_value()) {
            return std::nullopt;
        }
        explosion.shape = *shape;
    }
    auto readColors = [&json](const char* key, std::vector<i32>& out) -> bool {
        if (!json.contains(key)) {
            return true;
        }
        if (!json[key].is_array()) {
            return false;
        }
        out.clear();
        for (const auto& color : json[key]) {
            if (!color.is_number_integer()) {
                return false;
            }
            out.push_back(color.get<i32>());
        }
        return true;
    };
    if (!readColors("colors", explosion.colors) || !readColors("fade_colors", explosion.fadeColors)) {
        return std::nullopt;
    }
    for (const auto& [key, target] : {std::pair<const char*, bool*>("trail", &explosion.hasTrail),
             std::pair<const char*, bool*>("twinkle", &explosion.hasTwinkle)}) {
        if (json.contains(key)) {
            if (!json[key].is_boolean()) {
                return std::nullopt;
            }
            *target = json[key].get<bool>();
        }
    }
    return explosion;
}

/// 与数据包一致：资源位置可省略命名空间（默认 minecraft）
std::string _normalizeResourceId(const std::string& text)
{
    return text.find(':') == std::string::npos ? "minecraft:" + text : text;
}

/// 校验附魔是否存在
bool _isKnownEnchantment(const std::string& id)
{
    return item::enchant::EnchantmentRegistry::get(_normalizeResourceId(id)) != nullptr;
}

/// 解析 "include"/"exclude" 组件名列表（未知组件名返回 nullopt）
std::optional<std::vector<item::component::DataComponentType>> _parseComponentTypeList(const nlohmann::json& json)
{
    std::vector<item::component::DataComponentType> types;
    if (json.is_null()) {
        return types;
    }
    if (!json.is_array()) {
        return std::nullopt;
    }
    for (const auto& entry : json) {
        if (!entry.is_string()) {
            return std::nullopt;
        }
        auto type = item::component::componentTypeByName(_normalizeResourceId(entry.get<std::string>()));
        if (!type.has_value()) {
            return std::nullopt;
        }
        types.push_back(*type);
    }
    return types;
}

/// 读取带 "mode" 的列表操作对象；values 由调用方解析
bool _parseListOperationHeader(const nlohmann::json& json, ListOperationKind& kind, i32& offset, const char* valueKey)
{
    if (!json.is_object() || !json.contains(valueKey) || !json[valueKey].is_array()) {
        return false;
    }
    kind = ListOperationKind::ReplaceAll;
    offset = 0;
    if (json.contains("mode")) {
        if (!json["mode"].is_string()) {
            return false;
        }
        auto parsed = listOperationKindFromName(json["mode"].get<std::string>());
        if (!parsed.has_value()) {
            return false;
        }
        kind = *parsed;
    }
    if (json.contains("offset")) {
        if (!json["offset"].is_number_integer() || json["offset"].get<i64>() < 0) {
            return false;
        }
        offset = static_cast<i32>(json["offset"].get<i64>());
    }
    return true;
}

/// 解析数值列表（每项为整数或 uniform 对象），用于 custom_model_data
bool _parseNumberProviders(const nlohmann::json& json, std::vector<RandomValueRange>& out)
{
    if (!json.is_array()) {
        return false;
    }
    out.clear();
    for (const auto& entry : json) {
        auto parsed = LootSerializers::parseRandomValueRange(entry);
        if (!parsed.success()) {
            return false;
        }
        out.push_back(parsed.value());
    }
    return true;
}

/// 解析物品谓词（filtered 的 item_filter）
advancement::ItemPredicate _parseItemPredicateOrMakeAny(const nlohmann::json& json, bool& ok)
{
    auto parsed = advancement::ItemPredicate::fromJson(json);
    ok = parsed.success();
    return ok ? parsed.value() : advancement::ItemPredicate();
}

} // namespace

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetEnchantmentsFunction(const nlohmann::json& json)
{
    std::vector<SetEnchantmentsFunction::Entry> entries;
    if (json.contains("enchantments")) {
        if (!json["enchantments"].is_object()) {
            return Error(ErrorCode::InvalidData, "set_enchantments: 'enchantments' must be an object");
        }
        for (const auto& [enchantId, level] : json["enchantments"].items()) {
            if (!_isKnownEnchantment(enchantId)) {
                return Error(ErrorCode::InvalidData, "set_enchantments: unknown enchantment '" + enchantId + "'");
            }
            auto parsedLevel = parseRandomValueRange(level);
            if (!parsedLevel.success()) {
                return parsedLevel.error();
            }
            entries.push_back({ResourceLocation(_normalizeResourceId(enchantId)), parsedLevel.value()});
        }
    }
    bool add = false;
    if (json.contains("add")) {
        if (!json["add"].is_boolean()) {
            return Error(ErrorCode::InvalidData, "set_enchantments: 'add' must be a boolean");
        }
        add = json["add"].get<bool>();
    }
    return castToBase<SetEnchantmentsFunction, LootFunction>(
        std::make_unique<SetEnchantmentsFunction>(std::move(entries), add));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetPotionFunction(const nlohmann::json& json)
{
    if (!json.contains("id") || !json["id"].is_string()) {
        return Error(ErrorCode::InvalidData, "set_potion: missing string 'id' field");
    }
    const ResourceLocation potionId(_normalizeResourceId(json["id"].get<std::string>()));
    if (potion::PotionRegistry::instance().getPotion(potionId) == nullptr) {
        return Error(ErrorCode::InvalidData, "set_potion: unknown potion '" + potionId.toString() + "'");
    }
    return castToBase<SetPotionFunction, LootFunction>(std::make_unique<SetPotionFunction>(potionId));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetInstrumentFunction(const nlohmann::json& json)
{
    if (!json.contains("options") || !json["options"].is_string()) {
        return Error(ErrorCode::InvalidData, "set_instrument: missing string 'options' field");
    }
    std::string tagText = json["options"].get<std::string>();
    // 原版 TagKey fieldOf"options" 的 JSON 写作 "#minecraft:regular_goat_horns"
    if (!tagText.empty() && tagText[0] == '#') {
        tagText = tagText.substr(1);
    }
    const ResourceLocation tagId(_normalizeResourceId(tagText));
    if (SetInstrumentFunction::getInstrumentTag(tagId) == nullptr) {
        return Error(ErrorCode::InvalidData, "set_instrument: unknown instrument tag '" + tagId.toString() + "'");
    }
    return castToBase<SetInstrumentFunction, LootFunction>(std::make_unique<SetInstrumentFunction>(tagId));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseCopyComponentsFunction(const nlohmann::json& json)
{
    CopyComponentsFunction::Source source = CopyComponentsFunction::Source::This;
    if (json.contains("source")) {
        if (!json["source"].is_string()) {
            return Error(ErrorCode::InvalidData, "copy_components: 'source' must be a string");
        }
        const std::string sourceName = json["source"].get<std::string>();
        if (sourceName == "block_entity") {
            source = CopyComponentsFunction::Source::BlockEntity;
        } else if (sourceName == "this") {
            source = CopyComponentsFunction::Source::This;
        } else {
            return Error(ErrorCode::InvalidData, "copy_components: unknown source '" + sourceName + "'");
        }
    }
    auto include = _parseComponentTypeList(json.contains("include") ? json["include"] : nlohmann::json());
    if (!include.has_value()) {
        return Error(ErrorCode::InvalidData, "copy_components: invalid 'include' component list");
    }
    auto exclude = _parseComponentTypeList(json.contains("exclude") ? json["exclude"] : nlohmann::json());
    if (!exclude.has_value()) {
        return Error(ErrorCode::InvalidData, "copy_components: invalid 'exclude' component list");
    }
    return castToBase<CopyComponentsFunction, LootFunction>(
        std::make_unique<CopyComponentsFunction>(source, std::move(*include), std::move(*exclude)));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetComponentsFunction(const nlohmann::json& json)
{
    if (!json.contains("components") || !json["components"].is_object()) {
        return Error(ErrorCode::InvalidData, "set_components: missing object 'components' field");
    }
    // DataComponentPatch.CODEC：键 = 组件资源位置，'!' 前缀 = 移除（值为空）
    item::component::DataComponentPatch patch;
    for (const auto& [key, value] : json["components"].items()) {
        bool removed = !key.empty() && key[0] == '!';
        const std::string name = removed ? key.substr(1) : key;
        auto type = item::component::componentTypeByName(_normalizeResourceId(name));
        if (!type.has_value()) {
            return Error(ErrorCode::InvalidData, "set_components: unknown component '" + name + "'");
        }
        if (removed) {
            patch.remove(*type);
            continue;
        }
        item::component::DataComponentPayload payload{};
        if (!item::component::jsonToComponentPayload(*type, value, payload)) {
            return Error(ErrorCode::InvalidData, "set_components: failed to decode component '" + name + "'");
        }
        patch.add(*type, std::move(payload));
    }
    return castToBase<SetComponentsFunction, LootFunction>(std::make_unique<SetComponentsFunction>(
        std::make_shared<const item::component::DataComponentPatch>(std::move(patch))));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetOminousBottleAmplifierFunction(
    const nlohmann::json& json)
{
    if (!json.contains("amplifier")) {
        return Error(ErrorCode::InvalidData, "set_ominous_bottle_amplifier: missing 'amplifier' field");
    }
    auto parsed = parseRandomValueRange(json["amplifier"]);
    if (!parsed.success()) {
        return parsed.error();
    }
    return castToBase<SetOminousBottleAmplifierFunction, LootFunction>(
        std::make_unique<SetOminousBottleAmplifierFunction>(parsed.value()));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetItemFunction(const nlohmann::json& json)
{
    if (!json.contains("item") || !json["item"].is_string()) {
        return Error(ErrorCode::InvalidData, "set_item: missing string 'item' field");
    }
    const ResourceLocation itemId(_normalizeResourceId(json["item"].get<std::string>()));
    if (ItemRegistry::instance().getItem(itemId) == nullptr) {
        return Error(ErrorCode::InvalidData, "set_item: unknown item '" + itemId.toString() + "'");
    }
    return castToBase<SetItemFunction, LootFunction>(std::make_unique<SetItemFunction>(itemId));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseFilteredFunction(const nlohmann::json& json)
{
    if (!json.contains("item_filter")) {
        return Error(ErrorCode::InvalidData, "filtered: missing 'item_filter' field");
    }
    bool ok = false;
    auto filter = _parseItemPredicateOrMakeAny(json["item_filter"], ok);
    if (!ok) {
        return Error(ErrorCode::InvalidData, "filtered: invalid 'item_filter'");
    }
    std::optional<std::unique_ptr<LootFunction>> onPass;
    if (json.contains("on_pass")) {
        auto parsed = parseFunction(json["on_pass"]);
        if (!parsed.success()) {
            return parsed.error();
        }
        onPass = parsed.value();
    }
    std::optional<std::unique_ptr<LootFunction>> onFail;
    if (json.contains("on_fail")) {
        auto parsed = parseFunction(json["on_fail"]);
        if (!parsed.success()) {
            return parsed.error();
        }
        onFail = parsed.value();
    }
    return castToBase<FilteredFunction, LootFunction>(
        std::make_unique<FilteredFunction>(std::move(filter), std::move(onPass), std::move(onFail)));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseFunctionReferenceFunction(const nlohmann::json& json)
{
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::InvalidData, "reference: missing string 'name' field");
    }
    const std::string name = _normalizeResourceId(json["name"].get<std::string>());
    // 命名函数在函数管理器中注册；未注册即原版 MissingReferenceProblem，加载失败
    const LootFunction* target = LootFunctionManager::instance().getFunction(name);
    if (target == nullptr) {
        return Error(ErrorCode::InvalidData, "reference: unknown function '" + name + "'");
    }
    return castToBase<FunctionReferenceFunction, LootFunction>(
        std::make_unique<FunctionReferenceFunction>(name, target));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSequenceFunction(const nlohmann::json& json)
{
    if (!json.contains("functions") || !json["functions"].is_array()) {
        return Error(ErrorCode::InvalidData, "sequence: missing array 'functions' field");
    }
    std::vector<std::unique_ptr<LootFunction>> functions;
    for (const auto& funcJson : json["functions"]) {
        auto parsed = parseFunction(funcJson);
        if (!parsed.success()) {
            return parsed.error();
        }
        functions.push_back(parsed.value());
    }
    return castToBase<SequenceFunction, LootFunction>(std::make_unique<SequenceFunction>(std::move(functions)));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetFireworksFunction(const nlohmann::json& json)
{
    std::optional<SetFireworksFunction::ExplosionListOperation> explosions;
    if (json.contains("explosions")) {
        const auto& explosionsJson = json["explosions"];
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        i32 offset = 0;
        if (!_parseListOperationHeader(explosionsJson, kind, offset, "explosions")) {
            return Error(ErrorCode::InvalidData, "set_fireworks: invalid 'explosions' list operation");
        }
        SetFireworksFunction::ExplosionListOperation operation{kind, {}, offset};
        for (const auto& explosionJson : explosionsJson["explosions"]) {
            auto parsed = _parseFireworkExplosionObject(explosionJson);
            if (!parsed.has_value()) {
                return Error(ErrorCode::InvalidData, "set_fireworks: invalid firework explosion");
            }
            operation.explosions.push_back(*parsed);
        }
        if (operation.explosions.size() > static_cast<std::size_t>(item::component::Fireworks::MAX_EXPLOSIONS)) {
            return Error(ErrorCode::InvalidData, "set_fireworks: too many explosions");
        }
        explosions = std::move(operation);
    }
    std::optional<i32> flightDuration;
    if (json.contains("flight_duration")) {
        if (!json["flight_duration"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "set_fireworks: 'flight_duration' must be an integer");
        }
        const i64 value = json["flight_duration"].get<i64>();
        if (value < 0 || value > 255) {
            return Error(ErrorCode::InvalidData, "set_fireworks: 'flight_duration' out of range");
        }
        flightDuration = static_cast<i32>(value);
    }
    return castToBase<SetFireworksFunction, LootFunction>(
        std::make_unique<SetFireworksFunction>(std::move(explosions), flightDuration));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetFireworkExplosionFunction(const nlohmann::json& json)
{
    std::optional<item::component::FireworkShape> shape;
    if (json.contains("shape")) {
        if (!json["shape"].is_string()) {
            return Error(ErrorCode::InvalidData, "set_firework_explosion: 'shape' must be a string");
        }
        auto parsed = item::component::fireworkShapeFromName(json["shape"].get<std::string>());
        if (!parsed.has_value()) {
            return Error(ErrorCode::InvalidData,
                "set_firework_explosion: unknown shape '" + json["shape"].get<std::string>() + "'");
        }
        shape = *parsed;
    }
    auto readColors = [](const nlohmann::json& value) -> std::optional<std::vector<i32>> {
        if (!value.is_array()) {
            return std::nullopt;
        }
        std::vector<i32> colors;
        colors.reserve(value.size());
        for (const auto& color : value) {
            if (!color.is_number_integer()) {
                return std::nullopt;
            }
            colors.push_back(color.get<i32>());
        }
        return colors;
    };
    std::optional<std::vector<i32>> colors;
    if (json.contains("colors")) {
        colors = readColors(json["colors"]);
        if (!colors.has_value()) {
            return Error(ErrorCode::InvalidData, "set_firework_explosion: invalid 'colors'");
        }
    }
    std::optional<std::vector<i32>> fadeColors;
    if (json.contains("fade_colors")) {
        fadeColors = readColors(json["fade_colors"]);
        if (!fadeColors.has_value()) {
            return Error(ErrorCode::InvalidData, "set_firework_explosion: invalid 'fade_colors'");
        }
    }
    auto readBool = [&json](const char* key) -> std::optional<bool> {
        if (!json.contains(key)) {
            return std::nullopt;
        }
        if (!json[key].is_boolean()) {
            return std::nullopt;
        }
        return json[key].get<bool>();
    };
    std::optional<bool> trail = readBool("trail");
    std::optional<bool> twinkle = readBool("twinkle");
    if ((json.contains("trail") && !trail.has_value()) || (json.contains("twinkle") && !twinkle.has_value())) {
        return Error(ErrorCode::InvalidData, "set_firework_explosion: 'trail'/'twinkle' must be booleans");
    }
    return castToBase<SetFireworkExplosionFunction, LootFunction>(std::make_unique<SetFireworkExplosionFunction>(
        shape, std::move(colors), std::move(fadeColors), trail, twinkle));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseToggleTooltipsFunction(const nlohmann::json& json)
{
    if (!json.contains("toggles") || !json["toggles"].is_object()) {
        return Error(ErrorCode::InvalidData, "toggle_tooltips: missing object 'toggles' field");
    }
    std::vector<std::pair<item::component::DataComponentType, bool>> toggles;
    for (const auto& [name, value] : json["toggles"].items()) {
        auto type = item::component::componentTypeByName(_normalizeResourceId(name));
        if (!type.has_value()) {
            return Error(ErrorCode::InvalidData, "toggle_tooltips: unknown component '" + name + "'");
        }
        if (!value.is_boolean()) {
            return Error(ErrorCode::InvalidData, "toggle_tooltips: value of '" + name + "' must be a boolean");
        }
        toggles.emplace_back(*type, value.get<bool>());
    }
    return castToBase<ToggleTooltipsFunction, LootFunction>(
        std::make_unique<ToggleTooltipsFunction>(std::move(toggles)));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetCustomModelDataFunction(const nlohmann::json& json)
{
    std::optional<SetCustomModelDataFunction::FloatListOperation> floats;
    std::optional<SetCustomModelDataFunction::ColorListOperation> colors;
    std::optional<SetCustomModelDataFunction::BoolListOperation> flags;
    std::optional<SetCustomModelDataFunction::StringListOperation> strings;

    auto parseNumberOp = [](const nlohmann::json& value,
                             const char* key,
                             std::optional<std::vector<RandomValueRange>>& values,
                             ListOperationKind& kind,
                             i32& offset) -> std::optional<Error> {
        if (!_parseListOperationHeader(value, kind, offset, "values")) {
            return Error(ErrorCode::InvalidData, std::string("set_custom_model_data: invalid '") + key + "'");
        }
        if (!_parseNumberProviders(value["values"], values.emplace())) {
            return Error(ErrorCode::InvalidData, std::string("set_custom_model_data: invalid '") + key + "' values");
        }
        return std::nullopt;
    };
    if (json.contains("floats")) {
        std::optional<std::vector<RandomValueRange>> values;
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        i32 offset = 0;
        auto error = parseNumberOp(json["floats"], "floats", values, kind, offset);
        if (error.has_value()) {
            return *error;
        }
        floats = SetCustomModelDataFunction::FloatListOperation{kind, std::move(*values), offset};
    }
    if (json.contains("colors")) {
        std::optional<std::vector<RandomValueRange>> values;
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        i32 offset = 0;
        auto error = parseNumberOp(json["colors"], "colors", values, kind, offset);
        if (error.has_value()) {
            return *error;
        }
        colors = SetCustomModelDataFunction::ColorListOperation{kind, std::move(*values), offset};
    }
    if (json.contains("flags")) {
        const auto& value = json["flags"];
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        i32 offset = 0;
        if (!_parseListOperationHeader(value, kind, offset, "values")) {
            return Error(ErrorCode::InvalidData, "set_custom_model_data: invalid 'flags'");
        }
        std::vector<bool> values;
        for (const auto& entry : value["values"]) {
            if (!entry.is_boolean()) {
                return Error(ErrorCode::InvalidData, "set_custom_model_data: 'flags' values must be booleans");
            }
            values.push_back(entry.get<bool>());
        }
        flags = SetCustomModelDataFunction::BoolListOperation{kind, std::move(values), offset};
    }
    if (json.contains("strings")) {
        const auto& value = json["strings"];
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        i32 offset = 0;
        if (!_parseListOperationHeader(value, kind, offset, "values")) {
            return Error(ErrorCode::InvalidData, "set_custom_model_data: invalid 'strings'");
        }
        std::vector<std::string> values;
        for (const auto& entry : value["values"]) {
            if (!entry.is_string()) {
                return Error(ErrorCode::InvalidData, "set_custom_model_data: 'strings' values must be strings");
            }
            values.push_back(entry.get<std::string>());
        }
        strings = SetCustomModelDataFunction::StringListOperation{kind, std::move(values), offset};
    }
    return castToBase<SetCustomModelDataFunction, LootFunction>(std::make_unique<SetCustomModelDataFunction>(
        std::move(floats), std::move(flags), std::move(strings), std::move(colors)));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseModifyContentsFunction(const nlohmann::json& json)
{
    std::string component = "minecraft:container";
    if (json.contains("component")) {
        if (!json["component"].is_string()) {
            return Error(ErrorCode::InvalidData, "modify_contents: 'component' must be a string");
        }
        component = _normalizeResourceId(json["component"].get<std::string>());
    }
    if (component != "minecraft:container") {
        return Error(ErrorCode::InvalidData, "modify_contents: unsupported component '" + component + "'");
    }
    if (!json.contains("modifier")) {
        return Error(ErrorCode::InvalidData, "modify_contents: missing 'modifier' field");
    }
    auto modifier = parseFunction(json["modifier"]);
    if (!modifier.success()) {
        return modifier.error();
    }
    return castToBase<ModifyContentsFunction, LootFunction>(
        std::make_unique<ModifyContentsFunction>(component, modifier.value()));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetBookPagesFunction(
    const nlohmann::json& json, bool written)
{
    if (!json.contains("pages") || !json["pages"].is_array()) {
        return Error(ErrorCode::InvalidData,
            std::string("set_") + (written ? "written" : "writable") + "_book_pages: missing array 'pages' field");
    }
    for (const auto& page : json["pages"]) {
        if (!page.is_string() && !page.is_object()) {
            return Error(ErrorCode::InvalidData, "book pages must be strings or text components");
        }
    }
    ListOperationKind mode = ListOperationKind::ReplaceAll;
    if (json.contains("mode")) {
        if (!json["mode"].is_string()) {
            return Error(ErrorCode::InvalidData, "book pages: 'mode' must be a string");
        }
        auto parsed = listOperationKindFromName(json["mode"].get<std::string>());
        if (!parsed.has_value()) {
            return Error(ErrorCode::InvalidData, "book pages: unknown mode '" + json["mode"].get<std::string>() + "'");
        }
        mode = *parsed;
    }
    if (written) {
        return castToBase<SetWrittenBookPagesFunction, LootFunction>(
            std::make_unique<SetWrittenBookPagesFunction>(json["pages"], mode));
    }
    return castToBase<SetWritableBookPagesFunction, LootFunction>(
        std::make_unique<SetWritableBookPagesFunction>(json["pages"], mode));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetBookCoverFunction(const nlohmann::json& json)
{
    std::optional<std::string> author;
    if (json.contains("author")) {
        if (!json["author"].is_string()) {
            return Error(ErrorCode::InvalidData, "set_book_cover: 'author' must be a string");
        }
        author = json["author"].get<std::string>();
    }
    std::optional<nlohmann::json> title;
    if (json.contains("title")) {
        const auto& value = json["title"];
        // Filterable<T>：纯字符串或 { "raw": ..., "filtered": ... }
        if (!value.is_string() && !value.is_object()) {
            return Error(ErrorCode::InvalidData, "set_book_cover: invalid 'title'");
        }
        title = value;
    }
    std::optional<i32> generation;
    if (json.contains("generation")) {
        if (!json["generation"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "set_book_cover: 'generation' must be an integer");
        }
        generation = json["generation"].get<i32>();
    }
    return castToBase<SetBookCoverFunction, LootFunction>(
        std::make_unique<SetBookCoverFunction>(author, title, generation));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetBannerPatternFunction(const nlohmann::json& json)
{
    if (!json.contains("patterns") || !json["patterns"].is_array()) {
        return Error(ErrorCode::InvalidData, "set_banner_pattern: missing array 'patterns' field");
    }
    std::vector<std::string> colors = {"white",
        "orange",
        "magenta",
        "light_blue",
        "yellow",
        "lime",
        "pink",
        "gray",
        "light_gray",
        "cyan",
        "purple",
        "blue",
        "brown",
        "green",
        "red",
        "black"};
    for (const auto& pattern : json["patterns"]) {
        if (!pattern.is_object() || !pattern.contains("pattern") || !pattern["pattern"].is_string() ||
            !pattern.contains("color") || !pattern["color"].is_string()) {
            return Error(ErrorCode::InvalidData, "set_banner_pattern: invalid pattern entry");
        }
        if (std::find(colors.begin(), colors.end(), pattern["color"].get<std::string>()) == colors.end()) {
            return Error(ErrorCode::InvalidData,
                "set_banner_pattern: unknown color '" + pattern["color"].get<std::string>() + "'");
        }
    }
    bool append = false;
    if (json.contains("append")) {
        if (!json["append"].is_boolean()) {
            return Error(ErrorCode::InvalidData, "set_banner_pattern: 'append' must be a boolean");
        }
        append = json["append"].get<bool>();
    }
    return castToBase<SetBannerPatternFunction, LootFunction>(
        std::make_unique<SetBannerPatternFunction>(json["patterns"], append));
}

Result<std::unique_ptr<LootFunction>> LootSerializers::_parseSetStewEffectFunction(const nlohmann::json& json)
{
    auto function = std::make_unique<SetStewEffectFunction>();

    // 默认持续时间范围（秒）
    constexpr f32 DEFAULT_STEW_EFFECT_DURATION_MIN = 5.0f;
    constexpr f32 DEFAULT_STEW_EFFECT_DURATION_MAX = 10.0f;

    if (json.contains("effects") && json["effects"].is_array()) {
        for (const auto& effect : json["effects"]) {
            if (!effect.contains("type")) {
                continue;
            }

            std::string effectId = effect["type"].get<std::string>();
            RandomValueRange duration(DEFAULT_STEW_EFFECT_DURATION_MIN, DEFAULT_STEW_EFFECT_DURATION_MAX);

            if (effect.contains("duration")) {
                auto durationResult = parseRandomValueRange(effect["duration"]);
                if (durationResult.success()) {
                    duration = durationResult.value();
                }
            }

            function->addEffect(effectId, duration);
        }
    }

    return castToBase<SetStewEffectFunction, LootFunction>(std::move(function));
}

// ============================================================================
// LootEntry 解析
// ============================================================================

Result<std::unique_ptr<LootEntry>> LootSerializers::parseEntry(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Entry must be an object");
    }

    // 获取类型
    std::string entryType = "minecraft:item"; // 默认类型
    if (json.contains("type") && json["type"].is_string()) {
        entryType = json["type"].get<std::string>();
    }

    // 根据类型分发
    if (entryType == "minecraft:empty" || entryType == "empty") {
        return _parseEmptyEntry(json);
    } else if (entryType == "minecraft:item" || entryType == "item") {
        return _parseItemEntry(json);
    } else if (entryType == "minecraft:loot_table" || entryType == "loot_table") {
        return _parseTableEntry(json);
    } else if (entryType == "minecraft:tag" || entryType == "tag") {
        return _parseTagEntry(json);
    } else if (entryType == "minecraft:dynamic" || entryType == "dynamic") {
        return _parseDynamicEntry(json);
    } else if (entryType == "minecraft:alternatives" || entryType == "alternatives") {
        return _parseAlternativesEntry(json);
    } else if (entryType == "minecraft:sequence" || entryType == "sequence") {
        return _parseSequenceEntry(json);
    } else if (entryType == "minecraft:group" || entryType == "group") {
        return _parseGroupEntry(json);
    } else {
        return Error(ErrorCode::InvalidData, "Unknown entry type: " + entryType);
    }
}

Result<std::vector<std::unique_ptr<LootEntry>>> LootSerializers::parseEntries(const nlohmann::json& json)
{
    std::vector<std::unique_ptr<LootEntry>> entries;

    if (json.is_null() || !json.is_array()) {
        return entries; // 空条目列表
    }

    for (const auto& entryJson : json) {
        auto result = parseEntry(entryJson);
        if (!result.success()) {
            return result.error();
        }
        entries.push_back(result.value());
    }

    return entries;
}

nlohmann::json LootSerializers::toJson(const LootEntry& entry)
{
    nlohmann::json json;

    switch (entry.getType()) {
        case LootEntryType::Empty:
            json["type"] = "minecraft:empty";
            break;
        case LootEntryType::Item:
            json["type"] = "minecraft:item";
            if (const auto* itemEntry = dynamic_cast<const ItemLootEntry*>(&entry)) {
                json["name"] = itemEntry->getItemId();
                json["count"] = toJson(itemEntry->getCount());
            }
            break;
        case LootEntryType::Table:
            json["type"] = "minecraft:loot_table";
            if (const auto* tableEntry = dynamic_cast<const TableLootEntry*>(&entry)) {
                if (tableEntry->isInline()) {
                    // 内联表：value 为完整掉落表 JSON
                    const LootTable* inlineTable = tableEntry->getInlineTable();
                    if (inlineTable != nullptr) {
                        json["value"] = toJson(*inlineTable);
                    }
                } else {
                    // 外部引用：value 为表 ID 字符串
                    json["value"] = tableEntry->getTableId();
                }
            }
            break;
        case LootEntryType::Tag:
            json["type"] = "minecraft:tag";
            if (const auto* tagEntry = dynamic_cast<const TagLootEntry*>(&entry)) {
                json["name"] = tagEntry->getTagId();
                if (tagEntry->isExpand()) {
                    json["expand"] = true;
                }
            }
            break;
        case LootEntryType::Dynamic:
            json["type"] = "minecraft:dynamic";
            if (const auto* dynamicEntry = dynamic_cast<const DynamicLootEntry*>(&entry)) {
                json["name"] = dynamicEntry->getName();
            }
            break;
        case LootEntryType::Alternatives:
            json["type"] = "minecraft:alternatives";
            break;
        case LootEntryType::Sequence:
            json["type"] = "minecraft:sequence";
            break;
        case LootEntryType::Group:
            json["type"] = "minecraft:group";
            break;
        default:
            json["type"] = "minecraft:empty";
            break;
    }

    if (entry.getWeight() != 1) {
        json["weight"] = entry.getWeight();
    }
    if (entry.getQuality() != 0) {
        json["quality"] = entry.getQuality();
    }

    // 序列化条件
    const auto& conditions = entry.getConditions();
    if (!conditions.empty()) {
        nlohmann::json conditionsJson = nlohmann::json::array();
        for (const auto& condition : conditions) {
            conditionsJson.push_back(toJson(*condition));
        }
        json["conditions"] = conditionsJson;
    }

    return json;
}

// ============================================================================
// 条目解析辅助方法
// ============================================================================

Result<void> LootSerializers::_appendFunctions(LootEntry& entry, const nlohmann::json& json)
{
    // 函数列表逐项解析，任一项失败即整体失败（原版遇到未注册/非法函数会让整张战利品表加载失败），
    // 不允许静默跳过——被丢弃的函数会让掉落结果与数据包声明不一致。
    if (json.contains("functions") && json["functions"].is_array()) {
        for (const auto& funcJson : json["functions"]) {
            auto funcResult = parseFunction(funcJson);
            if (!funcResult.success()) {
                return funcResult.error();
            }
            entry.addFunction(funcResult.value());
        }
    }
    return Result<void>::ok();
}

Result<void> LootSerializers::_appendFunctions(LootPool& pool, const nlohmann::json& json)
{
    if (json.contains("functions") && json["functions"].is_array()) {
        for (const auto& funcJson : json["functions"]) {
            auto funcResult = parseFunction(funcJson);
            if (!funcResult.success()) {
                return funcResult.error();
            }
            pool.addFunction(funcResult.value());
        }
    }
    return Result<void>::ok();
}

Result<void> LootSerializers::_appendFunctions(LootTable& table, const nlohmann::json& json)
{
    if (json.contains("functions") && json["functions"].is_array()) {
        for (const auto& funcJson : json["functions"]) {
            auto funcResult = parseFunction(funcJson);
            if (!funcResult.success()) {
                return funcResult.error();
            }
            table.addFunction(funcResult.value());
        }
    }
    return Result<void>::ok();
}

Result<void> LootSerializers::_parseEntryBase(LootEntry& entry, const nlohmann::json& json)
{
    // 解析权重
    if (json.contains("weight") && json["weight"].is_number_integer()) {
        entry.setWeight(json["weight"].get<i32>());
    }

    // 解析质量
    if (json.contains("quality") && json["quality"].is_number_integer()) {
        entry.setQuality(json["quality"].get<i32>());
    }

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry.addCondition(condResult.value());
            }
        }
    }

    // 解析函数列表（任一项失败即整张表加载失败）
    return _appendFunctions(entry, json);
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseEmptyEntry(const nlohmann::json& json)
{
    i32 weight = 1;
    i32 quality = 0;

    if (json.contains("weight") && json["weight"].is_number_integer()) {
        weight = json["weight"].get<i32>();
    }
    if (json.contains("quality") && json["quality"].is_number_integer()) {
        quality = json["quality"].get<i32>();
    }

    auto entry = std::make_unique<EmptyLootEntry>(weight, quality);

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry->addCondition(condResult.value());
            }
        }
    }

    auto funcError = _appendFunctions(*entry, json);
    if (!funcError.success()) {
        return funcError.error();
    }

    return castToBase<EmptyLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseItemEntry(const nlohmann::json& json)
{
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::InvalidData, "item entry missing 'name' field");
    }

    std::string itemId = json["name"].get<std::string>();

    i32 weight = 1;
    i32 quality = 0;
    RandomValueRange count(1.0f, 1.0f);

    if (json.contains("weight") && json["weight"].is_number_integer()) {
        weight = json["weight"].get<i32>();
    }
    if (json.contains("quality") && json["quality"].is_number_integer()) {
        quality = json["quality"].get<i32>();
    }

    // 解析数量
    if (json.contains("count")) {
        auto countResult = parseRandomValueRange(json["count"]);
        if (countResult.success()) {
            count = countResult.value();
        }
    }
    // 注意：函数中的 set_count 会覆盖此值

    auto entry = std::make_unique<ItemLootEntry>(itemId, count, weight, quality);

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry->addCondition(condResult.value());
            }
        }
    }

    auto funcError = _appendFunctions(*entry, json);
    if (!funcError.success()) {
        return funcError.error();
    }

    return castToBase<ItemLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseTableEntry(const nlohmann::json& json)
{
    // MC 1.21+ 的 minecraft:loot_table entry 用 value 字段（Either<ResourceKey, LootTable>）：
    // - 字符串：外部掉落表引用 ID
    // - 对象：内联完整掉落表
    if (!json.contains("value")) {
        return Error(ErrorCode::InvalidData, "loot_table entry missing 'value' field");
    }

    const nlohmann::json& valueJson = json["value"];

    std::unique_ptr<LootEntry> entry;
    if (valueJson.is_string()) {
        // 外部引用
        entry = std::make_unique<TableLootEntry>(valueJson.get<std::string>(), 1, 0);
    } else if (valueJson.is_object()) {
        // 内联掉落表
        auto tableResult = parseLootTable(valueJson);
        if (!tableResult.success()) {
            return tableResult.error();
        }
        entry = std::make_unique<TableLootEntry>(tableResult.value(), 1, 0);
    } else {
        return Error(ErrorCode::InvalidData, "loot_table entry 'value' field must be a string or an object");
    }

    // 解析公共属性（weight/quality/conditions/functions）
    auto baseError = _parseEntryBase(*entry, json);
    if (!baseError.success()) {
        return baseError.error();
    }

    return entry;
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseTagEntry(const nlohmann::json& json)
{
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::InvalidData, "tag entry missing 'name' field");
    }

    std::string tagId = json["name"].get<std::string>();

    i32 weight = 1;
    i32 quality = 0;
    bool expand = false;

    if (json.contains("weight") && json["weight"].is_number_integer()) {
        weight = json["weight"].get<i32>();
    }
    if (json.contains("quality") && json["quality"].is_number_integer()) {
        quality = json["quality"].get<i32>();
    }
    if (json.contains("expand") && json["expand"].is_boolean()) {
        expand = json["expand"].get<bool>();
    }

    auto entry = std::make_unique<TagLootEntry>(tagId, expand, weight, quality);

    auto baseError = _parseEntryBase(*entry, json);
    if (!baseError.success()) {
        return baseError.error();
    }

    return castToBase<TagLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseDynamicEntry(const nlohmann::json& json)
{
    if (!json.contains("name") || !json["name"].is_string()) {
        return Error(ErrorCode::InvalidData, "dynamic entry missing 'name' field");
    }

    std::string name = json["name"].get<std::string>();

    i32 weight = 1;
    i32 quality = 0;

    if (json.contains("weight") && json["weight"].is_number_integer()) {
        weight = json["weight"].get<i32>();
    }
    if (json.contains("quality") && json["quality"].is_number_integer()) {
        quality = json["quality"].get<i32>();
    }

    auto entry = std::make_unique<DynamicLootEntry>(name, weight, quality);

    auto baseError = _parseEntryBase(*entry, json);
    if (!baseError.success()) {
        return baseError.error();
    }

    return castToBase<DynamicLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseAlternativesEntry(const nlohmann::json& json)
{
    if (!json.contains("children") || !json["children"].is_array()) {
        return Error(ErrorCode::InvalidData, "alternatives entry missing 'children' array");
    }

    std::vector<std::unique_ptr<LootEntry>> children;
    for (const auto& childJson : json["children"]) {
        auto childResult = parseEntry(childJson);
        if (!childResult.success()) {
            return childResult.error();
        }
        children.push_back(childResult.value());
    }

    auto entry = std::make_unique<AlternativesLootEntry>(std::move(children));

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry->addCondition(condResult.value());
            }
        }
    }

    auto funcError = _appendFunctions(*entry, json);
    if (!funcError.success()) {
        return funcError.error();
    }

    return castToBase<AlternativesLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseSequenceEntry(const nlohmann::json& json)
{
    if (!json.contains("children") || !json["children"].is_array()) {
        return Error(ErrorCode::InvalidData, "sequence entry missing 'children' array");
    }

    std::vector<std::unique_ptr<LootEntry>> children;
    for (const auto& childJson : json["children"]) {
        auto childResult = parseEntry(childJson);
        if (!childResult.success()) {
            return childResult.error();
        }
        children.push_back(childResult.value());
    }

    auto entry = std::make_unique<SequenceLootEntry>(std::move(children));

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry->addCondition(condResult.value());
            }
        }
    }

    auto funcError = _appendFunctions(*entry, json);
    if (!funcError.success()) {
        return funcError.error();
    }

    return castToBase<SequenceLootEntry, LootEntry>(std::move(entry));
}

Result<std::unique_ptr<LootEntry>> LootSerializers::_parseGroupEntry(const nlohmann::json& json)
{
    if (!json.contains("children") || !json["children"].is_array()) {
        return Error(ErrorCode::InvalidData, "group entry missing 'children' array");
    }

    std::vector<std::unique_ptr<LootEntry>> children;
    for (const auto& childJson : json["children"]) {
        auto childResult = parseEntry(childJson);
        if (!childResult.success()) {
            return childResult.error();
        }
        children.push_back(childResult.value());
    }

    auto entry = std::make_unique<GroupLootEntry>(std::move(children));

    // 解析条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                entry->addCondition(condResult.value());
            }
        }
    }

    auto funcError = _appendFunctions(*entry, json);
    if (!funcError.success()) {
        return funcError.error();
    }

    return castToBase<GroupLootEntry, LootEntry>(std::move(entry));
}

// ============================================================================
// LootPool 解析
// ============================================================================

Result<std::unique_ptr<LootPool>> LootSerializers::parsePool(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Pool must be an object");
    }

    // 解析掷骰次数
    if (!json.contains("rolls")) {
        return Error(ErrorCode::InvalidData, "Pool missing 'rolls' field");
    }

    auto rollsResult = parseRandomValueRange(json["rolls"]);
    if (!rollsResult.success()) {
        return rollsResult.error();
    }

    // 解析额外掷骰次数
    RandomValueRange bonusRolls(0.0f, 0.0f);
    if (json.contains("bonus_rolls")) {
        auto bonusResult = parseRandomValueRange(json["bonus_rolls"]);
        if (bonusResult.success()) {
            bonusRolls = bonusResult.value();
        }
    }

    auto pool = std::make_unique<LootPool>(rollsResult.value(), bonusRolls);

    // 解析名称
    if (json.contains("name") && json["name"].is_string()) {
        pool->setName(json["name"].get<std::string>());
    }

    // 解析条目
    if (json.contains("entries") && json["entries"].is_array()) {
        for (const auto& entryJson : json["entries"]) {
            auto entryResult = parseEntry(entryJson);
            if (!entryResult.success()) {
                return entryResult.error();
            }
            pool->addEntry(entryResult.value());
        }
    }

    // 解析池级条件
    if (json.contains("conditions") && json["conditions"].is_array()) {
        for (const auto& condJson : json["conditions"]) {
            auto condResult = parseCondition(condJson);
            if (condResult.success()) {
                pool->addCondition(condResult.value());
            }
        }
    }

    // 解析池级函数（任一项失败即整张表加载失败）
    auto poolFuncError = _appendFunctions(*pool, json);
    if (!poolFuncError.success()) {
        return poolFuncError.error();
    }

    return pool;
}

Result<std::vector<std::unique_ptr<LootPool>>> LootSerializers::parsePools(const nlohmann::json& json)
{
    std::vector<std::unique_ptr<LootPool>> pools;

    if (json.is_null() || !json.is_array()) {
        return pools; // 空池列表
    }

    for (const auto& poolJson : json) {
        auto result = parsePool(poolJson);
        if (!result.success()) {
            return result.error();
        }
        pools.push_back(result.value());
    }

    return pools;
}

nlohmann::json LootSerializers::toJson(const LootPool& pool)
{
    nlohmann::json json;

    if (!pool.getName().empty()) {
        json["name"] = pool.getName();
    }

    json["rolls"] = toJson(pool.getRolls());
    json["bonus_rolls"] = toJson(pool.getBonusRolls());

    nlohmann::json entries = nlohmann::json::array();
    for (const auto& entry : pool.getEntries()) {
        entries.push_back(toJson(*entry));
    }
    json["entries"] = entries;

    if (!pool.getConditions().empty()) {
        nlohmann::json conditions = nlohmann::json::array();
        for (const auto& cond : pool.getConditions()) {
            conditions.push_back(toJson(*cond));
        }
        json["conditions"] = conditions;
    }

    if (!pool.getFunctions().empty()) {
        nlohmann::json functions = nlohmann::json::array();
        for (const auto& func : pool.getFunctions()) {
            functions.push_back(toJson(*func));
        }
        json["functions"] = functions;
    }

    return json;
}

// ============================================================================
// LootTable 解析
// ============================================================================

Result<std::unique_ptr<LootTable>> LootSerializers::parseLootTable(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Loot table must be an object");
    }

    auto table = std::make_unique<LootTable>();

    // 解析参数集类型
    if (json.contains("type") && json["type"].is_string()) {
        std::string type = json["type"].get<std::string>();
        if (type == "minecraft:block" || type == "block") {
            table->setParameterSet(LootParameterSets::block());
        } else if (type == "minecraft:chest" || type == "chest") {
            table->setParameterSet(LootParameterSets::chest());
        } else if (type == "minecraft:entity" || type == "entity") {
            table->setParameterSet(LootParameterSets::entity());
        } else if (type == "minecraft:fishing" || type == "fishing") {
            table->setParameterSet(LootParameterSets::fishing());
        } else if (type == "minecraft:gift" || type == "gift") {
            table->setParameterSet(LootParameterSets::gift());
        } else if (type == "minecraft:barter" || type == "barter") {
            table->setParameterSet(LootParameterSets::barter());
        } else if (type == "minecraft:empty" || type == "empty") {
            table->setParameterSet(LootParameterSets::empty());
        } else {
            table->setParameterSet(LootParameterSets::generic());
        }
    }

    // 解析池
    if (json.contains("pools") && json["pools"].is_array()) {
        for (const auto& poolJson : json["pools"]) {
            auto poolResult = parsePool(poolJson);
            if (!poolResult.success()) {
                return poolResult.error();
            }
            table->addPool(poolResult.value());
        }
    }

    // 解析表级函数（任一项失败即整张表加载失败）
    auto tableFuncError = _appendFunctions(*table, json);
    if (!tableFuncError.success()) {
        return tableFuncError.error();
    }

    return table;
}

Result<std::unique_ptr<LootTable>> LootSerializers::parseLootTable(const std::string& jsonStr)
{
    try {
        nlohmann::json json = nlohmann::json::parse(jsonStr);
        return parseLootTable(json);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON error: ") + e.what());
    }
}

nlohmann::json LootSerializers::toJson(const LootTable& table)
{
    nlohmann::json json;

    json["type"] = table.getParameterSet().getName();

    nlohmann::json pools = nlohmann::json::array();
    for (const auto& pool : table.getPools()) {
        pools.push_back(toJson(*pool));
    }
    json["pools"] = pools;

    if (!table.getFunctions().empty()) {
        nlohmann::json functions = nlohmann::json::array();
        for (const auto& func : table.getFunctions()) {
            functions.push_back(toJson(*func));
        }
        json["functions"] = functions;
    }

    return json;
}

std::string LootSerializers::toJsonString(const LootTable& table, i32 indent)
{
    nlohmann::json json = toJson(table);
    return json.dump(indent);
}

} // namespace loot
} // namespace mc
