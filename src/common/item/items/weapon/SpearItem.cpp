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

#include "common/item/items/weapon/SpearItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/SpearEntity.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/tool/TieredItem.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace item {

// ========== 构造函数 ==========

SpearItem::SpearItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : tool::TieredItem(tier, properties)
    , m_attackDamage(static_cast<f32>(attackDamage) + tier.getAttackDamage())
    , m_attackSpeed(attackSpeed)
{}

// ========== Item 接口重写 ==========

i32 SpearItem::getUseDuration(const ItemStack& stack) const
{
    (void)stack;
    return MAX_USE_DURATION;
}

UseAction SpearItem::getUseAction(const ItemStack& stack) const
{
    (void)stack;
    return UseAction::Spear;
}

ItemActionResult SpearItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    (void)world;

    ItemStack spearStack = player.getHeldItem(hand);

    // 检查耐久度：耐久即将耗尽时无法投掷
    if (spearStack.getDamage() >= spearStack.getMaxDamage() - 1) {
        return ItemActionResult::fail(spearStack);
    }

    player.setActiveHand(hand);
    return ItemActionResult::success(spearStack);
}

void SpearItem::onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft)
{
    // 仅玩家可以投掷长矛
    Player* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return;
    }

    // 计算蓄力时间
    i32 chargeTicks = MAX_USE_DURATION - timeLeft;
    if (chargeTicks < MIN_CHARGE_TICKS) {
        // 蓄力不足，不投掷
        return;
    }

    // 消耗耐久度（非创造模式），若物品损坏则触发 onEquippedItemBroken 回调
    if (!player->isCreative()) {
        LivingEntity::hurtAndBreak(stack, 1, player, EquipmentSlot::MainHand);
    }

    // 创建长矛投掷实体
    auto spearEntity = std::make_unique<entity::SpearEntity>(EntityInstanceId(0));
    spearEntity->setTypeId(entity::EntityTypeKeys::SPEAR);
    spearEntity->setWorld(&world);
    spearEntity->setPosition(player->x(), player->y() + player->eyeHeight() - 0.1f, player->z());
    spearEntity->setShooter(player);

    // 设置发射参数（长矛投掷速度与三叉戟一致）
    spearEntity->shootFrom(*player, player->pitch(), player->yaw(), 0.0f, THROW_VELOCITY, 1.0f);

    // 设置长矛物品堆（用于拾取时还原物品）
    spearEntity->setItemStack(stack);

    // 创造模式下设置拾取状态为仅创造模式
    if (player->isCreative()) {
        spearEntity->setPickupStatus(entity::PickupStatus::CreativeOnly);
    }

    // 生成实体
    world.spawnEntity(std::move(spearEntity));

    // 播放投掷音效
    player->playSound(SoundEvents::ITEM_SPEAR_THROW, 1.0f, 1.0f);

    // 非创造模式从背包移除长矛
    if (!player->isCreative()) {
        stack.shrink(1);
    }
}

bool SpearItem::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    (void)target;
    // 长矛近战攻击消耗 1 点耐久（与剑一致），若物品损坏则触发 onEquippedItemBroken 回调
    LivingEntity::hurtAndBreak(stack, 1, &attacker, EquipmentSlot::MainHand);
    return true;
}

bool SpearItem::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& breaker)
{
    (void)world;
    (void)pos;
    // 长矛破坏方块消耗 2 点耐久（与剑一致），若物品损坏则触发 onEquippedItemBroken 回调
    if (state.hardness() > 0.0f) {
        LivingEntity::hurtAndBreak(stack, 2, &breaker, EquipmentSlot::MainHand);
    }
    return true;
}

item::ItemAttributeModifiers SpearItem::getAttributeModifiers(i32 equipmentSlot) const
{
    // 长矛在主手时提供攻击伤害和攻击速度修饰符
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

} // namespace item
} // namespace mc
