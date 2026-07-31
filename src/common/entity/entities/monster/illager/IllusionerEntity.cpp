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

#include "IllusionerEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/special/IllusionerGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// 继承链标识（parent = SpellcastingIllagerEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& IllusionerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"IllusionerEntity", &SpellcastingIllagerEntity::classInfo()};
    return s_classInfo;
}

IllusionerEntity::IllusionerEntity(EntityInstanceId id)
    : SpellcastingIllagerEntity(id)
{
    // 初始化镜像分身偏移数组为零向量
    for (auto& offsets : m_illusionOffsets) {
        offsets.fill(Vector3(0.0f, 0.0f, 0.0f));
    }

    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> IllusionerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<IllusionerEntity>(EntityInstanceId(0));
}

void IllusionerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(*this, world());
    if (arrow == nullptr) {
        return;
    }

    // 计算发射方向
    f64 dx = target->x() - x();
    f64 dy = target->getY(0.333) - (arrow->y());
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 不精确度：难度越高，不精确度越低，箭矢越精准
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(world()->difficulty());

    // 使用生物箭矢伤害公式设置基础伤害
    arrow->setBaseDamageFromMob(charge);

    // 发射箭矢
    arrow->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + horizontalDist * 0.2),
        static_cast<f32>(dz),
        ARROW_VELOCITY, // 1.6F
        inaccuracy);

    // 播放射箭音效
    math::Random& rng = getRandom();
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    playSound(SoundEvents::ENTITY_SKELETON_SHOOT, 1.0f, pitch);

    // 生成箭矢实体
    world()->spawnEntity(std::move(arrow));
}

std::array<Vector3, IllusionerEntity::NUM_ILLUSIONS> IllusionerEntity::getIllusionOffsets(f32 partialTick) const
{
    std::array<Vector3, NUM_ILLUSIONS> result;

    if (m_clientSideIllusionTicks <= 0) {
        // 无过渡动画，直接使用目标偏移
        result = m_illusionOffsets[1];
    } else {
        // 过渡动画：使用四次方根缓动从旧偏移插值到新偏移
        f64 t = static_cast<f64>(m_clientSideIllusionTicks - partialTick) / static_cast<f64>(ILLUSION_TRANSITION_TICKS);
        t = std::pow(t, 0.25); // 四次方根缓动

        f32 tf = static_cast<f32>(t);
        for (i32 i = 0; i < NUM_ILLUSIONS; ++i) {
            result[i] = m_illusionOffsets[1][i] * (1.0f - tf) + m_illusionOffsets[0][i] * tf;
        }
    }

    return result;
}

void IllusionerEntity::tick()
{
    SpellcastingIllagerEntity::tick();

    // 更新冷却时间
    if (m_blindnessCooldown > 0) {
        --m_blindnessCooldown;
    }
    if (m_mirrorCooldown > 0) {
        --m_mirrorCooldown;
    }

    // 客户端镜像分身逻辑
    _updateIllusionLogic();
}

void IllusionerEntity::_updateIllusionLogic()
{
    // 仅在客户端且实体隐身时执行
    if (!m_world || !m_world->isClientSide()) {
        return;
    }
    if (!hasEffect(entity::effect::EffectType::Invisibility)) {
        return;
    }

    // 递减过渡计时器
    if (m_clientSideIllusionTicks > 0) {
        --m_clientSideIllusionTicks;
    }

    // 受伤第1 tick 或每 1200 tick 重新生成分身偏移
    if (hurtTime() == 1 || static_cast<i32>(ticksExisted()) % 1200 == 0) {
        m_clientSideIllusionTicks = ILLUSION_TRANSITION_TICKS;

        math::Random& rng = getRandom();

        // 保存旧偏移作为过渡起点，生成新的目标偏移
        for (i32 i = 0; i < NUM_ILLUSIONS; ++i) {
            m_illusionOffsets[0][i] = m_illusionOffsets[1][i];
            m_illusionOffsets[1][i] = Vector3((-6.0f + static_cast<f32>(rng.nextInt(ILLUSION_SPREAD * 2 + 7))) * 0.5f,
                static_cast<f32>(std::max(0, rng.nextInt(6) - 4)),
                (-6.0f + static_cast<f32>(rng.nextInt(ILLUSION_SPREAD * 2 + 7))) * 0.5f);
        }

        // 生成16个云粒子
        for (i32 i = 0; i < 16; ++i) {
            f32 px = x() + (rng.nextFloat() - 0.5f) * width() * 0.5f;
            f32 py = y() + rng.nextFloat() * height();
            f32 pz = z() + (rng.nextFloat() - 0.5f) * width() * 0.5f;

            m_world->addParticle(particle::ParticleTypeId::Cloud,
                Vector3(static_cast<f64>(px), static_cast<f64>(py), static_cast<f64>(pz)),
                Vector3(0.0, 0.0, 0.0));
        }

        // 播放镜像移动音效
        m_world->playSound(
            SoundEvents::ENTITY_ILLUSIONER_MIRROR_MOVE, sound::SoundCategory::Hostile, position(), 1.0f, 1.0f);
    } else if (hurtTime() == maxHurtTime() - 1) {
        // 受伤结束时将分身偏移归零
        m_clientSideIllusionTicks = ILLUSION_TRANSITION_TICKS;

        for (i32 i = 0; i < NUM_ILLUSIONS; ++i) {
            m_illusionOffsets[0][i] = m_illusionOffsets[1][i];
            m_illusionOffsets[1][i] = Vector3(0.0f, 0.0f, 0.0f);
        }
    }
}

void IllusionerEntity::registerGoals()
{
    // 调用父类方法
    SpellcastingIllagerEntity::registerGoals();

    // 行为目标选择器 (goalSelector)
    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 施法时看向目标（父类已注册 CastingSpellGoal）

    // 优先级 4: 镜像法术（隐身）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::IllusionerMirrorSpellGoal>(this));

    // 优先级 5: 失明法术
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::IllusionerBlindnessSpellGoal>(this));

    // 优先级 6: 弓箭远程攻击
    // 参数：移动速度 0.5，攻击间隔 20 ticks
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::RangedBowAttackGoal>(this, 0.5, 20, 20));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6, 1));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(
        9, std::make_unique<entity::ai::goal::LookAtGoal>(this, 3.0f, 1.0f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            return entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(
        10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            // 看向所有 MobEntity
            return entity->entityType() != entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 目标选择器 (targetSelector)
    // 优先级 1: 被攻击后反击并呼叫支援
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 攻击玩家（穿透墙壁追踪15秒）
    {
        auto playerTarget = std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true);
        playerTarget->setUnseenMemoryTicks(300);
        m_targetSelector.addGoal(2, std::move(playerTarget));
    }

    // 优先级 3: 攻击村民（穿透墙壁感知）
    {
        auto villagerTarget =
            std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(
                this, false);
        villagerTarget->setUnseenMemoryTicks(300);
        m_targetSelector.addGoal(3, std::move(villagerTarget));
    }

    // 优先级 3: 攻击铁傀儡（穿透墙壁感知）
    {
        auto ironGolemTarget =
            std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, false);
        ironGolemTarget->setUnseenMemoryTicks(300);
        m_targetSelector.addGoal(3, std::move(ironGolemTarget));
    }
}

void IllusionerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();

    // 幻术师属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 32.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
