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

#include "GoatRamGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/special/GoatEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"

#include <algorithm>
#include <cmath>

namespace mc::entity::ai::goal {

namespace {
/// 整体超时（对齐 vanilla RamTarget 的 Behavior 超时时长 200 tick）
constexpr i32 RAM_TIMEOUT_TICKS = 200;
/// 击退衰减（护盾格挡后击退减半，vanilla `f4 = f3 > 0 ? 0.5 : 1.0`）
constexpr f32 BLOCKED_KNOCKBACK_FACTOR = 0.5f;
/// 冲撞速度上限（vanilla Mth.clamp(getSpeed() * 1.65F, 0.2F, 3.0F)）
constexpr f32 RAM_SPEED_CLAMP_MIN = 0.2f;
constexpr f32 RAM_SPEED_CLAMP_MAX = 3.0f;
constexpr f32 RAM_SPEED_FORCE_FACTOR = 1.65f;
/// 撞上可撞碎方块后的反弹速度（vanilla RamTarget：0.35 倍反向）
constexpr f32 RAM_BOUNCE_SPEED = 0.35f;
} // namespace

GoatRamGoal::GoatRamGoal(GoatEntity* goat)
    : m_goat(goat)
{
    // 冲撞期间自带移动与朝向，占用 Move + Look，避免漫步/注视目标抢占。
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool GoatRamGoal::shouldExecute()
{
    if (m_goat == nullptr || !m_goat->isAlive()) {
        return false;
    }
    // 冷却未结束 / 正在求爱或被玩家诱惑时不冲撞（对齐 vanilla RAM 活动的启动条件）。
    if (m_goat->ramCooldown() > 0 || m_goat->isInLove()) {
        return false;
    }
    const IWorld* worldPtr = m_goat->world();
    if (worldPtr == nullptr) {
        return false;
    }

    // 对齐 vanilla RAM_TARGET_CONDITIONS：非山羊、可被攻击（排除旁观者与同载具）。
    LivingEntity* candidate = EntityUtils::findClosestEntity<LivingEntity>(const_cast<IWorld*>(worldPtr),
        m_goat->position(),
        static_cast<f32>(GoatEntity::RAM_MAX_DISTANCE),
        m_goat,
        [](LivingEntity* entity) -> bool {
            if (entity == nullptr || !entity->isAlive()) {
                return false;
            }
            if (dynamic_cast<const GoatEntity*>(entity) != nullptr) {
                return false;
            }
            return entity->canBeHitByProjectile();
        });
    if (candidate == nullptr) {
        return false;
    }

    m_target = candidate;
    m_targetPos = candidate->onPos();
    return _chooseRamStartPosition();
}

bool GoatRamGoal::shouldContinueExecuting()
{
    if (m_goat == nullptr || !m_goat->isAlive() || m_timeoutTicks <= 0) {
        return false;
    }
    LivingEntity* target = m_target.get();
    return target != nullptr && target->isAlive();
}

void GoatRamGoal::startExecuting()
{
    m_phase = Phase::Preparing;
    m_prepareTicks = 0;
    m_timeoutTicks = RAM_TIMEOUT_TICKS;
    if (auto* nav = m_goat->navigator()) {
        (void)nav->moveTo(m_ramStartPos.x + 0.5, m_ramStartPos.y, m_ramStartPos.z + 0.5, GoatEntity::RAM_PREPARE_SPEED);
    }
}

void GoatRamGoal::resetTask()
{
    m_phase = Phase::Idle;
    m_target = nullptr;
    m_prepareTicks = 0;
    if (m_goat != nullptr) {
        m_goat->setLoweringHead(false);
        if (auto* nav = m_goat->navigator()) {
            nav->stop();
        }
        // 冲撞结束后进入冷却（vanilla RamTarget.finishRam / PrepareRamNearestTarget.stop）。
        if (m_goat->ramCooldown() <= 0) {
            m_goat->setRamCooldown(m_goat->pickNextRamCooldown());
        }
    }
}

void GoatRamGoal::tick()
{
    if (m_goat == nullptr) {
        return;
    }
    --m_timeoutTicks;

    LivingEntity* target = m_target.get();
    if (target == nullptr) {
        return;
    }

    switch (m_phase) {
        case Phase::Preparing: {
            // 目标换了方块 → 重新选起跑点（对齐 vanilla PrepareRamNearestTarget.tick）。
            const BlockPos currentTargetPos = target->onPos();
            if (currentTargetPos.x != m_targetPos.x || currentTargetPos.y != m_targetPos.y ||
                currentTargetPos.z != m_targetPos.z) {
                m_targetPos = currentTargetPos;
                if (!_chooseRamStartPosition()) {
                    m_timeoutTicks = 0;
                    return;
                }
                if (auto* nav = m_goat->navigator()) {
                    (void)nav->moveTo(
                        m_ramStartPos.x + 0.5, m_ramStartPos.y, m_ramStartPos.z + 0.5, GoatEntity::RAM_PREPARE_SPEED);
                }
                return;
            }

            const BlockPos goatPos = m_goat->onPos();
            const bool atStart =
                goatPos.x == m_ramStartPos.x && goatPos.y == m_ramStartPos.y && goatPos.z == m_ramStartPos.z;
            if (!atStart) {
                return;
            }

            // 到达起跑点：低头蓄力，满 RAM_PREPARE_TIME 后加速冲过去。
            m_goat->setLoweringHead(true);
            if (auto* nav = m_goat->navigator()) {
                nav->stop();
            }
            if (++m_prepareTicks < GoatEntity::RAM_PREPARE_TIME) {
                return;
            }

            // 对齐 vanilla RamTarget.start：方向 = 起跑点 → 目标边缘（水平单位向量）。
            const Vector3 startCenter(m_ramStartPos.x + 0.5, static_cast<f64>(m_ramStartPos.y), m_ramStartPos.z + 0.5);
            Vector3 toTarget(target->x() - startCenter.x, 0.0, target->z() - startCenter.z);
            if (toTarget.length() <= 1.0e-6) {
                toTarget = Vector3(1.0, 0.0, 0.0);
            }
            m_ramDirection = toTarget.normalized();
            m_phase = Phase::Ramming;
            if (auto* nav = m_goat->navigator()) {
                (void)nav->moveTo(target->x(), target->y(), target->z(), GoatEntity::RAM_SPEED_MULTIPLIER);
            }
            // 冲撞阶段保持低头，直到结算（对齐实体事件 58 开 / 59 关）。
            m_goat->setLoweringHead(true);
            return;
        }
        case Phase::Ramming: {
            // 命中结算：目标进入碰撞箱即结算伤害与击退。
            if (m_goat->distanceTo(*target) <= 2.0) {
                _finishRam();
                return;
            }
            // 撞到 `#snaps_goat_horn` 方块：掉落山羊角并结束。
            if (m_goat->world() != nullptr) {
                const Vector3 ahead(m_goat->x() + m_ramDirection.x, m_goat->y(), m_goat->z() + m_ramDirection.z);
                const BlockPos aheadPos(static_cast<i32>(std::floor(ahead.x)),
                    static_cast<i32>(std::floor(ahead.y)),
                    static_cast<i32>(std::floor(ahead.z)));
                const BlockState* state = m_goat->world()->getBlockState(aheadPos);
                // 对齐 vanilla RamTarget.tick：撞上 #snaps_goat_horn 方块时掉落山羊角并以
                // 0.35 倍速反向弹开（至此冲撞结束）。
                if (state != nullptr && BlockTags::SNAPS_GOAT_HORN().contains(*state)) {
                    if (m_goat->dropHorn()) {
                        m_goat->setVelocity(-m_ramDirection.x * RAM_BOUNCE_SPEED,
                            m_goat->velocity().y,
                            -m_ramDirection.z * RAM_BOUNCE_SPEED);
                    }
                    m_goat->setLoweringHead(false);
                    m_phase = Phase::Idle;
                    m_goat->setRamCooldown(m_goat->pickNextRamCooldown());
                }
            }
            return;
        }
        case Phase::Idle:
            break;
    }
}

LivingEntity* GoatRamGoal::target() const
{
    return m_target.get();
}

bool GoatRamGoal::_chooseRamStartPosition()
{
    // 对齐 vanilla PrepareRamNearestTarget.calculateRammingStartPosition：
    // 从目标所在方块沿四个水平方向直线延伸 1..RAM_MAX_DISTANCE，取曼哈顿距离 >= RAM_MIN_DISTANCE
    // 的可行走点，再按离山羊最近的顺序取第一个可到达的。
    LivingEntity* target = m_target.get();
    if (target == nullptr || m_goat == nullptr) {
        return false;
    }
    const BlockPos targetPos = target->onPos();

    struct Candidate {
        BlockPos pos;
        f64 distanceSq;
    };

    std::vector<Candidate> candidates;

    constexpr Direction HORIZONTAL[4] = {Direction::North, Direction::South, Direction::East, Direction::West};
    for (const Direction direction : HORIZONTAL) {
        for (i32 i = 1; i <= GoatEntity::RAM_MAX_DISTANCE; ++i) {
            const BlockPos pos = targetPos.offset(direction, i);
            // 对齐 vanilla isWalkableBlock：目的地必须可站立（脚下实心、身体空间空）。
            if (!_isWalkableBlock(pos)) {
                break;
            }
            const i32 manhattan = std::abs(pos.x - targetPos.x) + std::abs(pos.z - targetPos.z);
            if (manhattan < GoatEntity::RAM_MIN_DISTANCE) {
                continue;
            }
            const f64 dx = pos.x - m_goat->onPos().x;
            const f64 dz = pos.z - m_goat->onPos().z;
            candidates.push_back({pos, dx * dx + dz * dz});
        }
    }
    if (candidates.empty()) {
        return false;
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.distanceSq < b.distanceSq;
    });
    m_ramStartPos = candidates.front().pos;
    return true;
}

bool GoatRamGoal::_isWalkableBlock(const BlockPos& pos) const
{
    // TODO(pathwalk-loading): vanilla 用导航器的 isStableDestination + 路径代价判定；
    // Cubium 此处退化为"脚下是实心方块、身体所在两格为空气"的静态近似。
    const IWorld* worldPtr = m_goat->world();
    if (worldPtr == nullptr) {
        return false;
    }
    const BlockState* below = worldPtr->getBlockState(pos.x, pos.y - 1, pos.z);
    const BlockState* feet = worldPtr->getBlockState(pos.x, pos.y, pos.z);
    const BlockState* head = worldPtr->getBlockState(pos.x, pos.y + 1, pos.z);
    if (below == nullptr || feet == nullptr || head == nullptr) {
        return false;
    }
    return below->isSolid() && !feet->isSolid() && !head->isSolid();
}

void GoatRamGoal::_finishRam()
{
    LivingEntity* target = m_target.get();
    if (target == nullptr) {
        return;
    }

    // 对齐 vanilla RamTarget.tick 命中分支：
    //   先以 MobAttackNoAggro（不激怒）结算攻击伤害，再按当前速度计算击退力度。
    EntityDamageSource noAggro = DamageSources::mobAttackNoAggro(m_goat);
    const f32 attackDamage = static_cast<f32>(m_goat->getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE));
    (void)target->hurt(noAggro, attackDamage);

    const f32 ramSpeed =
        std::clamp(m_goat->aiMoveSpeed() * RAM_SPEED_FORCE_FACTOR, RAM_SPEED_CLAMP_MIN, RAM_SPEED_CLAMP_MAX);
    const f32 knockbackForce =
        m_goat->isChild() ? GoatEntity::BABY_RAM_KNOCKBACK_FORCE : GoatEntity::ADULT_RAM_KNOCKBACK_FORCE;
    target->applyKnockback(ramSpeed * knockbackForce, m_ramDirection.x, m_ramDirection.z);

    (void)BLOCKED_KNOCKBACK_FACTOR;
    m_goat->setLoweringHead(false);
    m_timeoutTicks = 0;
}

} // namespace mc::entity::ai::goal