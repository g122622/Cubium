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

#include "common/world/block/blocks/decorative/StandingBannerBlock.hpp"

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ========== StandingBannerBlock ==========

StandingBannerBlock::StandingBannerBlock(const BlockProperties& properties, DyeColor color)
    : AbstractBannerBlock(properties, color)
{
    // vanilla 1.21.11 BannerBlock 仅有 rotation 属性（无 waterlogged）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::ROTATION_0_15(), 0));

    // 旗杆碰撞形状：中心8x16x8像素
    m_shape = CollisionShape::box(4.0 / 16.0, 0.0, 4.0 / 16.0, 12.0 / 16.0, 1.0, 12.0 / 16.0);
}

BlockState StandingBannerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家朝向计算旋转值（0-15，每22.5度）
    i32 rotation = static_cast<i32>(std::floor((180.0f + context.getPlayerYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    return defaultState().with(BlockStateProperties::ROTATION_0_15(), rotation);
}

bool StandingBannerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 需要下方方块是实心的
    const BlockState* below = world.getBlockState(pos.down());
    return below != nullptr && below->isSolid();
}

BlockState StandingBannerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    MC_UNUSED(world);

    // 下方方块被移除时旗帜变为空气
    if (facing == Direction::Down) {
        const BlockState* below = world.getBlockState(currentPos.down());
        if (below == nullptr || !below->isSolid()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const BlockState& StandingBannerBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& StandingBannerBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& StandingBannerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
