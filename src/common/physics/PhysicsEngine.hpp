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

#pragma once

#include "../core/Constants.hpp"
#include "../util/AxisAlignedBB.hpp"
#include "../util/math/Vector3.hpp"
#include "../world/block/Block.hpp"
#include "PhysicsConstants.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <vector>

namespace mc {

using world::chunk::ChunkData;

// 前向声明：PhysicsEngine 仅持有 Entity 指针透传给方块碰撞形状判定，无需完整类型。
class Entity;

/**
 * @brief 碰撞世界接口
 *
 * 提供世界碰撞查询的抽象接口。
 * ClientWorld和ServerWorld需要实现此接口以支持物理引擎。
 *
 * 注意：所有坐标参数都是方块坐标（整数）
 */
class ICollisionWorld {
public:
    virtual ~ICollisionWorld() = default;

    /**
     * @brief 获取指定位置的方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态指针（如果超出世界范围或空气，返回nullptr）
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否在世界范围内
     * @param x, y, z 方块坐标
     * @return 是否在有效范围内
     */
    [[nodiscard]] virtual bool isWithinWorldBounds(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取区块数据
     * @param x, z 区块坐标
     * @return 区块数据指针（如果未加载返回nullptr）
     */
    [[nodiscard]] virtual const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 获取世界最小Y坐标
     */
    [[nodiscard]] virtual i32 getMinBuildHeight() const { return world::MIN_BUILD_HEIGHT; }

    /**
     * @brief 获取世界最大Y坐标
     */
    [[nodiscard]] virtual i32 getMaxBuildHeight() const { return world::MAX_BUILD_HEIGHT; }
};

/**
 * @brief 物理引擎
 *
 * 实现兼容的物理系统，包括：
 * - 逐轴碰撞检测和解决
 * - 重力和跳跃
 * - 自动步进（stairs/slabs）
 * - 地面检测
 *
 * 使用方法：
 * 1. 创建PhysicsEngine实例，传入ICollisionWorld实现
 * 2. 调用moveEntity()处理带碰撞的移动
 * 3. 使用isOnGround()检测是否在地面
 *
 * 需要注意物理常量定义在 PhysicsConstants.hpp 中。
 */
class PhysicsEngine {
public:
    explicit PhysicsEngine(ICollisionWorld& world);

    /**
     * @brief 带碰撞检测的实体移动
     *
     * 算法：
     * 1. 收集实体AABB扩展范围内的所有方块碰撞箱
     * 2. Y轴优先处理（重力/跳跃）
     * 3. X/Z按移动幅度排序处理
     * 4. 尝试step-up（如果水平碰撞且在地面）
     * 5. 更新碰撞状态
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param stepHeight 步进高度（玩家0.6，其他实体可能不同）
     * @return 实际移动向量（碰撞后）
     */
    Vector3 moveEntity(AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight = 0.0f);

    /**
     * @brief 带实体上下文的碰撞检测移动
     *
     * 与 moveEntity 算法相同，额外把实体碰撞上下文（entity 指针、实体 AABB、是否下降）
     * 透传给方块碰撞形状判定，使需要按实体区分碰撞形状的方块（如细雪 PowderSnowBlock：
     * 可行走实体得完整碰撞箱、下落实体得半穿透形状）能在物理移动中正确生效。
     *
     * 对齐 vanilla Entity.move 中经 EntityCollisionContext 传递实体的碰撞解析。
     *
     * @param entity 触发移动的实体（用于构造碰撞上下文）
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param stepHeight 步进高度
     * @return 实际移动向量（碰撞后）
     */
    Vector3 moveEntity(const Entity* entity, AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight = 0.0f);

    /**
     * @brief 检测实体是否在地面
     * @param entityBox 实体碰撞箱
     * @return 是否在地面（下方有支撑）
     */
    [[nodiscard]] bool isOnGround(const AxisAlignedBB& entityBox) const;

    /**
     * @brief 带实体上下文检测是否在地面
     *
     * 与 isOnGround 相同，额外把实体上下文透传给方块碰撞形状判定（如细雪可行走实体
     * 在细雪上方视为有支撑）。用于实体自身的地面检测（Entity::move / checkOnGround）。
     *
     * @param entity 触发检测的实体
     * @param entityBox 实体碰撞箱
     * @return 是否在地面
     */
    [[nodiscard]] bool isOnGround(const Entity* entity, const AxisAlignedBB& entityBox) const;

    /**
     * @brief 收集范围内的碰撞箱
     * @param searchBox 搜索范围
     * @param boxes 输出的碰撞箱列表
     */
    void collectCollisionBoxes(const AxisAlignedBB& searchBox, std::vector<AxisAlignedBB>& boxes) const;

    /**
     * @brief 带实体上下文收集范围内的碰撞箱
     *
     * 与 collectCollisionBoxes 相同，额外把实体上下文透传给方块碰撞形状判定。
     * 供需要按实体区分碰撞形状的场景（如玩家潜行边缘检测、自动跳跃）使用。
     *
     * @param ctx 实体碰撞上下文
     * @param searchBox 搜索范围
     * @param boxes 输出的碰撞箱列表
     */
    void collectCollisionBoxes(
        const EntityCollisionContext& ctx, const AxisAlignedBB& searchBox, std::vector<AxisAlignedBB>& boxes) const;

