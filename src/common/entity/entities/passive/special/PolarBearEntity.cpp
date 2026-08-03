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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PolarBearEntity.hpp"

#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/monster/MonsterEntity.hpp"
#include "../../../entities/passive/special/FoxEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// ==================== 静态成员初始化 ====================
entity::DataParameter<bool> PolarBearEntity::DATA_STANDING_PARAM = entity::EntityDataManager::createKey<bool>();

const entity::EntityClassInfo& PolarBearEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PolarBearEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

// ==================== Forward declarations for standalone AI goal classes ====================

/**
 * @brief 北极熊近战攻击目标
 *
 * 带有站立警告逻辑的近战攻击。
 * 当目标在攻击范围内时站立并发出警告声。
 */
class PolarBearMeleeAttackGoal : public entity::ai::goal::MeleeAttackGoal {
public:
    explicit PolarBearMeleeAttackGoal(PolarBearEntity* bear);
    void resetTask() override;
    void tick() override;

private:
    PolarBearEntity* m_bear;
};

/**
 * @brief 北极熊恐慌目标
 *
 * 只有幼熊或着火时才会恐慌逃跑。
 */
class PolarBearPanicGoal : public entity::ai::goal::PanicGoal {
public:
    explicit PolarBearPanicGoal(PolarBearEntity* bear);
    bool shouldExecute() override;

private:
    PolarBearEntity* m_bear;
};

/**
 * @brief 北极熊被攻击反击目标
 *
 * 被攻击后会反击，幼熊会呼唤成年熊。
 */
class PolarBearHurtByTargetGoal : public entity::ai::goal::HurtByTargetGoal {
public:
    explicit PolarBearHurtByTargetGoal(PolarBearEntity* bear);
    void startExecuting() override;

private:
    PolarBearEntity* m_bear;
};

/**
 * @brief 北极熊攻击玩家目标
 *
 * 保护幼崽：当幼熊附近有成年熊时，成年熊会攻击玩家。
 */
class PolarBearAttackPlayerGoal : public entity::ai::goal::NearestAttackableTargetGoal<Player> {
public:
    explicit PolarBearAttackPlayerGoal(PolarBearEntity* bear);
    bool shouldExecute() override;

private:
    PolarBearEntity* m_bear;
};

// ==================== PolarBearEntity ====================

PolarBearEntity::PolarBearEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> PolarBearEntity::create(IWorld* /*world*/)
{
    return std::make_unique<PolarBearEntity>(0);
}

void PolarBearEntity::setStanding(bool standing)
{
    m_standing = standing;
    m_dataManager.set(DATA_STANDING_PARAM, standing);
    if (standing) {
        math::Random& rng = getRandom();
        m_standTimer = rng.nextInt(STAND_DURATION_MIN, STAND_DURATION_MAX);
    }
}

void PolarBearEntity::setWarning(bool warning)
{
    m_warning = warning;
}

void PolarBearEntity::setRevengeTarget(LivingEntity* target)
{
    setAttackTarget(target);
    if (target != nullptr) {
        math::Random& rng = getRandom();
        m_angerTime = rng.nextInt(ANGER_TIME_MIN, ANGER_TIME_MAX);
        m_revengeTargetId = target->id();
        m_revengeTimer = MAX_ANGER_TIME;
    } else {
        m_angerTime = 0;
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* PolarBearEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void PolarBearEntity::setAngry(bool angry)
{
    if (angry) {
        math::Random& rng = getRandom();
        m_angerTime = rng.nextInt(ANGER_TIME_MIN, ANGER_TIME_MAX);
    } else {
        m_angerTime = 0;
        setAttackTarget(nullptr);
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

void PolarBearEntity::updateAnger()
{
    if (m_angerTime > 0) {
        --m_angerTime;
        if (m_angerTime <= 0) {
            setAttackTarget(nullptr);
            m_revengeTargetId = std::nullopt;
        }
    }
    if (m_revengeTimer > 0) {
        --m_revengeTimer;
        if (m_revengeTimer <= 0) {
            m_revengeTargetId = std::nullopt;
        }
    }
}

std::optional<ResourceLocation> PolarBearEntity::getAmbientSound() const
{
    if (isChild()) {
        return SoundEvents::ENTITY_POLAR_BEAR_AMBIENT_BABY;
    }
    return SoundEvents::ENTITY_POLAR_BEAR_AMBIENT;
}

std::optional<ResourceLocation> PolarBearEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_POLAR_BEAR_HURT;
}

std::optional<ResourceLocation> PolarBearEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_POLAR_BEAR_DEATH;
}

void PolarBearEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    playSound(SoundEvents::ENTITY_POLAR_BEAR_STEP, 0.15f, 1.0f);
}

void PolarBearEntity::playWarningSound()
{
    if (m_warningSoundTicks <= 0) {
        playSound(SoundEvents::ENTITY_POLAR_BEAR_WARNING, 1.0f, getSoundPitch());
        m_warningSoundTicks = WARNING_SOUND_COOLDOWN;
    }
}

bool PolarBearEntity::attackEntityAsMob(LivingEntity& target)
{
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0));
    EntityDamageSource damageSource(DamageType::MobAttack, this);
    bool success = target.hurt(damageSource, damage);
    return success;
}

