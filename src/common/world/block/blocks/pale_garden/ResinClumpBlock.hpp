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

#include "../MultifaceBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <cstddef>

namespace mc {
namespace blocks {

/**
 * @brief 树脂块（多面附着）方块
 *
 * 参考 MC 1.21.11: ResinClumpBlock (继承自 MultifaceBlock)
 * 可附着在任意六个面的树脂堆积物，支持含水。
 * 每个面是一个 1 像素厚的薄板，多个面激活时组合为联合形状。
 * 预计算 64 种形状组合（2^6 = DOWN|UP|NORTH|SOUTH|EAST|WEST）。
 *
 * 状态属性：DOWN, UP, NORTH, SOUTH, EAST, WEST, WATERLOGGED
 */
class ResinClumpBlock : public MultifaceBlock {
public:
    explicit ResinClumpBlock(const BlockProperties& properties);
    ~ResinClumpBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /**
     * @brief 根据六个面的激活状态计算形状索引
     * @return 形状索引 (0-63)
     */
    [[nodiscard]] static size_t _getShapeIndex(bool down, bool up, bool north, bool south, bool east, bool west);

    /// 预计算的形状缓存（64 种组合：2^6，六个布尔面属性）
    std::array<CollisionShape, 64> m_shapes;
};

} // namespace blocks
} // namespace mc
