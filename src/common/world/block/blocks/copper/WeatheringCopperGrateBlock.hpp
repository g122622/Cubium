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

#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/capability/IWaterLoggable.hpp"
#include "common/world/block/capability/oxidation/WeatheringCopperBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 可风化铜格栅方块
 *
 * 透明方块，支持含水，具有铜氧化特性。
 *
 */
class WeatheringCopperGrateBlock : public WeatheringCopperBlock, public IWaterLoggable {
public:
    WeatheringCopperGrateBlock(const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringCopperGrateBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 遮光亮度恒为 1.0（原版 `TransparentBlock#getShadeBrightness`）
     *
     * 铜格栅碰撞形状是整格但满是镂空，光能透过去，不该投下阴影。
     */
    [[nodiscard]] f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 涂蜡铜格栅方块
 *
 * 涂蜡后的铜格栅不会氧化，支持含水。
 *
 */
class WaxedCopperGrateBlock : public WaxedCopperBlock, public IWaterLoggable {
public:
    explicit WaxedCopperGrateBlock(const BlockProperties& properties);

    ~WaxedCopperGrateBlock() override = default;

    /**
     * @brief 遮光亮度恒为 1.0（原版 `TransparentBlock#getShadeBrightness`）
     *
     * 涂蜡不改光学行为，与未涂蜡的铜格栅一致。
     */
    [[nodiscard]] f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
