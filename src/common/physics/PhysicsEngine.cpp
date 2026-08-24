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

#include "PhysicsEngine.hpp"
#include "../util/math/MathConstants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockState.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mc {
namespace {

using math::EPSILON_COLLISION;
using math::EPSILON_GROUND_PROBE;

} // namespace

PhysicsEngine::PhysicsEngine(ICollisionWorld& world)
    : m_world(&world)
{}

Vector3 PhysicsEngine::moveEntity(AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight)
{
    return moveEntity(nullptr, entityBox, movement, stepHeight);
}

/**
 * @brief 移动实体并处理碰撞
 *
 * 核心流程：
 * 1. 收集潜在碰撞箱
 * 2. Y轴优先碰撞解决
 * 3. X/Z轴按移动幅度排序处理
 * 4. 尝试步进（如果水平碰撞且在地面）
 */
Vector3 PhysicsEngine::moveEntity(
    const Entity* entity, AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight)
{
    if (movement.x == 0.0f && movement.y == 0.0f && movement.z == 0.0f) {
        m_collidedVertically = false;
        m_collidedHorizontally = false;
        return Vector3::zero();
    }

    // 保存原始位置用于步进
    AxisAlignedBB originalBox = entityBox;

    // 构造实体碰撞上下文：entityBox 指向移动前的实体 AABB（用于细雪 isAbove 几何判定）。
    // descending 对齐 vanilla CollisionContext.isDescending() = Entity.isShiftKeyDown()（即潜行态，
    // Cubium 用 Entity::isSneaking()）：穿皮革靴的玩家按潜行键时主动陷入细雪（细雪返回 empty），
    // 非潜行时得完整碰撞箱可行走。注意 descending 与本次移动 Y 分量方向无关——自由下落不算 descending，
    // 否则可行走实体下落到细雪顶时会因 movement.y<0 误判 descending 而穿过细雪下沉。
    EntityCollisionContext ctx;
    ctx.entity = entity;
    ctx.entityBox = &entityBox;
    ctx.descending = entity != nullptr && entity->isSneaking();

    // 收集扩展范围内的碰撞箱
    // 使用 collisionBox.expand(vec)，不额外扩展
    // 但为了安全处理浮点误差，添加小量缓冲
    constexpr f32 COLLISION_BUFFER = 0.001f;
    AxisAlignedBB searchBox = entityBox.expand(std::abs(movement.x) + COLLISION_BUFFER,
        std::abs(movement.y) + COLLISION_BUFFER,
        std::abs(movement.z) + COLLISION_BUFFER);

    std::vector<AxisAlignedBB> boxes;
    collectCollisionBoxes(ctx, searchBox, boxes);

    // 处理初始轻微重叠（常见于浮点误差/网络同步边界）
    // 若不先去重叠，逐轴偏移算法不会把已嵌入地面的实体推出。
    f32 overlapPushUp = _resolveInitialOverlaps(entityBox, boxes);

    if (boxes.empty()) {
        // 无碰撞，直接移动
        entityBox.offset(movement.x, movement.y, movement.z);
        m_collidedVertically = false;
        m_collidedHorizontally = false;
        return movement;
    }

    // 执行碰撞解决
    Vector3 resolved = _resolveCollision(entityBox, movement, boxes);
    if (overlapPushUp > 0.0f) {
        resolved.y += overlapPushUp;
    }

    // 检测水平碰撞
    bool horizontalCollision = (std::abs(resolved.x - movement.x) > EPSILON_COLLISION ||
        std::abs(resolved.z - movement.z) > EPSILON_COLLISION);
    bool verticalCollision = (std::abs(resolved.y - movement.y) > EPSILON_COLLISION);

    // 尝试步进（仅当水平碰撞且之前在地面或有向下移动时）
    if (stepHeight > 0.0f && horizontalCollision && (movement.x != 0.0f || movement.z != 0.0f)) {
        // 检查: onGround || (verticalCollision && movement.y < 0)
        bool wasOnGround = isOnGround(ctx.entity, originalBox);
        if (wasOnGround || (verticalCollision && movement.y < 0.0f)) {
            Vector3 stepped = _attemptStepUp(entityBox, originalBox, movement, stepHeight, resolved, ctx);

            // 使用水平距离平方比较
            f32 resolvedHorizontalSq = resolved.x * resolved.x + resolved.z * resolved.z;
            f32 steppedHorizontalSq = stepped.x * stepped.x + stepped.z * stepped.z;

            if (steppedHorizontalSq > resolvedHorizontalSq + EPSILON_COLLISION) {
                // 更新碰撞状态
                m_collidedVertically = (std::abs(stepped.y - movement.y) > EPSILON_COLLISION);
                m_collidedHorizontally = (std::abs(stepped.x - movement.x) > EPSILON_COLLISION ||
                    std::abs(stepped.z - movement.z) > EPSILON_COLLISION);
                return stepped;
            }
        }
    }

    // 更新碰撞状态
    m_collidedVertically = verticalCollision;
    m_collidedHorizontally = horizontalCollision;

    return resolved;
}

