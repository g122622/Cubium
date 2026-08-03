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

#include "common/item/loot/conditions/LootConditionBuilder.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/item/loot/conditions/AndCondition.hpp"
#include "common/item/loot/conditions/BlockStateCondition.hpp"
#include "common/item/loot/conditions/DamageSourcePropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityScoresCondition.hpp"
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
#include "common/item/loot/conditions/ToolTypeCondition.hpp"
#include "common/item/loot/conditions/WeatherCheckCondition.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// LootConditionBuilder
// ============================================================================

std::unique_ptr<LootCondition> LootConditionBuilder::silkTouch()
{
    return std::make_unique<SilkTouchCondition>();
}

std::unique_ptr<LootCondition> LootConditionBuilder::fortune(i32 minLevel)
{
    return std::make_unique<FortuneCondition>(minLevel);
}

std::unique_ptr<LootCondition> LootConditionBuilder::randomChance(f32 chance)
{
    return std::make_unique<RandomChanceCondition>(chance);
}

std::unique_ptr<LootCondition> LootConditionBuilder::randomChanceWithLuck(f32 baseChance, f32 luckCoefficient)
{
    return std::make_unique<RandomChanceWithLuckCondition>(baseChance, luckCoefficient);
}

std::unique_ptr<LootCondition> LootConditionBuilder::not_(std::unique_ptr<LootCondition> condition)
{
    return std::make_unique<NotCondition>(std::move(condition));
}

std::unique_ptr<LootCondition> LootConditionBuilder::and_(std::vector<std::unique_ptr<LootCondition>> conditions)
{
    return std::make_unique<AndCondition>(std::move(conditions));
}

std::unique_ptr<LootCondition> LootConditionBuilder::or_(std::vector<std::unique_ptr<LootCondition>> conditions)
{
    return std::make_unique<OrCondition>(std::move(conditions));
}

std::unique_ptr<LootCondition> LootConditionBuilder::blockState(const std::string& blockId)
{
    return std::make_unique<BlockStateCondition>(blockId);
}

std::unique_ptr<LootCondition> LootConditionBuilder::blockState(
    const std::string& blockId, StatePropertiesPredicate properties)
{
    return std::make_unique<BlockStateCondition>(blockId, std::move(properties));
}

std::unique_ptr<LootCondition> LootConditionBuilder::toolType(u8 toolType)
{
    return std::make_unique<ToolTypeCondition>(toolType);
}

std::unique_ptr<LootCondition> LootConditionBuilder::matchTool(std::optional<advancement::ItemPredicate> predicate)
{
    if (predicate.has_value()) {
        return std::make_unique<MatchToolCondition>(std::move(predicate.value()));
    }
    return std::make_unique<MatchToolCondition>();
}

// ============================================================================
// 新增条件工厂方法
// ============================================================================

std::unique_ptr<LootCondition> LootConditionBuilder::survivesExplosion()
{
    return std::make_unique<SurvivesExplosionCondition>();
}

std::unique_ptr<LootCondition> LootConditionBuilder::killedByPlayer()
{
    return std::make_unique<KilledByPlayerCondition>();
}

std::unique_ptr<LootCondition> LootConditionBuilder::entityProperties(
    EntityPropertiesCondition::EntityTarget target, advancement::EntityPredicate predicate)
{
    return std::make_unique<EntityPropertiesCondition>(target, std::move(predicate));
}

std::unique_ptr<LootCondition> LootConditionBuilder::entityScores(
    EntityPropertiesCondition::EntityTarget target, std::unordered_map<std::string, RandomValueRange> scores)
{
    return std::make_unique<EntityScoresCondition>(target, std::move(scores));
}

std::unique_ptr<LootCondition> LootConditionBuilder::locationCheck(
    advancement::LocationPredicate predicate, i32 offsetX, i32 offsetY, i32 offsetZ)
{
    return std::make_unique<LocationCheckCondition>(std::move(predicate), offsetX, offsetY, offsetZ);
}

std::unique_ptr<LootCondition> LootConditionBuilder::weatherCheck(
    std::optional<bool> raining, std::optional<bool> thundering)
{
    return std::make_unique<WeatherCheckCondition>(std::move(raining), std::move(thundering));
}

std::unique_ptr<LootCondition> LootConditionBuilder::timeCheck(i64 period, RandomValueRange value)
{
    return std::make_unique<TimeCheckCondition>(period, std::move(value));
}

std::unique_ptr<LootCondition> LootConditionBuilder::damageSourceProperties(
    advancement::DamageSourcePredicate predicate)
{
    return std::make_unique<DamageSourcePropertiesCondition>(std::move(predicate));
}

std::unique_ptr<LootCondition> LootConditionBuilder::reference(const std::string& name)
{
    return std::make_unique<ReferenceCondition>(name);
}

std::unique_ptr<LootCondition> LootConditionBuilder::tableBonus(std::string enchantmentId, std::vector<f32> chances)
{
    return std::make_unique<TableBonusCondition>(std::move(enchantmentId), std::move(chances));
}

} // namespace loot
} // namespace mc
