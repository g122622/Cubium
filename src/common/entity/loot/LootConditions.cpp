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

#include "LootConditions.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>

namespace mc {
namespace loot {

namespace {

const Entity* getConditionTargetEntity(LootContext& context, EntityPropertiesCondition::EntityTarget target)
{
    switch (target) {
        case EntityPropertiesCondition::EntityTarget::This:
            return context.get<Entity>(LootParams::THIS_ENTITY);
        case EntityPropertiesCondition::EntityTarget::Killer:
            return context.get<Entity>(LootParams::KILLER_ENTITY);
        case EntityPropertiesCondition::EntityTarget::DirectKiller:
            return context.get<Entity>(LootParams::DIRECT_KILLER);
        case EntityPropertiesCondition::EntityTarget::KillerPlayer: {
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            return static_cast<const Entity*>(player);
        }
    }
    return nullptr;
}

std::string getScoreboardEntryName(const Entity& entity)
{
    if (const auto* player = dynamic_cast<const Player*>(&entity)) {
        return player->username();
    }
    return entity.uuid();
}

} // namespace

// ============================================================================
// SilkTouchCondition
// ============================================================================

bool SilkTouchCondition::test(LootContext& context) const
{
    // 检查是否有精准采集附魔
    // 使用 SILK_TOUCH_LEVEL 参数
    auto* silkTouchLevel = context.get<i32>(LootParams::SILK_TOUCH_LEVEL);
    if (silkTouchLevel && *silkTouchLevel > 0) {
        return true;
    }
    return false;
}

std::unique_ptr<LootCondition> SilkTouchCondition::clone() const
{
    return std::make_unique<SilkTouchCondition>();
}

// ============================================================================
// FortuneCondition
// ============================================================================

FortuneCondition::FortuneCondition(i32 minLevel)
    : m_minLevel(minLevel)
{}

bool FortuneCondition::test(LootContext& context) const
{
    i32 fortuneLevel = getFortuneLevel(context);
    return fortuneLevel >= m_minLevel;
}

std::unique_ptr<LootCondition> FortuneCondition::clone() const
{
    return std::make_unique<FortuneCondition>(m_minLevel);
}

i32 FortuneCondition::getFortuneLevel(LootContext& context)
{
    // 从上下文获取时运附魔等级
    auto* fortuneLevel = context.get<i32>(LootParams::FORTUNE_LEVEL);
    if (fortuneLevel && *fortuneLevel > 0) {
        return *fortuneLevel;
    }
    return 0;
}

i32 FortuneCondition::applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random)
{
    if (fortuneLevel <= 0) {
        return baseCount;
    }

    // MC 1.16.5 OreDropsFormula (乘法式):
    // int i = random.nextInt(fortune + 2) - 1;
    // if (i < 0) i = 0;
    // return baseCount * (i + 1);
    //
    // 注意：此方法已弃用，请使用 ApplyBonusFunction::calculateOreDrops()
    i32 i = random.nextInt(fortuneLevel + 2) - 1;
    if (i < 0) {
        i = 0;
    }

    return baseCount * (i + 1);
}

// ============================================================================
// RandomChanceCondition
// ============================================================================

RandomChanceCondition::RandomChanceCondition(f32 chance, bool affectedByLuck)
    : m_chance(chance)
    , m_affectedByLuck(affectedByLuck)
{}

bool RandomChanceCondition::test(LootContext& context) const
{
    f32 actualChance = m_chance;

    if (m_affectedByLuck) {
        // 幸运值增加概率
        actualChance += context.getLuck();
    }

    return context.getRandom().nextFloat() < actualChance;
}

std::unique_ptr<LootCondition> RandomChanceCondition::clone() const
{
    return std::make_unique<RandomChanceCondition>(m_chance, m_affectedByLuck);
}

// ============================================================================
// RandomChanceWithLuckCondition
// ============================================================================

RandomChanceWithLuckCondition::RandomChanceWithLuckCondition(f32 baseChance, f32 luckCoefficient)
    : m_baseChance(baseChance)
    , m_luckCoefficient(luckCoefficient)
{}

bool RandomChanceWithLuckCondition::test(LootContext& context) const
{
    f32 chance = m_baseChance + context.getLuck() * m_luckCoefficient;
    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> RandomChanceWithLuckCondition::clone() const
{
    return std::make_unique<RandomChanceWithLuckCondition>(m_baseChance, m_luckCoefficient);
}

// ============================================================================
// NotCondition
// ============================================================================

NotCondition::NotCondition(std::unique_ptr<LootCondition> condition)
    : m_condition(std::move(condition))
{}

bool NotCondition::test(LootContext& context) const
{
    if (!m_condition) {
        return true;
    }
    return !m_condition->test(context);
}

std::unique_ptr<LootCondition> NotCondition::clone() const
{
    if (m_condition) {
        return std::make_unique<NotCondition>(m_condition->clone());
    }
    return std::make_unique<NotCondition>(nullptr);
}

// ============================================================================
// AndCondition
// ============================================================================

AndCondition::AndCondition(std::vector<std::unique_ptr<LootCondition>> conditions)
    : m_conditions(std::move(conditions))
{}

bool AndCondition::test(LootContext& context) const
{
    return std::all_of(m_conditions.begin(),
        m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) { return cond && cond->test(context); });
}

std::unique_ptr<LootCondition> AndCondition::clone() const
{
    std::vector<std::unique_ptr<LootCondition>> cloned;
    for (const auto& cond : m_conditions) {
        if (cond) {
            cloned.push_back(cond->clone());
        }
    }
    return std::make_unique<AndCondition>(std::move(cloned));
}

void AndCondition::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

// ============================================================================
// OrCondition
// ============================================================================

OrCondition::OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions)
    : m_conditions(std::move(conditions))
{}