bool PhysicsEngine::isOnGround(const AxisAlignedBB& entityBox) const
{
    return isOnGround(nullptr, entityBox);
}

bool PhysicsEngine::isOnGround(const Entity* entity, const AxisAlignedBB& entityBox) const
{
    // 向下微移后检测碰撞。增大探测深度以提高浮点抖动容忍度，
    // 避免平地移动时偶发"离地一帧"导致累计下沉。
    AxisAlignedBB testBox = entityBox.offsetted(0.0f, -math::EPSILON_GROUND_PROBE, 0.0f);

    // 计算搜索范围（只需要检测底部一排方块）
    i32 minX = static_cast<i32>(std::floor(testBox.minX));
    i32 maxX = static_cast<i32>(std::ceil(testBox.maxX) - 1);
    i32 minY = static_cast<i32>(std::floor(testBox.minY));
    i32 maxY = static_cast<i32>(std::ceil(testBox.maxY) - 1); // 可能跨越两个Y层级
    i32 minZ = static_cast<i32>(std::floor(testBox.minZ));
    i32 maxZ = static_cast<i32>(std::ceil(testBox.maxZ) - 1);

    // 构造实体碰撞上下文：isAbove 几何判定须用实体真实位置（未下移的 entityBox），而非 testBox
    // （testBox 下移 0.01 会导致实体站在细雪顶时 isAbove 误判为 false，细雪返回 empty 误判离地）。
    // descending 取 false（地面检测不依赖下降态）。
    EntityCollisionContext ctx;
    ctx.entity = entity;
    ctx.entityBox = &entityBox;
    ctx.descending = false;

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (!m_world->isWithinWorldBounds(x, y, z)) continue;

                const BlockState* state = m_world->getBlockState(x, y, z);
                if (!state || state->isAir()) continue;

                const CollisionShape& shape = state->getCollisionShapeForEntity(ctx, y);
                if (shape.isEmpty()) continue;

                // 检测是否与测试框相交
                if (shape.intersects(testBox, x, y, z)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void PhysicsEngine::collectCollisionBoxes(const AxisAlignedBB& searchBox, std::vector<AxisAlignedBB>& boxes) const
{
    EntityCollisionContext emptyCtx;
    collectCollisionBoxes(emptyCtx, searchBox, boxes);
}

void PhysicsEngine::collectCollisionBoxes(
    const EntityCollisionContext& ctx, const AxisAlignedBB& searchBox, std::vector<AxisAlignedBB>& boxes) const
{
    boxes.clear();

    // 计算方块范围
    i32 minX = static_cast<i32>(std::floor(searchBox.minX));
    i32 maxX = static_cast<i32>(std::floor(searchBox.maxX));
    i32 minY = static_cast<i32>(std::floor(searchBox.minY));
    i32 maxY = static_cast<i32>(std::floor(searchBox.maxY));
    i32 minZ = static_cast<i32>(std::floor(searchBox.minZ));
    i32 maxZ = static_cast<i32>(std::floor(searchBox.maxZ));

    // 遍历所有可能的方块
    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                _getBlockCollisionBoxes(x, y, z, ctx, boxes);
            }
        }
    }
}

