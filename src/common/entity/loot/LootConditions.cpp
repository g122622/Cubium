#include "LootConditions.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>

namespace mc {
namespace loot {

// ============================================================================
// SilkTouchCondition
// ============================================================================

bool SilkTouchCondition::test(LootContext& context) const {
    // 检查是否有精准采集附魔
    // 使用 SILK_TOUCH_LEVEL 参数
    auto* silkTouchLevel = context.get<i32>(LootParams::SILK_TOUCH_LEVEL);
    if (silkTouchLevel && *silkTouchLevel > 0) {
        return true;
    }
    return false;
}

std::unique_ptr<LootCondition> SilkTouchCondition::clone() const {
    return std::make_unique<SilkTouchCondition>();
}

// ============================================================================
// FortuneCondition
// ============================================================================

FortuneCondition::FortuneCondition(i32 minLevel)
    : m_minLevel(minLevel)
{
}

bool FortuneCondition::test(LootContext& context) const {
    i32 fortuneLevel = getFortuneLevel(context);
    return fortuneLevel >= m_minLevel;
}

std::unique_ptr<LootCondition> FortuneCondition::clone() const {
    return std::make_unique<FortuneCondition>(m_minLevel);
}

i32 FortuneCondition::getFortuneLevel(LootContext& context) {
    // 从上下文获取时运附魔等级
    auto* fortuneLevel = context.get<i32>(LootParams::FORTUNE_LEVEL);
    if (fortuneLevel && *fortuneLevel > 0) {
        return *fortuneLevel;
    }
    return 0;
}

i32 FortuneCondition::applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random) {
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
{
}

bool RandomChanceCondition::test(LootContext& context) const {
    f32 actualChance = m_chance;

    if (m_affectedByLuck) {
        // 幸运值增加概率
        actualChance += context.getLuck();
    }

    return context.getRandom().nextFloat() < actualChance;
}

std::unique_ptr<LootCondition> RandomChanceCondition::clone() const {
    return std::make_unique<RandomChanceCondition>(m_chance, m_affectedByLuck);
}

// ============================================================================
// RandomChanceWithLuckCondition
// ============================================================================

RandomChanceWithLuckCondition::RandomChanceWithLuckCondition(f32 baseChance, f32 luckCoefficient)
    : m_baseChance(baseChance)
    , m_luckCoefficient(luckCoefficient)
{
}

bool RandomChanceWithLuckCondition::test(LootContext& context) const {
    f32 chance = m_baseChance + context.getLuck() * m_luckCoefficient;
    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> RandomChanceWithLuckCondition::clone() const {
    return std::make_unique<RandomChanceWithLuckCondition>(m_baseChance, m_luckCoefficient);
}

// ============================================================================
// NotCondition
// ============================================================================

NotCondition::NotCondition(std::unique_ptr<LootCondition> condition)
    : m_condition(std::move(condition))
{
}

bool NotCondition::test(LootContext& context) const {
    if (!m_condition) {
        return true;
    }
    return !m_condition->test(context);
}

std::unique_ptr<LootCondition> NotCondition::clone() const {
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
{
}

bool AndCondition::test(LootContext& context) const {
    return std::all_of(m_conditions.begin(), m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) {
            return cond && cond->test(context);
        });
}

std::unique_ptr<LootCondition> AndCondition::clone() const {
    std::vector<std::unique_ptr<LootCondition>> cloned;
    for (const auto& cond : m_conditions) {
        if (cond) {
            cloned.push_back(cond->clone());
        }
    }
    return std::make_unique<AndCondition>(std::move(cloned));
}

void AndCondition::addCondition(std::unique_ptr<LootCondition> condition) {
    m_conditions.push_back(std::move(condition));
}

// ============================================================================
// OrCondition
// ============================================================================

OrCondition::OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions)
    : m_conditions(std::move(conditions))
{
}

bool OrCondition::test(LootContext& context) const {
    return std::any_of(m_conditions.begin(), m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) {
            return cond && cond->test(context);
        });
}

std::unique_ptr<LootCondition> OrCondition::clone() const {
    std::vector<std::unique_ptr<LootCondition>> cloned;
    for (const auto& cond : m_conditions) {
        if (cond) {
            cloned.push_back(cond->clone());
        }
    }
    return std::make_unique<OrCondition>(std::move(cloned));
}

void OrCondition::addCondition(std::unique_ptr<LootCondition> condition) {
    m_conditions.push_back(std::move(condition));
}

// ============================================================================
// BlockStateCondition
// ============================================================================

BlockStateCondition::BlockStateCondition(const std::string& blockId)
    : m_blockId(blockId)
    , m_properties()
{
}

BlockStateCondition::BlockStateCondition(const std::string& blockId, StatePropertiesPredicate properties)
    : m_blockId(blockId)
    , m_properties(std::move(properties))
{
}

bool BlockStateCondition::test(LootContext& context) const {
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

std::unique_ptr<LootCondition> BlockStateCondition::clone() const {
    return std::make_unique<BlockStateCondition>(m_blockId, m_properties);
}

// ============================================================================
// ToolTypeCondition
// ============================================================================

ToolTypeCondition::ToolTypeCondition(u8 toolType)
    : m_toolType(toolType)
{
}

bool ToolTypeCondition::test(LootContext& context) const {
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

std::unique_ptr<LootCondition> ToolTypeCondition::clone() const {
    return std::make_unique<ToolTypeCondition>(m_toolType);
}

// ============================================================================
// LootConditionBuilder
// ============================================================================

std::unique_ptr<LootCondition> LootConditionBuilder::silkTouch() {
    return std::make_unique<SilkTouchCondition>();
}

std::unique_ptr<LootCondition> LootConditionBuilder::fortune(i32 minLevel) {
    return std::make_unique<FortuneCondition>(minLevel);
}

std::unique_ptr<LootCondition> LootConditionBuilder::randomChance(f32 chance) {
    return std::make_unique<RandomChanceCondition>(chance);
}

std::unique_ptr<LootCondition> LootConditionBuilder::randomChanceWithLuck(f32 baseChance, f32 luckCoefficient) {
    return std::make_unique<RandomChanceWithLuckCondition>(baseChance, luckCoefficient);
}

std::unique_ptr<LootCondition> LootConditionBuilder::not_(std::unique_ptr<LootCondition> condition) {
    return std::make_unique<NotCondition>(std::move(condition));
}

std::unique_ptr<LootCondition> LootConditionBuilder::and_(std::vector<std::unique_ptr<LootCondition>> conditions) {
    return std::make_unique<AndCondition>(std::move(conditions));
}

std::unique_ptr<LootCondition> LootConditionBuilder::or_(std::vector<std::unique_ptr<LootCondition>> conditions) {
    return std::make_unique<OrCondition>(std::move(conditions));
}

std::unique_ptr<LootCondition> LootConditionBuilder::blockState(const std::string& blockId) {
    return std::make_unique<BlockStateCondition>(blockId);
}

std::unique_ptr<LootCondition> LootConditionBuilder::blockState(
    const std::string& blockId,
    StatePropertiesPredicate properties) {
    return std::make_unique<BlockStateCondition>(blockId, std::move(properties));
}

std::unique_ptr<LootCondition> LootConditionBuilder::toolType(u8 toolType) {
    return std::make_unique<ToolTypeCondition>(toolType);
}

} // namespace loot
} // namespace mc
