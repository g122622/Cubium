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

#include "FrogLongJumpGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/entities/passive/frog/FrogEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace mc::entity::ai::goal {

namespace {
constexpr f64 PI = 3.14159265358979323846;
/// Cubium 重力常量（physics::GRAVITY，LivingEntity travel 每 tick 减去的值）
constexpr f64 GRAVITY = 0.08;
} // namespace

FrogLongJumpGoal::FrogLongJumpGoal(FrogEntity* frog)
    : m_frog(frog)
{
    // 长跳期间自主控制移动与朝向，占用 Move + Look。
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool FrogLongJumpGoal::shouldExecute()
{
    // 对齐 FrogAi LONG_JUMP 活动的条件：不在水中、不在求偶/被诱惑、冷却就绪。
    if (m_frog == nullptr || !m_frog->isAlive() || m_cooldown > 0) {
        return false;
    }
    if (!m_frog->onGround() || m_frog->isInWater() || m_frog->isBeingRidden()) {
        return false;
    }
    return true;
}

bool FrogLongJumpGoal::shouldContinueExecuting()
{
    if (m_frog == nullptr || !m_frog->isAlive()) {
        return false;
    }
    switch (m_phase) {
        case Phase::Preparing:
            return m_prepareTicks < PREPARE_JUMP_DURATION;
        case Phase::Jumping:
            return m_airTicks < TIME_OUT_DURATION;
        case Phase::Idle:
        default:
            return false;
    }
}

void FrogLongJumpGoal::startExecuting()
{
    if (m_frog == nullptr) {
        return;
    }
    m_phase = Phase::Idle;
    m_prepareTicks = 0;
    m_airTicks = 0;
    m_jumpVector = Vector3{0.0, 0.0, 0.0};
    if (auto* navigator = m_frog->navigator(); navigator != nullptr) {
        navigator->stop();
    }

    for (i32 tries = 0; tries < FIND_JUMP_TRIES && m_phase == Phase::Idle; ++tries) {
        _pickCandidate();
    }
    if (m_phase == Phase::Idle) {
        _setRandomCooldown();
    }
}

void FrogLongJumpGoal::tick()
{
    if (m_frog == nullptr) {
        return;
    }

    switch (m_phase) {
        case Phase::Preparing: {
            ++m_prepareTicks;
            if (m_prepareTicks < PREPARE_JUMP_DURATION) {
                return;
            }
            m_frog->setRotation(m_frog->yBodyRot(), m_frog->pitch());
            m_frog->setPose(EntityPose::LongJumping);
            m_frog->setVelocity(m_jumpVector);
            m_phase = Phase::Jumping;
            m_airTicks = 0;
            _playEntitySound("long_jump", 1.0f);
            return;
        }
        case Phase::Jumping: {
            ++m_airTicks;
            if (m_frog->onGround()) {
                const Vector3 velocity = m_frog->velocity();
                m_frog->setVelocity(
                    velocity.x * LANDING_HORIZONTAL_DAMPING, velocity.y, velocity.z * LANDING_HORIZONTAL_DAMPING);
                m_frog->setPose(EntityPose::Standing);
                _playEntitySound("step", 1.0f);
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

void FrogLongJumpGoal::resetTask()
{
    if (m_frog != nullptr && m_phase == Phase::Jumping) {
        m_frog->setPose(EntityPose::Standing);
    }
    m_phase = Phase::Idle;
    m_prepareTicks = 0;
    m_airTicks = 0;
    m_jumpVector = Vector3{0.0, 0.0, 0.0};
}

bool FrogLongJumpGoal::_isPreferredBlock(const BlockPos& pos) const
{
    if (m_frog == nullptr || m_frog->world() == nullptr) {
        return false;
    }
    // 对齐 BlockTags.FROG_PREFER_JUMP_TO = {lily_pad, big_dripleaf}
    const BlockState* state = m_frog->world()->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    const Block& block = state->getBlock();
    if (block_registry::NaturalBlocks::LILY_PAD != nullptr && &block == block_registry::NaturalBlocks::LILY_PAD) {
        return true;
    }
    return block_registry::CaveBlocks::BIG_DRIPLEAF != nullptr && &block == block_registry::CaveBlocks::BIG_DRIPLEAF;
}

bool FrogLongJumpGoal::_isAcceptableLandingSpot(const BlockPos& pos) const
{
    if (m_frog == nullptr || m_frog->world() == nullptr) {
        return false;
    }
    IWorld* world = m_frog->world();

    // 对齐 FrogAi.isAcceptableLandingSpot：落点、上方、下方三处均无流体。
    const BlockPos above(pos.x, pos.y + 1, pos.z);
    const BlockPos below(pos.x, pos.y - 1, pos.z);
    for (const BlockPos& check : {pos, above, below}) {
        const fluid::FluidState* fluidState = world->getFluidState(check);
        if (fluidState == nullptr || !fluidState->isEmpty()) {
            return false;
        }
    }

    // 落点或其下方为偏好方块（睡莲/垂滴叶）时直接接受。
    if (_isPreferredBlock(pos) || _isPreferredBlock(below)) {
        return true;
    }

    // 否则要求落点下方为实心方块（且落点本身不阻挡身体）。
    const BlockState* belowState = world->getBlockState(below);
    if (belowState == nullptr || !belowState->isSolid()) {
        return false;
    }
    const BlockState* at = world->getBlockState(pos);
    return at == nullptr || !at->isSolid();
}

void FrogLongJumpGoal::_pickCandidate()
{
    if (m_frog == nullptr || m_frog->world() == nullptr) {
        return;
    }

    const BlockPos origin(static_cast<f64>(m_frog->x()), static_cast<f64>(m_frog->y()), static_cast<f64>(m_frog->z()));

    // 对齐 LongJumpToPreferredBlock.start：每次有 50% 概率只优先考虑偏好方块落点。
    math::IRandom& rng = m_frog->getRandom();
    const bool wantPreferred = rng.nextFloat() < PREFERRED_BLOCK_CHANCE;

    std::vector<BlockPos> preferred;
    std::vector<BlockPos> others;
    for (i32 dx = -MAX_JUMP_WIDTH; dx <= MAX_JUMP_WIDTH; ++dx) {
        for (i32 dy = -MAX_JUMP_HEIGHT; dy <= MAX_JUMP_HEIGHT; ++dy) {
            for (i32 dz = -MAX_JUMP_WIDTH; dz <= MAX_JUMP_WIDTH; ++dz) {
                const BlockPos candidate(origin.x + dx, origin.y + dy, origin.z + dz);
                if (candidate == origin || (candidate.x == origin.x && candidate.z == origin.z)) {
                    continue;
                }
                if (!_isAcceptableLandingSpot(candidate)) {
                    continue;
                }
                if (_isPreferredBlock(candidate) ||
                    _isPreferredBlock(BlockPos(candidate.x, candidate.y - 1, candidate.z))) {
                    preferred.push_back(candidate);
                } else {
                    others.push_back(candidate);
                }
            }
        }
    }

    const std::vector<BlockPos>& pool = (wantPreferred && !preferred.empty()) ? preferred : others;
    if (pool.empty()) {
        return;
    }

    const BlockPos target = pool[static_cast<std::size_t>(rng.nextInt(static_cast<i32>(pool.size())))];

    // 若寻路可直达则不必跳（对齐 createPath(..., 8).canReach() 的"能走过去就不跳"语义）。
    if (auto* navigator = m_frog->navigator(); navigator != nullptr) {
        const bool reachable = navigator->moveTo(
            static_cast<f64>(target.x) + 0.5, static_cast<f64>(target.y), static_cast<f64>(target.z) + 0.5, 1.0);
        navigator->stop();
        if (reachable) {
            return;
        }
    }

    const f64 jumpStrength = m_frog->attributes().getValue(entity::attribute::Attributes::JUMP_STRENGTH);
    const f64 maxVelocity = jumpStrength * MAX_JUMP_VELOCITY_MULTIPLIER;
    const Vector3 from(m_frog->x(), m_frog->y(), m_frog->z());
    const Vector3 to(static_cast<f64>(target.x) + 0.5, static_cast<f64>(target.y), static_cast<f64>(target.z) + 0.5);

    std::vector<i32> angles(std::begin(ALLOWED_ANGLES), std::end(ALLOWED_ANGLES));
    for (i32 i = static_cast<i32>(angles.size()) - 1; i > 0; --i) {
        const i32 j = rng.nextInt(i + 1);
        std::swap(angles[static_cast<std::size_t>(i)], angles[static_cast<std::size_t>(j)]);
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

std::optional<Vector3> FrogLongJumpGoal::calculateJumpVector(
    const Vector3& from, const Vector3& target, f64 maxVelocity, i32 angleDeg) const
{
    // 与 vanilla LongJumpUtil.calculateJumpVectorForAngle 一致（详见 GoatLongJumpGoal 同名实现）。
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

    const f64 sinDoubleAngle = std::sin(2.0 * angle);
    const f64 cosSq = std::cos(angle) * std::cos(angle);
    const f64 denominator = horizontalDist * sinDoubleAngle - 2.0 * dy * cosSq;
    if (std::abs(denominator) < 1.0e-9) {
        return std::nullopt;
    }
    const f64 speedSq = horizontalSq * GRAVITY / denominator;
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

void FrogLongJumpGoal::_setRandomCooldown()
{
    m_cooldown = COOLDOWN_MIN_TICKS +
        (m_frog != nullptr ? m_frog->getRandom().nextInt(COOLDOWN_MAX_TICKS - COOLDOWN_MIN_TICKS + 1) : 0);
}

void FrogLongJumpGoal::_playEntitySound(const char* suffix, f32 pitch) const
{
    if (m_frog == nullptr) {
        return;
    }
    // vanilla：起跳 FROG_LONG_JUMP、落地 FROG_STEP，均以 entity.frog.<suffix> 命名。
    std::string name = "minecraft:entity.frog.";
    name += suffix;
    m_frog->playSound(ResourceLocation(name), 1.0f, pitch);
}

} // namespace mc::entity::ai::goal
