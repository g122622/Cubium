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
 * IMPLIED, INCLUDING NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BreezeGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include <cmath>
#include <cstdlib>
#include <optional>
#include <utility>

namespace mc::entity::ai::goal {

// ============================================================================
// BreezeShootGoal
// ============================================================================

BreezeShootGoal::BreezeShootGoal(BreezeEntity* breeze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_breeze(breeze)
{
    MC_ASSERT_RELEASE(breeze != nullptr);
}

bool BreezeShootGoal::shouldExecute()
{
    if (m_breeze == nullptr) {
        return false;
    }

    // 必须有攻击目标
    LivingEntity* target = m_breeze->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 射击冷却期间不启动
    if (m_breeze->shootCooldown() > 0) {
        return false;
    }

    // 充能或恢复期间不启动
    if (m_isCharging || m_hasFired) {
        return false;
    }

    // 射击许可已获得（由 Slide/LongJump/ShootWhenStuck 设置）
    if (!m_breeze->hasShootPermit()) {
        return false;
    }

    // 必须在站立姿态
    if (m_breeze->pose() != EntityPose::Standing) {
        return false;
    }

    // 目标必须在攻击范围内（16 格以内）
    f64 distSq = m_breeze->distanceSqTo(target->x(), target->y(), target->z());
    if (distSq > ATTACK_RANGE_MAX_SQ) {
        return false;
    }

    m_target = target;
    return true;
}

bool BreezeShootGoal::shouldContinueExecuting()
{
    if (m_breeze == nullptr || m_target == nullptr) {
        return false;
    }

    if (!m_target->isAlive()) {
        return false;
    }

    // 如果还在充能或恢复阶段，继续执行
    if (m_isCharging || m_hasFired) {
        return true;
    }

    // 目标和射击许可仍然需要存在
    return m_breeze->hasShootPermit();
}

void BreezeShootGoal::startExecuting()
{
    m_isCharging = true;
    m_hasFired = false;
    m_chargeTime = CHARGE_TICKS;
    m_recoverTime = 0;

    // 切换到射击姿态（参考 MC Shoot.start）
    m_breeze->setPose(EntityPose::Shooting);

    // 播放吸气音效
    if (m_breeze->world() != nullptr) {
        m_breeze->world()->playSound(
            SoundEvents::ENTITY_BREEZE_INHALE, sound::SoundCategory::Hostile, m_breeze->position(), 1.0f, 1.0f);
    }
}

void BreezeShootGoal::resetTask()
{
    m_target = nullptr;
    m_isCharging = false;
    m_hasFired = false;
    m_chargeTime = 0;
    m_recoverTime = 0;

    // 仅在当前处于射击姿态时恢复站立（参考 MC Shoot.stop）
    if (m_breeze->pose() == EntityPose::Shooting) {
        m_breeze->setPose(EntityPose::Standing);
    }

    // 清除射击许可并设置冷却
    m_breeze->clearShootPermit();
    m_breeze->setShootCooldown(SHOOT_COOLDOWN_TICKS);
}

void BreezeShootGoal::tick()
{
    if (m_breeze == nullptr || m_target == nullptr) {
        return;
    }

    // 看向目标
    auto* lookCtrl = m_breeze->lookController();
    if (lookCtrl != nullptr) {
        lookCtrl->setLookPositionWithEntity(*m_target, 10.0f, 10.0f);
    }

    if (m_isCharging) {
        // 充能阶段
        m_chargeTime--;
        if (m_chargeTime <= 0) {
            // 充能完毕，发射风弹
            m_isCharging = false;
            m_hasFired = true;
            m_recoverTime = RECOVER_TICKS;

            // 调用 BreezeEntity 的射击方法
            m_breeze->shootWindCharge();

            // 播放射击音效（音量1.5，与 MC 原版一致）
            if (m_breeze->world() != nullptr) {
                m_breeze->world()->playSound(
                    SoundEvents::ENTITY_BREEZE_SHOOT, sound::SoundCategory::Hostile, m_breeze->position(), 1.5f, 1.0f);
            }
        }
    } else if (m_hasFired) {
        // 恢复阶段
        m_recoverTime--;
        if (m_recoverTime <= 0) {
            m_hasFired = false;
            // 发射完成后结束目标，resetTask 会清除射击许可并设置冷却
        }
    }
}

// ============================================================================
// BreezeLongJumpGoal
// ============================================================================

BreezeLongJumpGoal::BreezeLongJumpGoal(BreezeEntity* breeze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Jump})
    , m_breeze(breeze)
    , m_jumpTargetPos(0, 0, 0)
{
    MC_ASSERT_RELEASE(breeze != nullptr);
}

bool BreezeLongJumpGoal::shouldExecute()
{
    if (m_breeze == nullptr) {
        return false;
    }

    // 长跳冷却期间不启动
    if (m_jumpCooldown > 0) {
        return false;
    }

    // 必须有攻击目标
    LivingEntity* target = m_breeze->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 不在射击状态
    if (m_breeze->hasShootPermit()) {
        return false;
    }

    // 必须在地面或水中
    if (!m_breeze->onGround() && !m_breeze->isInWater()) {
        return false;
    }

    // 目标不能太近（内圈范围内不适合长跳）
    f64 distSq = m_breeze->distanceSqTo(target->x(), target->y(), target->z());
    f64 dist = std::sqrt(distSq);
    if (dist - TOO_CLOSE_RANGE <= 0.0) {
        return false;
    }

    // 超出追踪范围则放弃目标
    f64 followRange = m_breeze->getAttributeValue(
        entity::attribute::Attributes::FOLLOW_RANGE, static_cast<f64>(BreezeEntity::FOLLOW_RANGE));
    if (dist > followRange) {
        m_breeze->setAttackTarget(nullptr);
        return false;
    }

    // 检查是否可以跳跃
    if (!_canJumpFromCurrentPosition()) {
        return false;
    }

    // 寻找跳跃目标位置
    auto jumpTarget = _findJumpTargetBehindAttackTarget();
    if (!jumpTarget.has_value()) {
        return false;
    }

    m_target = target;
    m_jumpTargetPos = jumpTarget.value();
    return true;
}

bool BreezeLongJumpGoal::shouldContinueExecuting()
{
    if (m_breeze == nullptr) {
        return false;
    }

    // 如果还在吸气或跳跃中，继续执行
    if (m_isInhaling || m_isJumping) {
        // 跳跃中但冷却已开始（着陆），停止
        if (m_isJumping && m_jumpCooldown > 0) {
            return false;
        }
        return true;
    }

    return false;
}

void BreezeLongJumpGoal::startExecuting()
{
    m_isInhaling = true;
    m_isJumping = false;
    m_inhaleTime = INHALE_TICKS;

    // 切换到吸气姿态（参考 MC LongJump.start）
    m_breeze->setPose(EntityPose::Inhaling);

    // 播放蓄力音效
    if (m_breeze->world() != nullptr) {
        m_breeze->world()->playSound(
            SoundEvents::ENTITY_BREEZE_CHARGE, sound::SoundCategory::Hostile, m_breeze->position(), 1.0f, 1.0f);
    }

    // 看向跳跃目标位置
    auto* lookCtrl = m_breeze->lookController();
    if (lookCtrl != nullptr) {
        lookCtrl->setLookPosition(m_jumpTargetPos.x, m_jumpTargetPos.y, m_jumpTargetPos.z);
    }
}

void BreezeLongJumpGoal::resetTask()
{
    // 若仍处于长跳或吸气姿态，恢复站立（参考 MC LongJump.stop）
    if (m_breeze != nullptr) {
        const EntityPose pose = m_breeze->pose();
        if (pose == EntityPose::LongJumping || pose == EntityPose::Inhaling) {
            m_breeze->setPose(EntityPose::Standing);
        }
    }

    m_isInhaling = false;
    m_isJumping = false;
    m_inhaleTime = 0;
    m_target = nullptr;
}

void BreezeLongJumpGoal::tick()
{
    if (m_breeze == nullptr) {
        return;
    }

    if (m_isInhaling) {
        // 吸气阶段
        m_inhaleTime--;

        // 看向跳跃目标位置
        if (m_target != nullptr) {
            auto* lookCtrl = m_breeze->lookController();
            if (lookCtrl != nullptr) {
                lookCtrl->setLookPosition(m_jumpTargetPos.x, m_jumpTargetPos.y, m_jumpTargetPos.z);
            }
        }

        if (m_inhaleTime <= 0) {
            // 吸气完毕，计算跳跃向量
            auto jumpVec = _calculateOptimalJumpVector();
            if (jumpVec.has_value()) {
                // 播放跳跃音效
                if (m_breeze->world() != nullptr) {
                    m_breeze->world()->playSound(SoundEvents::ENTITY_BREEZE_JUMP,
                        sound::SoundCategory::Hostile,
                        m_breeze->position(),
                        1.0f,
                        1.0f);
                }

                // 切换到长跳姿态（参考 MC LongJump.tick 中 isFinishedInhaling 分支）
                m_breeze->setPose(EntityPose::LongJumping);
                m_breeze->setLongJumping(true);

                // 施加跳跃速度
                const Vector3& vel = jumpVec.value();
                m_breeze->setVelocity(vel.x, vel.y, vel.z);

                m_isInhaling = false;
                m_isJumping = true;
            } else {
                // 无法找到有效跳跃向量，取消并恢复站立
                m_breeze->setPose(EntityPose::Standing);
                resetTask();
            }
        }
    } else if (m_isJumping) {
        // 跳跃阶段 - 检测着陆
        if (m_breeze->onGround()) {
            // 着陆
            if (m_breeze->world() != nullptr) {
                m_breeze->world()->playSound(
                    SoundEvents::ENTITY_BREEZE_LAND, sound::SoundCategory::Hostile, m_breeze->position(), 1.0f, 1.0f);
            }

            // 切换回站立姿态（参考 MC LongJump.tick 中 isFinishedJumping 分支）
            m_breeze->setPose(EntityPose::Standing);

            m_isJumping = false;

            // 设置跳跃冷却
            // MC 原版：受伤后冷却2 ticks，正常冷却10 ticks
            i32 cooldown = (m_breeze->hurtTime() > 0) ? JUMP_COOLDOWN_HURT_TICKS : JUMP_COOLDOWN_TICKS;
            m_jumpCooldown = cooldown;

            // 设置射击许可（长跳着陆后可以射击）
            m_breeze->setShootPermit(SHOOT_PERMIT_TICKS);
        }
    }

    // 冷却递减（在目标未激活时也需要递减）
    if (m_jumpCooldown > 0 && !m_isInhaling && !m_isJumping) {
        m_jumpCooldown--;
    }
}

bool BreezeLongJumpGoal::_canJumpFromCurrentPosition() const
{
    if (m_breeze == nullptr || m_breeze->world() == nullptr) {
        return false;
    }

    // 检查头顶4格是否有空间（空气或水）
    IWorld* world = m_breeze->world();
    i32 bx = static_cast<i32>(std::floor(m_breeze->x()));
    i32 by = static_cast<i32>(std::floor(m_breeze->y()));
    i32 bz = static_cast<i32>(std::floor(m_breeze->z()));

    for (i32 dy = 1; dy <= 4; ++dy) {
        const BlockState* state = world->getBlockState(bx, by + dy, bz);
        if (state != nullptr && !state->isAir() && !state->isLiquid()) {
            // 如果方块不是空气也不是液体，则无法跳跃
            return false;
        }
    }

    return true;
}

std::optional<Vector3> BreezeLongJumpGoal::_findJumpTargetBehindAttackTarget() const
{
    if (m_breeze == nullptr || m_target == nullptr || m_breeze->world() == nullptr) {
        return std::nullopt;
    }

    math::Random& rng = m_breeze->world()->getRandom();

    // MC 原版：在目标身后选择一个随机位置
    // 角度 = target.yHeadRot + 180 ± gaussian*45
    f64 angle = static_cast<f64>(m_target->rotationYawHead()) + 180.0 + static_cast<f64>(rng.nextGaussian()) * 45.0;
    angle = static_cast<f64>(math::wrapDegrees(static_cast<f32>(angle))) * static_cast<f64>(math::DEG_TO_RAD);

    // 距离 = lerp(random, 4, 8)
    f64 distance = math::lerp(rng.nextDouble(), BEHIND_TARGET_MIN, BEHIND_TARGET_MAX);

    // 计算目标身后位置
    f64 targetX = m_target->x() - std::sin(angle) * distance;
    f64 targetZ = m_target->z() + std::cos(angle) * distance;

    // 从上方寻找可站立表面
    IWorld* world = m_breeze->world();
    i32 x = static_cast<i32>(std::floor(targetX));
    i32 z = static_cast<i32>(std::floor(targetZ));

    // 向下搜索地面
    for (i32 y = static_cast<i32>(std::floor(m_target->y())) + 4; y >= static_cast<i32>(std::floor(m_target->y())) - 10;
        --y) {
        const BlockState* state = world->getBlockState(x, y, z);
        if (state != nullptr && !state->isAir()) {
            // 找到地面，返回上方一格的位置
            return Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y + 1), static_cast<f32>(z) + 0.5f);
        }
    }

    return std::nullopt;
}

