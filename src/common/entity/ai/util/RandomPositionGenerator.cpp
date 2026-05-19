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

#include "RandomPositionGenerator.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/WorldConstants.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../core/CreatureEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../pathfinding/PathNavigator.hpp"
#include "../pathfinding/PathPoint.hpp"
#include <algorithm>
#include <cmath>

namespace mc::entity::ai::util {

using namespace math;

// ==================== 公开方法实现 ====================

bool RandomPositionGenerator::findRandomTarget(CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos)
{
    return findRandomTargetTowards(creature, xzRange, yRange, Vector3::zero(), outPos);
}

bool RandomPositionGenerator::findRandomTargetBlockAwayFrom(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& avoidPos, Vector3& outPos)
{
    if (!creature) return false;

    // MC 1.16.5: 生成远离指定位置的方向
    // 计算从avoidPos到creature的方向向量，反转它
    Vector3 creaturePos(creature->x(), creature->y(), creature->z());
    Vector3 awayDirection = creaturePos - avoidPos;

    // 归一化方向向量
    f32 length = awayDirection.length();
    if (length > 0.001f) {
        awayDirection = awayDirection * (1.0f / length);
    } else {
        // 如果距离太近，使用随机方向
        Random rng = creature->getRandom();
        awayDirection = Vector3(rng.nextFloat() * 2.0f - 1.0f, 0.0f, rng.nextFloat() * 2.0f - 1.0f).normalized();
    }

    return findBestPosition(creature, xzRange, yRange, awayDirection, outPos);
}

bool RandomPositionGenerator::findRandomTargetTowards(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, Vector3& outPos)
{
    if (!creature) return false;

    Vector3 directionBias(0.0f, 0.0f, 0.0f);

    // 如果有目标位置，计算方向偏好
    if (targetPos.lengthSquared() > 0.001f) {
        Vector3 creaturePos(creature->x(), creature->y(), creature->z());
        directionBias = (targetPos - creaturePos).normalized();
    }

    return findBestPosition(creature, xzRange, yRange, directionBias, outPos);
}

bool RandomPositionGenerator::findRandomTargetTowardsScaled(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, f64 angleRange, Vector3& outPos)
{
    // MC 1.16.5: RandomPositionGenerator.findRandomTargetTowardsScaled
    // 在目标方向生成一个缩放后的随机目标位置，角度限制在指定范围内
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Vector3 creaturePos(creature->x(), creature->y(), creature->z());

    // 计算到目标的方向向量
    Vector3 toTarget = targetPos - creaturePos;
    f64 distanceToTarget = toTarget.length();

    if (distanceToTarget < 0.001) {
        // 目标位置太近，使用普通方法
        return findRandomTarget(creature, xzRange, yRange, outPos);
    }

    toTarget = toTarget * (1.0 / distanceToTarget); // 归一化

    // 计算目标方向的角度（弧度）
    f64 targetAngle = std::atan2(toTarget.x, toTarget.z);

    Random rng = creature->getRandom();

    // 尝试多次生成有效位置
    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 在角度范围内随机偏移
        f64 angleOffset = (rng.nextFloat() * 2.0 - 1.0) * angleRange / 2.0;
        f64 actualAngle = targetAngle + angleOffset;

        // 随机距离（在xzRange范围内）
        f64 distance = rng.nextFloat() * static_cast<f64>(xzRange);

        // 计算目标位置
        f64 dx = std::sin(actualAngle) * distance;
        f64 dz = std::cos(actualAngle) * distance;
        f64 dy = (rng.nextFloat() * 2.0 - 1.0) * static_cast<f64>(yRange);

        i32 x = floorTo<i32>(creaturePos.x + dx);
        i32 y = floorTo<i32>(creaturePos.y + dy);
        i32 z = floorTo<i32>(creaturePos.z + dz);

        // 检查坐标是否在世界范围内
        if (!world->isWithinWorldBounds(x, y, z)) {
            continue;
        }

        // 检查位置是否可行走
        if (isPositionWalkable(creature, x, y, z)) {
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
            return true;
        }

        // 尝试找到地面
        i32 groundY = getGroundHeight(world, x, y, z);
        if (groundY >= world::MIN_BUILD_HEIGHT && isPositionWalkable(creature, x, groundY, z)) {
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(groundY), static_cast<f32>(z) + 0.5f);
            return true;
        }
    }

    // 如果找不到有效位置，尝试使用 findRandomTargetBlockTowards 作为后备
    // MC 1.16.5: 如果第一个方法失败，尝试 findRandomTargetBlockTowards
    return findRandomTargetBlock(creature, xzRange / 2, yRange, std::nullopt, outPos);
}

bool RandomPositionGenerator::getLandPos(CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos)
{
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Random rng = creature->getRandom();

    // 尝试多次找到陆地位置
    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 生成随机偏移
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dy = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(yRange);

        i32 x = floorTo<i32>(creature->x() + dx);
        i32 z = floorTo<i32>(creature->z() + dz);

        // 寻找地面高度
        i32 groundY = getGroundHeight(world, x, floorTo<i32>(creature->y() + dy), z);

        if (groundY >= world::MIN_BUILD_HEIGHT) {
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(groundY), static_cast<f32>(z) + 0.5f);
            return true;
        }
    }

    return false;
}

