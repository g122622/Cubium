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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "AxolotlEntity.hpp"

#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/FindWaterGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomSwimmingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/AxolotlGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

AxolotlEntity::AxolotlEntity(EntityId id)
    : WaterMobEntity(id)
{
    // 设置空气值（6000 tick = 5分钟）
    setAir(MAX_AIR_SUPPLY);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> AxolotlEntity::create(IWorld* /*world*/)
{
    return std::make_unique<AxolotlEntity>(0);
}

void AxolotlEntity::randomizeVariant()
{
    math::Random rng = getRandom();
    // 从四种普通变体中随机选择
    i32 variantIndex = rng.nextInt(0, 3);
    m_variant = static_cast<AxolotlVariant>(variantIndex);
}

void AxolotlEntity::setPlayingDead(bool playingDead)
{
    m_playingDead = playingDead;
    if (playingDead) {
        m_playingDeadTimer = PLAY_DEAD_DURATION;
    }
}

bool AxolotlEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 美西螈的食物是热带鱼桶
    const Item* item = itemStack.getItem();
    return item == Items::TROPICAL_FISH_BUCKET;
}

bool AxolotlEntity::canBeSeenAsEnemy() const
{
    // 装死时不能被作为敌人看到
    return !m_playingDead;
}

bool AxolotlEntity::canDespawn(double distanceToClosestPlayer) const
{
    MC_UNUSED(distanceToClosestPlayer);
    // 来自桶或有自定义名称的美西螈不会消失
    return !m_fromBucket && !hasCustomName();
}

bool AxolotlEntity::preventDespawn() const
{
    return WaterMobEntity::preventDespawn() || m_fromBucket;
}

std::optional<ResourceLocation> AxolotlEntity::getAmbientSound() const
{
    if (isInWater()) {
        return SoundEvents::ENTITY_AXOLOTL_IDLE_WATER;
    }
    return SoundEvents::ENTITY_AXOLOTL_IDLE_AIR;
}

std::optional<ResourceLocation> AxolotlEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_AXOLOTL_HURT;
}

std::optional<ResourceLocation> AxolotlEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_AXOLOTL_DEATH;
}

void AxolotlEntity::playAttackSound(LivingEntity& /*target*/)
{
    playSound(SoundEvents::ENTITY_AXOLOTL_ATTACK, 1.0f, 1.0f);
}

void AxolotlEntity::applySupportingEffects(Player& player)
{
    // 给予再生I效果，持续时间 = 基础100tick + 现有剩余（上限2400tick）
    // TODO: 当药水效果系统实现后，添加 Regeneration I 效果
    // i32 currentDuration = player.getEffectDuration(MobEffects::REGENERATION);
    // i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
    // player.addEffect(MobEffectInstance(MobEffects::REGENERATION, newDuration, 0));
    MC_UNUSED(player);
}

void AxolotlEntity::tick()
{
    WaterMobEntity::tick();

    // 更新装死状态
    _updatePlayingDead();

    // 更新狩猎冷却
    _updateHuntingCooldown();
}

void AxolotlEntity::registerGoals()
{
    // 美西螈 AI 目标优先级

    // 优先级 0: 水中浮起和寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::FindWaterGoal>(this));

    // 优先级 1: 装死
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::AxolotlPlayDeadGoal>(this));

    // 优先级 2: 恐慌逃跑
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 3: 跟随食物（热带鱼桶）
    // 注意：BreedGoal 和 FollowParentGoal 需要 AnimalEntity，美西螈继承自 WaterMobEntity
    // 繁殖通过水桶交互机制实现，而非 BreedGoal
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool { return stack.getItem() == Items::TROPICAL_FISH_BUCKET; },
            false));

    // 优先级 4: 近战攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.5, true));

    // 优先级 5: 随机游泳
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomSwimmingGoal>(this, 1.0, 40));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));
    // 优先级 2: 攻击水生敌对生物和鱼类
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::AxolotlTargetGoal>(this));
}

void AxolotlEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();

    // 美西螈属性
    // 最大生命值: 14.0 (7颗心)
    // 移动速度: 1.0
    // 攻击伤害: 2.0 (1颗心)
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

bool AxolotlEntity::hurt(DamageSource& source, f32 amount)
{
    // 调用父类受伤处理
    bool result = WaterMobEntity::hurt(source, amount);

    if (result && isInWater() && !isPlayingDead()) {
        // 在水中受击时有概率触发装死
        // 条件：在水中 + 攻击者存在 + 33%概率 + 伤害不超过当前生命值
        f32 healthAmount = health();
        if (healthAmount > 0.0f && amount < healthAmount) {
            math::Random rng = getRandom();
            if (rng.nextInt(3) == 0) {
                // 检查攻击者是否存在
                const Entity* attacker = source.getEntity();
                if (attacker != nullptr) {
                    setPlayingDead(true);
                }
            }
        }
    }

    return result;
}

void AxolotlEntity::_updatePlayingDead()
{
    if (m_playingDead && m_playingDeadTimer > 0) {
        m_playingDeadTimer--;
        if (m_playingDeadTimer <= 0) {
            m_playingDead = false;
        }
    }
}

void AxolotlEntity::_updateHuntingCooldown()
{
    if (m_huntingCooldown > 0) {
        m_huntingCooldown--;
    }
}

} // namespace mc
