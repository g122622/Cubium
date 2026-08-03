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

#include "TrapDoorBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

TrapDoorBlock::TrapDoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::OPEN())
            .add(BlockStateProperties::HALF())
            .add(BlockStateProperties::POWERED())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::OPEN(), false)
            .with(BlockStateProperties::HALF(), BlockStateProperties::Half::Bottom)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 关闭状态的碰撞形状（上半和下半不同）
    // 形状尺寸：厚度为 3/16 格
    constexpr f32 P = 1.0f / 16.0f;

    CollisionShape closedBottom = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 3.0f * P, 1.0f);
    CollisionShape closedTop = CollisionShape::box(0.0f, 13.0f * P, 0.0f, 1.0f, 1.0f, 1.0f);

    // 打开状态的碰撞形状（贴墙放置）
    CollisionShape openNorth = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 1.0f, 1.0f, 1.0f);
    CollisionShape openSouth = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 3.0f * P);
    CollisionShape openEast = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape openWest = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 1.0f, 1.0f);

    // 关闭状态的形状（上半和下半不同）
    m_shapes[_getShapeIndex(Direction::North, false, BlockStateProperties::Half::Bottom)] = closedBottom;
    m_shapes[_getShapeIndex(Direction::North, false, BlockStateProperties::Half::Top)] = closedTop;
    m_shapes[_getShapeIndex(Direction::South, false, BlockStateProperties::Half::Bottom)] = closedBottom;
    m_shapes[_getShapeIndex(Direction::South, false, BlockStateProperties::Half::Top)] = closedTop;
    m_shapes[_getShapeIndex(Direction::East, false, BlockStateProperties::Half::Bottom)] = closedBottom;
    m_shapes[_getShapeIndex(Direction::East, false, BlockStateProperties::Half::Top)] = closedTop;
    m_shapes[_getShapeIndex(Direction::West, false, BlockStateProperties::Half::Bottom)] = closedBottom;
    m_shapes[_getShapeIndex(Direction::West, false, BlockStateProperties::Half::Top)] = closedTop;

    // 打开状态的形状（根据朝向决定，上半和下半打开后形状相同）
    m_shapes[_getShapeIndex(Direction::North, true, BlockStateProperties::Half::Bottom)] = openNorth;
    m_shapes[_getShapeIndex(Direction::North, true, BlockStateProperties::Half::Top)] = openNorth;
    m_shapes[_getShapeIndex(Direction::South, true, BlockStateProperties::Half::Bottom)] = openSouth;
    m_shapes[_getShapeIndex(Direction::South, true, BlockStateProperties::Half::Top)] = openSouth;
    m_shapes[_getShapeIndex(Direction::East, true, BlockStateProperties::Half::Bottom)] = openEast;
    m_shapes[_getShapeIndex(Direction::East, true, BlockStateProperties::Half::Top)] = openEast;
    m_shapes[_getShapeIndex(Direction::West, true, BlockStateProperties::Half::Bottom)] = openWest;
    m_shapes[_getShapeIndex(Direction::West, true, BlockStateProperties::Half::Top)] = openWest;
}

BlockState TrapDoorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const BlockPos pos = context.placementPos();
    const Direction clickedFace = context.getClickedFace();
    IWorld& world = const_cast<IWorld&>(context.getWorld());

    Direction facing;
    BlockStateProperties::Half half;
    if (!context.replacingClickedBlock() && Directions::isHorizontal(clickedFace)) {
        facing = clickedFace;
        half = context.getHitY() > 0.5f ? BlockStateProperties::Half::Top : BlockStateProperties::Half::Bottom;
    } else {
        facing = Directions::opposite(context.horizontalDirection());
        half = clickedFace == Direction::Up ? BlockStateProperties::Half::Bottom : BlockStateProperties::Half::Top;
    }

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);
    bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::HALF(), half)
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool TrapDoorBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 活板门没有特殊的放置位置检查，如果支撑丢失由 updatePostPlacement 处理移除
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return true;
}

BlockState TrapDoorBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 调用父类处理
    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

void TrapDoorBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    bool wasPowered = state.get(BlockStateProperties::POWERED());
    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);

    if (isPowered == wasPowered) {
        return;
    }

    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState =
        state.with(BlockStateProperties::POWERED(), isPowered).with(BlockStateProperties::OPEN(), isPowered);
    world.setBlockState(pos, &newState, 2);

    if (newState.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, pos);
    }

    if (wasOpen != isPowered) {
        _playSound(world, pos, isPowered);
    }
}

BlockActionResult TrapDoorBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (m_isIron) {
        return ActionResultType::Pass;
    }

    toggle(world, pos, state, !state.get(BlockStateProperties::OPEN()));
    return ActionResultType::Success;
}

const CollisionShape& TrapDoorBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    BlockStateProperties::Half half = state.get(BlockStateProperties::HALF());

    size_t index = _getShapeIndex(facing, open, half);
    MC_ASSERT(index < 16);
    return m_shapes[index];
}

const CollisionShape& TrapDoorBlock::getCollisionShape(const BlockState& state) const
{
    if (state.get(BlockStateProperties::OPEN())) {
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }
    return getShape(state);
}

const BlockState& TrapDoorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& TrapDoorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

bool TrapDoorBlock::isOpen(const BlockState& state)
{
    return state.get(BlockStateProperties::OPEN());
}

void TrapDoorBlock::toggle(IWorld& world, const BlockPos& pos, const BlockState& state, bool open)
{
    if (state.get(BlockStateProperties::OPEN()) == open) {
        return;
    }

    BlockState newState = state.with(BlockStateProperties::OPEN(), open);
    world.setBlockState(pos, &newState, 10);

    if (newState.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, pos);
    }

    _playSound(world, pos, open);
}

void TrapDoorBlock::_playSound(IWorld& world, const BlockPos& pos, bool isOpening)
{
    const BlockState* state = world.getBlockState(pos);
    const auto* trapDoor = state != nullptr ? dynamic_cast<const TrapDoorBlock*>(&state->getBlock()) : nullptr;
    const bool isIron = trapDoor != nullptr && trapDoor->isIronTrapdoor();

    const char* soundId = nullptr;
    if (isIron) {
        soundId = isOpening ? "minecraft:block.iron_trapdoor.open" : "minecraft:block.iron_trapdoor.close";
    } else {
        soundId = isOpening ? "minecraft:block.wooden_trapdoor.open" : "minecraft:block.wooden_trapdoor.close";
    }

    world.playSound(ResourceLocation(soundId), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

size_t TrapDoorBlock::_getShapeIndex(Direction facing, bool open, BlockStateProperties::Half half)
{
    size_t facingIdx = 0;
    switch (facing) {
        case Direction::North:
            facingIdx = 0;
            break;
        case Direction::South:
            facingIdx = 1;
            break;
        case Direction::East:
            facingIdx = 2;
            break;
        case Direction::West:
            facingIdx = 3;
            break;
        default:
            facingIdx = 0;
            break;
    }

    size_t halfIdx = (half == BlockStateProperties::Half::Top) ? 1 : 0;
    size_t openIdx = open ? 2 : 0;
    return facingIdx * 4 + openIdx + halfIdx;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* TrapDoorBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== 攀爬实现 ==========

bool TrapDoorBlock::isLadder(const BlockState& state, IWorld* world, const BlockPos* pos, const Entity* entity) const
{
    // 只有打开的活板门才能攀爬
    if (!state.get(BlockStateProperties::OPEN())) {
        return false;
    }

    // 如果没有世界信息，只检查是否打开
    if (world == nullptr || pos == nullptr) {
        return true;
    }

    MC_UNUSED(entity);

    // 检查下方是否有梯子，且朝向与活板门相同
    BlockPos belowPos(pos->x, pos->y - 1, pos->z);
    const BlockState* belowState = world->getBlockState(belowPos);
    if (belowState == nullptr) {
        return true; // 如果没有下方方块信息，默认允许攀爬
    }

    // 检查下方是否为梯子
    if (belowState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        Direction belowFacing = belowState->get(BlockStateProperties::HORIZONTAL_FACING());
        Direction trapdoorFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
        if (belowFacing == trapdoorFacing) {
            return true;
        }
    }

    // 即使下方没有梯子，打开的活板门也可以攀爬
    return true;
}

} // namespace blocks
} // namespace mc
