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

#include "BowItem.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/inventory/PlayerInventory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../Items.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"
#include "ArrowItem.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 常量 ==========
namespace {
constexpr i32 MAX_USE_DURATION = 72000; // MC 1.16.5: 几乎无限制
constexpr f32 MIN_VELOCITY = 0.1f;      // 最小发射速度
constexpr i32 FULL_CHARGE_TICKS = 20;   // 满蓄力 tick 数
} // namespace

// ========== 构造函数 ==========

BowItem::BowItem(const ItemProperties& properties)
    : Item(properties)
{}

// ========== Item 接口重写 ==========

i32 BowItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return MAX_USE_DURATION;
}

UseAction BowItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Bow;
}

ItemActionResult BowItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand)
{

    ItemStack bowStack = player.getHeldItem(hand);

    // 检查是否有箭矢或无限附魔
    bool hasInfinity = item::enchant::EnchantmentHelper::getEnchantmentLevel(
                           bowStack, &item::enchant::AllEnchantments::INFINITY_ARROW) > 0;
    bool isCreative = player.isCreative();

    ItemStack ammoStack = findAmmo(player, bowStack);
    bool hasAmmo = !ammoStack.isEmpty();

    // 创造模式或有箭矢或无限附魔时才能使用
    if (isCreative || hasAmmo || hasInfinity) {
        player.setActiveHand(hand);
        return ItemActionResult::success(bowStack);
    }

    return ItemActionResult::fail(bowStack);
}

void BowItem::onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft)
{
    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return;
    }

    // 计算蓄力时间
    i32 chargeTicks = getUseDuration(stack) - timeLeft;
    if (chargeTicks < 0) {
        return;
    }

    // 计算箭矢速度
    f32 velocity = getArrowVelocity(chargeTicks);
    if (velocity < MIN_VELOCITY) {
        // 蓄力不足，不发射
        return;
    }

    // 检查是否有无限附魔
    bool hasInfinity = item::enchant::EnchantmentHelper::getEnchantmentLevel(
                           stack, &item::enchant::AllEnchantments::INFINITY_ARROW) > 0;
    bool isCreative = player->isCreative();

    // 查找箭矢
    ItemStack ammoStack = findAmmo(*player, stack);
    bool hasAmmo = !ammoStack.isEmpty();

    // 无箭矢时，无限附魔使用普通箭
    if (!hasAmmo) {
        if (hasInfinity || isCreative) {
            // 使用普通箭
            ammoStack = ItemStack(Items::ARROW, 1);
        } else {
            return; // 无箭矢可用
        }
    }

    // 检查箭矢是否无限（不消耗）
    bool infiniteArrow = isInfiniteArrow(ammoStack, stack, *player);

    // 创建箭矢实体
    const ArrowItem* arrowItem = dynamic_cast<const ArrowItem*>(ammoStack.getItem());
    if (arrowItem != nullptr) {
        entity::AbstractArrowEntity* arrow = arrowItem->createArrow(world, ammoStack, *player);
        if (arrow != nullptr) {
            // 设置箭矢属性
            arrow = customArrow(arrow);

            // 设置发射参数
            // MC 1.16.5: func_234612_a_ = shoot()
            f32 actualVelocity = velocity * 3.0f; // 最大速度 3.0
            arrow->shootFrom(*player, player->pitch(), player->yaw(), 0.0f, actualVelocity, 1.0f);

            // 满蓄力时暴击
            if (velocity >= 1.0f) {
                arrow->setCritical(true);
            }

            // 力量附魔
            i32 powerLevel =
                item::enchant::EnchantmentHelper::getEnchantmentLevel(stack, &item::enchant::AllEnchantments::POWER);
            if (powerLevel > 0) {
                // 每级增加 0.5 伤害 + 0.5 基础
                f32 bonusDamage = static_cast<f32>(powerLevel) * 0.5f + 0.5f;
                arrow->setDamage(arrow->damage() + bonusDamage);
            }

            // 冲击附魔
            i32 punchLevel =
                item::enchant::EnchantmentHelper::getEnchantmentLevel(stack, &item::enchant::AllEnchantments::PUNCH);
            if (punchLevel > 0) {
                arrow->setKnockbackStrength(punchLevel);
            }

            // 火矢附魔
            if (item::enchant::EnchantmentHelper::getEnchantmentLevel(stack, &item::enchant::AllEnchantments::FLAME) >
                0) {
                arrow->setFire(100); // 5秒燃烧
            }

            // 消耗耐久度（非创造模式）
            if (!isCreative) {
                stack.attemptDamageItem(1);
            }

            // 设置拾取状态
            if (infiniteArrow || isCreative) {
                arrow->setPickupStatus(entity::PickupStatus::CreativeOnly);
            } else {
                arrow->setPickupStatus(entity::PickupStatus::Allowed);
            }

            // 生成箭矢实体
            world.spawnEntity(std::unique_ptr<Entity>(arrow));

            // 播放音效
            // MC 1.16.5: ENTITY_ARROW_SHOOT 音效，音调基于蓄力
            math::Random rng;
            f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 1.2f) + velocity * 0.5f;
            player->playSound(SoundEvents::ENTITY_ARROW_SHOOT, 1.0f, pitch);
        }
    }

    // 消耗箭矢（非无限、非创造）
    if (!infiniteArrow && !isCreative && !ammoStack.isEmpty()) {
        ammoStack.shrink(1);
        // 如果箭矢堆为空，从背包删除
        // player->inventory().deleteStack(ammoStack);
    }
}

