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

#include "ToolItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierIds.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/TieredItem.hpp"
#include "common/item/items/tool/ToolType.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/registry/HarvestToolTable.hpp"
#include <string>
#include <unordered_set>
#include <utility>

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
    // 与 SwordItem 同理：注册处的攻击伤害已含材质加成（斧/镐/锹/锄的原版数值各不相同），
    // 这里不再重复叠加 tier.getAttackDamage()。
    , m_attackDamage(attackDamage)
    , m_attackSpeed(attackSpeed)
    , m_efficiency(tier.getEfficiency())
{}

f32 ToolItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    (void)stack;
    // 只返回工具自身的材质速度。效率附魔经 mining_efficiency 属性生效
    // （对齐 vanilla Item.getDestroySpeed：物品侧只读 TOOL 组件的 miningSpeed，
    // 效率加成由 Player.getDestroySpeed 在拿到属性值后叠加）。把加成放在物品侧会让
    // 僵尸持附魔工具、掉落物、发射器等非玩家实体的挖掘速度也跟着变快，且无法叠加多件装备的修饰符。
    if (isEffectiveMaterial(state.getMaterial()) || isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }
    return 1.0f;
}

bool ToolItem::canHarvestBlock(const BlockState& state) const
{
    // 对齐原版 `Tool.Rule`（ToolMaterial.applyToolProperties 生成的两条规则）：
    //   1. deniesDrops(incorrect_for_<tier>_tool) —— 该档位挖不动的方块，返回 false；
    //   2. minesAndDrops(mineable/<tool>, speed)   —— 属于该工具的方块，返回 true。
    // 规则按**方块标签**匹配，而不是「方块属于哪个材质」。两者并不等价：绿宝石矿、
    // 涂蜡铜块、深板岩矿石都只能用镐，但材质各不相同；反过来同材质的方块（如黏土）
    // 又不需要工具。因此这里查数据包展开出的采集表，不再看 Material。
    const block_registry::HarvestToolRuleSet* rules = block_registry::harvestToolRuleSetFor(state.blockLocation());
    const u8 toolTypeValue = static_cast<u8>(m_toolType);

    // 方块不强制要求正确工具：空手都能挖动，任何手持物都可以。
    if (!state.requiresTool()) {
        return true;
    }
    // 要求正确工具，却没有任何 mineable 规则（命令方块、结构方块、拼图方块等）：
    // 原版这类方块在生存中谁挖都不掉落，因此这里返回 false 而不是放行。
    if (rules == nullptr) {
        return false;
    }

    // 方块需要工具，但要求的不是这一种：挖不动。
    const i32 requiredLevel = block_registry::harvestToolRuleLevel(*rules, toolTypeValue);
    if (requiredLevel < 0) {
        return false;
    }

    // 工具种类匹配，再看档位：工具的 harvestLevel 必须不低于方块的最低档位。
    return m_tier.getHarvestLevel() >= requiredLevel;
}

bool ToolItem::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    (void)target;
    // 攻击实体消耗 2 点耐久，若物品损坏则触发 onEquippedItemBroken 回调
    LivingEntity::hurtAndBreak(stack, 2, &attacker, EquipmentSlot::MainHand);
    return true;
}

bool ToolItem::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity)
{
    (void)world;
    (void)pos;
    // 只有硬度 > 0 的方块才消耗耐久，若物品损坏则触发 onEquippedItemBroken 回调
    if (state.hardness() > 0.0f) {
        LivingEntity::hurtAndBreak(stack, 1, &entity, EquipmentSlot::MainHand);
    }
    return true;
}

bool ToolItem::isEffectiveBlock(const Block& block) const
{
    return m_effectiveBlocks.find(&block) != m_effectiveBlocks.end();
}

bool ToolItem::isEffectiveMaterial(const Material& material) const
{
    (void)material;
    // 基类默认不检查材质，由子类重写
    return false;
}

item::ItemAttributeModifiers ToolItem::getAttributeModifiers(i32 equipmentSlot) const
{
    // 工具在主手时提供攻击伤害和攻击速度修饰符
    if (equipmentSlot == static_cast<i32>(EquipmentSlot::MainHand)) {
        item::ItemAttributeModifiers modifiers;
        std::string modifierId = entity::attribute::ids::ATTACK_DAMAGE_MODIFIER;

        // 添加攻击伤害修饰符
        auto attackDamageModifier = entity::attribute::AttributeModifier(
            modifierId, "Tool modifier", static_cast<f64>(m_attackDamage), entity::attribute::Operation::Addition);
        modifiers.add(entity::attribute::Attributes::ATTACK_DAMAGE, attackDamageModifier, equipmentSlot);

        // 添加攻击速度修饰符
        auto attackSpeedModifier = entity::attribute::AttributeModifier(entity::attribute::ids::ATTACK_SPEED_MODIFIER,
            "Tool modifier",
            static_cast<f64>(m_attackSpeed),
            entity::attribute::Operation::Addition);
        modifiers.add(entity::attribute::Attributes::ATTACK_SPEED, attackSpeedModifier, equipmentSlot);

        return modifiers;
    }
    return Item::getAttributeModifiers(equipmentSlot);
}

} // namespace tool
} // namespace item
} // namespace mc
