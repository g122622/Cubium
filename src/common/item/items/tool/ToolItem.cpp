#include "ToolItem.hpp"
#include "../../../world/block/Block.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item {
namespace tool {

ToolItem::ToolItem(f32 attackDamage,
                   f32 attackSpeed,
                   const tier::IItemTier& tier,
                   std::unordered_set<const Block*> effectiveBlocks,
                   ToolType toolType,
                   ItemProperties properties)
    : TieredItem(tier, properties)
    , m_effectiveBlocks(std::move(effectiveBlocks))
    , m_toolType(toolType)
    , m_attackDamage(attackDamage + tier.getAttackDamage())
    , m_attackSpeed(attackSpeed)
    , m_efficiency(tier.getEfficiency()) {
}

f32 ToolItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 1. 检查材质有效性
    if (isEffectiveMaterial(state.getMaterial())) {
        return m_efficiency;
    }

    // 2. 检查特定方块有效性
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }

    // 3. 基础速度
    return 1.0f;
}

bool ToolItem::canHarvestBlock(const BlockState& state) const {
    // 获取工具类型的u8值
    u8 toolTypeValue = static_cast<u8>(m_toolType);

    // 检查工具类型和挖掘等级
    if (state.getHarvestTool() == toolTypeValue) {
        return m_tier.getHarvestLevel() >= state.getHarvestLevel();
    }

    // 如果方块不需要工具，总是可以采集
    if (state.getHarvestTool() == TOOL_TYPE_NONE) {
        return true;
    }

    // 如果方块需要工具但我们没有匹配的类型
    // 默认情况下不能采集（但子类如镐可能有特殊逻辑）
    return false;
}

bool ToolItem::hitEntity(ItemStack& stack,
                         LivingEntity& target,
                         LivingEntity& attacker) {
    (void)target;
    (void)attacker;
    // 攻击实体消耗 2 点耐久
    stack.attemptDamageItem(2);
    return true;
}

bool ToolItem::onBlockDestroyed(ItemStack& stack,
                                IWorld& world,
                                const BlockState& state,
                                const BlockPos& pos,
                                LivingEntity& entity) {
    (void)world;
    (void)pos;
    (void)entity;
    // 只有硬度 > 0 的方块才消耗耐久
    if (state.hardness() > 0.0f) {
        stack.attemptDamageItem(1);
    }
    return true;
}

bool ToolItem::isEffectiveBlock(const Block& block) const {
    return m_effectiveBlocks.find(&block) != m_effectiveBlocks.end();
}

bool ToolItem::isEffectiveMaterial(const Material& material) const {
    (void)material;
    // 基类默认不检查材质，由子类重写
    return false;
}

} // namespace tool
} // namespace item
} // namespace mc