bool RandomPositionGenerator::findRandomTargetAvoidWater(
    CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos)
{
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Random rng = creature->getRandom();

    // 尝试多次找到避开水域的位置
    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dy = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(yRange);

        i32 x = floorTo<i32>(creature->x() + dx);
        i32 y = floorTo<i32>(creature->y() + dy);
        i32 z = floorTo<i32>(creature->z() + dz);

        // 检查不是水
        if (!world->isWaterAt(x, y, z) && !world->isWaterAt(x, y + 1, z)) {
            // 检查位置可行走
            if (isPositionWalkable(creature, x, y, z)) {
                outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
                return true;
            }
        }
    }

    return false;
}

bool RandomPositionGenerator::findRandomTargetBlock(
    CreatureEntity* creature, i32 xzRange, i32 yRange, std::optional<Vector3> avoidPos, Vector3& outPos)
{
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Random rng = creature->getRandom();

    // MC 1.16.5: RandomPositionGenerator.findRandomTargetBlock
    // 飞行实体使用此方法选择随机的方块位置
    // 不要求位置可行走，只需要是空气方块即可

    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 生成随机偏移
        i32 dx = rng.nextInt(2 * xzRange + 1) - xzRange;
        i32 dy = rng.nextInt(2 * yRange + 1) - yRange;
        i32 dz = rng.nextInt(2 * xzRange + 1) - xzRange;

        i32 x = floorTo<i32>(creature->x()) + dx;
        i32 y = floorTo<i32>(creature->y()) + dy;
        i32 z = floorTo<i32>(creature->z()) + dz;

        // 检查坐标是否在世界范围内
        if (!world->isWithinWorldBounds(x, y, z)) {
            continue;
        }

        // 如果有回避位置，检查是否远离
        if (avoidPos.has_value()) {
            Vector3 candidatePos(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
            Vector3 creaturePos(creature->x(), creature->y(), creature->z());
            Vector3 awayDir = creaturePos - avoidPos.value();

            // 检查候选位置是否在回避方向上
            Vector3 toCandidate = candidatePos - creaturePos;
            f32 dot = toCandidate.x * awayDir.x + toCandidate.y * awayDir.y + toCandidate.z * awayDir.z;
            if (dot < 0) {
                // 候选位置在回避方向的反方向，跳过
                continue;
            }
        }

        // 对于飞行实体，只需要检查目标位置是空气或可以通过
        const BlockState* block = world->getBlockState(x, y, z);
        if (block && (block->isAir() || !block->isSolid())) {
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
            return true;
        }
    }

    return false;
}

bool RandomPositionGenerator::findRandomTargetBlockTowards(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, Vector3& outPos)
{
    // MC 1.16.5: RandomPositionGenerator.findRandomTargetBlockTowards
    // 用于水生生物（如海豚），在朝向目标的方向选择一个方块位置
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Vector3 creaturePos(creature->x(), creature->y(), creature->z());

    // 计算到目标的方向向量
    Vector3 toTarget = targetPos - creaturePos;
    f64 distanceToTarget = toTarget.length();

    if (distanceToTarget < 0.001) {
        // 目标位置太近，使用普通方法
        return findRandomTargetBlock(creature, xzRange, yRange, std::nullopt, outPos);
    }

    toTarget = toTarget * (1.0 / distanceToTarget); // 归一化

    Random rng = creature->getRandom();

    // 尝试多次生成有效位置
    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 在目标方向上随机偏移
        // MC 1.16.5: 使用偏移后的方向向量
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dy = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(yRange);
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);

        // 添加方向偏移（朝向目标）
        dx += static_cast<f32>(toTarget.x * xzRange * 0.5);
        dy += static_cast<f32>(toTarget.y * yRange * 0.5);
        dz += static_cast<f32>(toTarget.z * xzRange * 0.5);

        i32 x = floorTo<i32>(creaturePos.x + dx);
        i32 y = floorTo<i32>(creaturePos.y + dy);
        i32 z = floorTo<i32>(creaturePos.z + dz);

        // 检查坐标是否在世界范围内
        if (!world->isWithinWorldBounds(x, y, z)) {
            continue;
        }

        // 检查是否是水或可通过的方块（水生生物使用）
        const fluid::FluidState* fluidState = world->getFluidState(x, y, z);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            // 是流体，对水生生物有效
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
            return true;
        }

        // 检查是否是空气
        const BlockState* block = world->getBlockState(x, y, z);
        if (block && (block->isAir() || !block->getMaterial().blocksMovement())) {
            outPos = Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f);
            return true;
        }
    }

    return false;
}

// ==================== 辅助方法实现 ====================

