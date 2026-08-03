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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 日光探测器方块
 *
 * 日光探测器根据天空光照输出红石信号。
 * 信号强度取决于时间、天气和是否被遮挡。
 *
 * ## 特性
 * - 根据天空光照计算信号强度
 * - 可以被右键切换为"夜间模式"（反向）
 * - 输出强度范围：0-15
 *
 * ## 容易踩的坑
 * - 需要正确获取天空光照
 * - 夜间模式需要反转信号
 * - 遮挡会影响输出
 *
 * 参考: net.minecraft.block.DaylightDetectorBlock
 */
class DaylightDetectorBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DaylightDetectorBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 日光探测器特有方法 ==========

    /**
     * @brief 获取当前信号强度
     *
     * @param state 方块状态
     * @return i32 信号强度（0-15）
     */
    [[nodiscard]] static i32 getPower(const BlockState& state);

    /**
     * @brief 设置信号强度
     *
     * @param state 方块状态
     * @param power 信号强度
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPower(BlockState state, i32 power);

    /**
     * @brief 检查是否为夜间模式（反向）
     *
     * @param state 方块状态
     * @return true 如果是夜间模式
     */
    [[nodiscard]] static bool isInverted(const BlockState& state);

    /**
     * @brief 设置夜间模式
     *
     * @param state 方块状态
     * @param inverted 是否为夜间模式
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withInverted(BlockState state, bool inverted);

    /**
     * @brief 切换模式
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    static void toggleMode(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /// 更新延迟（tick）
    static constexpr i32 UPDATE_DELAY = 20;

    /**
     * @brief 计算当前信号强度
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param inverted 是否为夜间模式
     * @return i32 计算出的信号强度
     */
    [[nodiscard]] static i32 _calculateSignalStrength(IWorld& world, const BlockPos& pos, bool inverted);

    /**
     * @brief 更新信号强度
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    static void _updatePower(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 通知相邻方块更新
     *
     * @param world 世界引用
     * @param pos 方块位置
     */
    static void _notifyNeighbors(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
