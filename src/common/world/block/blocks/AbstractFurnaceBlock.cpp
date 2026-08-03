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

#include "AbstractFurnaceBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

AbstractFurnaceBlock::AbstractFurnaceBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::LIT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::LIT(), false));
}

// ========== 放置和更新 ==========

BlockState AbstractFurnaceBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 熔炉朝向玩家的反方向
    Direction facing = context.horizontalDirection();
    Direction opposite = Directions::opposite(facing);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), opposite)
        .with(BlockStateProperties::LIT(), false);
}

// ========== 光照 ==========

u8 AbstractFurnaceBlock::getLightLevel(const BlockState& state, IWorld* /*world*/, const BlockPos* /*pos*/) const
{
    // 对应 MC 1.21.11 litBlockEmission(13)：LIT=true 时发光 13，否则不发光
    return state.get(BlockStateProperties::LIT()) ? 13 : 0;
}

// ========== 交互 ==========

BlockActionResult AbstractFurnaceBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (interactWith(world, pos, player)) {
        return ActionResultType::Consume;
    }

    return world.asServerWorld() == nullptr ? ActionResultType::Success : ActionResultType::Pass;
}

// ========== 红石 ==========

i32 AbstractFurnaceBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return 0;
    }

    // 检查是否是熔炉实体
    if (blockEntity->getType() != BlockEntityType::Furnace && blockEntity->getType() != BlockEntityType::BlastFurnace &&
        blockEntity->getType() != BlockEntityType::Smoker) {
        return 0;
    }

    auto* furnace = static_cast<blockentity::AbstractFurnaceEntity*>(blockEntity);
    return furnace->getComparatorSignal();
}

// ========== 旋转和镜像 ==========

const BlockState& AbstractFurnaceBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& AbstractFurnaceBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

// ========== 静态工具方法 ==========

bool AbstractFurnaceBlock::isLit(const BlockState& state)
{
    return state.get(BlockStateProperties::LIT());
}

} // namespace blocks
} // namespace mc
