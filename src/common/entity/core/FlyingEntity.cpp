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

#include "FlyingEntity.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../attribute/Attributes.hpp"
#include "MoverType.hpp"
#include <cmath>

namespace mc {

FlyingEntity::FlyingEntity(EntityInstanceId id)
    : MobEntity(id)
{
    // 飞行生物默认不受重力影响
    // 重力由 hasGravity() 返回 false 来控制
}

void FlyingEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 飞行实体的移动逻辑，分为三种情况：
    // 1. 在水中
    // 2. 在岩浆中
    // 3. 正常飞行（空中/地面）

    if (isInWater()) {
        // ========== 在水中 ==========
        // 使用固定的低加速因子，忽略地面状态和滑度
        moveRelative(physics::SWIM_SPEED_BASE, strafing, vertical, forward);

        // 执行移动（带碰撞检测）
        move(entity::MoverType::Self, m_velocity);

        // 水中阻力：保留 80% 速度
        scaleVelocity(physics::WATER_DRAG);

    } else if (isInLava()) {
        // ========== 在岩浆中 ==========
        // 使用固定的低加速因子，与水中相同
        moveRelative(physics::LAVA_SWIM_SPEED, strafing, vertical, forward);

        // 执行移动（带碰撞检测）
        move(entity::MoverType::Self, m_velocity);

        // 岩浆阻力：保留 50% 速度（比水更粘稠）
        scaleVelocity(physics::LAVA_DRAG);

    } else {
        // ========== 正常飞行（空中/地面）==========

        // 获取脚下方块的滑度
        f32 slipperiness = physics::SLIPPERINESS_DEFAULT; // 默认滑度 0.6

        if (m_onGround && m_world != nullptr) {
            BlockPos groundPos(static_cast<i32>(std::floor(m_position.x)),
                static_cast<i32>(std::floor(m_position.y - 1.0)),
                static_cast<i32>(std::floor(m_position.z)));
            const BlockState* blockState = m_world->getBlockState(groundPos);
            if (blockState != nullptr) {
                slipperiness = blockState->getBlock().getSlipperiness(*blockState, m_world, &groundPos, this);
            }
        }

        // 计算摩擦因子
        f32 frictionFactor = m_onGround ? slipperiness * 0.91f : 0.91f;

        // 计算加速因子修正值
        // 这个公式使得在不同滑度的地面上有相同的加速度
        // 对于标准滑度 0.6，f = 0.546，f³ ≈ 0.1628，f1 ≈ 1.0
        f32 frictionCubed = frictionFactor * frictionFactor * frictionFactor;
        f32 accelerationCorrection = 0.16277137f / frictionCubed;

        // 重新获取摩擦因子用于最终阻力
        f32 finalFriction = 0.91f;
        if (m_onGround && m_world != nullptr) {
            BlockPos groundPos(static_cast<i32>(std::floor(m_position.x)),
                static_cast<i32>(std::floor(m_position.y - 1.0)),
                static_cast<i32>(std::floor(m_position.z)));
            const BlockState* blockState = m_world->getBlockState(groundPos);
            if (blockState != nullptr) {
                finalFriction = blockState->getBlock().getSlipperiness(*blockState, m_world, &groundPos, this) * 0.91f;
            }
        }

        // 计算移动因子
        // 地面上的加速度是空中的约 5 倍
        f32 moveFactor = m_onGround ? 0.1f * accelerationCorrection : 0.02f;

        // 应用移动
        moveRelative(moveFactor, strafing, vertical, forward);

        // 执行移动（带碰撞检测）
        move(entity::MoverType::Self, m_velocity);

        // 应用阻力
        scaleVelocity(finalFriction);
    }

    // 更新肢体摆动动画
    // 第二个参数 false 表示不计算垂直位移
    // 这个方法用于更新 walkDistance 和 limbSwing 等动画参数
    updateTravelAnimation(false);
}

} // namespace mc