void PolarBearEntity::tick()
{
    AnimalEntity::tick();

    // 客户端动画更新
    if (world() != nullptr && world()->isClientSide()) {
        // 检查动画值是否变化（需要重新计算碰撞箱）
        if (m_clientSideStandAnimation != m_clientSideStandAnimation0) {
            refreshDimensions();
        }

        // 保存上一帧动画值
        m_clientSideStandAnimation0 = m_clientSideStandAnimation;

        // 根据站立状态更新动画
        if (isStanding()) {
            // 站立时动画增加（最大 STAND_ANIMATION_TICKS）
            m_clientSideStandAnimation = math::clamp(m_clientSideStandAnimation + 1.0f, 0.0f, STAND_ANIMATION_TICKS);
        } else {
            // 非站立时动画减少（最小 0.0）
            m_clientSideStandAnimation = math::clamp(m_clientSideStandAnimation - 1.0f, 0.0f, STAND_ANIMATION_TICKS);
        }
    }

    // 服务端逻辑
    if (world() != nullptr && !world()->isClientSide()) {
        if (m_standing && m_standTimer > 0) {
            m_standTimer--;
            if (m_standTimer <= 0) {
                setStanding(false);
            }
        }
    }

    if (m_warningSoundTicks > 0) {
        m_warningSoundTicks--;
    }

    updateAnger();
}

f32 PolarBearEntity::getStandingAnimationScale(f32 partialTick) const
{
    // 注意：MC 的 lerp 签名是 lerp(t, a, b)，我们的签名是 lerp(a, b, t)
    return math::lerp(m_clientSideStandAnimation0, m_clientSideStandAnimation, partialTick) / STAND_ANIMATION_TICKS;
}

entity::EntitySize PolarBearEntity::getDimensions(EntityPose pose) const
{
    // 获取基础尺寸（含幼崽缩放）
    entity::EntitySize baseSize = AnimalEntity::getDimensions(pose);

    // 站立动画期间，高度随动画进度逐渐增大
    if (m_clientSideStandAnimation > 0.0f) {
        f32 standProgress = m_clientSideStandAnimation / STAND_ANIMATION_TICKS;
        f32 heightScale = 1.0f + standProgress;
        return baseSize.scale(1.0f, heightScale);
    }

    return baseSize;
}

void PolarBearEntity::registerData()
{
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    m_dataManager.registerParam(DATA_STANDING_PARAM, false);
}

void PolarBearEntity::registerGoals()
{
    // 注意：北极熊不调用 AnimalEntity::registerGoals() 因为它没有繁殖行为

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 近战攻击（使用自定义内部类）
    m_goalSelector.addGoal(1, new PolarBearMeleeAttackGoal(this));

    // 优先级 1: 恐慌逃跑（只有幼熊或着火时）
    m_goalSelector.addGoal(1, new PolarBearPanicGoal(this));

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.25));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择器
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new PolarBearHurtByTargetGoal(this));

    // 优先级 2: 攻击玩家（保护幼崽）
    m_targetSelector.addGoal(2, new PolarBearAttackPlayerGoal(this));

    // 优先级 3: 攻击玩家（有条件）
    m_targetSelector.addGoal(3,
        new entity::ai::goal::NearestAttackableTargetGoal<Player>(
            this, true, 10, [this](const LivingEntity* /*entity*/) -> bool {
                IWorld* world = this->world();
                if (world == nullptr) return false;

                AxisAlignedBB searchBox = this->boundingBox().expand(8.0, 4.0, 8.0);
                auto nearbyEntities = world->getEntitiesInAABB(searchBox, this);

                for (Entity* nearby : nearbyEntities) {
                    PolarBearEntity* bear = dynamic_cast<PolarBearEntity*>(nearby);
                    if (bear != nullptr && bear != this && bear->isChild()) {
                        return true;
                    }
                }

                return false;
            }));

    // 优先级 4: 攻击狐狸
    m_targetSelector.addGoal(4, new entity::ai::goal::NearestAttackableTargetGoal<FoxEntity>(this, true, 10, nullptr));
}

void PolarBearEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 北极熊属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

// ==================== PolarBearMeleeAttackGoal ====================

PolarBearMeleeAttackGoal::PolarBearMeleeAttackGoal(PolarBearEntity* bear)
    : entity::ai::goal::MeleeAttackGoal(bear, 1.25, true)
    , m_bear(bear)
{}

void PolarBearMeleeAttackGoal::resetTask()
{
    m_bear->setStanding(false);
    MeleeAttackGoal::resetTask();
}

void PolarBearMeleeAttackGoal::tick()
{
    MeleeAttackGoal::tick();

    // 在tick中处理站立警告逻辑
    LivingEntity* target = m_bear->getAttackTarget();
    if (target != nullptr && m_bear != nullptr) {
        f64 distSq = m_bear->distanceSqTo(*target);
        f32 attackReach = m_bear->width() * 2.0f;
        f32 attackReachSq = attackReach * attackReach + target->width();

        if (distSq <= static_cast<f64>(attackReachSq) * 2.0) {
            // 在警告范围内
            if (m_attackCooldown <= 10) {
                m_bear->setStanding(true);
                m_bear->playWarningSound();
            }
        } else {
            m_bear->setStanding(false);
        }
    }
}

// ==================== PolarBearPanicGoal ====================

PolarBearPanicGoal::PolarBearPanicGoal(PolarBearEntity* bear)
    : entity::ai::goal::PanicGoal(bear, 2.0)
    , m_bear(bear)
{}

bool PolarBearPanicGoal::shouldExecute()
{
    // 只有幼熊或着火的北极熊才会恐慌
    if (!m_bear->isChild() && !m_bear->isOnFire()) {
        return false;
    }
    return PanicGoal::shouldExecute();
}

// ==================== PolarBearHurtByTargetGoal ====================

PolarBearHurtByTargetGoal::PolarBearHurtByTargetGoal(PolarBearEntity* bear)
    : entity::ai::goal::HurtByTargetGoal(bear, false)
    , m_bear(bear)
{}

void PolarBearHurtByTargetGoal::startExecuting()
{
    // 幼熊被攻击时会呼唤成年熊
    if (m_bear->isChild()) {
        IWorld* world = m_bear->world();
        if (world != nullptr) {
            AxisAlignedBB alertBox = m_bear->boundingBox().expand(16.0, 4.0, 16.0);
            auto nearbyEntities = world->getEntitiesInAABB(alertBox, m_bear);

            for (Entity* entity : nearbyEntities) {
                PolarBearEntity* nearbyBear = dynamic_cast<PolarBearEntity*>(entity);
                if (nearbyBear != nullptr && !nearbyBear->isChild()) {
                    nearbyBear->setAttackTarget(m_target);
                    nearbyBear->setAngry(true);
                }
            }
        }
        resetTask();
        return;
    }
    HurtByTargetGoal::startExecuting();
}

// ==================== PolarBearAttackPlayerGoal ====================

PolarBearAttackPlayerGoal::PolarBearAttackPlayerGoal(PolarBearEntity* bear)
    : entity::ai::goal::NearestAttackableTargetGoal<Player>(bear, true, 20)
    , m_bear(bear)
{}

bool PolarBearAttackPlayerGoal::shouldExecute()
{
    // 只有成年熊在附近有幼熊时才会攻击玩家
    if (m_bear->isChild()) {
        return false;
    }

    if (!NearestAttackableTargetGoal<Player>::shouldExecute()) {
        return false;
    }

    IWorld* world = m_bear->world();
    if (world == nullptr) return false;

    AxisAlignedBB searchBox = m_bear->boundingBox().expand(8.0, 4.0, 8.0);
    auto nearbyEntities = world->getEntitiesInAABB(searchBox, m_bear);

    for (Entity* entity : nearbyEntities) {
        PolarBearEntity* bear = dynamic_cast<PolarBearEntity*>(entity);
        if (bear != nullptr && bear != m_bear && bear->isChild()) {
            return true;
        }
    }

    return false;
}

} // namespace mc
