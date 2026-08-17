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

namespace mc {

// 前向声明
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 绊线钩方块
 *
 * 绊线钩是绊线系统的触发器，用于检测绊线的状态变化并输出红石信号。
 *
 * ## 特性
 * - 可附着在方块侧面
 * - 与绊线配合使用
 * - 最大检测距离42格
 * - 红石信号输出
 * - 被剪断时触发
 */
class TripWireHookBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TripWireHookBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    // 放置朝向：facing = 玩家点击的墙面（水平四向）。绊线钩只能附在墙面（无 Floor/Ceiling 概念），
    // 朝向直接等于点击的水平面。若点击面非水平（Up/Down，理论上不会发生因 isValidPosition 限制），
    // 回退到玩家水平朝向。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒
    // North），与预期按点击墙面决定朝向的行为不一致。重写后修正。注意 HORIZONTAL_FACING 是水平四向
    // 枚举，不可写入 Up/Down（会越界），须先水平化收窄。
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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
     * @brief 获取绊线钩形状
     *
     * 根据FACING方向返回不同的形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== 绊线钩特有方法 ==========

    /**
     * @brief 检查绊线钩是否被触发
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 检查绊线钩是否连接
     * @param state 方块状态
     * @return true 如果连接
     */
    [[nodiscard]] static bool isConnected(const BlockState& state);

    /**
     * @brief 获取绊线钩朝向
     * @param state 方块状态
     * @return Direction 朝向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 设置触发状态
     * @param state 方块状态
     * @param powered 是否触发
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

    /**
     * @brief 设置连接状态
     * @param state 方块状态
     * @param connected 是否连接
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withConnected(BlockState state, bool connected);

private:
    /**
     * @brief 计算并更新绊线钩状态
     * @param world 世界引用
     * @param pos 绊线钩位置
     * @param facing 朝向
     * @param currentState 当前方块状态
     * @param shouldTriggerOnChange 是否在状态改变时触发
     * @return true 如果成功连接
     */
    bool _calculateState(IWorld& world,
        const BlockPos& pos,
        Direction facing,
        const BlockState& currentState,
        bool shouldTriggerOnChange);

    /**
     * @brief 检测绊线链
     * @param world 世界引用
     * @param pos 绊线钩位置
     * @param facing 朝向
     * @param outOtherHookPos 输出：另一端绊线钩位置
     * @return true 如果找到完整的绊线链
     */
    [[nodiscard]] bool _checkForTripwire(
        IWorld& world, const BlockPos& pos, Direction facing, BlockPos& outOtherHookPos) const;
};

} // namespace blocks
} // namespace mc
