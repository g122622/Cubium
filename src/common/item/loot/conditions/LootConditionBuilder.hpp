/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#pragma once

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace loot {

// Forward-declared condition classes (headers included in .cpp)
class SilkTouchCondition;
class FortuneCondition;
class RandomChanceCondition;
class RandomChanceWithLuckCondition;
class NotCondition;
class AndCondition;
class OrCondition;
class BlockStateCondition;
class ToolTypeCondition;
class SurvivesExplosionCondition;
class KilledByPlayerCondition;
class EntityScoresCondition;
class LocationCheckCondition;
class WeatherCheckCondition;
class TimeCheckCondition;
class DamageSourcePropertiesCondition;
class ReferenceCondition;
class FishingOpenWaterCondition;
class TableBonusCondition;

/**
 * @brief 掉落条件构建器
 *
 * 提供流畅的条件构建接口。
 *
 * 示例:
 * @code
 * auto condition = LootConditionBuilder::silkTouch();
 * auto notSilkTouch = LootConditionBuilder::not_(LootConditionBuilder::silkTouch());
 * auto orCondition = LootConditionBuilder::or_({
 *     LootConditionBuilder::silkTouch(),
 *     LootConditionBuilder::toolType(HarvestTool::Pickaxe)
 * });
 * @endcode
 */
class LootConditionBuilder {
public:
    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建精准采集条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> silkTouch();

    /**
     * @brief 创建时运条件
     * @param minLevel 最小时运等级
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> fortune(i32 minLevel = 0);

    /**
     * @brief 创建随机概率条件
     * @param chance 概率值 (0.0 - 1.0)
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> randomChance(f32 chance);

    /**
     * @brief 创建受幸运影响的随机概率条件
     * @param baseChance 基础概率
     * @param luckCoefficient 幸运系数
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> randomChanceWithLuck(f32 baseChance, f32 luckCoefficient);

    /**
     * @brief 创建取反条件
     * @param condition 要取反的条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> not_(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 创建与条件
     * @param conditions 子条件列表
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> and_(std::vector<std::unique_ptr<LootCondition>> conditions);

    /**
     * @brief 创建或条件
     * @param conditions 子条件列表
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> or_(std::vector<std::unique_ptr<LootCondition>> conditions);

    /**
     * @brief 创建方块状态条件（仅检查方块ID）
     * @param blockId 方块ID
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> blockState(const std::string& blockId);

    /**
     * @brief 创建方块状态条件（检查方块ID和属性）
     * @param blockId 方块ID
     * @param properties 属性匹配谓词
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> blockState(
        const std::string& blockId, StatePropertiesPredicate properties);

    /**
     * @brief 创建工具类型条件
     * @param toolType 工具类型
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> toolType(u8 toolType);

    // ========== 新增条件工厂方法 ==========

    /**
     * @brief 创建爆炸存活条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> survivesExplosion();

    /**
     * @brief 创建被玩家击杀条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> killedByPlayer();

    /**
     * @brief 创建实体属性条件
     * @param target 实体目标
     * @param predicate 实体谓词
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> entityProperties(
        EntityPropertiesCondition::EntityTarget target, advancement::EntityPredicate predicate);

    /**
     * @brief 创建实体分数条件
     * @param target 实体目标
     * @param scores 分数映射
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> entityScores(
        EntityPropertiesCondition::EntityTarget target, std::unordered_map<std::string, RandomValueRange> scores);

    /**
     * @brief 创建位置检查条件
     * @param predicate 位置谓词
     * @param offsetX X偏移
     * @param offsetY Y偏移
     * @param offsetZ Z偏移
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> locationCheck(
        advancement::LocationPredicate predicate, i32 offsetX = 0, i32 offsetY = 0, i32 offsetZ = 0);

    /**
     * @brief 创建天气检查条件
     * @param raining 是否要求下雨（nullopt 不检查）
     * @param thundering 是否要求雷暴（nullopt 不检查）
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> weatherCheck(
        std::optional<bool> raining = std::nullopt, std::optional<bool> thundering = std::nullopt);

    /**
     * @brief 创建时间检查条件
     * @param period 取模周期（0不取模）
     * @param value 时间值范围
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> timeCheck(i64 period, RandomValueRange value);

    /**
     * @brief 创建伤害源属性条件
     * @param predicate 伤害源谓词
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> damageSourceProperties(
        advancement::DamageSourcePredicate predicate);

    /**
     * @brief 创建引用条件
     * @param name 引用的谓词名称
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> reference(const std::string& name);

    /**
     * @brief 创建附魔等级概率表条件
     * @param enchantmentId 附魔ID（如 "minecraft:fortune"）
     * @param chances 概率表，每个元素对应一个附魔等级的概率
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> tableBonus(std::string enchantmentId, std::vector<f32> chances);
};

} // namespace loot
} // namespace mc
