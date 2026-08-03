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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石灯方块
 *
 * 红石灯在被充能时会点亮并发出光照等级15的光。
 *
 * ## 特性
 * - 默认状态：熄灭
 * - 被充能时：点亮，发出光照等级15
 * - 移除信号：熄灭
 * - 无延迟：即时响应红石信号
 *
 * 参考: net.minecraft.block.RedstoneLampBlock
 */
class RedstoneLampBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneLampBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石灯特有方法 ==========

    /**
     * @brief 检查红石灯是否点亮
     * @param state 方块状态
     * @return true 如果点亮
     */
    [[nodiscard]] static bool isLit(const BlockState& state) noexcept;

    /**
     * @brief 设置点亮状态
     * @param state 方块状态
     * @param lit 是否点亮
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withLit(BlockState state, bool lit) noexcept;
};

} // namespace blocks
} // namespace mc
