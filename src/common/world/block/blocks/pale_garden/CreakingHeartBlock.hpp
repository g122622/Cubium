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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 嘎枝之心方块
 *
 * 苍白橡木中的核心方块，具有AXIS、CREAKING_HEART_STATE和NATURAL属性。
 * 支持红石比较器输出。
 *
 * 参考: net.minecraft.block.CreakingHeartBlock
 */
class CreakingHeartBlock : public RotatedPillarBlock {
public:
    explicit CreakingHeartBlock(const BlockProperties& properties);

    ~CreakingHeartBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块更新：调度 1 刻后的周期检查
     *
     * 对齐 vanilla CreakingHeartBlock.updateShape（:94）：任一方向邻居变化即
     * scheduleTick(1)，由 tick 重新解析状态（两端原木被破坏/补齐时切换 uprooted↔dormant/awake）。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 周期检查：重解析状态并写回
     *
     * 对齐 vanilla CreakingHeartBlock.tick（:99-104）。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否有比较器输入覆盖
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取比较器信号强度
     *
     * Uprooted=0, Dormant=1, Awake=2
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 判断轴两端是否为同轴苍白橡木原木（对齐 vanilla hasRequiredLogs）
     *
     * 沿 state 的 AXIS 正负两向各取一块，须均属于 PALE_OAK_LOGS 标签且其 AXIS 与该轴一致。
     */
    [[nodiscard]] static bool hasRequiredLogs(const BlockState& state, IWorld& world, const BlockPos& pos);

    /**
     * @brief 重解析状态（对齐 vanilla updateState）
     *
     * 若轴两端满足 hasRequiredLogs 且当前为 UPROOTED，则按 CREAKING_ACTIVE 环境属性
     * 置 AWAKE（活跃）或 DORMANT（休眠）；否则状态不变。
     */
    [[nodiscard]] static BlockState updateState(const BlockState& state, IWorld& world, const BlockPos& pos);

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
