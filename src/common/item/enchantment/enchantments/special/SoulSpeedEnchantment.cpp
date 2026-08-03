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
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include <cmath>
#include <string>

namespace mc {
namespace item {
namespace enchant {

// 灵魂疾行速度修饰器 ID
static const std::string SOUL_SPEED_MODIFIER_ID = "enchantment.soul_speed";

// 灵魂疾行效率修饰器 ID
static const std::string SOUL_SPEED_EFFICIENCY_MODIFIER_ID = "enchantment.soul_speed.efficiency";

bool SoulSpeedEnchantment::onLocationChanged(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const
{
    (void)stack;

    // 灵魂疾行仅在以下条件下激活：
    // 1. 实体在地面上
    // 2. 实体没有骑乘
    // 3. 实体没有在飞行（创造/旁观模式飞行或鞘翅滑翔）
    // 4. 脚下是灵魂沙/灵魂土
    if (!entity.onGround()) {
        return false;
    }
    if (entity.isRiding()) {
        return false;
    }
    if (isFlying(entity)) {
        return false;
    }
    if (!isOnSoulSpeedBlock(entity)) {
        return false;
    }

    // 如果之前不活跃，添加属性修饰符
    if (!isActive) {
        applySoulSpeedModifiers(entity, level);
    }

    // 尝试消耗耐久（每次位置变化事件有4%概率消耗1点耐久）
    // ChangeItemDamage 效果：随机概率 + 站在灵魂沙/土上
    tryConsumeDurability(entity, slot);

    // 生成灵魂粒子效果和音效
    spawnSoulParticles(entity);

    return true; // 灵魂疾行在灵魂沙上处于活跃状态
}

void SoulSpeedEnchantment::onLocationEffectDeactivated(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const
{
    (void)stack;
    (void)slot;
    (void)level;

    // 移除灵魂疾行属性修饰符（MOVEMENT_SPEED 和 MOVEMENT_EFFICIENCY）
    removeSoulSpeedModifiers(entity);
}

bool SoulSpeedEnchantment::isOnSoulSpeedBlock(LivingEntity& entity) const
{
    IWorld* world = entity.world();
    if (world == nullptr) {
        return false;
    }

    // 检查实体脚下的方块是否是灵魂沙或灵魂土
    // 使用 boundingBox.minY - 0.001f 确定脚下方块位置，与 getBlockSpeedFactor() 和
    // LivingEntity::travel() 中的脚下方块检测逻辑一致
    BlockPos belowPos(static_cast<i32>(std::floor(entity.position().x)),
        static_cast<i32>(std::floor(entity.boundingBox().minY - 0.001f)),
        static_cast<i32>(std::floor(entity.position().z)));

    const BlockState* state = world->getBlockState(belowPos);
    if (state == nullptr) {
        return false;
    }

    // 使用 SOUL_FIRE_BASE_BLOCKS 标签（包含 soul_sand 和 soul_soil）
    return BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(state->getBlock());
}

bool SoulSpeedEnchantment::isFlying(const LivingEntity& entity) const
{
    // 检查鞘翅滑翔（对所有实体类型有效）
    if (entity.isElytraFlying()) {
        return true;
    }

    // 检查创造/旁观模式飞行（仅对玩家有效）
    const Player* player = dynamic_cast<const Player*>(&entity);
    if (player != nullptr && player->abilities().flying) {
        return true;
    }

    return false;
}

void SoulSpeedEnchantment::applySoulSpeedModifiers(LivingEntity& entity, i32 level) const
{
    // MOVEMENT_SPEED 修饰符：使用 Addition 操作
    // LevelBasedValue.perLevel(0.0405F, 0.0105F)
    // Level I: +0.0405, Level II: +0.051, Level III: +0.0615
    f32 speedBonus = getMovementSpeedBonus(level);

    entity::attribute::AttributeModifier speedModifier(
        SOUL_SPEED_MODIFIER_ID, "Soul Speed", static_cast<f64>(speedBonus), entity::attribute::Operation::Addition);

    entity.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, speedModifier);

    // MOVEMENT_EFFICIENCY 修饰符：使用 Addition 操作
    // LevelBasedValue.constant(1.0F)，所有等级均为 +1.0
    // 配合 LivingEntity.getBlockSpeedFactor() 中的插值逻辑：
    //   finalSpeedFactor = lerp(movementEfficiency, blockSpeedFactor, 1.0)
    // 当 movementEfficiency=1.0 时，lerp(1.0, 0.4, 1.0) = 1.0，完全抵消灵魂沙减速
    f32 efficiencyBonus = getMovementEfficiencyBonus();

    entity::attribute::AttributeModifier efficiencyModifier(SOUL_SPEED_EFFICIENCY_MODIFIER_ID,
        "Soul Speed Efficiency",
        static_cast<f64>(efficiencyBonus),
        entity::attribute::Operation::Addition);

    entity.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_EFFICIENCY, efficiencyModifier);
}

void SoulSpeedEnchantment::removeSoulSpeedModifiers(LivingEntity& entity) const
{
    // 移除 MOVEMENT_SPEED 修饰符
    entity.attributes().removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, SOUL_SPEED_MODIFIER_ID);