std::optional<Vector3> BreezeLongJumpGoal::_calculateJumpVector(f32 angle) const
{
    if (m_breeze == nullptr || m_target == nullptr) {
        return std::nullopt;
    }

    // 计算水平方向到目标位置
    f64 dx = static_cast<f64>(m_jumpTargetPos.x) - m_breeze->x();
    f64 dz = static_cast<f64>(m_jumpTargetPos.z) - m_breeze->z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    if (horizontalDist < 1.0) {
        return std::nullopt;
    }

    // 计算跳跃速度
    f64 followRange = m_breeze->getAttributeValue(
        entity::attribute::Attributes::FOLLOW_RANGE, static_cast<f64>(BreezeEntity::FOLLOW_RANGE));
    f64 jumpSpeed = static_cast<f64>(JUMP_VELOCITY_SCALE) * followRange;

    // 计算仰角弧度
    f32 angleRad = angle * math::DEG_TO_RAD;
    f32 sinAngle = std::sin(angleRad);
    f32 cosAngle = std::cos(angleRad);

    // 根据仰角计算跳跃向量
    // 使用简化的弹道公式计算初速度
    f64 horizontalSpeed = jumpSpeed * cosAngle;
    f64 verticalSpeed = jumpSpeed * sinAngle;

    // 归一化水平方向
    f64 normX = dx / horizontalDist;
    f64 normZ = dz / horizontalDist;

    return Vector3(static_cast<f32>(normX * horizontalSpeed),
        static_cast<f32>(verticalSpeed),
        static_cast<f32>(normZ * horizontalSpeed));
}

