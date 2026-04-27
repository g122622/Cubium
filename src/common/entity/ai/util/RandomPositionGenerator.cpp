#include "RandomPositionGenerator.hpp"
#include "../../core/CreatureEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../pathfinding/PathNavigator.hpp"
#include "../pathfinding/PathPoint.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>
#include <algorithm>

namespace mc::entity::ai::util {

using namespace math;

// ==================== 公开方法实现 ====================

bool RandomPositionGenerator::findRandomTarget(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    Vector3& outPos
) {
    return findRandomTargetTowards(creature, xzRange, yRange, Vector3::ZERO, outPos);
}

bool RandomPositionGenerator::findRandomTargetBlockAwayFrom(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    const Vector3& avoidPos,
    Vector3& outPos
) {
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
        awayDirection = Vector3(
            rng.nextFloat() * 2.0f - 1.0f,
            0.0f,
            rng.nextFloat() * 2.0f - 1.0f
        ).normalized();
    }

    return findBestPosition(creature, xzRange, yRange, awayDirection, outPos);
}

bool RandomPositionGenerator::findRandomTargetTowards(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    const Vector3& targetPos,
    Vector3& outPos
) {
    if (!creature) return false;

    Vector3 directionBias(0.0f, 0.0f, 0.0f);

    // 如果有目标位置，计算方向偏好
    if (targetPos.lengthSquared() > 0.001f) {
        Vector3 creaturePos(creature->x(), creature->y(), creature->z());
        directionBias = (targetPos - creaturePos).normalized();
    }

    return findBestPosition(creature, xzRange, yRange, directionBias, outPos);
}

bool RandomPositionGenerator::getLandPos(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    Vector3& outPos
) {
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

        i32 x = static_cast<i32>(std::floor(creature->x() + dx));
        i32 z = static_cast<i32>(std::floor(creature->z() + dz));

        // 寻找地面高度
        i32 groundY = getGroundHeight(world, x, static_cast<i32>(std::floor(creature->y() + dy)), z);

        if (groundY >= 0) {
            outPos = Vector3(
                static_cast<f32>(x) + 0.5f,
                static_cast<f32>(groundY),
                static_cast<f32>(z) + 0.5f
            );
            return true;
        }
    }

    return false;
}

bool RandomPositionGenerator::findRandomTargetAvoidWater(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    Vector3& outPos
) {
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    Random rng = creature->getRandom();

    // 尝试多次找到避开水域的位置
    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(xzRange);
        f32 dy = (rng.nextFloat() * 2.0f - 1.0f) * static_cast<f32>(yRange);

        i32 x = static_cast<i32>(std::floor(creature->x() + dx));
        i32 y = static_cast<i32>(std::floor(creature->y() + dy));
        i32 z = static_cast<i32>(std::floor(creature->z() + dz));

        // 检查不是水
        if (!world->isWaterAt(x, y, z) && !world->isWaterAt(x, y + 1, z)) {
            // 检查位置可行走
            if (isPositionWalkable(creature, x, y, z)) {
                outPos = Vector3(
                    static_cast<f32>(x) + 0.5f,
                    static_cast<f32>(y),
                    static_cast<f32>(z) + 0.5f
                );
                return true;
            }
        }
    }

    return false;
}

// ==================== 辅助方法实现 ====================

bool RandomPositionGenerator::isPositionWalkable(CreatureEntity* creature, i32 x, i32 y, i32 z) {
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
        return false;  // 被阻挡
    }
    if (block2 && !block2->isAir() && !block2->isLiquid()) {
        return false;  // 头部被阻挡
    }

    // 检查脚下方块是否可以站立
    if (groundBlock && !groundBlock->isAir() && !groundBlock->isLiquid()) {
        return true;
    }

    // 水中或岩浆中也可以游动（检查实体是否在水中）
    if (world->isWaterAt(x, y, z)) {
        return true;  // 水中可以游泳
    }

    return false;
}

i32 RandomPositionGenerator::getGroundHeight(IWorld* world, i32 x, i32 startY, i32 z) {
    if (!world) return -1;

    // 从指定高度向下搜索
    for (i32 y = startY; y > startY - MAX_GROUND_SEARCH && y >= 0; --y) {
        const BlockState* block = world->getBlockState(x, y, z);
        if (block && !block->isAir() && !block->isLiquid()) {
            return y + 1;  // 返回地面上的Y坐标
        }
    }

    return -1;
}

f32 RandomPositionGenerator::calculatePositionScore(CreatureEntity* creature, const Vector3& pos) {
    if (!creature) return -1000.0f;

    IWorld* world = creature->world();
    if (!world) return -1000.0f;

    f32 score = 0.0f;

    i32 x = static_cast<i32>(std::floor(pos.x));
    i32 y = static_cast<i32>(std::floor(pos.y));
    i32 z = static_cast<i32>(std::floor(pos.z));

    // 基础评分
    score += 10.0f;

    // 距离评分：不要太近也不要太远
    f32 distSq = creature->distanceSqTo(pos.x, pos.y, pos.z);
    if (distSq < MIN_DISTANCE_SQ) {
        score -= 50.0f;  // 太近
    } else if (distSq > 400.0f) {  // 20格以上
        score -= 10.0f;  // 太远
    }

    // 安全性评分
    if (world->isLavaAt(x, y, z) || world->isLavaAt(x, y - 1, z)) {
        score -= 100.0f;  // 岩浆危险
    }
    // 水中位置对大多数生物来说是可接受的
    if (world->isWaterAt(x, y, z)) {
        // 水中位置，略微降低评分
        score -= 5.0f;
    }

    // 可达性评分（简化版）
    if (isPositionWalkable(creature, x, y, z)) {
        score += 20.0f;
    } else {
        score -= 50.0f;
    }

    // 方块类型评分
    const BlockState* groundBlock = world->getBlockState(x, y - 1, z);
    if (groundBlock) {
        // 某些方块类型会影响评分
        // TODO: 根据方块类型调整评分
    }

    return score;
}

// ==================== 私有方法实现 ====================

Vector3 RandomPositionGenerator::generateRandomOffset(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    const Vector3& directionBias
) {
    if (!creature) return Vector3::ZERO;

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

bool RandomPositionGenerator::validateAndAdjustPosition(
    CreatureEntity* creature,
    Vector3& pos
) {
    if (!creature) return false;

    IWorld* world = creature->world();
    if (!world) return false;

    i32 x = static_cast<i32>(std::floor(pos.x));
    i32 y = static_cast<i32>(std::floor(pos.y));
    i32 z = static_cast<i32>(std::floor(pos.z));

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
    if (groundY >= 0 && isPositionWalkable(creature, x, groundY, z)) {
        pos.y = static_cast<f32>(groundY);
        return true;
    }

    return false;
}

bool RandomPositionGenerator::findBestPosition(
    CreatureEntity* creature,
    i32 xzRange,
    i32 yRange,
    const Vector3& directionBias,
    Vector3& outPos
) {
    if (!creature) return false;

    Random rng = creature->getRandom();

    // MC 1.16.5: 尝试生成多个候选位置，选择评分最高的
    Vector3 bestPos;
    f32 bestScore = -10000.0f;
    bool found = false;

    for (i32 attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        Vector3 offset = generateRandomOffset(creature, xzRange, yRange, directionBias);

        Vector3 candidatePos(
            creature->x() + offset.x,
            creature->y() + offset.y,
            creature->z() + offset.z
        );

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