// ========== 弓特有方法 ==========

std::function<bool(const ItemStack&)> BowItem::getAmmoPredicate() const
{
    return [](const ItemStack& stack) -> bool {
        if (stack.isEmpty()) {
            return false;
        }
        // 检查是否为箭矢物品
        return dynamic_cast<const ArrowItem*>(stack.getItem()) != nullptr;
    };
}

std::function<bool(const ItemStack&)> BowItem::getInventoryAmmoPredicate() const
{
    return getAmmoPredicate();
}

f32 BowItem::getArrowVelocity(i32 chargeTicks)
{
    // MC 1.16.5 BowItem.getArrowVelocity()
    // f = charge / 20.0
    // f = (f * f + f * 2.0) / 3.0
    // 最大 1.0
    if (chargeTicks <= 0) {
        return 0.0f;
    }

    f32 f = static_cast<f32>(chargeTicks) / static_cast<f32>(FULL_CHARGE_TICKS);
    f = (f * f + f * 2.0f) / 3.0f;

    return math::clamp(f, 0.0f, 1.0f);
}

entity::AbstractArrowEntity* BowItem::customArrow(entity::AbstractArrowEntity* arrow)
{
    return arrow;
}

// ========== 私有方法 ==========

ItemStack BowItem::findAmmo(Player& player, const ItemStack& /*bowStack*/) const
{

    // 检查副手
    ItemStack offhand = player.getHeldItem(Hand::OffHand);
    if (getAmmoPredicate()(offhand)) {
        return offhand;
    }

    // 检查主手（弓可能在副手）
    ItemStack mainhand = player.getHeldItem(Hand::MainHand);
    if (getAmmoPredicate()(mainhand)) {
        return mainhand;
    }

    // 检查背包
    PlayerInventory& inventory = player.inventory();
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack slot = inventory.getItem(i);
        if (getInventoryAmmoPredicate()(slot)) {
            return slot;
        }
    }

    return ItemStack();
}

bool BowItem::isInfiniteArrow(const ItemStack& arrowStack, const ItemStack& bowStack, Player& player) const
{
    // 创造模式总是无限
    if (player.isCreative()) {
        return true;
    }

    // 检查无限附魔
    i32 infinityLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        bowStack, &item::enchant::AllEnchantments::INFINITY_ARROW);

    if (infinityLevel <= 0) {
        return false;
    }

    // MC 1.16.5: 只有普通箭矢受益于无限附魔
    // 光灵箭和药水箭不受无限影响
    const ArrowItem* arrowItem = dynamic_cast<const ArrowItem*>(arrowStack.getItem());
    if (arrowItem != nullptr) {
        return arrowItem->isInfinite(arrowStack, bowStack, player);
    }

    return false;
}

} // namespace item
} // namespace mc
