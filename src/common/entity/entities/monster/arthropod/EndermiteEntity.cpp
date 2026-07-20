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

#include "EndermiteEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/SilverfishGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/mob/InfestedBlock.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// EndermiteEntity 实现
// ============================================================================

std::unique_ptr<Entity> EndermiteEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EndermiteEntity>(EntityInstanceId(0));
}

EndermiteEntity::EndermiteEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 末影螨不在阳光下燃烧
    setBurnsInDaylight(false);
    // 经验值 3
    setExperienceValue(3);

    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

void EndermiteEntity::tick()
{
    // 同步渲染偏航角和旋转偏航角
    m_prevRenderYawOffset = m_renderYawOffset;
    m_renderYawOffset = yaw();

    MonsterEntity::tick();

    // 服务端：处理消失逻辑
    // 注意：客户端粒子效果在客户端渲染器中处理
    if (!isNoDespawnRequired()) {
        m_lifetime++;
        if (m_lifetime >= DESPAWN_TIME) {
            remove();
        }
    }
}

void EndermiteEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 行为目标
    goalSelector().addGoal(1, new entity::ai::goal::SwimGoal(this));
    goalSelector().addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));
    goalSelector().addGoal(3, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));
    goalSelector().addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));
    goalSelector().addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    // MC 原版: targetSelector.addGoal(1, HurtByTargetGoal(this).setAlertOthers())
    // 末影螨被攻击后会警醒附近的同类末影螨
    {
        auto hurtByTarget = std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true);
        m_targetSelector.addGoal(1, std::move(hurtByTarget));
    }
    targetSelector().addGoal(2,
        new entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                // 攻击最近的玩家
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
            }));
}

void EndermiteEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

// ============================================================================
// SilverfishEntity 实现
// ============================================================================

std::unique_ptr<Entity> SilverfishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SilverfishEntity>(EntityInstanceId(0));
}

SilverfishEntity::SilverfishEntity(EntityInstanceId id)
    : MonsterEntity(id)
    , m_summonGoal(nullptr)
{
    // 蠹虫不在阳光下燃烧
    setBurnsInDaylight(false);
    // 经验值 5
    setExperienceValue(5);

    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

void SilverfishEntity::tick()
{
    // 同步渲染偏航角和旋转偏航角
    m_prevRenderYawOffset = m_renderYawOffset;
    m_renderYawOffset = yaw();

    MonsterEntity::tick();
}

bool SilverfishEntity::hurt(DamageSource& source, f32 amount)
{
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 如果受到实体或魔法伤害，通知召唤同伴目标
    if (m_summonGoal != nullptr) {
        // 检查是否是实体伤害或魔法伤害
        if (source.isEntitySource() || source.isMagic()) {
            m_summonGoal->notifyHurt();
        }
    }

    return MonsterEntity::hurt(source, amount);
}

void SilverfishEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 创建召唤同伴目标
    m_summonGoal = new entity::ai::goal::SilverfishSummonOthersGoal(this);

    // 行为目标
    goalSelector().addGoal(1, new entity::ai::goal::SwimGoal(this));
    goalSelector().addGoal(3, m_summonGoal); // 召唤同伴目标（优先级 3）
    goalSelector().addGoal(4, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));
    goalSelector().addGoal(5, new entity::ai::goal::SilverfishHideInStoneGoal(this)); // 藏入石头目标
    goalSelector().addGoal(6, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));
    goalSelector().addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));
    goalSelector().addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    targetSelector().addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, true));
    targetSelector().addGoal(2,
        new entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
            }));
}

void SilverfishEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0);
}

void SilverfishEntity::notifySummonCooldown()
{
    if (m_summonGoal != nullptr) {
        m_summonGoal->notifyHurt();
    }
}

f32 SilverfishEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // MC Silverfish.getWalkTargetValue: 脚下是可被虫蚀的方块（宿主方块）返回 10.0f，否则委托父类
    // 对应 MC: InfestedBlock.isCompatibleHostBlock(level.getBlockState(pos.below()))
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    // 检查脚下方块（y - 1）是否为可被虫蚀的宿主方块
    BlockPos belowPos(
        static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)) - 1, static_cast<i32>(std::floor(z)));
    const BlockState* belowState = worldPtr->getBlockState(belowPos);
    if (belowState != nullptr && blocks::InfestedBlock::canContainSilverfish(*belowState)) {
        return 10.0f;
    }

    // 非宿主方块：委托给 MonsterEntity 的默认实现
    return MonsterEntity::getPathWeight(x, y, z);
}

} // namespace mc
