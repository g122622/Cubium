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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/RedstoneDiodeBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石中继器方块
 *
 * 红石中继器可以延长和延迟红石信号，还可以锁定信号状态。
 *
 * ## 特性
 * - 信号再生：输出始终为15强度
 * - 延迟可调：1-4档，对应2-8 tick延迟
 * - 锁定机制：侧面有信号时锁定当前输出
 * - 方向性：只能从背面输入，正面输出
 *
 * ## 容易踩的坑
 * - 延迟 = 档位 × 2 tick
 * - 锁定时保持当前输出不变
 * - 面向其他中继器时使用更高优先级
 * - updatePostPlacement 需要更新 LOCKED 状态
 *
 * 参考: net.minecraft.block.RepeaterBlock
 */
class RedstoneRepeaterBlock : public RedstoneDiodeBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneRepeaterBlock(const BlockProperties& properties);

    // ========== Block 接口重写 ==========

    /**
     * @brief 更新方块状态
     *
     * 重写以更新 LOCKED 状态。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 红石二极管接口实现 ==========

    [[nodiscard]] i32 getDelay(const BlockState& state) const override;

    [[nodiscard]] bool shouldBePowered(IWorld& world, const BlockPos& pos, const BlockState& state) const override;

    [[nodiscard]] bool isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const override;

    // ========== 中继器特有方法 ==========

    /**
     * @brief 获取延迟档位
     *
     * @param state 方块状态
     * @return i32 延迟档位（1-4）
     */
    [[nodiscard]] static i32 getDelaySetting(const BlockState& state);

    /**
     * @brief 设置延迟档位
     *
     * @param state 方块状态
     * @param delay 延迟档位（1-4）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withDelay(BlockState state, i32 delay);

    /**
     * @brief 检查是否锁定
     *
     * @param state 方块状态
     * @return true 如果锁定
     */
    [[nodiscard]] static bool isLockedState(const BlockState& state);

    /**
     * @brief 设置锁定状态
     *
     * @param state 方块状态
     * @param locked 是否锁定
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withLocked(BlockState state, bool locked);

    /**
     * @brief 右键交互 - 调整延迟档位
     *
     * 右键点击中继器可以在 1-4 档延迟之间循环切换。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

private:
    /// 最小延迟档位
    static constexpr i32 MIN_DELAY = 1;
    /// 最大延迟档位
    static constexpr i32 MAX_DELAY = 4;
    /// 每档延迟 tick 数
    static constexpr i32 DELAY_MULTIPLIER = 2;
};

} // namespace blocks
} // namespace mc