std::optional<Vector3> BreezeLongJumpGoal::_calculateOptimalJumpVector() const
{
    if (m_breeze == nullptr) {
        return std::nullopt;
    }

    math::Random& rng = m_breeze->world()->getRandom();

    // MC 原版：尝试随机排列的 [40, 55, 60, 75, 80] 度角度
    f32 angles[ALLOWED_ANGLES_COUNT] = {
        ALLOWED_ANGLES[0], ALLOWED_ANGLES[1], ALLOWED_ANGLES[2], ALLOWED_ANGLES[3], ALLOWED_ANGLES[4]};

    // 随机排列
    for (i32 i = ALLOWED_ANGLES_COUNT - 1; i > 0; --i) {
        i32 j = rng.nextInt(i + 1);
        std::swap(angles[i], angles[j]);
    }

    // 尝试每个角度，返回第一个有效的
    for (i32 i = 0; i < ALLOWED_ANGLES_COUNT; ++i) {
        auto vec = _calculateJumpVector(angles[i]);
        if (vec.has_value()) {
            return vec;
        }
    }

    return std::nullopt;
}

// ============================================================================
// BreezeSlideGoal
// ============================================================================

BreezeSlideGoal::BreezeSlideGoal(BreezeEntity* breeze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_breeze(breeze)
{
    MC_ASSERT_RELEASE(breeze != nullptr);
}

bool BreezeSlideGoal::shouldExecute()
{
    if (m_breeze == nullptr) {
        return false;
    }

    // 必须有攻击目标
    LivingEntity* target = m_breeze->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 长跳冷却期间不启动
    if (m_breeze->jumpCooldown() > 0) {
        return false;
    }

    // 不在射击状态
    if (m_breeze->hasShootPermit()) {
        return false;
    }

    // 必须在地面
    if (!m_breeze->onGround()) {
        return false;
    }

    // 不在水中
    if (m_breeze->isInWater()) {
        return false;
    }

    // 必须在站立姿态
    if (m_breeze->pose() != EntityPose::Standing) {
        return false;
    }

    m_target = target;
    return true;
}

bool BreezeSlideGoal::shouldContinueExecuting()
{
    // Slide 是一次性目标：设置行走目标后立即结束
    // 实际的滑行移动由 MovementController 处理
    return false;
}

void BreezeSlideGoal::startExecuting()
{
    if (m_breeze == nullptr || m_target == nullptr) {
        return;
    }

    Vector3 slideTarget(0, 0, 0);

    if (_isWithinInnerCircle(m_target->position())) {
        // 在内圈范围内，向远离目标的方向逃跑
        Vector3 escapePos;
        if (entity::ai::util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(
                m_breeze, 8, 4, m_target->position(), escapePos)) {
            // 确保逃跑位置比当前位置更远离目标
            f64 currentDistSq = m_breeze->distanceSqTo(m_target->x(), m_target->y(), m_target->z());
            f64 newDistSq = static_cast<f64>((escapePos.x - m_target->x()) * (escapePos.x - m_target->x())) +
                static_cast<f64>((escapePos.z - m_target->z()) * (escapePos.z - m_target->z()));
            if (newDistSq > currentDistSq) {
                slideTarget = escapePos;
            } else {
                // 逃跑位置不够远，回退到目标身后
                slideTarget = _randomPointBehindTarget();
            }
        } else {
            slideTarget = _randomPointBehindTarget();
        }
    } else {
        // 不在内圈，随机选择目标身后或中圈
        math::Random& rng = m_breeze->world()->getRandom();
        if (rng.nextBoolean()) {
            slideTarget = _randomPointBehindTarget();
        } else {
            slideTarget = _randomPointInMiddleCircle();
        }
    }

    // 设置滑行状态和移动目标
    m_breeze->setSliding(true);

    // 切换到滑行姿态（参考 MC BreezeAi.SlideToTargetSink.start）
    m_breeze->setPose(EntityPose::Sliding);

    auto* moveCtrl = m_breeze->moveController();
    if (moveCtrl != nullptr) {
        moveCtrl->setMoveTo(static_cast<f64>(slideTarget.x),
            static_cast<f64>(slideTarget.y),
            static_cast<f64>(slideTarget.z),
            static_cast<f64>(SLIDE_SPEED));
    }

    // 播放滑行音效
    if (m_breeze->world() != nullptr) {
        m_breeze->world()->playSound(
            SoundEvents::ENTITY_BREEZE_SLIDE, sound::SoundCategory::Hostile, m_breeze->position(), 1.0f, 1.0f);
    }

    // 设置射击许可（滑行结束后可以射击）
    m_breeze->setShootPermit(SHOOT_PERMIT_TICKS);
}

void BreezeSlideGoal::resetTask()
{
    m_target = nullptr;
    if (m_breeze != nullptr) {
        m_breeze->setSliding(false);

        // 切换回站立姿态（参考 MC BreezeAi.SlideToTargetSink.stop）
        m_breeze->setPose(EntityPose::Standing);
    }
}

bool BreezeSlideGoal::_isWithinInnerCircle(const Vector3& targetPos) const
{
    if (m_breeze == nullptr) {
        return false;
    }

    // MC 原版：水平距离 < 4 格且垂直距离 < 10 格
    f64 dx = m_breeze->x() - static_cast<f64>(targetPos.x);
    f64 dz = m_breeze->z() - static_cast<f64>(targetPos.z);
    f64 horizontalDistSq = dx * dx + dz * dz;
    f64 dy = std::abs(m_breeze->y() - static_cast<f64>(targetPos.y));

    return horizontalDistSq < INNER_CIRCLE_HORIZONTAL * INNER_CIRCLE_HORIZONTAL && dy < INNER_CIRCLE_VERTICAL;
}

Vector3 BreezeSlideGoal::_randomPointBehindTarget() const
{
    if (m_breeze == nullptr || m_target == nullptr || m_breeze->world() == nullptr) {
        return m_breeze != nullptr ? m_breeze->position() : Vector3(0, 0, 0);
    }

    math::Random& rng = m_breeze->world()->getRandom();

    // MC 原版 BreezeUtil.randomPointBehindTarget：
    // 角度 = target.yHeadRot + 180 ± gaussian*45
    f64 angle = static_cast<f64>(m_target->rotationYawHead()) + 180.0 + static_cast<f64>(rng.nextGaussian()) * 45.0;
    angle = static_cast<f64>(math::wrapDegrees(static_cast<f32>(angle))) * static_cast<f64>(math::DEG_TO_RAD);

    // 距离 = lerp(random, 4, 8)
    f64 distance = math::lerp(rng.nextDouble(), BEHIND_TARGET_MIN, BEHIND_TARGET_MAX);

    f64 targetX = m_target->x() - std::sin(angle) * distance;
    f64 targetZ = m_target->z() + std::cos(angle) * distance;

    // 查找地面高度
    IWorld* world = m_breeze->world();
    i32 x = static_cast<i32>(std::floor(targetX));
    i32 z = static_cast<i32>(std::floor(targetZ));
    f32 y = static_cast<f32>(world->getHeight(x, z));

    return Vector3(static_cast<f32>(targetX), y, static_cast<f32>(targetZ));
}

Vector3 BreezeSlideGoal::_randomPointInMiddleCircle() const
{
    if (m_breeze == nullptr || m_target == nullptr) {
        return m_breeze != nullptr ? m_breeze->position() : Vector3(0, 0, 0);
    }

    math::Random& rng = m_breeze->world()->getRandom();

    // MC 原版 BreezeAi.randomPointInMiddleCircle：
    // 计算从旋风人到目标的方向向量
    f64 dx = m_target->x() - m_breeze->x();
    f64 dz = m_target->z() - m_breeze->z();
    f64 dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.01) {
        // 几乎重合，随机方向
        f64 angle = rng.nextDouble() * 2.0 * static_cast<f64>(math::PI);
        dx = std::cos(angle);
        dz = std::sin(angle);
        dist = 1.0;
    }

    // 沿方向移动 lerp(random, 4, 8) 距离
    f64 moveDistance = math::lerp(rng.nextDouble(), MIDDLE_CIRCLE_MIN, MIDDLE_CIRCLE_MAX);
    f64 normX = dx / dist;
    f64 normZ = dz / dist;

    f64 targetX = m_breeze->x() + normX * moveDistance;
    f64 targetZ = m_breeze->z() + normZ * moveDistance;

    // 查找地面高度
    IWorld* world = m_breeze->world();
    i32 x = static_cast<i32>(std::floor(targetX));
    i32 z = static_cast<i32>(std::floor(targetZ));
    f32 y = static_cast<f32>(world->getHeight(x, z));

    return Vector3(static_cast<f32>(targetX), y, static_cast<f32>(targetZ));
}

