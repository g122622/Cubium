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

#include "ShelfBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/ShelfBlockEntity.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// ShelfBlock 构造函数
// ============================================================================

ShelfBlock::ShelfBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // HorizontalBlock 已添加 HORIZONTAL_FACING，需额外添加 POWERED、SIDE_CHAIN_PART、WATERLOGGED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::SIDE_CHAIN_PART())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::SIDE_CHAIN_PART(), BlockStateProperties::SideChainPart::Unconnected)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ============================================================================
// 放置和更新
// ============================================================================

BlockState ShelfBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state =
        defaultState()
            .with(FACING(), Directions::opposite(context.horizontalDirection()))
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::SIDE_CHAIN_PART(), BlockStateProperties::SideChainPart::Unconnected)
            .with(BlockStateProperties::WATERLOGGED(), false);

    // 检测放置位置是否含水
    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState ShelfBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态的流体 tick 调度
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

// ============================================================================
// 红石
// ============================================================================

i32 ShelfBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 从 ShelfBlockEntity 读取3位二进制编码的比较器信号
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Shelf) {
        auto* shelf = static_cast<blockentity::ShelfBlockEntity*>(blockEntity);
        return shelf->getAnalogOutputSignal();
    }

    return 0;
}

// ============================================================================
// 方块实体
// ============================================================================

std::unique_ptr<BlockEntity> ShelfBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::ShelfBlockEntity>(pos);
}

// ============================================================================
// IWaterLoggable 接口实现
// ============================================================================

const fluid::FluidState* ShelfBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ============================================================================
// 旋转和镜像
// ============================================================================

const BlockState& ShelfBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 旋转书架的朝向（HORIZONTAL_FACING）
    Direction facing = state.get(FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(FACING(), newFacing);
}

const BlockState& ShelfBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 镜像书架的朝向
    Direction facing = state.get(FACING());
    Direction newFacing = facing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

// ============================================================================
// 状态容器
// ============================================================================

void ShelfBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

} // namespace blocks
} // namespace mc
