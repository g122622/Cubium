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

#include "FireBlock.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 灵魂火方块
 *
 * 在下界生成的蓝色火焰，伤害更高。
 * 只能在灵魂沙或灵魂土上点燃。
 *
 * MC ID: minecraft:soul_fire
 *
 * 参考: net.minecraft.block.SoulFireBlock
 */
class SoulFireBlock : public FireBlock {
public:
    explicit SoulFireBlock(const BlockProperties& properties);
    ~SoulFireBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== Tick ==========
    // vanilla 中 SoulFireBlock 继承 BaseFireBlock（非 FireBlock），完全不参与刻：不老化、不蔓延，
    // 仅靠 updatePostPlacement 在失去灵魂沙/土支撑时自毁。本项目 SoulFireBlock 为复用 FireBlock 的
    // 放置/碰撞逻辑而继承 FireBlock，但 FireBlock 的 tick/getAge 依赖 AGE_0_15 属性，而 SoulFireBlock
    // 构造时把状态容器覆盖为空（无 age，与 vanilla 一致）。若不隔离 fire 专属的 tick 逻辑，
    // FireBlock::tick（被 onBlockAdded 调度的计划刻触发）会 getAge→state.get(AGE_0_15) 抛
    // std::invalid_argument，冒泡为 fatal。故在此重写 tick 为空、ticksRandomly()=false，对齐 vanilla 行为。
    // （FireBlock 已迁移到计划刻范式且 ticksRandomly()=false，SoulFireBlock 继承 onBlockAdded 会调度
    // 一次计划刻，命中此空 tick 后不再续期，灵魂火不参与刻，符合 vanilla。）
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;
    [[nodiscard]] bool ticksRandomly() const noexcept override { return false; }

    /**
     * @brief 检查方块是否可以作为灵魂火的基座
     *
     * @param block 要检查的方块
     * @return 如果方块是灵魂沙或灵魂土，返回 true
     */
    [[nodiscard]] static bool isSoulFireBase(const Block* block);

protected:
    [[nodiscard]] bool canBurn(IBlockReader& world, const BlockPos& pos) const override;
};

} // namespace blocks
} // namespace mc
