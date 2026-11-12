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

#include "SculkSensorBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 校准幽匿感测体
 *
 * 可通过红石信号过滤振动频率的高级感测体。
 * 状态属性：FACING, SCULK_SENSOR_PHASE, POWER, WATERLOGGED
 */
class CalibratedSculkSensorBlock : public SculkSensorBlock {
public:
    explicit CalibratedSculkSensorBlock(const BlockProperties& properties);

    ~CalibratedSculkSensorBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 校准幽匿感测体仅在非输入面方向输出红石信号
     *
     * FACING 方向是输入面（从该方向读取红石信号频率过滤），
     * 红石信号只在非 FACING 方向输出。
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    /// 校准感测体活跃阶段更短（10 tick）
    static constexpr i32 CALIBRATED_ACTIVE_TICKS = 10;

    [[nodiscard]] i32 getActiveTicks() const override
    {
        return CALIBRATED_ACTIVE_TICKS;
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
