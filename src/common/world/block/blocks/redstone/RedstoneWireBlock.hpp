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

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <unordered_map>
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 红石线方块
 *
 * 红石线是红石系统的核心组件，负责信号传输和衰减。
 *
 * ## 核心机制
 * - 信号强度：0-15，每传输一格衰减1
 * - 连接状态：四个方向独立计算
 * - 十字形连接：信号向四个方向传输
 * - T形/L形连接：根据相邻方块动态调整
 *
 * ## 容易踩的坑
 * - 向上/向下连接需要特殊处理
 * - 信号传播顺序影响性能
 * - 更新时需要防止无限递归
 */
class RedstoneWireBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneWireBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 检查是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强信号强度
     *
     * 红石线也输出强信号（委托给 getWeakPower）。
     * 这是因为红石线可以直接充能相邻的实体方块。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 方向
     * @return i32 强信号强度
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 红石线特有方法 ==========

    /**
     * @brief 更新信号强度和连接状态
     */
    bool updatePower(IWorld& world, const BlockPos& pos);

    /**
     * @brief 计算连接状态
     */
    [[nodiscard]] BlockState calculateConnections(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查指定方向的连接类型
     */
    [[nodiscard]] BlockStateProperties::RedstoneSide getConnection(
        IWorld& world, const BlockPos& pos, Direction direction) const;

    /**
     * @brief 判断方块是否可以连接红石
     */
    [[nodiscard]] static bool canConnectTo(const BlockState& state);

    /**
     * @brief 判断方块是否可以在指定方向连接红石
     *
     * - 红石线总是可以连接
     * - 中继器/比较器只有输出端朝向该方向时才连接
     * - 观察者只有输出端朝向该方向时才连接
     * - 其他方块通过 canProvidePower 或 canConnectRedstone 判断
     *
     * @param state 方块状态
     * @param side 连接方向（从红石线指向相邻方块的方向）
     * @return true 如果可以连接
     */
    [[nodiscard]] static bool canConnectTo(const BlockState& state, Direction side);

    /**
     * @brief 检查方块是否是实体方块
     */
    [[nodiscard]] static bool isNormalCube(const BlockState& state);

    /**
     * @brief 获取当前信号强度
     */
    [[nodiscard]] static i32 getPower(const BlockState& state);

    /**
     * @brief 设置信号强度
     */
    [[nodiscard]] static BlockState withPower(BlockState state, i32 power);

    /**
     * @brief 获取红石线连接属性
     * 使用 BlockStateProperties::REDSTONE_NORTH() 等方法
     */

    // ========== Block 接口实现 ==========

    /**
     * @brief 获取方块形状
     *
     * 红石线形状由中心点和四个方向的连接组成：
     * - 中心点: (3, 0, 3) -> (13, 1, 13)
     * - 水平连接: 向各方向延伸到边缘
     * - 向上连接: 包含向上爬升的部分
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（红石线无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return VoxelShapes::empty();
    }

    /**
     * @brief 检查是否可以使用形状进行光照遮挡
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 右键交互 - 切换十字/点状连接
     *
     * 右键点击红石线可以在十字和点状连接之间切换。
     * 这个功能用于控制红石信号的传播方向。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

private:
    /**
     * @brief 计算输入信号强度
     */
    [[nodiscard]] i32 _calculateInputPower(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取相邻红石线的信号强度
     */
    [[nodiscard]] i32 _getWirePower(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 通知相邻红石组件更新
     */
    void _notifyWireNeighbors(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查是否是十字连接（四个方向都有连接）
     */
    [[nodiscard]] bool _isCrossConnection(const BlockState& state) const;

    /**
     * @brief 检查是否是点状连接（四个方向都没有连接）
     */
    [[nodiscard]] bool _isDotConnection(const BlockState& state) const;

    /**
     * @brief 创建点状连接状态（所有方向都无连接）
     */
    [[nodiscard]] BlockState _createDotState(const BlockState& state) const;

    /**
     * @brief 创建十字连接状态（所有方向都有 Side 连接）
     */
    [[nodiscard]] BlockState _createCrossState(const BlockState& state) const;

    /**
     * @brief 通知对角方向的方块更新
     *
     * 当连接状态改变时，通知对角方向的方块更新。
     */
    void _notifyDiagonalNeighbors(
        IWorld& world, const BlockPos& pos, const BlockState& oldState, const BlockState& newState);

    /// 临时变量：防止递归调用时检测自己的信号输出
    mutable bool m_canProvidePower = true;

    // ========== 形状缓存 ==========

    /**
     * @brief 计算给定状态的形状
     */
    [[nodiscard]] CollisionShape _computeShapeForState(const BlockState& state) const;

    /// 形状缓存：以连接状态为键（忽略POWER）
    mutable std::unordered_map<u32, CollisionShape> m_shapeCache;

    /// 基础形状常量
    static const CollisionShape s_centerShape;         // 中心点 (3, 0, 3) -> (13, 1, 13)
    static const CollisionShape s_northSideShape;      // 北面水平连接
    static const CollisionShape s_southSideShape;      // 南面水平连接
    static const CollisionShape s_eastSideShape;       // 东面水平连接
    static const CollisionShape s_westSideShape;       // 西面水平连接
    static const CollisionShape s_northAscendingShape; // 北面向上连接
    static const CollisionShape s_southAscendingShape; // 南面向上连接
    static const CollisionShape s_eastAscendingShape;  // 东面向上连接
    static const CollisionShape s_westAscendingShape;  // 西面向上连接
};

} // namespace blocks
} // namespace mc
