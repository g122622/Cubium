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

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 拉杆方块
 *
 * 拉杆可以持续输出红石信号。
 *
 * ## 特性
 * - 可以切换开关状态
 * - 持续输出信号（不像按钮会自动复位）
 * - 可附着在不同面上
 * - 输出最大信号强度（15）
 *
 * ## 容易踩的坑
 * - 附着面变化时需要检测支撑
 * - 方向和附着面的组合复杂
 * - 需要正确处理所有输出方向
 *
 * 参考: net.minecraft.block.LeverBlock
 */
class LeverBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LeverBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

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

    // ========== 拉杆特有方法 ==========

    /**
     * @brief 检查拉杆是否开启
     *
     * @param state 方块状态
     * @return true 如果开启
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 设置拉杆的开关状态
     *
     * @param state 方块状态
     * @param powered 是否开启
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

    /**
     * @brief 获取拉杆朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 切换拉杆状态
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return BlockState 切换后的状态
     */
    static BlockState toggle(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /**
     * @brief 播放点击音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param powered true为开启，false为关闭
     */
    static void _playClickSound(IWorld& world, const BlockPos& pos, bool powered);

    /**
     * @brief 通知相邻方块更新
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    static void _notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