/**
 * @brief 核心碰撞解决算法
 *
 * 逐轴处理碰撞：
 * 1. 先处理Y轴（重力最重要）
 * 2. 按移动幅度处理X/Z轴（幅度大的先处理）
 *
 * 注意：每次轴移动后更新entityBox位置
 */
Vector3 PhysicsEngine::_resolveCollision(
    AxisAlignedBB& entityBox, const Vector3& movement, const std::vector<AxisAlignedBB>& boxes)
{
    f32 dx = movement.x;
    f32 dy = movement.y;
    f32 dz = movement.z;

    // 1. Y轴优先处理
    if (dy != 0.0f) {
        for (const auto& box : boxes) {
            dy = entityBox.calculateYOffset(box, dy);
        }
        entityBox.offset(0.0f, dy, 0.0f);
    }

    // 2. X/Z按移动幅度排序处理
    if (std::abs(dx) >= std::abs(dz)) {
        // X轴优先
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
    } else {
        // Z轴优先
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
    }

    return Vector3(dx, dy, dz);
}

/**
 * @brief 计算水平距离平方
 */
inline f32 horizontalMagSq(const Vector3& v)
{
    return v.x * v.x + v.z * v.z;
}

Vector3 PhysicsEngine::_applyHorizontalCollision(
    AxisAlignedBB& entityBox, const Vector3& movement, const std::vector<AxisAlignedBB>& boxes)
{
    f32 dx = movement.x;
    f32 dz = movement.z;

    // X/Z按移动幅度排序（标准逻辑）
    if (std::abs(dx) >= std::abs(dz)) {
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
    } else {
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
    }

    return Vector3(dx, 0.0f, dz);
}

/**
 * @brief 尝试步进
 *
 * 使用三种策略竞争最优结果：
 * 1. 策略A：整体抬起stepHeight后水平移动
 * 2. 策略B：先抬起，然后水平移动
 * 3. 策略C：在抬起高度不足时，在部分抬起高度水平移动
 *
 * 最后选择水平移动距离最远的策略。
 *
 * @param entityBox 实体碰撞箱（会被修改）
 * @param originalBox 移动前的原始碰撞箱
 * @param movement 期望移动向量
 * @param stepHeight 步进高度
 * @return 实际移动向量
 */