    /**
     * @brief 设置碰撞世界
     */
    void setWorld(ICollisionWorld& world) { m_world = &world; }

    /**
     * @brief 获取碰撞世界
     */
    [[nodiscard]] ICollisionWorld* getWorld() const { return m_world; }

    /**
     * @brief 检测上次移动是否有垂直碰撞
     */
    [[nodiscard]] bool collidedVertically() const { return m_collidedVertically; }

    /**
     * @brief 检测上次移动是否有水平碰撞
     */
    [[nodiscard]] bool collidedHorizontally() const { return m_collidedHorizontally; }

private:
    /**
     * @brief 尝试将实体从初始重叠中向上推出
     *
     * 逐轴碰撞算法假设初始时不与方块重叠。
     * 当浮点误差导致实体轻微嵌入地面时，先做一次向上去重叠，
     * 避免后续 calculateYOffset 无法修正而持续下陷。
     */
    [[nodiscard]] f32 _resolveInitialOverlaps(AxisAlignedBB& entityBox, const std::vector<AxisAlignedBB>& boxes) const;

    /**
     * @brief 核心碰撞解决
     *
     * 逐轴处理碰撞：
     * 1. Y轴优先（重力）
     * 2. X/Z按移动幅度排序
     * 3. 每次移动后更新entityBox位置
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param boxes 碰撞箱列表
     * @return 实际移动向量
     */
    Vector3 _resolveCollision(
        AxisAlignedBB& entityBox, const Vector3& movement, const std::vector<AxisAlignedBB>& boxes);

    /**
     * @brief 尝试步进
     *
     * 当水平方向移动受阻时，尝试抬起实体继续移动。
     * 使用三种策略竞争最优结果：
     * 1. 策略A：整体抬起stepHeight后水平移动
     * 2. 策略B：先抬起，然后水平移动
     *
     * 最后选择水平移动距离最远的结果。
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param originalBox 移动前的原始碰撞箱
     * @param movement 期望移动向量
     * @param stepHeight 步进高度
     * @param fallbackResult 直接碰撞结果（作为备选）
     * @return 实际移动向量
     */
    Vector3 _attemptStepUp(AxisAlignedBB& entityBox,
        const AxisAlignedBB& originalBox,
        const Vector3& movement,
        f32 stepHeight,
        const Vector3& fallbackResult,
        const EntityCollisionContext& ctx);

    /**
     * @brief 策略A：整体抬起 + 水平移动
     *
     * 将抬起和水平移动作为一个整体处理。
     */
    Vector3 _tryStepStrategyA(
        AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight, const EntityCollisionContext& ctx);

    /**
     * @brief 策略B：先抬起后水平移动（带高度输出）
     *
     * 标准步进逻辑。
     * @param actualStepUp 输出参数，实际抬起的高度
     */
    Vector3 _tryStepStrategyBWithHeight(AxisAlignedBB& entityBox,
        const Vector3& movement,
        f32 stepHeight,
        f32& actualStepUp,
        const EntityCollisionContext& ctx);

    /**
     * @brief 策略B：先抬起后水平移动
     *
     * 标准步进逻辑。
     */
    Vector3 _tryStepStrategyB(
        AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight, const EntityCollisionContext& ctx);

    /**
     * @brief 策略C：在部分抬起高度水平移动
     *
     * 当策略B抬起不足stepHeight时，在部分抬起高度尝试水平移动。
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param partialStepHeight 实际抬起高度（小于stepHeight）
     * @return 从原始位置的移动距离
     */
    Vector3 _tryStepStrategyC(
        AxisAlignedBB& entityBox, const Vector3& movement, f32 partialStepHeight, const EntityCollisionContext& ctx);

    /**
     * @brief 应用下落直到碰到地面
     */
    Vector3 _applyFallDown(AxisAlignedBB& entityBox, f32 originalYMovement, const EntityCollisionContext& ctx);

    /**
     * @brief 应用水平移动碰撞解决
     *
     * 按移动幅度排序处理X/Z轴碰撞。
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量（仅使用x和z分量）
     * @param boxes 碰撞箱列表
     * @return 实际水平移动向量
     */
    static Vector3 _applyHorizontalCollision(
        AxisAlignedBB& entityBox, const Vector3& movement, const std::vector<AxisAlignedBB>& boxes);

    /**
     * @brief 获取方块碰撞箱
     * @param x, y, z 方块坐标
     * @param ctx 实体碰撞上下文（entity 可能为 nullptr，表示无实体上下文）
     * @param boxes 输出的碰撞箱列表
     */
    void _getBlockCollisionBoxes(
        i32 x, i32 y, i32 z, const EntityCollisionContext& ctx, std::vector<AxisAlignedBB>& boxes) const;

    ICollisionWorld* m_world;
    bool m_collidedVertically = false;
    bool m_collidedHorizontally = false;
};

} // namespace mc