bool RandomPositionGenerator::isPositionWalkable(CreatureEntity* creature, i32 x, i32 y, i32 z)
{
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    // 检查是否有实心地面
    const BlockState* groundBlock = world->getBlockState(x, y - 1, z);
    if (!groundBlock || groundBlock->isAir()) {
        // 检查是否可以在当前位置站立（比如在水中或岩浆中）
        const BlockState* currentBlock = world->getBlockState(x, y, z);
        if (!currentBlock) return false;
    }

    // 检查是否有足够空间站立
    const BlockState* block1 = world->getBlockState(x, y, z);
    const BlockState* block2 = world->getBlockState(x, y + 1, z);

    if (block1 && !block1->isAir() && !block1->isLiquid()) {
        return false; // 被阻挡
    }
    if (block2 && !block2->isAir() && !block2->isLiquid()) {
        return false; // 头部被阻挡
    }

    // 检查脚下方块是否可以站立
    if (groundBlock && !groundBlock->isAir() && !groundBlock->isLiquid()) {
        return true;
    }

    // 水中或岩浆中也可以游动（检查实体是否在水中）
    if (world->isWaterAt(x, y, z)) {
        return true; // 水中可以游泳
    }

    return false;
}

i32 RandomPositionGenerator::getGroundHeight(IWorld* world, i32 x, i32 startY, i32 z)
{
    if (!world) return -1;

    // 从指定高度向下搜索
    for (i32 y = startY; y > startY - MAX_GROUND_SEARCH && y >= world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* block = world->getBlockState(x, y, z);
        if (block && !block->isAir() && !block->isLiquid()) {
            return y + 1; // 返回地面上的Y坐标
        }
    }

    return -1;
}

f32 RandomPositionGenerator::calculatePositionScore(CreatureEntity* creature, const Vector3& pos)
{
    if (!creature) return -1000.0f;

    IWorld* world = creature->world();
    if (!world) return -1000.0f;

    i32 x = floorTo<i32>(pos.x);
    i32 y = floorTo<i32>(pos.y);
    i32 z = floorTo<i32>(pos.z);

    // 使用实体特定的 getPathWeight 方法
    // MC 1.16.5: RandomPositionGenerator 使用 creature.getBlockPathWeight() 评估位置
    f32 score = creature->getPathWeight(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));

    // 危险位置检查
    if (world->isLavaAt(x, y, z) || world->isLavaAt(x, y - 1, z)) {
        return -1000.0f; // 岩浆位置无效
    }

    // 可达性评分
    if (!isPositionWalkable(creature, x, y, z)) {
        score -= 50.0f;
    }

    return score;
}

// ==================== 私有方法实现 ====================

Vector3 RandomPositionGenerator::generateRandomOffset(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& directionBias)
{
    if (!creature) return Vector3::zero();

    Random rng = creature->getRandom();

    // 生成基础随机偏移
    f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
    f32 dy = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(yRange);
    f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);

    // 如果有方向偏好，添加一些偏向
    if (directionBias.lengthSquared() > 0.001f) {
        // MC 1.16.5: 50% 概率使用方向偏好
        if (rng.nextFloat() < 0.5f) {
            f32 biasStrength = static_cast<f32>(xzRange) * 0.3f;
            dx += directionBias.x * biasStrength;
            dy += directionBias.y * biasStrength;
            dz += directionBias.z * biasStrength;
        }
    }

    return Vector3(dx, dy, dz);
}

bool RandomPositionGenerator::validateAndAdjustPosition(CreatureEntity* creature, Vector3& pos)
{
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    i32 x = floorTo<i32>(pos.x);
    i32 y = floorTo<i32>(pos.y);
    i32 z = floorTo<i32>(pos.z);

    // 检查坐标是否在世界范围内
    if (!world->isWithinWorldBounds(x, y, z)) {
        return false;
    }

    // 检查当前位置是否可行走
    if (isPositionWalkable(creature, x, y, z)) {
        pos.y = static_cast<f32>(y);
        return true;
    }

    // 尝试找到地面
    i32 groundY = getGroundHeight(world, x, y, z);
    if (groundY >= world::MIN_BUILD_HEIGHT && isPositionWalkable(creature, x, groundY, z)) {
        pos.y = static_cast<f32>(groundY);
        return true;
    }

    return false;
}

bool RandomPositionGenerator::findBestPosition(
    CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& directionBias, Vector3& outPos)
{
    if (!creature) return false;

    Random rng = creature->getRandom();

    // MC 1.16.5: 尝试生成多个候选位置，选择评分最高的
    Vector3 bestPos;
    f32 bestScore = -10000.0f;
    bool found = false;

    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        Vector3 offset = generateRandomOffset(creature, xzRange, yRange, directionBias);

        Vector3 candidatePos(creature->x() + offset.x, creature->y() + offset.y, creature->z() + offset.z);

        // 验证并调整位置
        if (validateAndAdjustPosition(creature, candidatePos)) {
            f32 score = calculatePositionScore(creature, candidatePos);

            if (score > bestScore) {
                bestScore = score;
                bestPos = candidatePos;
                found = true;
            }
        }
    }

    if (found) {
        outPos = bestPos;
        return true;
    }

    return false;
}

} // namespace mc::entity::ai::util
