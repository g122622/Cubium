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
 * @brief 发光地衣方块
 *
 * 可附着在任意六个面的发光方块，支持含水。
 * 根据附着的面数量决定形状和光照。
 * 每个面是一个1像素厚的薄板，多个面激活时组合为联合形状。
 * 预计算64种形状组合（2^6 = NORTH|SOUTH|EAST|WEST|UP|DOWN）。
 */
class GlowLichenBlock : public MultifaceBlock {
public:
    explicit GlowLichenBlock(const BlockProperties& properties);

    ~GlowLichenBlock() override = default;

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

    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /**
     * @brief 根据六个面的激活状态计算形状索引
     * @param down 下面是否激活
     * @param up 上面是否激活
     * @param north 北面是否激活
     * @param south 南面是否激活
     * @param east 东面是否激活
     * @param west 西面是否激活
     * @return 形状索引 (0-63)
     */
    [[nodiscard]] static size_t _getShapeIndex(bool down, bool up, bool north, bool south, bool east, bool west);

    /// 预计算的形状缓存（64种组合：2^6，六个布尔面属性）
    std::array<CollisionShape, 64> m_shapes;
};

} // namespace blocks
} // namespace mc
