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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MaceItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/mace/DensityEnchantment.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include <cmath>

namespace mc {
namespace item {

MaceItem::MaceItem(const ItemProperties& properties)
    : Item(properties)
{}

bool MaceItem::canSmashAttack(const LivingEntity& entity)
{
    // 下落距离 > 1.5 且不在鞘翅滑翔状态时触发砸地攻击
    return entity.fallDistance() > SMASH_ATTACK_FALL_THRESHOLD && !entity.isElytraFlying();
}

// calculateSmashAttackDamage 已内联到 MaceMath.hpp，MaceItem 通过 inline 转发调用

f32 MaceItem::getSmashAttackDamageBonus(const LivingEntity& attacker, f32 fallDistance, const ItemStack& weapon)
{
    if (!canSmashAttack(attacker)) {
        return 0.0f;
    }

    // 基础伤害加成
    f32 baseDamage = calculateSmashAttackDamage(fallDistance);

    // 致密魔咒加成：每级 +0.5 * fallDistance
    i32 densityLevel =
        enchant::EnchantmentHelper::getEnchantmentLevel(weapon, enchant::DensityEnchantment::ENCHANTMENT_ID);
    if (densityLevel > 0) {
        f32 densityBonus = enchant::DensityEnchantment::getDamagePerFallenBlock(densityLevel);
        baseDamage += densityBonus * fallDistance;
    }

    return baseDamage;
}

bool MaceItem::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    // 消耗耐久
    LivingEntity::hurtAndBreak(stack, 1, &attacker, EquipmentSlot::MainHand);

    if (canSmashAttack(attacker)) {
        // 停止攻击者的下落（将Y轴速度设为极小值）
        attacker.setVelocity(attacker.velocity().x, 0.01, attacker.velocity().z);

        // 设置冲量冲击位置和坠落伤害免疫
        // 对应 MC MaceItem.hurtEnemy 中设置 currentImpulseImpactPos 和
        // setIgnoreFallDamageFromCurrentImpulse(true) 的逻辑
        auto* player = dynamic_cast<Player*>(&attacker);
        if (player != nullptr) {
            player->setCurrentImpulseImpactPos(player->calculateMaceImpactPosition());
            player->setIgnoreFallDamageFromCurrentImpulse(true);
        }

        // 播放音效
        auto* world = attacker.world();
        if (world != nullptr) {
            f32 x = static_cast<f32>(attacker.position().x);
            f32 y = static_cast<f32>(attacker.position().y);
            f32 z = static_cast<f32>(attacker.position().z);

            if (target.onGround()) {
                // 目标在地面：播放地面砸击音效
                const auto& soundEvent = attacker.fallDistance() > SMASH_ATTACK_HEAVY_THRESHOLD
                    ? SoundEvents::ITEM_MACE_SMASH_GROUND_HEAVY
                    : SoundEvents::ITEM_MACE_SMASH_GROUND;
                world->playSound(soundEvent, sound::SoundCategory::Players, {x, y, z}, 1.0f, 1.0f);
            } else {
                // 目标在空中：播放空中砸击音效
                world->playSound(
                    SoundEvents::ITEM_MACE_SMASH_AIR, sound::SoundCategory::Players, {x, y, z}, 1.0f, 1.0f);
            }

            // 对目标周围实体施加击退
            applySmashAttackKnockback(*world, attacker, target);
        }
    }

    return true;
}

void MaceItem::postHitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    (void)stack;
    (void)target;

    if (canSmashAttack(attacker)) {
        // 重置攻击者的下落距离
        // 对应 MC MaceItem.postHurtEnemy 中的 resetFallDistance()
        attacker.setFallDistance(0.0f);
    }
}

void MaceItem::applySmashAttackKnockback(IWorld& world, Entity& attacker, Entity& target)
{
    // 广播砸地攻击粒子事件（对应 MC levelEvent(2013, target.getOnPos(), 750)）
    world.playEvent(world::WorldEvents::SMASH_ATTACK, target.onPos(), 750);

    // 查找目标周围3.5格内的所有生物实体
    auto targetPos = target.position();
    auto targetBB = target.boundingBox();
    auto searchBB = targetBB.grow(SMASH_ATTACK_KNOCKBACK_RADIUS);

    auto entities = world.getEntitiesInAABB(searchBB);
    for (auto* entity : entities) {
        auto* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr) {
            continue;
        }

        // 排除条件：
        // 1. 旁观者模式的玩家
        // 2. 攻击者自身或目标自身
        // 3. 与攻击者同盟的实体
        // 4. 超出击退范围的实体
        auto* player = dynamic_cast<Player*>(living);
        if (player != nullptr && player->isSpectator()) {
            continue;
        }
        // 5. 创造模式飞行的玩家
        if (player != nullptr && player->isCreative() && player->abilities().flying) {
            continue;
        }
        if (living == &attacker || living == &target) {
            continue;
        }
        if (attacker.isAlliedTo(*living)) {
            continue;
        }

        // 检查距离是否在范围内
        auto livingPos = living->position();
        f64 distanceSq = targetPos.distanceSquared(livingPos);
        f64 radiusSq =
            static_cast<f64>(SMASH_ATTACK_KNOCKBACK_RADIUS) * static_cast<f64>(SMASH_ATTACK_KNOCKBACK_RADIUS);
        if (distanceSq > radiusSq) {
            continue;
        }

        // 计算击退方向（从目标指向被击退实体）
        f64 dx = livingPos.x - targetPos.x;
        f64 dz = livingPos.z - targetPos.z;
        f64 dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 0.001) {
            continue;
        }
        dx /= dist;
        dz /= dist;

        // 计算击退力度
        // 力度 = (3.5 - 距离) * 0.7 * (重击? 2 : 1) * (1 - 击退抗性)
        f32 distance = static_cast<f32>(std::sqrt(distanceSq));
        f64 knockbackPower =
            (SMASH_ATTACK_KNOCKBACK_RADIUS - distance) * static_cast<f64>(SMASH_ATTACK_KNOCKBACK_POWER);

        // 重击（下落距离>5格）时击退翻倍
        auto* attackerLiving = dynamic_cast<LivingEntity*>(&attacker);
        if (attackerLiving != nullptr && attackerLiving->fallDistance() > SMASH_ATTACK_HEAVY_THRESHOLD) {
            knockbackPower *= 2.0;
        }

        // 击退抗性削减
        f32 knockbackResistance = 0.0f;
        if (attackerLiving != nullptr) {
            knockbackResistance =
                static_cast<f32>(living->attributes().getValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE));
        }
        knockbackPower *= (1.0 - static_cast<f64>(knockbackResistance));

        if (knockbackPower <= 0.0) {
            continue;
        }

        // 应用击退：水平方向 + Y方向 0.7
        living->addVelocity(static_cast<f32>(dx * knockbackPower), 0.7f, static_cast<f32>(dz * knockbackPower));
    }
}

} // namespace item
} // namespace mc
