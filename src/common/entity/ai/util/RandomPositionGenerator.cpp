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
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../core/CreatureEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <functional>
#include <optional>

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
        Random& rng = creature->getRandom();
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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

    Random& rng = creature->getRandom();

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

// ==================== 飞行位置生成方法实现 ====================

bool RandomPositionGenerator::findHoverPosition(CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    f64 xDir,
    f64 zDir,
    f32 maxAngle,
    i32 maxAboveSolid,
    i32 minAboveSolid,
    Vector3& outPos)
{
    if (!creature) return false;
    IWorld* world = creature->world();
    if (!world) return false;

    const bool restricted = isMobRestricted(creature, static_cast<f64>(xzRange));
    const i32 maxY = world->getMaxBuildHeight();

    // 最佳位置选择：尝试10次，选择评分最高的
    Vector3 bestPos;
    f32 bestScore = -10000.0f;
    bool found = false;

    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 1. 在方向角度范围内生成随机偏移
        auto offsetOpt = generateRandomDirectionWithinRadians(
            creature->getRandom(), 0.0, static_cast<f64>(xzRange), yRange, 0, xDir, zDir, static_cast<f64>(maxAngle));
        if (!offsetOpt.has_value()) {
            continue;
        }

        // 2. 转换为世界坐标（含家区域约束）
        BlockPos worldPos =
            generatePosTowardDirection(creature, static_cast<f64>(xzRange), restricted, offsetOpt.value());

        // 3. 检查建造高度和家区域约束
        if (isOutsideBuildHeight(creature, worldPos)) {
            continue;
        }
        if (restricted && !creature->isWithinHomeDistanceFromPosition(worldPos)) {
            continue;
        }

        // 4. 检查导航目标稳定性：目标下方的方块不应是空气
        BlockPos belowPos(worldPos.x, worldPos.y - 1, worldPos.z);
        const BlockState* belowState = world->getBlockState(belowPos);
        if (belowState != nullptr && belowState->isAir()) {
            continue;
        }

        // 5. 向上移动到固体方块上方 minAboveSolid~maxAboveSolid 格
        math::Random& rng = creature->getRandom();
        i32 aboveSolidAmount = rng.nextInt(maxAboveSolid - minAboveSolid + 1) + minAboveSolid;
        worldPos = moveUpToAboveSolid(
            worldPos, aboveSolidAmount, maxY, [&creature](const BlockPos& p) { return isSolidAt(creature, p); });

        // 6. 检查不在水中且无寻路惩罚
        if (isWaterAt(creature, worldPos) || hasPathfindingMalus(creature, worldPos)) {
            continue;
        }

        // 7. 计算评分
        f32 score = creature->getPathWeight(
            static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
        if (score > bestScore) {
            bestScore = score;
            bestPos = Vector3(
                static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
            found = true;
        }
    }

    if (found) {
        outPos = bestPos;
        return true;
    }
    return false;
}

bool RandomPositionGenerator::findAirAndWaterPosition(
    CreatureEntity* creature, i32 xzRange, i32 yRange, i32 yOffset, f64 xDir, f64 zDir, f32 maxAngle, Vector3& outPos)
{
    if (!creature) return false;
    IWorld* world = creature->world();
    if (!world) return false;

    const bool restricted = isMobRestricted(creature, static_cast<f64>(xzRange));
    const i32 maxY = world->getMaxBuildHeight();

    // 最佳位置选择：尝试10次，选择评分最高的
    Vector3 bestPos;
    f32 bestScore = -10000.0f;
    bool found = false;

    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        // 1. 在方向角度范围内生成随机偏移
        auto offsetOpt = generateRandomDirectionWithinRadians(creature->getRandom(),
            0.0,
            static_cast<f64>(xzRange),
            yRange,
            yOffset,
            xDir,
            zDir,
            static_cast<f64>(maxAngle));
        if (!offsetOpt.has_value()) {
            continue;
        }

        // 2. 转换为世界坐标（含家区域约束）
        BlockPos worldPos =
            generatePosTowardDirection(creature, static_cast<f64>(xzRange), restricted, offsetOpt.value());

        // 3. 检查建造高度和家区域约束
        if (isOutsideBuildHeight(creature, worldPos)) {
            continue;
        }
        if (restricted && !creature->isWithinHomeDistanceFromPosition(worldPos)) {
            continue;
        }

        // 4. 向上移出固体方块
        worldPos = moveUpOutOfSolid(worldPos, maxY, [&creature](const BlockPos& p) { return isSolidAt(creature, p); });

        // 5. 检查无寻路惩罚
        if (hasPathfindingMalus(creature, worldPos)) {
            continue;
        }

        // 6. 计算评分
        f32 score = creature->getPathWeight(
            static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
        if (score > bestScore) {
            bestScore = score;
            bestPos = Vector3(
                static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
            found = true;
        }
    }

    if (found) {
        outPos = bestPos;
        return true;
    }
    return false;
}

bool RandomPositionGenerator::findAirPositionTowards(CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    i32 yOffset,
    const Vector3& targetPos,
    f32 maxAngle,
    Vector3& outPos)
{
    if (!creature) return false;

    // 计算从实体到目标的方向向量
    Vector3 dirVec = targetPos - creature->position();
    f64 xDir = static_cast<f64>(dirVec.x);
    f64 zDir = static_cast<f64>(dirVec.z);

    // 先尝试 AirRandomPos（排除水中位置）
    if (findAirAndWaterPosition(creature, xzRange, yRange, yOffset, xDir, zDir, maxAngle, outPos)) {
        // 额外检查：不在水中
        BlockPos posBlock(static_cast<i32>(outPos.x), static_cast<i32>(outPos.y), static_cast<i32>(outPos.z));
        if (!isWaterAt(creature, posBlock)) {
            return true;
        }
    }

    return false;
}

// ==================== 飞行位置生成辅助方法实现 ====================

std::optional<BlockPos> RandomPositionGenerator::generateRandomDirectionWithinRadians(
    math::Random& rng, f64 minRange, f64 maxRange, i32 verticalRange, i32 yOffset, f64 xDir, f64 zDir, f64 maxAngle)
{
    // 计算基础角度：从方向向量计算角度，减去PI/2使0度对应正前方
    f64 baseAngle = std::atan2(zDir, xDir) - (math::PI_DOUBLE / 2.0);

    // 在基础角度附近随机偏移，偏移范围在 [-maxAngle, +maxAngle]
    f64 angleOffset = (2.0 * rng.nextDouble() - 1.0) * maxAngle;
    f64 actualAngle = baseAngle + angleOffset;

    // 使用 sqrt 分布生成距离，确保均匀覆盖面积
    f64 distance = math::lerp(std::sqrt(rng.nextDouble()), minRange, maxRange) * SQRT_OF_TWO;

    // 从角度和距离计算水平偏移
    f64 offsetX = -distance * std::sin(actualAngle);
    f64 offsetZ = distance * std::cos(actualAngle);

    // 检查水平偏移是否超出范围
    if (std::abs(offsetX) > maxRange || std::abs(offsetZ) > maxRange) {
        return std::nullopt;
    }

    // 随机垂直偏移
    i32 offsetY = rng.nextInt(2 * verticalRange + 1) - verticalRange + yOffset;

    return BlockPos(static_cast<i32>(std::floor(offsetX)), offsetY, static_cast<i32>(std::floor(offsetZ)));
}

BlockPos RandomPositionGenerator::generatePosTowardDirection(
    CreatureEntity* creature, f64 range, bool isRestricted, const BlockPos& offset)
{
    math::Random& rng = creature->getRandom();

    f64 offsetX = static_cast<f64>(offset.x);
    f64 offsetZ = static_cast<f64>(offset.z);

    // 如果实体有家区域约束，将位置拉向家方向
    if (isRestricted && range > 1.0) {
        BlockPos homePos = creature->homePosition();
        if (creature->x() > static_cast<f64>(homePos.x)) {
            offsetX -= rng.nextDouble() * range / 2.0;
        } else {
            offsetX += rng.nextDouble() * range / 2.0;
        }
        if (creature->z() > static_cast<f64>(homePos.z)) {
            offsetZ -= rng.nextDouble() * range / 2.0;
        } else {
            offsetZ += rng.nextDouble() * range / 2.0;
        }
    }

    // 加上实体当前位置
    return BlockPos(static_cast<i32>(std::floor(offsetX + creature->x())),
        offset.y + static_cast<i32>(std::floor(creature->y())),
        static_cast<i32>(std::floor(offsetZ + creature->z())));
}

BlockPos RandomPositionGenerator::moveUpOutOfSolid(
    const BlockPos& pos, i32 maxY, const std::function<bool(const BlockPos&)>& isSolid)
{
    // 如果起始位置不是固体，直接返回
    if (!isSolid(pos)) {
        return pos;
    }

    // 向上移动直到找到非固体方块或到达 maxY
    i32 x = pos.x;
    i32 y = pos.y + 1;
    i32 z = pos.z;

    while (y <= maxY) {
        BlockPos current(x, y, z);
        if (!isSolid(current)) {
            return current;
        }
        ++y;
    }

    // 如果到达 maxY 仍在固体中，返回 maxY 位置
    return BlockPos(x, maxY, z);
}

BlockPos RandomPositionGenerator::moveUpToAboveSolid(
    const BlockPos& pos, i32 aboveSolidAmount, i32 maxY, const std::function<bool(const BlockPos&)>& isSolid)
{
    MC_ASSERT_RELEASE(aboveSolidAmount >= 0);

    // 如果起始位置不是固体，直接返回
    if (!isSolid(pos)) {
        return pos;
    }

    // 第一阶段：向上移动直到离开固体方块
    i32 x = pos.x;
    i32 y = pos.y + 1;
    i32 z = pos.z;

    while (y <= maxY && isSolid(BlockPos(x, y, z))) {
        ++y;
    }

    // 记录第一个非固体方块的 Y 坐标
    i32 firstAirY = y;

    // 第二阶段：继续向上移动 aboveSolidAmount 格
    for (i32 i = 0; i < aboveSolidAmount && y <= maxY; ++i) {
        ++y;
        if (isSolid(BlockPos(x, y, z))) {
            // 遇到固体方块，回退一格
            --y;
            break;
        }
    }

    return BlockPos(x, y, z);
}

bool RandomPositionGenerator::isSolidAt(CreatureEntity* creature, const BlockPos& pos)
{
    IWorld* world = creature->world();
    if (!world) return false;

    const BlockState* state = world->getBlockState(pos);
    return state != nullptr && state->isSolid();
}

bool RandomPositionGenerator::isWaterAt(CreatureEntity* creature, const BlockPos& pos)
{
    IWorld* world = creature->world();
    if (!world) return false;

    return world->isWaterAt(pos.x, pos.y, pos.z);
}

bool RandomPositionGenerator::hasPathfindingMalus(CreatureEntity* creature, const BlockPos& pos)
{
    // 检查实体在该位置的寻路惩罚值是否非零
    // 对应 MC 的 GoalUtils.hasMalus：检查路径类型的惩罚值
    f32 pathWeight = creature->getPathWeight(
        static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
    // getPathWeight 对于危险位置（岩浆等）返回负值或很低的值
    // 对于蜜蜂等飞行实体，空气位置返回较高值
    // 惩罚值非零意味着该位置不理想
    // 使用与 calculatePositionScore 类似的逻辑：岩浆位置视为有惩罚
    IWorld* world = creature->world();
    if (world && (world->isLavaAt(pos.x, pos.y, pos.z) || world->isLavaAt(pos.x, pos.y - 1, pos.z))) {
        return true;
    }
    return false;
}

bool RandomPositionGenerator::isMobRestricted(CreatureEntity* creature, f64 range)
{
    if (!creature->hasHome()) {
        return false;
    }
    BlockPos home = creature->homePosition();
    f64 dx = creature->x() - static_cast<f64>(home.x);
    f64 dy = creature->y() - static_cast<f64>(home.y);
    f64 dz = creature->z() - static_cast<f64>(home.z);
    f64 distSq = dx * dx + dy * dy + dz * dz;
    f64 homeRadius = static_cast<f64>(creature->maximumHomeDistance());
    f64 threshold = homeRadius + range + 1.0;
    return distSq < threshold * threshold;
}

bool RandomPositionGenerator::isOutsideBuildHeight(CreatureEntity* creature, const BlockPos& pos)
{
    IWorld* world = creature->world();
    if (!world) return true;
    return pos.y < world->getMinBuildHeight() || pos.y >= world->getMaxBuildHeight();
}

} // namespace mc::entity::ai::util
