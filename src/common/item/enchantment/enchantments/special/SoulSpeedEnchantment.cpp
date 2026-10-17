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

// 灵魂疾行属性修饰符 ID。
// 原版 soul_speed.json 中 MOVEMENT_SPEED 与 MOVEMENT_EFFICIENCY 两个属性效果共用同一个 id
// （Enchantments.SOUL_SPEED 注册处为 Identifier.withDefaultNamespace("enchantment.soul_speed")），
// 修饰符 id 仅在所属属性实例内要求唯一，故两个属性复用同一 id 不冲突。
static const std::string SOUL_SPEED_MODIFIER_ID = "minecraft:enchantment.soul_speed";

bool SoulSpeedEnchantment::onLocationChanged(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const
{
    (void)stack;

    // 灵魂疾行的 location_changed 条件（vanilla soul_speed.json）：
    //   !riding AND is_flying==false AND
    //     ( (active==true  AND (movement_affected_by ∈ #soul_speed_blocks OR is_on_ground==false))
    //       OR
    //       (active==false AND movement_affected_by ∈ #soul_speed_blocks) )
    // 即：已激活时允许离地（起跳后仍保留修饰符，直到离开灵魂沙上方或开始飞行）；
    //     未激活时必须站在灵魂沙/灵魂土上。
    if (entity.isRiding()) {
        return false;
    }
    if (isFlying(entity)) {
        return false;
    }
    const bool onSoulBlock = isOnSoulSpeedBlock(entity);
    const bool activeCondition = isActive ? (onSoulBlock || !entity.onGround()) : onSoulBlock;
    if (!activeCondition) {
        return false;
    }

    // 如果之前不活跃，添加属性修饰符
    if (!isActive) {
        applySoulSpeedModifiers(entity, level);
    }

    // 耐久消耗（vanilla soul_speed.json 的 change_item_damage 要求 is_on_ground==true +
    // movement_affected_by ∈ #soul_speed_blocks + 4%/级 随机概率），故仅在站在灵魂方块上时消耗。
    if (entity.onGround() && onSoulBlock) {
        tryConsumeDurability(entity, slot);
    }

    // 生成灵魂粒子效果和音效
    spawnSoulParticles(entity);

    return true; // 灵魂疾行处于活跃状态
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

    // 使用 SOUL_SPEED_BLOCKS 标签（soul_sand + soul_soil），对齐 vanilla soul_speed.json 的
    // movement_affected_by 引用 #minecraft:soul_speed_blocks。
    return BlockTags::SOUL_SPEED_BLOCKS().contains(state->getBlock());
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

    // MOVEMENT_EFFICIENCY 修饰符：使用 Addition 操作，与速度修饰符共用同一 id
    // LevelBasedValue.constant(1.0F)，所有等级均为 +1.0
    // 配合 LivingEntity.getBlockSpeedFactor() 中的插值逻辑：
    //   finalSpeedFactor = lerp(movementEfficiency, blockSpeedFactor, 1.0)
    // 当 movementEfficiency=1.0 时，lerp(1.0, 0.4, 1.0) = 1.0，完全抵消灵魂沙减速
    f32 efficiencyBonus = getMovementEfficiencyBonus();

    entity::attribute::AttributeModifier efficiencyModifier(SOUL_SPEED_MODIFIER_ID,
        "Soul Speed Efficiency",
        static_cast<f64>(efficiencyBonus),
        entity::attribute::Operation::Addition);

    entity.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_EFFICIENCY, efficiencyModifier);
}

void SoulSpeedEnchantment::removeSoulSpeedModifiers(LivingEntity& entity) const
{
    // 移除 MOVEMENT_SPEED 修饰符
    entity.attributes().removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, SOUL_SPEED_MODIFIER_ID);

    // 移除 MOVEMENT_EFFICIENCY 修饰符（与速度修饰符同 id，按属性区分）
    entity.attributes().removeModifier(entity::attribute::Attributes::MOVEMENT_EFFICIENCY, SOUL_SPEED_MODIFIER_ID);
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
    // TODO: 原版把粒子与音效放在 EnchantmentEffectComponents.TICK 组件中，由实体每 tick 的
    //       tickEffects 派发；本项目尚无该派发点，只能在位置变化事件（含每 20 tick 的周期性重评估）
    //       里触发，故原地站立时粒子/音效频率低于原版。待补齐附魔 tick 派发后应迁移到 tick 路径。
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