Vector3 PhysicsEngine::_attemptStepUp(AxisAlignedBB& entityBox,
    const AxisAlignedBB& originalBox,
    const Vector3& movement,
    f32 stepHeight,
    const Vector3& fallbackResult,
    const EntityCollisionContext& ctx)
{
    // =====================================================
    // 策略A：整体抬起 + 水平移动
    // =====================================================
    AxisAlignedBB strategyABox = originalBox;
    Vector3 strategyA = _tryStepStrategyA(strategyABox, movement, stepHeight, ctx);

    // =====================================================
    // 策略B：先抬起后水平移动
    // =====================================================
    AxisAlignedBB strategyBBox = originalBox;
    f32 actualStepUp = 0.0f; // 记录实际抬起高度
    Vector3 strategyB = _tryStepStrategyBWithHeight(strategyBBox, movement, stepHeight, actualStepUp, ctx);

    // 选择水平移动距离最远的策略
    Vector3 bestResult = strategyA;
    AxisAlignedBB bestBox = strategyABox;
    f32 bestDistSq = horizontalMagSq(strategyA);
    f32 bestStepY = strategyA.y; // 策略A的Y位移

    f32 bDistSq = horizontalMagSq(strategyB);
    if (bDistSq > bestDistSq) {
        bestResult = strategyB;
        bestBox = strategyBBox;
        bestDistSq = bDistSq;
        bestStepY = actualStepUp;
    }

    // =====================================================
    // 策略C：当策略B抬起高度不足stepHeight时，在部分抬起高度水平移动
    // =====================================================
    if (actualStepUp < stepHeight && actualStepUp > 0.0f) {
        AxisAlignedBB strategyCBox = originalBox;
        Vector3 strategyC = _tryStepStrategyC(strategyCBox, movement, actualStepUp, ctx);

        f32 cDistSq = horizontalMagSq(strategyC);
        if (cDistSq > bestDistSq) {
            bestResult = strategyC;
            bestBox = strategyCBox;
            bestDistSq = cDistSq;
            bestStepY = strategyC.y;
        }
    }

    // 与直接碰撞结果比较
    f32 fallbackDistSq = horizontalMagSq(fallbackResult);
    if (bestDistSq > fallbackDistSq) {
        // 最后下落回地面
        // 使用 -stepY + movementY，即从步进后位置下落到目标Y位置
        f32 fallDistance = -bestStepY + movement.y;
        Vector3 fallDown = _applyFallDown(bestBox, fallDistance, ctx);
        entityBox = bestBox;
        // 计算最终移动向量（X和Z已经移动过了，Y是下落距离）
        return Vector3(bestBox.minX - originalBox.minX, fallDown.y, bestBox.minZ - originalBox.minZ);
    }

    return fallbackResult;
}

/**
 * @brief 策略A：整体抬起 + 水平移动 + 下落
 *
 * 将抬起和水平移动作为一个整体处理。
 * 返回从原始位置到最终位置的完整移动向量。
 */
Vector3 PhysicsEngine::_tryStepStrategyA(
    AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight, const EntityCollisionContext& ctx)
{
    // 保存原始位置
    f32 origMinX = entityBox.minX;
    f32 origMinY = entityBox.minY;
    f32 origMinZ = entityBox.minZ;

    // 创建抬起后的碰撞箱
    AxisAlignedBB raisedBox = entityBox.offsetted(0.0f, stepHeight, 0.0f);

    // 收集抬起位置的碰撞箱
    AxisAlignedBB searchBox = raisedBox.expand(
        std::abs(movement.x) + EPSILON_COLLISION, EPSILON_COLLISION, std::abs(movement.z) + EPSILON_COLLISION);
    // 局部上下文：isAbove 几何判定须用当前操作的 raisedBox 位置（抬起后），而非 moveEntity 入口位置。
    EntityCollisionContext localCtx = ctx;
    localCtx.entityBox = &raisedBox;
    std::vector<AxisAlignedBB> boxes;
    collectCollisionBoxes(localCtx, searchBox, boxes);

    // 在抬起状态下尝试水平移动
    _applyHorizontalCollision(raisedBox, movement, boxes);

    entityBox = raisedBox;
    // 返回从原始位置的移动距离
    return Vector3(raisedBox.minX - origMinX, raisedBox.minY - origMinY, raisedBox.minZ - origMinZ);
}

/**
 * @brief 策略B：先抬起 -> 水平移动
 *
 * 标准步进逻辑。
 * 返回从原始位置到最终位置的完整移动向量。
 */
Vector3 PhysicsEngine::_tryStepStrategyB(
    AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight, const EntityCollisionContext& ctx)
{
    f32 unused = 0.0f;
    return _tryStepStrategyBWithHeight(entityBox, movement, stepHeight, unused, ctx);
}

/**
 * @brief 策略B：先抬起 -> 水平移动（带高度输出）
 *
 * 标准步进逻辑。
 * 返回从原始位置到最终位置的完整移动向量。
 * @param actualStepUp 输出实际抬起高度
 */
