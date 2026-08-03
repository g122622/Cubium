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
    math::Random& rng = m_ghast->getRandom();
    m_courseChangeCooldown = rng.nextInt(5) + 2; // 2-6 ticks

    // 计算到目标位置的向量
    f64 dx = m_posX - m_ghast->x();
    f64 dy = m_posY - m_ghast->y();
    f64 dz = m_posZ - m_ghast->z();
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
        // 路径安全，添加速度
        Vector3 velocity = m_ghast->velocity();
        velocity.x += direction.x * 0.1f;
        velocity.y += direction.y * 0.1f;
        velocity.z += direction.z * 0.1f;
        m_ghast->setVelocity(velocity);
    } else {
        // 路径不安全，停止移动
        m_action = MoveAction::Wait;
    }
}

bool GhastMovementController::_isPathSafe(const Vector3f& direction, i32 distance) const
{
    // 检查从当前位置到目标位置的路径是否安全
    IWorld* world = m_ghast->world();
    if (world == nullptr) {
        return false;
    }

    // 获取恶魂的碰撞箱
    AxisAlignedBB currentBox = m_ghast->boundingBox();

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
