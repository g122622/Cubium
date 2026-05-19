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
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/special/SilverfishGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"

namespace mc {

// ============================================================================
// EndermiteEntity 实现
// ============================================================================

std::unique_ptr<Entity> EndermiteEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EndermiteEntity>(EntityId(0));
}

EndermiteEntity::EndermiteEntity(EntityId id)
    : MonsterEntity(id)
{
    // MC 1.16.5: 末影螨不在阳光下燃烧
    setBurnsInDaylight(false);
    // MC 1.16.5: 经验值 3
    setExperienceValue(3);

    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

void EndermiteEntity::tick()
{
    // MC 1.16.5 EndermiteEntity.tick()
    // 同步渲染偏航角和旋转偏航角
    m_prevRenderYawOffset = m_renderYawOffset;
    m_renderYawOffset = yaw();

    MonsterEntity::tick();

    // MC 1.16.5 EndermiteEntity.livingTick()
    // 服务端：处理消失逻辑
    // 注意：客户端粒子效果在客户端渲染器中处理
    if (!isNoDespawnRequired()) {
        m_lifetime++;
        if (m_lifetime >= DESPAWN_TIME) {
            // MC 1.16.5: 调用 remove() 移除实体
            remove();
        }
    }
}

void EndermiteEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // MC 1.16.5 EndermiteEntity.registerGoals()
    // 行为目标
    goalSelector().addGoal(1, new entity::ai::goal::SwimGoal(this));
    goalSelector().addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));
    goalSelector().addGoal(3, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));
    goalSelector().addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));
    goalSelector().addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    targetSelector().addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));
    targetSelector().addGoal(2,
        new entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                // 攻击最近的玩家
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
            }));
}

void EndermiteEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // MC 1.16.5 EndermiteEntity.func_234288_m_()
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

// ============================================================================
// SilverfishEntity 实现
// ============================================================================

std::unique_ptr<Entity> SilverfishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SilverfishEntity>(EntityId(0));
}

SilverfishEntity::SilverfishEntity(EntityId id)
    : MonsterEntity(id)
    , m_summonGoal(nullptr)
{
    // MC 1.16.5: 蠹虫不在阳光下燃烧
    setBurnsInDaylight(false);
    // MC 1.16.5: 经验值 5
    setExperienceValue(5);

    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

void SilverfishEntity::tick()
{
    // MC 1.16.5 SilverfishEntity.tick()
    // 同步渲染偏航角和旋转偏航角
    m_prevRenderYawOffset = m_renderYawOffset;
    m_renderYawOffset = yaw();

    MonsterEntity::tick();
}

bool SilverfishEntity::hurt(DamageSource& source, f32 amount)
{
    // MC 1.16.5 SilverfishEntity.attackEntityFrom()
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 如果受到实体或魔法伤害，通知召唤同伴目标
    // MC 1.16.5: if ((source instanceof EntityDamageSource || source == DamageSource.MAGIC) &&
    // this.summonSilverfish != null) { this.summonSilverfish.notifyHurt(); }
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

    // MC 1.16.5 SilverfishEntity.registerGoals()
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
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));
    goalSelector().addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    // MC 1.16.5: setCallsForHelp() - 呼唤同伴
    targetSelector().addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, true));
    targetSelector().addGoal(2,
        new entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
            }));
}

void SilverfishEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // MC 1.16.5 SilverfishEntity.func_234317_e_()
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

} // namespace mc
