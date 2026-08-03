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

#include "HopperBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/transport/HopperEntity.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

HopperBlock::HopperBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING_EXCEPT_UP())
            .add(BlockStateProperties::ENABLED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING_EXCEPT_UP(), Direction::Down)
            .with(BlockStateProperties::ENABLED(), true));
    _initShapes();
}

BlockState HopperBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.getClickedFace();
    Direction outputDir = Directions::opposite(facing);
    if (outputDir == Direction::Up) {
        outputDir = Direction::Down;
    }

    return defaultState()
        .with(BlockStateProperties::FACING_EXCEPT_UP(), outputDir)
        .with(BlockStateProperties::ENABLED(), true);
}

void HopperBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    _updateState(world, pos, state);
}

void HopperBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (state != nullptr) {
        _updateState(world, pos, *state);
    }
}

const CollisionShape& HopperBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    return m_shapes[static_cast<size_t>(facing)];
}

const CollisionShape& HopperBlock::getRaytraceShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    return m_raytraceShapes[static_cast<size_t>(facing)];
}

std::unique_ptr<BlockEntity> HopperBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::HopperEntity>(pos);
}

BlockActionResult HopperBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.asServerWorld() == nullptr) {
        return ActionResultType::Success;
    }

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Hopper) {
        auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
        if (world.openContainer(ContainerType::Hopper, pos, player)) {
            hopper->openContainer(&player);
            return ActionResultType::Consume;
        }
    }

    return ActionResultType::Pass;
}

i32 HopperBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Hopper) {
        return 0;
    }

    auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
    IInventory* inventory = hopper->getInventory();
    if (inventory == nullptr) {
        return 0;
    }

    i32 totalItems = 0;
    i32 totalSlots = inventory->getContainerSize();
    for (i32 i = 0; i < totalSlots; ++i) {
        const ItemStack& stack = inventory->getItem(i);
        if (!stack.isEmpty()) {
            totalItems += stack.getCount();
        }
    }

    i32 maxItems = totalSlots * 64;
    if (maxItems == 0) {
        return 0;
    }

    f32 fillRatio = static_cast<f32>(totalItems) / static_cast<f32>(maxItems);
    i32 signal = static_cast<i32>(fillRatio * 14.0f);
    if (totalItems > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

void HopperBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{

    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Hopper) {
        return;
    }

    auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
    hopper->onEntityCollision(world, &entity);
}

const BlockState& HopperBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    if (rotated == Direction::Up) {
        return state;
    }
    return state.with(BlockStateProperties::FACING_EXCEPT_UP(), rotated);
}

const BlockState& HopperBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

Direction HopperBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING_EXCEPT_UP());
}

bool HopperBlock::isEnabled(const BlockState& state)
{
    return state.get(BlockStateProperties::ENABLED());
}

void HopperBlock::_updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
    const bool enabled = !powered;
    if (enabled != isEnabled(state)) {
        const BlockState newState = state.with(BlockStateProperties::ENABLED(), enabled);
        world.setBlockState(pos, &newState, 2);
    }
}

void HopperBlock::_initShapes()
{
    CollisionShape inputShape = CollisionShape::fromPixelBox(0, 10, 0, 16, 16, 16);
    CollisionShape middleShape = CollisionShape::fromPixelBox(4, 4, 4, 12, 10, 12);
    CollisionShape baseShape = CollisionShape::combine(inputShape, middleShape, CollisionShape::CombineOp::OR);

    CollisionShape downSpout = CollisionShape::fromPixelBox(6, 0, 6, 10, 4, 10);
    m_shapes[static_cast<size_t>(Direction::Down)] =
        CollisionShape::combine(baseShape, downSpout, CollisionShape::CombineOp::OR);

    CollisionShape northSpout = CollisionShape::fromPixelBox(6, 4, 0, 10, 8, 4);
    m_shapes[static_cast<size_t>(Direction::North)] =
        CollisionShape::combine(baseShape, northSpout, CollisionShape::CombineOp::OR);

    CollisionShape southSpout = CollisionShape::fromPixelBox(6, 4, 12, 10, 8, 16);
    m_shapes[static_cast<size_t>(Direction::South)] =
        CollisionShape::combine(baseShape, southSpout, CollisionShape::CombineOp::OR);

    CollisionShape westSpout = CollisionShape::fromPixelBox(0, 4, 6, 4, 8, 10);
    m_shapes[static_cast<size_t>(Direction::West)] =
        CollisionShape::combine(baseShape, westSpout, CollisionShape::CombineOp::OR);

    CollisionShape eastSpout = CollisionShape::fromPixelBox(12, 4, 6, 16, 8, 10);
    m_shapes[static_cast<size_t>(Direction::East)] =
        CollisionShape::combine(baseShape, eastSpout, CollisionShape::CombineOp::OR);

    m_shapes[static_cast<size_t>(Direction::Up)] = baseShape;

    CollisionShape insideBowl = CollisionShape::fromPixelBox(2, 11, 2, 14, 16, 14);
    m_raytraceShapes[static_cast<size_t>(Direction::Down)] = insideBowl;
    m_raytraceShapes[static_cast<size_t>(Direction::Up)] = insideBowl;
    m_raytraceShapes[static_cast<size_t>(Direction::North)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(6, 8, 0, 10, 10, 4), CollisionShape::CombineOp::OR);
    m_raytraceShapes[static_cast<size_t>(Direction::South)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(6, 8, 12, 10, 10, 16), CollisionShape::CombineOp::OR);
    m_raytraceShapes[static_cast<size_t>(Direction::West)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(0, 8, 6, 4, 10, 10), CollisionShape::CombineOp::OR);
    m_raytraceShapes[static_cast<size_t>(Direction::East)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(12, 8, 6, 16, 10, 10), CollisionShape::CombineOp::OR);
}

} // namespace blocks
} // namespace mc
