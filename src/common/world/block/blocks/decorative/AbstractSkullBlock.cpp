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

#include "common/world/block/blocks/decorative/AbstractSkullBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// AbstractSkullBlock
// ============================================================================

NoteBlockInstrument AbstractSkullBlock::_instrumentForSkullType(SkullType type) noexcept
{
    switch (type) {
        case SkullType::Skeleton:
            return NoteBlockInstrument::Skeleton;
        case SkullType::WitherSkeleton:
            return NoteBlockInstrument::WitherSkeleton;
        case SkullType::Zombie:
            return NoteBlockInstrument::Zombie;
        case SkullType::Creeper:
            return NoteBlockInstrument::Creeper;
        case SkullType::Dragon:
            return NoteBlockInstrument::Dragon;
        case SkullType::Piglin:
            return NoteBlockInstrument::Piglin;
        case SkullType::Player:
            return NoteBlockInstrument::CustomHead;
        default:
            return NoteBlockInstrument::Harp;
    }
}

AbstractSkullBlock::AbstractSkullBlock(SkullType type, const BlockProperties& properties)
    : Block(properties)
    , m_type(type)
{
    // 乐器在各具体子类（SkullBlock / WallSkullBlock）构造函数里设置：两者都要设——站立头颅
    // 注册时带 `.instrument(...)`，墙面变体经 `Blocks.wallVariant(parent, true)` 继承它。
    // 不放在基类只是为了把「谁该带乐器」显式写在各子类里，语义上两者一致。

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::POWERED(), false));
}

std::unique_ptr<BlockEntity> AbstractSkullBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SkullBlockEntity>(pos);
}

BlockState AbstractSkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(BlockStateProperties::POWERED(),
        world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos()));
}

void AbstractSkullBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    if (world.isClientSide()) {
        return;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    const bool powered = world::redstone::RedstonePower::isPowered(world, pos);
    if (powered != state->get(BlockStateProperties::POWERED())) {
        BlockState newState = state->with(BlockStateProperties::POWERED(), powered);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }
}

bool AbstractSkullBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 这三个类族在原版里都显式返回 false：形状不完整不等于可以穿过。
    // 楼梯的台阶、墙的柱与臂、头颅的薄片都占据生物的路径空间。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