// ============================================================================
// BreezeShootWhenStuckGoal
// ============================================================================

BreezeShootWhenStuckGoal::BreezeShootWhenStuckGoal(BreezeEntity* breeze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_breeze(breeze)
{
    MC_ASSERT_RELEASE(breeze != nullptr);
}

bool BreezeShootWhenStuckGoal::shouldExecute()
{
    if (m_breeze == nullptr) {
        return false;
    }

    // 必须有攻击目标
    LivingEntity* target = m_breeze->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 不在吸气/跳跃状态
    if (m_breeze->isLongJumping()) {
        return false;
    }

    // 没有行走目标
    auto* moveCtrl = m_breeze->moveController();
    if (moveCtrl != nullptr && moveCtrl->isUpdating()) {
        return false;
    }

    // 没有射击许可
    if (m_breeze->hasShootPermit()) {
        return false;
    }

    // 处于困境：在水中、骑乘实体、或受到飘浮效果
    bool isInWater = m_breeze->isInWater();
    bool isRiding = m_breeze->isRiding();
    bool hasLevitation = m_breeze->hasEffect(entity::effect::EffectType::Levitation);

    return isInWater || isRiding || hasLevitation;
}

bool BreezeShootWhenStuckGoal::shouldContinueExecuting()
{
    // 一次性目标：启动后立即结束
    return false;
}

void BreezeShootWhenStuckGoal::startExecuting()
{
    // 设置射击许可，使 BreezeShootGoal 可以激活
    m_breeze->setShootPermit(SHOOT_PERMIT_TICKS);
}

} // namespace mc::entity::ai::goal
