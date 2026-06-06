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

#include "../building/TrapDoorBlock.hpp"
#include "WeatheringCopperBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 可风化铜活板门方块
 *
 * 继承TrapDoorBlock的活板门行为，同时具有铜氧化特性。
 *
 * 参考: net.minecraft.block.OxidizableTrapDoorBlock
 */
class WeatheringCopperTrapDoorBlock : public TrapDoorBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 初始氧化等级
     */
    WeatheringCopperTrapDoorBlock(const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringCopperTrapDoorBlock() override = default;

    /**
     * @brief 获取当前氧化等级
     */
    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const { return m_oxidationLevel; }

    /**
     * @brief 设置下一氧化等级对应的方块
     */
    void setNextOxidationBlock(Block* nextBlock) { m_nextOxidationBlock = nextBlock; }

    /**
     * @brief 随机Tick - 尝试氧化
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
    }

protected:
    /// 当前氧化等级
    BlockStateProperties::OxidationLevel m_oxidationLevel;

    /// 下一氧化等级对应的方块
    Block* m_nextOxidationBlock = nullptr;
};

/**
 * @brief 涂蜡铜活板门方块
 *
 * 涂蜡后的铜活板门不会氧化。
 */
class WaxedCopperTrapDoorBlock : public TrapDoorBlock {
public:
    using TrapDoorBlock::TrapDoorBlock;
};

} // namespace blocks
} // namespace mc