    // 移除 MOVEMENT_EFFICIENCY 修饰符
    entity.attributes().removeModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, SOUL_SPEED_EFFICIENCY_MODIFIER_ID);
}

void SoulSpeedEnchantment::spawnSoulParticles(LivingEntity& entity) const
{
    IWorld* world = entity.world();
    if (world == nullptr || world->isClientSide()) {
        return;
    }

    // 粒子效果条件：
    // - periodicTick(5)：每5tick触发一次
    // - 非飞行、在地面、在灵魂沙/土上、正在移动
    // 粒子类型: ParticleTypes.SOUL
    // 粒子偏移: SpawnParticlesEffect.inBoundingBox() + offsetFromEntityPosition(0.1F)
    // 粒子速度: movementScaled(-0.2F) + fixedVelocity(ConstantFloat.of(0.1F))
    // 粒子数量: ConstantFloat.of(1.0F) = 1个粒子
    if (entity.ticksExisted() % 5 == 0) {
        // 检查实体是否在水平方向移动
        f32 horizontalSpeed =
            std::sqrt(entity.velocity().x * entity.velocity().x + entity.velocity().z * entity.velocity().z);
        if (horizontalSpeed > 1.0E-5f) {
            world->addParticle(
                particle::ParticleTypeId::Soul, entity.position() + Vector3(0.0, 0.1, 0.0), Vector3(0.0, 0.1, 0.0));
        }
    }

    // 音效条件：35% 随机概率，与粒子条件相同
    // 音效: SoundEvents.PARTICLE_SOUL_ESCAPE
    // 音量: 0.6
    // 音调: 0.6~1.0 随机
    if (entity.getRandom().nextFloat() < 0.35f) {
        f32 pitch = 0.6f + entity.getRandom().nextFloat() * 0.4f;
        entity.playSound(SoundEvents::PARTICLE_SOUL_ESCAPE, 0.6f, pitch);
    }
}

void SoulSpeedEnchantment::tryConsumeDurability(LivingEntity& entity, i32 slot) const
{
    IWorld* world = entity.world();
    if (world == nullptr || world->isClientSide()) {
        return;
    }

    // ChangeItemDamage 效果：
    // - 损坏量: 1点耐久
    // - 概率: 固定4%，与附魔等级无关
    if (entity.getRandom().nextFloat() >= getDurabilityConsumeChance(0)) {
        return;
    }

    // 将槽位索引转换为 EquipmentSlot
    auto equipmentSlot = static_cast<EquipmentSlot>(slot);
    ItemStack& stack = entity.getMutableEquipment(equipmentSlot);
    if (stack.isEmpty() || !stack.isDamageable()) {
        return;
    }

    // 使用 LivingEntity::hurtAndBreak 消耗耐久
    // hurtAndBreak 会处理耐久消耗、Unbreaking 附魔检测和物品损坏回调
    LivingEntity::hurtAndBreak(stack, 1, &entity, equipmentSlot);
}

} // namespace enchant
} // namespace item
} // namespace mc
