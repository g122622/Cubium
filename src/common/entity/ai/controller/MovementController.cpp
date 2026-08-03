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

#include "MovementController.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/blocks/DoorBlock.hpp"
#include "../../../world/block/blocks/FenceGateBlock.hpp"
#include "../../../world/block/blocks/building/FenceBlock.hpp"
#include "../../../world/block/blocks/building/WallBlock.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../core/MobEntity.hpp"
#include "../pathfinding/PathFinder.hpp"
#include "../pathfinding/PathNavigator.hpp"
#include "../pathfinding/PathNodeType.hpp"
#include "JumpController.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include <algorithm>
#include <cmath>

// 使用命名空间简化代码
using mc::blocks::DoorBlock;
using mc::blocks::FenceBlock;
using mc::blocks::FenceGateBlock;
using mc::blocks::WallBlock;
using namespace mc::math;

namespace mc::entity::ai::controller {

MovementController::MovementController(MobEntity* mob)
    : m_mob(mob)
{}

void MovementController::setMoveTo(f64 x, f64 y, f64 z, f64 speed)
{
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_speed = speed;
    // 跳跃中不应覆盖为MOVE_TO
    if (m_action != MoveAction::Jumping) {
        m_action = MoveAction::MoveTo;
    }
}

void MovementController::strafe(f32 forward, f32 strafe)
{
    m_action = MoveAction::Strafe;
    m_moveForward = forward;
    m_moveStrafe = strafe;
    m_speed = 0.25; // 默认横向移动速度
}

void MovementController::tick()
{
    if (!m_mob) return;

    if (m_action == MoveAction::Strafe) {
        // STRAFE 模式实现
        // 计算移动速度
        f32 baseSpeed = static_cast<f32>(m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        f32 moveSpeed = static_cast<f32>(m_speed) * baseSpeed;

        // 归一化和缩放移动向量
        f32 forward = m_moveForward;
        f32 strafe = m_moveStrafe;
        f32 length = std::sqrt(forward * forward + strafe * strafe);
        if (length < 1.0f) {
            length = 1.0f;
        }
        f32 f4 = moveSpeed / length;
        forward = forward * f4;
        strafe = strafe * f4;

        // 基于实体偏航角进行向量旋转变换
        f32 yaw = m_mob->yaw() * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yaw);
        f32 cosYaw = std::cos(yaw);

        // 计算世界坐标系的移动向量
        f32 moveX = forward * cosYaw - strafe * sinYaw;
        f32 moveZ = strafe * cosYaw + forward * sinYaw;

        // 检查目标位置是否可行走
        f64 targetX = m_mob->x() + moveX;
        f64 targetZ = m_mob->z() + moveZ;

        if (!canWalkAt(targetX, targetZ)) {
            // 如果检查失败，设置为向前移动
            m_mob->setAIMoveSpeed(moveSpeed);
            m_mob->setMoveForward(1.0f);
            m_mob->setMoveStrafing(0.0f);
        } else {
            m_mob->setAIMoveSpeed(moveSpeed);
            m_mob->setMoveForward(m_moveForward);
            m_mob->setMoveStrafing(m_moveStrafe);
        }
        m_action = MoveAction::Wait;
    } else if (m_action == MoveAction::MoveTo) {
        // MOVE_TO 状态在tick开头立即转为WAIT
        m_action = MoveAction::Wait;

        f64 dx = m_posX - m_mob->x();
        f64 dy = m_posY - m_mob->y();
        f64 dz = m_posZ - m_mob->z();

        // 使用3D距离平方，阈值极小（2.5000003E-7F ≈ 0.0005格）
        f64 distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < 2.5000003E-7) {
            // 已到达目标
            m_mob->setMoveForward(0.0f);
            m_mob->setMoveStrafing(0.0f);
            return;
        }

        // 计算目标偏航角
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

        // 限制旋转速度为90度/tick
        // limitAngle 结果必须包装到 [0, 360)
        f32 currentYaw = m_mob->yaw();
        f32 newYaw = math::wrapDegreesPositive(math::clampedRotate(currentYaw, targetYaw, 90.0f));

        m_mob->setRotation(newYaw, m_mob->pitch());

        // 设置移动速度
        f32 moveSpeed =
            static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        m_mob->setAIMoveSpeed(moveSpeed);
        m_mob->setMoveForward(1.0f); // 向前移动

        // 检查是否需要跳跃
        // 跳跃有两个条件，满足其一即可
        f64 horizontalDistSq = dx * dx + dz * dz;
        f32 entityWidth = m_mob->width();
        f32 maxDist = std::max(1.0f, entityWidth);
        bool shouldJump = false;

        // 条件1: 目标位置更高且水平距离近
        if (dy > m_mob->stepHeight() && horizontalDistSq < static_cast<f64>(maxDist * maxDist)) {
            shouldJump = true;
        }

        // 条件2: 检查实体所在位置方块的碰撞形状（门、栅栏等特殊情况）
        // 检查的是实体当前位置的方块，而不是前方方块
        if (!shouldJump && m_mob->world()) {
            // 使用实体当前位置
            i32 blockX = floorTo<i32>(m_mob->x());
            i32 blockY = floorTo<i32>(m_mob->y());
            i32 blockZ = floorTo<i32>(m_mob->z());

            if (const BlockState* state = m_mob->world()->getBlockState(blockX, blockY, blockZ)) {
                const Block& block = state->getBlock();
                const CollisionShape& shape = state->getCollisionShape();

                // 检查碰撞形状是否非空
                if (!shape.isEmpty()) {
                    // 获取碰撞形状的最大Y值
                    f64 shapeMaxY = 0.0;
                    for (const auto& box : shape.boxes()) {
                        shapeMaxY = std::max(shapeMaxY, static_cast<f64>(box.maxY));
                    }

                    // 检查实体是否在碰撞形状上方
                    // 如果实体位置低于碰撞形状顶部，需要跳跃
                    f64 entityY = m_mob->y();
                    f64 shapeTopY = static_cast<f64>(blockY) + shapeMaxY;

                    if (entityY < shapeTopY) {
                        // 只检查 DOORS 和 FENCES 标签，不检查 WALLS
                        // 使用 RTTI 检查方块类型
                        bool isDoorOrFence = false;

                        // 检查门
                        if (dynamic_cast<const DoorBlock*>(&block) != nullptr) {
                            isDoorOrFence = true;
                        }
                        // 检查栅栏（不包括栅栏门，栅栏门会触发跳跃）
                        else if (dynamic_cast<const FenceBlock*>(&block) != nullptr) {
                            isDoorOrFence = true;
                        }
                        // 不检查墙和栅栏门，它们会触发跳跃

                        if (!isDoorOrFence) {
                            shouldJump = true;
                        }
                    }
                }
            }
        }

        if (shouldJump) {
            if (auto* jumpCtrl = m_mob->jumpController()) {
                jumpCtrl->setJumping();
            }
            m_action = MoveAction::Jumping;
        }
    } else if (m_action == MoveAction::Jumping) {
        // JUMPING 状态设置移动速度
        f32 moveSpeed =
            static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        m_mob->setAIMoveSpeed(moveSpeed);

        if (m_mob->onGround()) {
            m_action = MoveAction::Wait; // 着陆后设为WAIT
        }
    } else {
        // Wait 状态
        m_mob->setMoveForward(0.0f);
        m_mob->setMoveStrafing(0.0f);
    }
}

bool MovementController::canWalkAt(f64 x, f64 z) const
{
    if (!m_mob) {
        return true; // 无法检查时默认可行走
    }

    // 使用 NodeProcessor.getPathNodeType 检查目标位置是否可行走
    auto* navigator = m_mob->navigator();
    if (navigator && navigator->getPathFinder()) {
        auto* nodeProcessor = navigator->getPathFinder()->getNodeProcessor();
        if (nodeProcessor) {
            i32 blockX = floorTo<i32>(x);
            i32 blockY = floorTo<i32>(m_mob->y());
            i32 blockZ = floorTo<i32>(z);

            auto nodeType = nodeProcessor->getNodeType(blockX, blockY, blockZ);
            return nodeType == pathfinding::PathNodeType::Walkable;
        }
    }

    // 如果没有 NodeProcessor，默认返回 true
    return true;
}

} // namespace mc::entity::ai::controller