Vector3 PhysicsEngine::_tryStepStrategyBWithHeight(AxisAlignedBB& entityBox,
    const Vector3& movement,
    f32 stepHeight,
    f32& actualStepUp,
    const EntityCollisionContext& ctx)
{
    // 保存原始位置
    f32 origMinX = entityBox.minX;
    f32 origMinY = entityBox.minY;
    f32 origMinZ = entityBox.minZ;

    // Step 1: 尝试向上抬起
    AxisAlignedBB raisedBox = entityBox;
    f32 upDist = stepHeight;

    // 收集原始位置的碰撞箱用于向上抬起检测
    AxisAlignedBB upSearchBox = entityBox.expand(EPSILON_COLLISION, stepHeight + EPSILON_COLLISION, EPSILON_COLLISION);
    EntityCollisionContext localCtx = ctx;
    localCtx.entityBox = &entityBox;
    std::vector<AxisAlignedBB> upBoxes;
    collectCollisionBoxes(localCtx, upSearchBox, upBoxes);

    for (const auto& box : upBoxes) {
        upDist = raisedBox.calculateYOffset(box, upDist);
    }

    // 向上移动
    raisedBox.offset(0.0f, upDist, 0.0f);
    actualStepUp = upDist; // 记录实际抬起高度

    // Step 2: 在抬起状态下水平移动
    AxisAlignedBB horizontalSearchBox = raisedBox.expand(
        std::abs(movement.x) + EPSILON_COLLISION, EPSILON_COLLISION, std::abs(movement.z) + EPSILON_COLLISION);
    localCtx.entityBox = &raisedBox;
    std::vector<AxisAlignedBB> horizontalBoxes;
    collectCollisionBoxes(localCtx, horizontalSearchBox, horizontalBoxes);

    _applyHorizontalCollision(raisedBox, movement, horizontalBoxes);

    entityBox = raisedBox;
    // 返回从原始位置的移动距离
    return Vector3(raisedBox.minX - origMinX, raisedBox.minY - origMinY, raisedBox.minZ - origMinZ);
}

/**
 * @brief 策略C：在部分抬起高度水平移动
 *
 * 当策略B抬起高度不足stepHeight时，在部分抬起高度尝试水平移动。
 * 这是第三种步进策略，用于处理台阶等特殊情况。
 */
Vector3 PhysicsEngine::_tryStepStrategyC(
    AxisAlignedBB& entityBox, const Vector3& movement, f32 partialStepHeight, const EntityCollisionContext& ctx)
{
    // 保存原始位置
    f32 origMinX = entityBox.minX;
    f32 origMinY = entityBox.minY;
    f32 origMinZ = entityBox.minZ;

    // Step 1: 先抬起到partialStepHeight高度
    AxisAlignedBB raisedBox = entityBox;
    f32 upDist = partialStepHeight;

    // 收集碰撞箱用于向上抬起检测
    AxisAlignedBB upSearchBox =
        entityBox.expand(EPSILON_COLLISION, partialStepHeight + EPSILON_COLLISION, EPSILON_COLLISION);
    EntityCollisionContext localCtx = ctx;
    localCtx.entityBox = &entityBox;
    std::vector<AxisAlignedBB> upBoxes;
    collectCollisionBoxes(localCtx, upSearchBox, upBoxes);

    for (const auto& box : upBoxes) {
        upDist = raisedBox.calculateYOffset(box, upDist);
    }

    // 向上移动
    raisedBox.offset(0.0f, upDist, 0.0f);

    // Step 2: 在部分抬起高度水平移动（不使用完整stepHeight）
    AxisAlignedBB horizontalSearchBox = raisedBox.expand(
        std::abs(movement.x) + EPSILON_COLLISION, EPSILON_COLLISION, std::abs(movement.z) + EPSILON_COLLISION);
    localCtx.entityBox = &raisedBox;
    std::vector<AxisAlignedBB> horizontalBoxes;
    collectCollisionBoxes(localCtx, horizontalSearchBox, horizontalBoxes);

    _applyHorizontalCollision(raisedBox, movement, horizontalBoxes);

    entityBox = raisedBox;
    // 返回从原始位置的移动距离
    return Vector3(raisedBox.minX - origMinX, raisedBox.minY - origMinY, raisedBox.minZ - origMinZ);
}

