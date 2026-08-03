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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/Block.hpp"
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 绊线方块
 *
 * 绊线是一种由实体触发红石信号的方块，需要配合绊线钩使用。
 *
 * ## 特性
 * - 检测实体碰撞
 * - 与绊线钩配合使用
 * - 最大长度42格
 * - 被剪断时掉落线
 * - 需要支撑方块
 */
class TripWireBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TripWireBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取绊线形状
     *
     * - ATTACHED=true: 绷紧状态，形状为 (0, 1, 0) -> (16, 2.5, 16)
     * - ATTACHED=false: 松弛状态，形状为 (0, 0, 0) -> (16, 8, 16)
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（绊线无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return VoxelShapes::empty();
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== 绊线特有方法 ==========

    /**
     * @brief 检查绊线是否被触发
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 检查绊线是否连接
     * @param state 方块状态
     * @param direction 方向
     * @return true 如果连接
     */
    [[nodiscard]] static bool isConnected(const BlockState& state, Direction direction);

    /**
     * @brief 检查绊线是否被触发（实体检测）
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isActivated(const BlockState& state);

    /**
     * @brief 更新绊线状态
     *
     * 检查实体碰撞并更新状态
     *
     * @param world 世界引用
     * @param pos 绊线位置
     */
    void updateState(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查是否应该连接到指定方向的方块
     *
     * 如果相邻方块是绊线钩，检查其 FACING 是否朝向当前方向；
     * 如果相邻方块是绊线，返回 true；
     * 其他情况返回 false。
     *
     * @param neighborState 相邻方块的状态
     * @param direction 当前检测的方向（从当前方块指向相邻方块）
     * @return true 如果应该连接
     */
    [[nodiscard]] bool shouldConnectTo(const BlockState& neighborState, Direction direction) const;

private:
    /**
     * @brief 检测实体碰撞
     * @param world 世界引用
     * @param pos 绊线位置
     * @return true 如果有实体碰撞
     */
    [[nodiscard]] bool _checkEntityCollision(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 通知绊线钩更新
     * @param world 世界引用
     * @param pos 绊线位置
     */
    void _notifyHooks(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