bool OrCondition::test(LootContext& context) const
{
    return std::any_of(m_conditions.begin(),
        m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) { return cond && cond->test(context); });
}

std::unique_ptr<LootCondition> OrCondition::clone() const
{
    std::vector<std::unique_ptr<LootCondition>> cloned;
    for (const auto& cond : m_conditions) {
        if (cond) {
            cloned.push_back(cond->clone());
        }
    }
    return std::make_unique<OrCondition>(std::move(cloned));
}

void OrCondition::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

// ============================================================================
// BlockStateCondition
// ============================================================================

BlockStateCondition::BlockStateCondition(const std::string& blockId)
    : m_blockId(blockId)
    , m_properties()
{}

BlockStateCondition::BlockStateCondition(const std::string& blockId, StatePropertiesPredicate properties)
    : m_blockId(blockId)
    , m_properties(std::move(properties))
{}

bool BlockStateCondition::test(LootContext& context) const
{
    // 从上下文获取 BLOCK_STATE 参数
    auto* blockState = context.get<BlockState>(LootParams::BLOCK_STATE);
    if (!blockState) {
        return false;
    }

    // 检查方块ID是否匹配
    if (blockState->blockLocation().toString() != m_blockId) {
        return false;
    }

    // 如果有属性匹配条件，检查属性
    if (!m_properties.isEmpty()) {
        return m_properties.matches(*blockState);
    }

    return true;
}

std::unique_ptr<LootCondition> BlockStateCondition::clone() const
{
    return std::make_unique<BlockStateCondition>(m_blockId, m_properties);
}

// ============================================================================
// ToolTypeCondition
// ============================================================================

ToolTypeCondition::ToolTypeCondition(u8 toolType)
    : m_toolType(toolType)
{}

bool ToolTypeCondition::test(LootContext& context) const
{
    // 从上下文获取工具参数
    auto* tool = context.get<ItemStack>(LootParams::TOOL);
    if (!tool || tool->isEmpty()) {
        // 空手不满足任何工具类型条件
        return false;
    }

    // 获取物品
    const Item* item = tool->getItem();
    if (!item) {
        return false;
    }

    // 检查是否为工具物品，并获取工具类型
    const item::tool::ToolItem* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
    if (toolItem) {
        return static_cast<u8>(toolItem->getToolType()) == m_toolType;
    }

    // 非工具物品不满足工具类型条件
    return false;
}

std::unique_ptr<LootCondition> ToolTypeCondition::clone() const
{
    return std::make_unique<ToolTypeCondition>(m_toolType);
}

// ============================================================================
// SurvivesExplosionCondition
// ============================================================================

