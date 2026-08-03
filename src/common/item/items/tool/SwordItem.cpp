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

#include "common/item/items/tool/SwordItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/TieredItem.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <string>

namespace mc {
namespace item {
namespace tool {

SwordItem::SwordItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : TieredItem(tier, properties)
    , m_attackDamage(static_cast<f32>(attackDamage) + tier.getAttackDamage())
    , m_attackSpeed(attackSpeed)
{}

f32 SwordItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    (void)stack;

    // 剑对蜘蛛网有极高的挖掘效率
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return 15.0f;
    }

    // 剑对植物、珊瑚、葫芦材质有轻微效率
    const Material& mat = state.getMaterial();
    if (mat == Material::PLANT || mat == Material::REPLACEABLE_PLANT || mat == Material::TALL_PLANTS ||
        mat == Material::LEAVES || mat == Material::CORAL || mat == Material::GOURD) {
        return 1.5f;
    }

    // 其他方块返回基础速度
    return 1.0f;
}

bool SwordItem::canHarvestBlock(const BlockState& state) const
{
    // 剑只能采集蜘蛛网
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return true;
    }

    // 其他方块不能采集（除非不需要工具）
    return state.getHarvestTool() == TOOL_TYPE_NONE;
}

bool SwordItem::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    (void)target;
    // 剑攻击实体只消耗 1 点耐久（其他工具消耗 2 点），若物品损坏则触发 onEquippedItemBroken 回调
    LivingEntity::hurtAndBreak(stack, 1, &attacker, EquipmentSlot::MainHand);
    return true;
}

bool SwordItem::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity)
{
    (void)world;
    (void)pos;
    // 剑破坏方块消耗 2 点耐久（其他工具消耗 1 点），若物品损坏则触发 onEquippedItemBroken 回调
    if (state.hardness() > 0.0f) {
        LivingEntity::hurtAndBreak(stack, 2, &entity, EquipmentSlot::MainHand);
    }
    return true;
}

item::ItemAttributeModifiers SwordItem::getAttributeModifiers(i32 equipmentSlot) const
{
    // 剑在主手时提供攻击伤害和攻击速度修饰符
    if (equipmentSlot == static_cast<i32>(EquipmentSlot::MainHand)) {
        item::ItemAttributeModifiers modifiers;
        std::string uuid = entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_DAMAGE_MODIFIER_UUID);

        // 添加攻击伤害修饰符
        auto attackDamageModifier = entity::attribute::AttributeModifier(
            uuid, "Weapon modifier", static_cast<f64>(m_attackDamage), entity::attribute::Operation::Addition);
        modifiers.add(entity::attribute::Attributes::ATTACK_DAMAGE, attackDamageModifier, equipmentSlot);

        // 添加攻击速度修饰符
        auto attackSpeedModifier = entity::attribute::AttributeModifier(
            entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_SPEED_MODIFIER_UUID),
            "Weapon modifier",
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
