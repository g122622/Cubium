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

#include "GoatLongJumpGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/special/GoatEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"

#include <string>

#include <algorithm>
#include <cmath>
#include <vector>

namespace mc::entity::ai::goal {

namespace {
/// 弧度换算（vanilla 抛射角以度为单位参与三角运算）
constexpr f64 PI = 3.14159265358979323846;
} // namespace

GoatLongJumpGoal::GoatLongJumpGoal(GoatEntity* goat)
    : m_goat(goat)
{
    // 长跳期间自主控制移动与朝向，占用 Move + Look。
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool GoatLongJumpGoal::shouldExecute()
{
    // vanilla LongJumpToRandomPos.checkExtraStartConditions：
    //   着地 && 不在水中 && 不在岩浆中 && 脚下不是蜂蜜块。
    if (m_goat == nullptr || !m_goat->isAlive() || m_cooldown > 0) {
        return false;
    }
    if (!m_goat->onGround() || m_goat->isInWater() || m_goat->isInLava()) {
        return false;
    }
    if (m_goat->isBeingRidden()) {
        return false;
    }
    return true;
}

bool GoatLongJumpGoal::shouldContinueExecuting()
{
    if (m_goat == nullptr || !m_goat->isAlive()) {
        return false;
    }
    switch (m_phase) {
        case Phase::Preparing:
            return m_prepareTicks < PREPARE_JUMP_DURATION;
        case Phase::Jumping:
            // vanilla LongJumpMidJump.canStillUse：直到落地为止（且有超时保护）。
            return m_airTicks < TIME_OUT_DURATION;
        case Phase::Idle:
        default:
            return false;
    }
}

void GoatLongJumpGoal::startExecuting()
{
    if (m_goat == nullptr) {
        return;
    }
    m_phase = Phase::Idle;
    m_prepareTicks = 0;
    m_airTicks = 0;
    m_jumpVector = Vector3{0.0, 0.0, 0.0};
    if (auto* navigator = m_goat->navigator(); navigator != nullptr) {
        navigator->stop();
    }

    // 每 tick 尝试挑选一个可跳落点（对齐 vanilla tick 中 findJumpTries-- 后 pickCandidate）。
    for (i32 tries = 0; tries < FIND_JUMP_TRIES && m_phase == Phase::Idle; ++tries) {
        _pickCandidate();
    }
    if (m_phase == Phase::Idle) {
        // 20 次都挑不出落点：直接进入冷却，避免每 tick 重复全量搜索。
        _setRandomCooldown();
    }
}

void GoatLongJumpGoal::tick()
{
    if (m_goat == nullptr) {
        return;
    }

    switch (m_phase) {
        case Phase::Preparing: {
            ++m_prepareTicks;
            if (m_prepareTicks < PREPARE_JUMP_DURATION) {
                return;
            }
            // vanilla LongJumpToRandomPos.tick：蓄力结束后按"朝向 = 身体朝向"起跳，
            // 初速度 = chosenJump 归一化后乘以（长度 + 跳跃加成）。
            m_goat->setRotation(m_goat->yBodyRot(), m_goat->pitch());
            m_goat->setPose(EntityPose::LongJumping);
            m_goat->setVelocity(m_jumpVector);
            m_phase = Phase::Jumping;
            m_airTicks = 0;
            _playEntitySound("long_jump");
            return;
        }
        case Phase::Jumping: {
            ++m_airTicks;
            if (m_goat->onGround()) {
                // vanilla LongJumpMidJump.stop：落地时水平速度乘 0.1、播放落地音效。
                const Vector3 velocity = m_goat->velocity();
                m_goat->setVelocity(
                    velocity.x * LANDING_HORIZONTAL_DAMPING, velocity.y, velocity.z * LANDING_HORIZONTAL_DAMPING);
                m_goat->setPose(EntityPose::Standing);
                _playEntitySound("step");
                _setRandomCooldown();
                m_phase = Phase::Idle;
            }
            return;
        }
        case Phase::Idle:
        default:
            return;
    }
}

void GoatLongJumpGoal::resetTask()
{
    // 空中被打断时恢复站立姿态（落地分支已自行收尾）。
    if (m_goat != nullptr && m_phase == Phase::Jumping) {
        m_goat->setPose(EntityPose::Standing);
    }
    m_phase = Phase::Idle;
    m_prepareTicks = 0;
    m_airTicks = 0;
    m_jumpVector = Vector3{0.0, 0.0, 0.0};
}

bool GoatLongJumpGoal::_isAcceptableLandingSpot(const BlockPos& pos) const
{
    if (m_goat == nullptr || m_goat->world() == nullptr) {
        return false;
    }
    IWorld* world = m_goat->world();

    // 对齐 LongJumpToRandomPos.defaultAcceptableLandingSpot：
    //   落点下方方块须为实心渲染方块。
    const BlockState* below = world->getBlockState(BlockPos(pos.x, pos.y - 1, pos.z));
    if (below == nullptr || !below->isSolid()) {
        return false;
    }

    // vanilla 还要求 getPathfindingMalus(...) == 0（即原版寻路可站立）。Cubium 无等价的
    // 静态代价查询，用"落点及其上方一格为空（可容纳山羊身体）"近似，避免选中嵌入方块的落点。
    const BlockState* at = world->getBlockState(pos);
    if (at != nullptr && at->isSolid()) {
        return false;
    }
    return true;
}

void GoatLongJumpGoal::_pickCandidate()
{
    if (m_goat == nullptr || m_goat->world() == nullptr) {
        return;
    }

    const BlockPos origin(static_cast<f64>(m_goat->x()), static_cast<f64>(m_goat->y()), static_cast<f64>(m_goat->z()));

    // 1. 收集周围立方体内的候选方块（对齐 Start 的 betweenClosedStream 收集）。
    std::vector<BlockPos> candidates;
    for (i32 dx = -MAX_JUMP_RADIUS; dx <= MAX_JUMP_RADIUS; ++dx) {
        for (i32 dy = -MAX_JUMP_RADIUS; dy <= MAX_JUMP_RADIUS; ++dy) {
            for (i32 dz = -MAX_JUMP_RADIUS; dz <= MAX_JUMP_RADIUS; ++dz) {
                const BlockPos candidate(origin.x + dx, origin.y + dy, origin.z + dz);
                if (candidate == origin) {
                    continue;
                }
                if (_isAcceptableLandingSpot(candidate)) {
                    candidates.push_back(candidate);
                }
            }
        }
    }
    if (candidates.empty()) {
        return;
    }

    // 2. 随机抽取（vanilla 按 distSqr 权重；此处等权抽取，选中的落点本身仍受距离与可达性约束）。
    math::IRandom& rng = m_goat->getRandom();
    const i32 index = rng.nextInt(static_cast<i32>(candidates.size()));
    const BlockPos target = candidates[static_cast<size_t>(index)];

    // 3. 必须与本体不同列（vanilla isAcceptableLandingPosition 排除同 x/z）。
    if (target.x == origin.x && target.z == origin.z) {
        return;
    }

    // 4. 若寻路可直达（距离 <= 8 且可达），则不必跳；vanilla 用 createPath(blockpos, 0, 8).canReach() 判定。
    if (auto* navigator = m_goat->navigator(); navigator != nullptr) {
        // vanilla 用 createPath(blockpos, 0, 8).canReach() 判定"能走过去就不用跳"；
        // Cubium 以 moveTo 的返回值（是否找到路径）等价近似。
        const bool reachable = navigator->moveTo(
            static_cast<f64>(target.x) + 0.5, static_cast<f64>(target.y), static_cast<f64>(target.z) + 0.5, 1.0);
        navigator->stop();
        if (reachable) {
            return;
        }
    }

    // 5. 计算抛射初速度（vanilla calculateOptimalJumpVector：打乱角度顺序后逐个尝试）。
    const f64 jumpStrength = m_goat->attributes().getValue(entity::attribute::Attributes::JUMP_STRENGTH);
    const f64 maxVelocity = jumpStrength * MAX_JUMP_VELOCITY_MULTIPLIER;
    const Vector3 from(m_goat->x(), m_goat->y(), m_goat->z());
    const Vector3 to(static_cast<f64>(target.x) + 0.5, static_cast<f64>(target.y), static_cast<f64>(target.z) + 0.5);

    std::vector<i32> angles(std::begin(ALLOWED_ANGLES), std::end(ALLOWED_ANGLES));
    for (i32 i = static_cast<i32>(angles.size()) - 1; i > 0; --i) {
        const i32 j = rng.nextInt(i + 1);
        std::swap(angles[static_cast<size_t>(i)], angles[static_cast<size_t>(j)]);
    }
    for (const i32 angle : angles) {
        const std::optional<Vector3> vector = calculateJumpVector(from, to, maxVelocity, angle);
        if (vector.has_value()) {
            m_target = target;
            m_jumpVector = *vector;
            m_prepareTicks = 0;
            m_phase = Phase::Preparing;
            return;
        }
    }
}

std::optional<Vector3> GoatLongJumpGoal::calculateJumpVector(
    const Vector3& from, const Vector3& target, f64 maxVelocity, i32 angleDeg) const
{
    // 对齐 vanilla LongJumpUtil.calculateJumpVectorForAngle。
    // 落点中心沿水平方向回退 0.5 格（vec32 = target - normalize(dir).scale(0.5)）。
    const f64 dxRaw = target.x - from.x;
    const f64 dzRaw = target.z - from.z;
    const f64 horizontalLength = std::sqrt(dxRaw * dxRaw + dzRaw * dzRaw);
    if (horizontalLength < 1.0e-7) {
        return std::nullopt;
    }
    const f64 dirX = dxRaw / horizontalLength * 0.5;
    const f64 dirZ = dzRaw / horizontalLength * 0.5;

    const f64 dx = target.x - dirX - from.x;
    const f64 dy = target.y - from.y;
    const f64 dz = target.z - dirZ - from.z;

    const f64 angle = static_cast<f64>(angleDeg) * PI / 180.0;
    const f64 azimuth = std::atan2(dz, dx);
    const f64 horizontalSq = dx * dx + dz * dz;
    const f64 horizontalDist = std::sqrt(horizontalSq);
    if (horizontalDist < 1.0e-7) {
        return std::nullopt;
    }

    // Cubium 的重力常量（physics::GRAVITY，需求 LivingEntity 在 travel 中逐 tick 减去的值）。
    const f64 gravity = 0.08;
    const f64 sinDoubleAngle = std::sin(2.0 * angle);
    const f64 cosSq = std::cos(angle) * std::cos(angle);
    const f64 denominator = horizontalDist * sinDoubleAngle - 2.0 * dy * cosSq;
    if (std::abs(denominator) < 1.0e-9) {
        return std::nullopt;
    }
    const f64 speedSq = horizontalSq * gravity / denominator;
    if (speedSq < 0.0) {
        return std::nullopt;
    }
    const f64 speed = std::sqrt(speedSq);
    if (speed > maxVelocity) {
        return std::nullopt;
    }

    const f64 horizontalSpeed = speed * std::cos(angle);
    const f64 verticalSpeed = speed * std::sin(angle);
    const f64 sinAzimuth = std::sin(azimuth);
    const f64 cosAzimuth = std::cos(azimuth);
    return Vector3(horizontalSpeed * cosAzimuth * JUMP_VECTOR_SCALE,
        verticalSpeed * JUMP_VECTOR_SCALE,
        horizontalSpeed * sinAzimuth * JUMP_VECTOR_SCALE);
}

void GoatLongJumpGoal::_playEntitySound(const char* suffix) const
{
    if (m_goat == nullptr) {
        return;
    }
    // 对齐 vanilla：起跳播放 GOAT_LONG_JUMP（尖叫山羊为 GOAT_SCREAMING_LONG_JUMP），
    // 落地播放 GOAT_STEP；两者音效名都以 entity.goat.<suffix> 为前缀。
    std::string name = "minecraft:entity.goat.";
    if (std::string(suffix) == "long_jump" && m_goat->isScreamingGoat()) {
        name = "minecraft:entity.goat.screaming.long_jump";
    } else {
        name += suffix;
    }
    m_goat->playSound(ResourceLocation(name), 1.0f, 1.0f);
}

void GoatLongJumpGoal::_setRandomCooldown()
{
    m_cooldown = COOLDOWN_MIN_TICKS +
        (m_goat != nullptr ? m_goat->getRandom().nextInt(COOLDOWN_MAX_TICKS - COOLDOWN_MIN_TICKS + 1) : 0);
}

} // namespace mc::entity::ai::goal