bool SurvivesExplosionCondition::test(LootContext& context) const
{
    // 检查上下文中是否有爆炸半径参数
    auto* radius = context.get<f32>(LootParams::EXPLOSION_RADIUS);
    if (!radius) {
        // 非爆炸破坏，物品总是存活
        return true;
    }

    // 爆炸半径为0或负数，物品存活
    if (*radius <= 0.0f) {
        return true;
    }

    // 以 1/radius 的概率存活
    f32 chance = 1.0f / *radius;
    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> SurvivesExplosionCondition::clone() const
{
    return std::make_unique<SurvivesExplosionCondition>();
}

// ============================================================================
// KilledByPlayerCondition
// ============================================================================

bool KilledByPlayerCondition::test(LootContext& context) const
{
    // 检查 KILLER_PLAYER 参数是否存在且非 null
    return context.has(LootParams::KILLER_PLAYER);
}

std::unique_ptr<LootCondition> KilledByPlayerCondition::clone() const
{
    return std::make_unique<KilledByPlayerCondition>();
}

// ============================================================================
// EntityPropertiesCondition
// ============================================================================

EntityPropertiesCondition::EntityPropertiesCondition(EntityTarget target, advancement::EntityPredicate predicate)
    : m_target(target)
    , m_predicate(std::move(predicate))
    , m_isAny(m_predicate.isAny())
{}

bool EntityPropertiesCondition::test(LootContext& context) const
{
    const Entity* entity = getConditionTargetEntity(context, m_target);

    if (!entity) {
        return false;
    }

    // 空谓词只检查实体是否存在
    if (m_isAny) {
        return true;
    }

    // 使用 EntityPredicate 的 test(world, x, y, z, entity) 方法
    // 位置使用实体的当前位置
    return m_predicate.test(context.getWorld(),
        static_cast<f64>(entity->x()),
        static_cast<f64>(entity->y()),
        static_cast<f64>(entity->z()),
        *entity);
}

std::unique_ptr<LootCondition> EntityPropertiesCondition::clone() const
{
    return std::make_unique<EntityPropertiesCondition>(m_target, m_predicate);
}

EntityPropertiesCondition::EntityTarget EntityPropertiesCondition::parseEntityTarget(const std::string& str)
{
    if (str == "this") {
        return EntityTarget::This;
    } else if (str == "killer") {
        return EntityTarget::Killer;
    } else if (str == "direct_killer") {
        return EntityTarget::DirectKiller;
    } else if (str == "killer_player") {
        return EntityTarget::KillerPlayer;
    }
    return EntityTarget::This;
}

std::string EntityPropertiesCondition::entityTargetToString(EntityTarget target)
{
    switch (target) {
        case EntityTarget::This:
            return "this";
        case EntityTarget::Killer:
            return "killer";
        case EntityTarget::DirectKiller:
            return "direct_killer";
        case EntityTarget::KillerPlayer:
            return "killer_player";
        default:
            return "this";
    }
}

// ============================================================================
// EntityScoresCondition
// ============================================================================

EntityScoresCondition::EntityScoresCondition(
    EntityPropertiesCondition::EntityTarget target, std::unordered_map<std::string, RandomValueRange> scores)
    : m_target(target)
    , m_scores(std::move(scores))
{}

bool EntityScoresCondition::test(LootContext& context) const
{
    const Entity* entity = getConditionTargetEntity(context, m_target);
    if (!entity) {
        return false;
    }

    const auto* serverWorld = context.getWorld().asServerWorld();
    if (serverWorld == nullptr) {
        return false;
    }

    const auto* playerEntity = dynamic_cast<const Player*>(entity);
    if (playerEntity == nullptr) {
        return false;
    }

    const auto* serverPlayer = dynamic_cast<const ServerPlayer*>(playerEntity);
    if (serverPlayer == nullptr || serverPlayer->getServer() == nullptr) {
        return false;
    }

    auto& scoreboard = serverPlayer->getServer()->scoreboard();
    const std::string entryName = getScoreboardEntryName(*entity);

    for (const auto& [objectiveName, range] : m_scores) {
        auto* objective = scoreboard.getObjective(objectiveName);
        if (objective == nullptr) {
            return false;
        }

        const auto* score = scoreboard.getScore(entryName, *objective);
        if (score == nullptr) {
            return false;
        }

        const f32 points = static_cast<f32>(score->getScorePoints());
        if (points < range.getMin() || points > range.getMax()) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<LootCondition> EntityScoresCondition::clone() const
{
    return std::make_unique<EntityScoresCondition>(m_target, m_scores);
}

// ============================================================================
// LocationCheckCondition
// ============================================================================

LocationCheckCondition::LocationCheckCondition(
    advancement::LocationPredicate predicate, i32 offsetX, i32 offsetY, i32 offsetZ)
    : m_predicate(std::move(predicate))
    , m_offsetX(offsetX)
    , m_offsetY(offsetY)
    , m_offsetZ(offsetZ)
    , m_isAny(m_predicate.isAny())
{}

bool LocationCheckCondition::test(LootContext& context) const
{
    // 从 BLOCK_POS 获取位置
    auto* blockPos = context.get<BlockPos>(LootParams::BLOCK_POS);
    if (blockPos) {
        f64 x = static_cast<f64>(blockPos->x + m_offsetX);
        f64 y = static_cast<f64>(blockPos->y + m_offsetY);
        f64 z = static_cast<f64>(blockPos->z + m_offsetZ);

        if (m_isAny) {
            return true;
        }
        return m_predicate.test(context.getWorld(), x, y, z);
    }

    // 从实体获取位置
    auto* entity = context.get<Entity>(LootParams::THIS_ENTITY);
    if (entity) {
        f64 x = static_cast<f64>(entity->x()) + static_cast<f64>(m_offsetX);
        f64 y = static_cast<f64>(entity->y()) + static_cast<f64>(m_offsetY);
        f64 z = static_cast<f64>(entity->z()) + static_cast<f64>(m_offsetZ);

        if (m_isAny) {
            return true;
        }
        return m_predicate.test(context.getWorld(), x, y, z);
    }

    return false;
}

std::unique_ptr<LootCondition> LocationCheckCondition::clone() const
{
    return std::make_unique<LocationCheckCondition>(m_predicate, m_offsetX, m_offsetY, m_offsetZ);
}

// ============================================================================
// WeatherCheckCondition
// ============================================================================

WeatherCheckCondition::WeatherCheckCondition(std::optional<bool> raining, std::optional<bool> thundering)
    : m_raining(std::move(raining))
    , m_thundering(std::move(thundering))
{}

bool WeatherCheckCondition::test(LootContext& context) const
{
    IWorld& world = context.getWorld();

    if (m_raining.has_value() && *m_raining != world.isRaining()) {
        return false;
    }

    if (m_thundering.has_value() && *m_thundering != world.isThundering()) {
        return false;
    }

    return true;
}

std::unique_ptr<LootCondition> WeatherCheckCondition::clone() const
{
    return std::make_unique<WeatherCheckCondition>(m_raining, m_thundering);
}

// ============================================================================
// TimeCheckCondition
// ============================================================================

TimeCheckCondition::TimeCheckCondition(i64 period, RandomValueRange value)
    : m_period(period)
    , m_value(std::move(value))
    , m_hasPeriod(period > 0)
{}

bool TimeCheckCondition::test(LootContext& context) const
{
    IWorld& world = context.getWorld();
    i64 dayTime = world.dayTime();

    if (m_hasPeriod && m_period > 0) {
        dayTime = dayTime % m_period;
    }

    i32 timeValue = static_cast<i32>(dayTime);
    f32 floatTime = static_cast<f32>(timeValue);
    return floatTime >= m_value.getMin() && floatTime <= m_value.getMax();
}

std::unique_ptr<LootCondition> TimeCheckCondition::clone() const
{
    return std::make_unique<TimeCheckCondition>(m_period, m_value);
}

// ============================================================================
// DamageSourcePropertiesCondition
// ============================================================================

DamageSourcePropertiesCondition::DamageSourcePropertiesCondition(advancement::DamageSourcePredicate predicate)
    : m_predicate(std::move(predicate))
    , m_isAny(m_predicate.isAny())
{}

bool DamageSourcePropertiesCondition::test(LootContext& context) const
{
    auto* damageSource = context.get<DamageSource>(LootParams::DAMAGE_SOURCE);
    if (!damageSource) {
        return false;
    }

    if (m_isAny) {
        return true;
    }

    return m_predicate.test(*damageSource);
}

std::unique_ptr<LootCondition> DamageSourcePropertiesCondition::clone() const
{
    return std::make_unique<DamageSourcePropertiesCondition>(m_predicate);
}

// ============================================================================
// ReferenceCondition
// ============================================================================

ReferenceCondition::ReferenceCondition(const std::string& name)
    : m_name(name)
{}

bool ReferenceCondition::test(LootContext& context) const
{
    MC_UNUSED(context);
    return true;
}

std::unique_ptr<LootCondition> ReferenceCondition::clone() const
{
    return std::make_unique<ReferenceCondition>(m_name);
}

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

// ============================================================================
// FishingOpenWaterCondition
// ============================================================================

FishingOpenWaterCondition::FishingOpenWaterCondition(bool requireOpenWater)
    : m_requireOpenWater(requireOpenWater)
{}

bool FishingOpenWaterCondition::test(LootContext& context) const
{
    // 从上下文中获取开放水域状态
    bool* openWaterPtr = context.get<bool>(LootParams::IS_IN_OPEN_WATER);
    if (openWaterPtr == nullptr) {
        // 如果没有设置开放水域参数，默认返回 false（非开放水域）
        return !m_requireOpenWater;
    }

    // 检查是否满足开放水域条件
    return m_requireOpenWater == *openWaterPtr;
}

std::unique_ptr<LootCondition> FishingOpenWaterCondition::clone() const
{
    return std::make_unique<FishingOpenWaterCondition>(m_requireOpenWater);
}

} // namespace loot
} // namespace mc
