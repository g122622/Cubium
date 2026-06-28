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

#include "SoulSpeedEnchantment.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc {
namespace item {
namespace enchant {

// 灵魂疾行速度修饰器 ID
static const std::string SOUL_SPEED_MODIFIER_ID = "enchantment.soul_speed";

bool SoulSpeedEnchantment::onLocationChanged(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const
{
    (void)stack;
    (void)slot;

    // 灵魂疾行仅在以下条件下激活：
    // 1. 实体在地面上
    // 2. 实体没有骑乘
    // 3. 实体没有在飞行
    // 4. 脚下是灵魂沙/灵魂土
    if (!entity.onGround()) {
        return false;
    }
    if (entity.isRiding()) {
        return false;
    }

    // 检查脚下是否是灵魂沙/灵魂土
    if (!isOnSoulSpeedBlock(entity)) {
        return false;
    }

    // 检查实体是否在飞行（适用于创造模式玩家）
    // TODO: 当 Player 飞行状态检查完善后，添加飞行检测

    // 如果之前不活跃，添加速度修饰符
    if (!isActive) {
        applySoulSpeedModifier(entity, level);
    }

    // 生成灵魂粒子效果
    if (entity.world() != nullptr && !entity.world()->isClientSide()) {
        // 使用简单的概率检查，MC Java 中每 tick 有 35% 概率生成粒子
        // 这里使用 ticksExisted 做伪随机
        if ((entity.ticksExisted() * 31 + 17) % 100 < 35) {
            entity.world()->addParticle(
                particle::ParticleTypeId::Soul, entity.position() + Vector3(0.0, 0.1, 0.0), Vector3(0.0, 0.02, 0.0));
        }
    }

    return true; // 灵魂疾行在灵魂沙上处于活跃状态
}

void SoulSpeedEnchantment::onLocationEffectDeactivated(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const
{
    (void)stack;
    (void)slot;
    (void)level;

    // 移除灵魂疾行速度修饰符
    removeSoulSpeedModifier(entity);
}

bool SoulSpeedEnchantment::isOnSoulSpeedBlock(LivingEntity& entity) const
{
    IWorld* world = entity.world();
    if (world == nullptr) {
        return false;
    }

    // 检查实体脚下的方块是否是灵魂沙或灵魂土
    BlockPos belowPos(static_cast<i32>(std::floor(entity.position().x)),
        static_cast<i32>(std::floor(entity.position().y)) - 1,
        static_cast<i32>(std::floor(entity.position().z)));

    const BlockState* state = world->getBlockState(belowPos);
    if (state == nullptr) {
        return false;
    }

    return BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(state->getBlock());
}

void SoulSpeedEnchantment::applySoulSpeedModifier(LivingEntity& entity, i32 level) const
{
    // 添加速度修饰符
    // 当前实现使用 MultiplyTotal 操作：I: +40%, II: +60%, III: +80%
    // TODO: MC 1.21.11 改为 AddValue 操作，值为 perLevel(0.0405, 0.0105)
    //       即 I: +0.0405, II: +0.0510, III: +0.0615（直接加到基础移动速度上）
    //       当前 MultiplyTotal 实现与 MC 1.16.5 风格一致，待属性系统完善后对齐
    f32 multiplier = getSoulSpeedMultiplier(level);
    f32 modifierAmount = multiplier - 1.0f; // 转换为属性修饰符值

    entity::attribute::AttributeModifier modifier(SOUL_SPEED_MODIFIER_ID,
        "Soul Speed",
        static_cast<f64>(modifierAmount),
        entity::attribute::Operation::MultiplyTotal);

    entity.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, modifier);
}

void SoulSpeedEnchantment::removeSoulSpeedModifier(LivingEntity& entity) const
{
    // 移除速度修饰符
    entity.attributes().removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, SOUL_SPEED_MODIFIER_ID);
}

} // namespace enchant
} // namespace item
} // namespace mc