/**
 * @brief 应用下落直到碰到地面
 */
Vector3 PhysicsEngine::_applyFallDown(
    AxisAlignedBB& entityBox, f32 originalYMovement, const EntityCollisionContext& ctx)
{
    // 收集下落路径上的碰撞箱
    // Y轴扩展：原始移动距离 + 1.0f 用于处理步进后的额外下落空间
    AxisAlignedBB fallSearchBox =
        entityBox.expand(EPSILON_COLLISION, std::abs(originalYMovement) + 1.0f, EPSILON_COLLISION);
    // descending 继承上游 ctx（ctx.descending = Entity::isSneaking()，潜行态），不强制为 true：
    // 自由下落不算 descending——否则可行走实体（如狐狸/穿皮革靴牛）下落到细雪顶时进入此下落分支，
    // descending=true 致 getCollisionShapeForEntity 的 !descending 条件失败，细雪返回 empty，
    // 实体穿过细雪下沉（对齐 vanilla CollisionContext.isDescending = isShiftKeyDown 语义）。
    // isAbove 几何判定须用当前 entityBox 位置（更新指向移动后框）。
    EntityCollisionContext localCtx = ctx;
    localCtx.entityBox = &entityBox;
    std::vector<AxisAlignedBB> fallBoxes;
    collectCollisionBoxes(localCtx, fallSearchBox, fallBoxes);

    f32 downDist = originalYMovement;
    for (const auto& box : fallBoxes) {
        downDist = entityBox.calculateYOffset(box, downDist);
    }
    entityBox.offset(0.0f, downDist, 0.0f);

    return Vector3(0.0f, downDist, 0.0f);
}

void PhysicsEngine::_getBlockCollisionBoxes(
    i32 x, i32 y, i32 z, const EntityCollisionContext& ctx, std::vector<AxisAlignedBB>& boxes) const
{
    if (!m_world->isWithinWorldBounds(x, y, z)) return;

    const BlockState* state = m_world->getBlockState(x, y, z);
    if (!state || state->isAir()) return;

    const CollisionShape& shape = state->getCollisionShapeForEntity(ctx, y);
    if (shape.isEmpty()) return;

    // 获取世界坐标碰撞箱
    auto worldBoxes = shape.getWorldBoxes(x, y, z);
    boxes.insert(boxes.end(), worldBoxes.begin(), worldBoxes.end());
}

f32 PhysicsEngine::_resolveInitialOverlaps(AxisAlignedBB& entityBox, const std::vector<AxisAlignedBB>& boxes) const
{
    // 仅处理小范围向上推出，避免对合法穿插场景造成过度修正
    constexpr f32 MAX_DEPENETRATION_UP = 0.45f;
    constexpr f32 EPSILON = 1e-4f;

    f32 maxPushUp = 0.0f;
    for (const auto& box : boxes) {
        if (!entityBox.intersects(box)) {
            continue;
        }

        // 仅考虑把实体从"地面/下方方块"向上推出
        f32 pushUp = box.maxY - entityBox.minY;
        if (pushUp > 0.0f && pushUp <= MAX_DEPENETRATION_UP) {
            maxPushUp = std::max(maxPushUp, pushUp);
        }
    }

    if (maxPushUp > 0.0f) {
        const f32 pushed = maxPushUp + EPSILON;
        entityBox.offset(0.0f, pushed, 0.0f);
        return pushed;
    }

    return 0.0f;
}

} // namespace mc
