/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "../LightningRodBlock.hpp"
#include "WeatheringCopperBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 可风化避雷针方块
 *
 * 继承 LightningRodBlock 并实现 IOxidizableBlock 接口，
 * 使避雷针能够随时间氧化并通过斧头刮削退化。
 *
 * MC 原版中 WeatheringLightningRodBlock 扩展 LightningRodBlock
 * 并实现 WeatheringCopper（即 ChangeOverTimeBlock<WeatherState>）。
 * 本项目采用 IOxidizableBlock 接口替代 ChangeOverTimeBlock。
 *
 * 风化避雷针拥有与普通避雷针相同的方块状态属性（FACING, POWERED, WATERLOGGED），
 * 并额外通过 IOxidizableBlock 提供氧化链支持。
 * 当氧化等级非 Oxidized 时，方块会随机 tick 尝试氧化。
 *
 * 参考: net.minecraft.block.WeatheringLightningRodBlock (MC 1.21+)
 */
class WeatheringLightningRodBlock : public LightningRodBlock, public IOxidizableBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 初始氧化等级
     */
    WeatheringLightningRodBlock(const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringLightningRodBlock() override = default;

    // ========== IOxidizableBlock 接口实现 ==========

    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const override { return m_oxidationLevel; }

    [[nodiscard]] Block* getNextOxidationBlock() const override { return m_nextOxidationBlock; }

    [[nodiscard]] Block* getPreviousOxidationBlock() const override { return m_previousOxidationBlock; }

    void setNextOxidationBlock(Block* block) { m_nextOxidationBlock = block; }

    void setPreviousOxidationBlock(Block* block) { m_previousOxidationBlock = block; }

    // ========== 随机 Tick（氧化） ==========

    /**
     * @brief 随机 Tick - 尝试氧化
     *
     * 委托给 IOxidizableBlock::tryOxidize() 执行曼哈顿距离氧化算法。
     * 仅在非 Oxidized 等级时执行（由 m_ticksRandomly 控制）。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否随机 Tick
     *
     * 仅在非 Oxidized 等级时返回 true，避免对已完全氧化的方块浪费 tick。
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override;

    // ========== 放置 ==========

    /**
     * @brief 获取放置状态
     *
     * 与 LightningRodBlock 相同（FACING + POWERED + WATERLOGGED），
     * 氧化等级由成员变量持有，不进入 block state。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /// 氧化等级
    BlockStateProperties::OxidationLevel m_oxidationLevel;

    /// 氧化链：指向下一级氧化方块的指针
    Block* m_nextOxidationBlock = nullptr;

    /// 反向氧化链：指向上一级氧化方块的指针（斧头刮削用）
    Block* m_previousOxidationBlock = nullptr;
};

/**
 * @brief 涂蜡避雷针方块
 *
 * 涂蜡后的避雷针不会氧化，但保留所有避雷针功能
 * （红石信号输出、闪电吸引、含水支持等）。
 *
 * 涂蜡避雷针继承 LightningRodBlock（不实现 IOxidizableBlock），
 * 因此不会被氧化系统影响，也不会响应斧头刮削。
 * 蜂巢右键可以涂蜡，斧头右键可以除蜡。
 *
 * 参考: net.minecraft.block.LightningRodBlock (waxed variants, MC 1.21+)
 */
class WaxedLightningRodBlock : public LightningRodBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WaxedLightningRodBlock(const BlockProperties& properties);

    ~WaxedLightningRodBlock() override = default;
};

} // namespace blocks
} // namespace mc
