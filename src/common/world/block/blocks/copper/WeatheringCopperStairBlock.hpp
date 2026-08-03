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

#include "../building/StairsBlock.hpp"
#include "IOxidizableBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 可风化铜楼梯方块
 *
 * 继承StairsBlock的楼梯行为，同时具有铜氧化特性。
 *
 * 参考: net.minecraft.block.OxidizableStairsBlock
 */
class WeatheringCopperStairBlock : public StairsBlock, public IOxidizableBlock {
public:
    WeatheringCopperStairBlock(const BlockState& baseState,
        const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringCopperStairBlock() override = default;

    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const override { return m_oxidationLevel; }

    [[nodiscard]] Block* getNextOxidationBlock() const override { return m_nextOxidationBlock; }

    void setNextOxidationBlock(Block* nextBlock) { m_nextOxidationBlock = nextBlock; }

    void setPreviousOxidationBlock(Block* prevBlock) { m_previousOxidationBlock = prevBlock; }

    [[nodiscard]] Block* getPreviousOxidationBlock() const override { return m_previousOxidationBlock; }

    [[nodiscard]] float getOxidationChanceModifier() const override
    {
        return m_oxidationLevel == BlockStateProperties::OxidationLevel::Unaffected ? 0.75f : 1.0f;
    }

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
    }

protected:
    BlockStateProperties::OxidationLevel m_oxidationLevel;
    Block* m_nextOxidationBlock = nullptr;
    Block* m_previousOxidationBlock = nullptr;
};

/**
 * @brief 涂蜡铜楼梯方块
 *
 * 涂蜡后的铜楼梯不会氧化。
 *
 * 参考: net.minecraft.block.StairsBlock (涂蜡变体不实现Oxidizable)
 */
class WaxedCopperStairBlock : public StairsBlock {
public:
    using StairsBlock::StairsBlock;
};

} // namespace blocks
} // namespace mc
