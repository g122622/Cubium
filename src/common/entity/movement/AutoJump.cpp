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

#include "AutoJump.hpp"

#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/movement/AutoJumpConstants.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace mc {
namespace entity {
namespace movement {

using namespace AutoJumpConstants;
using namespace physics;

void AutoJump::tick()
{
    if (m_autoJumpTime > 0) {
        m_autoJumpTime--;
    }
}

void AutoJump::resetCooldown()
{
    m_autoJumpTime = AUTO_JUMP_COOLDOWN;
}

AutoJumpResult AutoJump::check(const Player& player, PhysicsEngine& physicsEngine, const Vector2& movementInput)
{

    AutoJumpResult result;

    // 1. 计算移动输入长度（用于判断是否有移动输入）
    f32 inputLengthSq = movementInput.x * movementInput.x + movementInput.y * movementInput.y;
    bool hasMovementInput = inputLengthSq > MOVEMENT_THRESHOLD_SQ;

    // 2. 检查是否应该进行自动跳跃检测
    if (!_shouldCheckForAutoJump(player, hasMovementInput)) {
        return result;
    }

    // 3. 计算移动方向
    Vector3 movementDir = calculateMovementDirection(player, movementInput);
    f32 movementLength = std::sqrt(movementDir.x * movementDir.x + movementDir.z * movementDir.z);
    if (movementLength < 0.0001f) {
        return result; // 无有效移动方向
    }
    movementDir.x /= movementLength;
    movementDir.z /= movementLength;

    // 4. 检查是否在向前移动
    // 玩家朝向向量（仅水平方向）
    f32 yawRad = player.yaw() * math::DEG_TO_RAD;
    Vector3 forwardDir(-std::sin(yawRad), 0.0f, std::cos(yawRad));

    if (!isMovingForward(movementDir, forwardDir)) {
        return result; // 向后移动，不触发
    }

    // 5. 检查头部空间（玩家当前位置上方）
    Vector3 playerPos = player.position();
    if (!_hasHeadSpace(player, physicsEngine, playerPos)) {
        return result; // 头部空间不足，不能跳
    }

    // 6. 计算最大跳跃高度
    f32 maxJumpHeight = calculateMaxJumpHeight();

    // 7. 计算检测距离
    f32 moveSpeed = player.abilities().walkSpeed;
    if (player.isSprinting()) {
        moveSpeed *= SPRINT_SPEED_MULTIPLIER;
    }
    f32 detectionDistance = std::max(moveSpeed * DETECTION_DISTANCE_MULTIPLIER, 1.0f / movementLength);

    // 限制检测距离，避免过远检测
    detectionDistance = std::min(detectionDistance, 5.0f);

    // 8. 构建检测线起点
    f32 detectionY = playerPos.y + DETECTION_HEIGHT_OFFSET;
    Vector3 playerFeet(playerPos.x, detectionY, playerPos.z);

    // 9. 计算检测线偏移（玩家左右边缘）
    Vector3 perpendicular(-movementDir.z, 0.0f, movementDir.x);
    f32 halfWidth = player.width() * LINE_OFFSET_RATIO;
    Vector3 leftOffset = perpendicular * halfWidth;
    Vector3 rightOffset = perpendicular * (-halfWidth);

    // 左右两条检测线的起点
    Vector3 leftStart = playerFeet + leftOffset;
    Vector3 rightStart = playerFeet + rightOffset;

    // 检测线终点
    Vector3 leftEnd = leftStart + movementDir * detectionDistance;
    Vector3 rightEnd = rightStart + movementDir * detectionDistance;

    // 10. 收集检测区域内的碰撞箱
    // 创建一个包含所有检测线的 AABB
    f32 minY = detectionY - 0.1f;
    f32 maxY = detectionY + maxJumpHeight + player.height();
    f32 minX = std::min({leftStart.x, leftEnd.x, rightStart.x, rightEnd.x}) - 0.1f;
    f32 maxX = std::max({leftStart.x, leftEnd.x, rightStart.x, rightEnd.x}) + 0.1f;
    f32 minZ = std::min({leftStart.z, leftEnd.z, rightStart.z, rightEnd.z}) - 0.1f;
    f32 maxZ = std::max({leftStart.z, leftEnd.z, rightStart.z, rightEnd.z}) + 0.1f;

    AxisAlignedBB searchBox(minX, minY, minZ, maxX, maxY, maxZ);

    std::vector<AxisAlignedBB> collisionBoxes;
    physicsEngine.collectCollisionBoxes(searchBox, collisionBoxes);

    if (collisionBoxes.empty()) {
        return result; // 没有障碍物
    }

    // 11. 沿检测线查找障碍物
    f32 obstacleHeight = -1.0f;

    // 检查左检测线
    f32 leftHeight = _detectObstacleHeight(
        player, physicsEngine, leftStart, movementDir, detectionDistance, maxJumpHeight, collisionBoxes);

    // 检查右检测线
    f32 rightHeight = _detectObstacleHeight(
        player, physicsEngine, rightStart, movementDir, detectionDistance, maxJumpHeight, collisionBoxes);

    // 取两条检测线中检测到的最高障碍物
    obstacleHeight = std::max(leftHeight, rightHeight);

    // 12. 检查障碍物高度是否在可跳跃范围内
    if (obstacleHeight > 0.0f) {
        f32 heightDiff = obstacleHeight - playerPos.y;

        // 高度差必须在 [MIN_JUMP_HEIGHT, maxJumpHeight] 范围内
        if (heightDiff > MIN_JUMP_HEIGHT && heightDiff <= maxJumpHeight) {
            result.shouldJump = true;
            result.obstacleHeight = heightDiff;
            resetCooldown();
        }
    }

    return result;
}

bool AutoJump::_shouldCheckForAutoJump(const Player& player, bool hasMovementInput) const
{

    // 1. 检查是否启用
    if (!m_enabled) {
        return false;
    }

    // 2. 检查冷却
    if (m_autoJumpTime > 0) {
        return false;
    }

    // 3. 检查是否在地面
    if (!player.onGround()) {
        return false;
    }

    // 4. 检查是否在潜行
    if (player.isSneaking()) {
        return false;
    }

    // 5. 检查是否在骑乘
    if (player.isRiding()) {
        return false;
    }

    // 6. 检查是否有移动输入
    if (!hasMovementInput) {
        return false;
    }

    // 7. 检查跳跃因子（蜂蜜块等会降低）
    // 蜂蜜块会让跳跃因子变为 0.5f，此时禁用自动跳跃
    if (player.getJumpFactor() < static_cast<f32>(JUMP_FACTOR_THRESHOLD)) {
        return false;
    }

    // 8. 检查是否在飞行
    if (player.abilities().flying) {
        return false;
    }

    return true;
}

Vector3 AutoJump::calculateMovementDirection(const Player& player, const Vector2& movementInput)
{

    Vector3 velocity = player.velocity();

    // 检查速度是否足够大
    f32 speedSq = velocity.x * velocity.x + velocity.z * velocity.z;
    if (speedSq > MOVEMENT_THRESHOLD_SQ) {
        // 使用速度方向
        f32 length = std::sqrt(speedSq);
        return Vector3(velocity.x / length, 0.0f, velocity.z / length);
    }

    // 使用移动输入 + yaw 计算方向
    f32 inputLength = std::sqrt(movementInput.x * movementInput.x + movementInput.y * movementInput.y);
    if (inputLength < 0.0001f) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    // 归一化输入
    f32 strafe = movementInput.x / inputLength;  // 左右
    f32 forward = movementInput.y / inputLength; // 前后

    // 坐标系: yaw=0 看向 -Z, yaw=90 看向 +X
    f32 yawRad = player.yaw() * math::DEG_TO_RAD;
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    f32 moveX = strafe * cosYaw - forward * sinYaw;
    f32 moveZ = forward * cosYaw + strafe * sinYaw;

    // 归一化
    f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (length < 0.0001f) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    return Vector3(moveX / length, 0.0f, moveZ / length);
}

bool AutoJump::isMovingForward(const Vector3& movementDir, const Vector3& forwardDir)
{

    // 计算点积（仅 XZ 平面）
    f32 dot = movementDir.x * forwardDir.x + movementDir.z * forwardDir.z;

    return dot > FORWARD_THRESHOLD;
}

f32 AutoJump::calculateMaxJumpHeight() const
{
    return BASE_JUMP_HEIGHT + JUMP_BOOST_PER_LEVEL * static_cast<f32>(m_jumpBoostLevel);
}

bool AutoJump::_hasHeadSpace(const Player& player, PhysicsEngine& physicsEngine, const Vector3& testPos)
{
    // 检查玩家眼睛上方一格和两格位置是否有障碍物
    f32 playerHeight = player.height();
    f32 eyeY = testPos.y + player.eyeHeight();

    // 检查眼睛上方一格和两格
    for (i32 i = 0; i < HEAD_SPACE_CHECK_HEIGHT; ++i) {
        f32 checkY = eyeY + static_cast<f32>(i + 1);

        // 创建检测碰撞箱
        AxisAlignedBB checkBox(testPos.x - player.width() * 0.5f,
            checkY,
            testPos.z - player.width() * 0.5f,
            testPos.x + player.width() * 0.5f,
            checkY + 1.0f, // 检查一格高度
            testPos.z + player.width() * 0.5f);

        std::vector<AxisAlignedBB> boxes;
        physicsEngine.collectCollisionBoxes(checkBox, boxes);

        if (!boxes.empty()) {
            return false; // 有障碍物，头部空间不足
        }
    }

    return true;
}

f32 AutoJump::_detectObstacleHeight(const Player& player,
    PhysicsEngine& physicsEngine,
    const Vector3& origin,
    const Vector3& direction,
    f32 distance,
    f32 maxJumpHeight,
    std::vector<AxisAlignedBB>& collisionBoxes)
{

    f32 playerY = player.position().y;
    f32 playerHeight = player.height();

    // 遍历所有碰撞箱，查找与检测线相交的最高障碍物
    f32 maxObstacleHeight = -1.0f;

    for (const auto& box : collisionBoxes) {
        // 检查碰撞箱是否与检测线相交
        // 使用简化的线段-AABB 相交检测

        // 检测线段
        Vector3 lineEnd = origin + direction * distance;

        // 检查碰撞箱是否在检测范围内
        // AABB 的 Y 范围必须在 [playerY, playerY + maxJumpHeight + playerHeight] 内
        if (box.maxY < playerY || box.minY > playerY + maxJumpHeight + playerHeight) {
            continue; // 不在高度范围内
        }

        // 检查检测线是否穿过碰撞箱的 XZ 投影
        // 使用 2D 线段-AABB 相交检测
        f32 minX = std::min(origin.x, lineEnd.x);
        f32 maxX = std::max(origin.x, lineEnd.x);
        f32 minZ = std::min(origin.z, lineEnd.z);
        f32 maxZ = std::max(origin.z, lineEnd.z);

        // 扩展检测范围以包含玩家宽度
        f32 halfWidth = player.width() * 0.5f;

        // 检查 X 范围
        if (maxX + halfWidth < box.minX || minX - halfWidth > box.maxX) {
            continue;
        }

        // 检查 Z 范围
        if (maxZ + halfWidth < box.minZ || minZ - halfWidth > box.maxZ) {
            continue;
        }

        // 碰撞箱与检测线相交，检查高度是否合适
        // 障碍物顶部高度必须在可跳跃范围内
        if (box.maxY > playerY + MIN_JUMP_HEIGHT && box.maxY <= playerY + maxJumpHeight) {
            // 检查障碍物顶部是否有站立空间
            // 玩家需要能站在障碍物上

            // 创建玩家站在障碍物上时的碰撞箱
            AxisAlignedBB standingBox(origin.x - player.width() * 0.5f,
                box.maxY,
                origin.z - player.width() * 0.5f,
                origin.x + player.width() * 0.5f,
                box.maxY + playerHeight,
                origin.z + player.width() * 0.5f);

            // 检查站立位置是否有碰撞
            std::vector<AxisAlignedBB> standingBoxes;
            physicsEngine.collectCollisionBoxes(standingBox, standingBoxes);

            // 如果有碰撞，说明上方空间不足
            bool canStand = true;
            for (const auto& standingBox_collision : standingBoxes) {
                // 忽略当前障碍物本身
                if (&standingBox_collision == &box) {
                    continue;
                }
                canStand = false;
                break;
            }

            if (canStand && box.maxY > maxObstacleHeight) {
                maxObstacleHeight = box.maxY;
            }
        }
    }

    return maxObstacleHeight;
}

} // namespace movement
} // namespace entity
} // namespace mc
