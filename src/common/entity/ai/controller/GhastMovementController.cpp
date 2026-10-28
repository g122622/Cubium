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

#include "GhastMovementController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/passive/happyghast/HappyGhastEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

GhastMovementController::GhastMovementController(GhastEntity* ghast)
    : MovementController(ghast)
    , m_ghast(ghast)
    , m_courseChangeCooldown(0)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

GhastMovementController::GhastMovementController(HappyGhastEntity* ghast)
    : MovementController(ghast)
    , m_ghast(nullptr)
    , m_happyGhast(ghast)
    , m_courseChangeCooldown(0)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

/// 当前宿主实体（恶魂或快乐恶魂，二者互斥）
MobEntity* GhastMovementController::_host() const
{
    return m_ghast != nullptr ? static_cast<MobEntity*>(m_ghast) : static_cast<MobEntity*>(m_happyGhast);
}

void GhastMovementController::tick()
{
    if (m_action != MoveAction::MoveTo) {
        return;
    }

    if (m_courseChangeCooldown > 0) {
        --m_courseChangeCooldown;
        return;
    }

    // 每次更新后添加随机冷却，避免频繁调整
    math::IRandom& rng = _host()->getRandom();
    m_courseChangeCooldown = rng.nextInt(5) + 2; // 2-6 ticks

    // 计算到目标位置的向量
    f64 dx = m_posX - _host()->x();
    f64 dy = m_posY - _host()->y();
    f64 dz = m_posZ - _host()->z();
    f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance < 0.0001) {
        // 已到达目标，停止移动
        m_action = MoveAction::Wait;
        return;
    }

    // 归一化方向向量
    Vector3f direction(
        static_cast<f32>(dx / distance), static_cast<f32>(dy / distance), static_cast<f32>(dz / distance));

    // 检查飞行路径是否安全（避免碰撞）
    i32 stepsToCheck = static_cast<i32>(std::ceil(distance));
    if (_isPathSafe(direction, stepsToCheck)) {
        // 路径安全，添加加速度。
        //
        // 对齐 vanilla Ghast.travel → travelFlying(vec, 0.02F) 的速度收敛行为：
        // 原版每 tick 对速度施加阻尼（等效于速度趋近一个有限终速）。此前这里只做
        // velocity += direction * 0.1 而没有任何阻尼/上限，速度会无限累积：实测恶魂
        // 从 ghast_arena 内的 (3,2,3) 一路加速飞出到 (84.9,-9.7,52.8)，既撞不到
        // 竞技场墙壁的约束效果，也离玩家越来越远，永远看不到目标、
        // 于是从不发射火球（ghast_shoots_fireball_at_player 失败）。
        //
        // 这里按 travelFlying 的终速给出硬上限：每 tick 最大位移约 0.1 格（与加速度同量级），
        // 既保留原版的「缓慢漂浮」手感，又不让速度无界增长。
        constexpr f32 kMaxSpeed = 0.1f;
        Vector3 velocity = _host()->velocity();
        velocity.x += direction.x * 0.1f;
        velocity.y += direction.y * 0.1f;
        velocity.z += direction.z * 0.1f;

        // 钳制到终速上限（原版 travelFlying 的阻尼等价效果）。
        const f32 speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
        if (speed > kMaxSpeed) {
            const f32 scale = kMaxSpeed / speed;
            velocity.x *= scale;
            velocity.y *= scale;
            velocity.z *= scale;
        }

        _host()->setVelocity(velocity);
    } else {
        // 路径不安全，停止移动
        m_action = MoveAction::Wait;
    }
}

bool GhastMovementController::_isPathSafe(const Vector3f& direction, i32 distance) const
{
    // 检查从当前位置到目标位置的路径是否安全
    IWorld* world = _host()->world();
    if (world == nullptr) {
        return false;
    }

    // 获取恶魂的碰撞箱
    AxisAlignedBB currentBox = _host()->boundingBox();

    // 沿路径逐步检查碰撞
    for (i32 i = 1; i <= distance; ++i) {
        // 计算下一步的碰撞箱位置
        AxisAlignedBB nextBox = currentBox;
        nextBox.offset(direction.x, direction.y, direction.z);

        // 检查是否有碰撞
        if (!world->hasNoCollisions(nextBox)) {
            return false; // 路径不安全
        }

        // 更新当前位置用于下一次检查
        currentBox = nextBox;
    }

    return true; // 路径安全
}

} // namespace mc::entity::ai::controller
