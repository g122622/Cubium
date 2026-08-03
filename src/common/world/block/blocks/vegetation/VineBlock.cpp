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

#include "VineBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../../WorldConstants.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

VineBlock::VineBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::UP())
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：无连接
    setDefaultState(defaultState()
            .with(BlockStateProperties::UP(), false)
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false));

    // 创建各方向的形状（薄层）
    constexpr f32 thickness = 0.0625f; // 1/16 块厚度
    m_northShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    m_southShape = CollisionShape::box(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    m_eastShape = CollisionShape::box(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_westShape = CollisionShape::box(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
}

BlockState VineBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据点击的面确定初始连接方向
    Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否可以附着
    if (!_canAttachTo(const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world)), pos, clickedFace)) {
        return defaultState();
    }

    // 设置对应方向的连接
    bool north = (clickedFace == Direction::North);
    bool south = (clickedFace == Direction::South);
    bool east = (clickedFace == Direction::East);
    bool west = (clickedFace == Direction::West);
    bool up = (clickedFace == Direction::Up);

    return defaultState()
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west);
}

bool VineBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    // 检查至少有一个方向可以附着
    if (state.get(BlockStateProperties::UP()) && _canAttachTo(world, pos, Direction::Up)) {
        return true;
    }
    if (state.get(BlockStateProperties::NORTH()) && _canAttachTo(world, pos, Direction::North)) {
        return true;
    }
    if (state.get(BlockStateProperties::SOUTH()) && _canAttachTo(world, pos, Direction::South)) {
        return true;
    }
    if (state.get(BlockStateProperties::EAST()) && _canAttachTo(world, pos, Direction::East)) {
        return true;
    }
    if (state.get(BlockStateProperties::WEST()) && _canAttachTo(world, pos, Direction::West)) {
        return true;
    }

    // 检查下方是否有藤蔓（可以向下延伸）
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && belowState->is(this);
}

BlockState VineBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 更新对应方向的连接状态
    const BooleanProperty* prop = nullptr;

    switch (facing) {
        case Direction::Up:
            prop = &BlockStateProperties::UP();
            break;
        case Direction::North:
            prop = &BlockStateProperties::NORTH();
            break;
        case Direction::South:
            prop = &BlockStateProperties::SOUTH();
            break;
        case Direction::East:
            prop = &BlockStateProperties::EAST();
            break;
        case Direction::West:
            prop = &BlockStateProperties::WEST();
            break;
        default:
            return state;
    }

    bool currentState = state.get(*prop);
    if (currentState) {
        // 检查是否仍然可以附着
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!_canAttachTo(blockReader, currentPos, facing)) {
            // 移除该方向的连接
            BlockState newState = state.with(*prop, false);
            // 如果没有任何连接，检查是否可以保持
            if (_getConnectionCount(newState) == 0 && !isValidPosition(newState, blockReader, currentPos)) {
                if (auto* airState = BlockRegistry::instance().airState()) {
                    return *airState;
                }
            }
            return newState;
        }
    }

    return state;
}

const BlockState& VineBlock::rotate(const BlockState& state, Rotation rotation) const
{
    bool north = state.get(BlockStateProperties::NORTH());
    bool south = state.get(BlockStateProperties::SOUTH());
    bool east = state.get(BlockStateProperties::EAST());
    bool west = state.get(BlockStateProperties::WEST());

    switch (rotation) {
        case Rotation::None:
            return state;
        case Rotation::Clockwise90:
            return state.with(BlockStateProperties::NORTH(), west)
                .with(BlockStateProperties::SOUTH(), east)
                .with(BlockStateProperties::EAST(), north)
                .with(BlockStateProperties::WEST(), south);
        case Rotation::Clockwise180:
            return state.with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north)
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        case Rotation::CounterClockwise90:
            return state.with(BlockStateProperties::NORTH(), east)
                .with(BlockStateProperties::SOUTH(), west)
                .with(BlockStateProperties::EAST(), south)
                .with(BlockStateProperties::WEST(), north);
        default:
            return state;
    }
}

const BlockState& VineBlock::mirror(const BlockState& state, Mirror mirror) const
{
    switch (mirror) {
        case Mirror::None:
            return state;
        case Mirror::LeftRight: {
            bool north = state.get(BlockStateProperties::NORTH());
            bool south = state.get(BlockStateProperties::SOUTH());
            return state.with(BlockStateProperties::NORTH(), south).with(BlockStateProperties::SOUTH(), north);
        }
        case Mirror::FrontBack: {
            bool east = state.get(BlockStateProperties::EAST());
            bool west = state.get(BlockStateProperties::WEST());
            return state.with(BlockStateProperties::EAST(), west).with(BlockStateProperties::WEST(), east);
        }
        default:
            return state;
    }
}

const CollisionShape& VineBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 返回空形状（藤蔓是薄层，没有碰撞）
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& VineBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void VineBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 25% 概率生长
    if (random.nextInt(4) != 0) {
        return;
    }

    // 选择随机方向
    Direction direction = Directions::fromIndex(random.nextInt(6));

    if (Directions::isHorizontal(direction)) {
        // 水平方向蔓延
        const BooleanProperty* prop = _getPropertyFor(direction);
        if (prop == nullptr || state.get(*prop)) {
            return;
        }

        // 检查周围藤蔓密度（最多允许5个藤蔓在9x3x9范围内）
        if (!_hasRoomToSpread(static_cast<IBlockReader&>(world), pos)) {
            return;
        }

        BlockPos adjPos = pos.offset(direction);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->isAir()) {
            // 目标位置是空气，尝试蔓延
            Direction cwDir = Directions::rotateY(direction);     // 顺时针
            Direction ccwDir = Directions::rotateYCCW(direction); // 逆时针
            const BooleanProperty* cwProp = _getPropertyFor(cwDir);
            const BooleanProperty* ccwProp = _getPropertyFor(ccwDir);

            bool hasCW = (cwProp != nullptr && state.get(*cwProp));
            bool hasCCW = (ccwProp != nullptr && state.get(*ccwProp));

            BlockPos cwAdjPos = adjPos.offset(cwDir);
            BlockPos ccwAdjPos = adjPos.offset(ccwDir);

            if (hasCW && _canAttachTo(static_cast<IBlockReader&>(world), cwAdjPos, cwDir)) {
                // 从顺时针方向蔓延
                const BlockState& newState = defaultState().with(*cwProp, true);
                world.setBlockState(adjPos, &newState, 2);
            } else if (hasCCW && _canAttachTo(static_cast<IBlockReader&>(world), ccwAdjPos, ccwDir)) {
                // 从逆时针方向蔓延
                const BlockState& newState = defaultState().with(*ccwProp, true);
                world.setBlockState(adjPos, &newState, 2);
            } else {
                // 尝试向对面蔓延
                Direction oppositeDir = Directions::opposite(direction);
                BlockPos cwSourcePos = pos.offset(cwDir);
                BlockPos ccwSourcePos = pos.offset(ccwDir);

                if (hasCW && adjState->isAir() &&
                    _canAttachTo(static_cast<IBlockReader&>(world), cwSourcePos, oppositeDir)) {
                    const BooleanProperty* oppositeProp = _getPropertyFor(oppositeDir);
                    if (oppositeProp != nullptr) {
                        const BlockState& newState = defaultState().with(*oppositeProp, true);
                        world.setBlockState(cwAdjPos, &newState, 2);
                    }
                } else if (hasCCW && adjState->isAir() &&
                    _canAttachTo(static_cast<IBlockReader&>(world), ccwSourcePos, oppositeDir)) {
                    const BooleanProperty* oppositeProp = _getPropertyFor(oppositeDir);
                    if (oppositeProp != nullptr) {
                        const BlockState& newState = defaultState().with(*oppositeProp, true);
                        world.setBlockState(ccwAdjPos, &newState, 2);
                    }
                } else if (random.nextFloat() < 0.05f) {
                    // 小概率向上附着
                    BlockPos aboveAdjPos = adjPos.up();
                    if (_canAttachTo(static_cast<IBlockReader&>(world), aboveAdjPos, Direction::Up)) {
                        const BlockState& newState = defaultState().with(BlockStateProperties::UP(), true);
                        world.setBlockState(adjPos, &newState, 2);
                    }
                }
            }
        } else if (_canAttachTo(static_cast<IBlockReader&>(world), adjPos, direction)) {
            // 目标位置是可附着的固体方块
            world.setBlockState(pos, &state.with(*prop, true), 2);
        }
    } else if (direction == Direction::Up && pos.y < world::MAX_BUILD_HEIGHT - 1) {
        // 向上蔓延
        if (_canAttachTo(static_cast<IBlockReader&>(world), pos, Direction::Up)) {
            world.setBlockState(pos, &state.with(BlockStateProperties::UP(), true), 2);
            return;
        }

        BlockPos abovePos = pos.up();
        const BlockState* aboveState = world.getBlockState(abovePos);

        if (aboveState != nullptr && aboveState->isAir()) {
            // 检查周围藤蔓密度
            if (!_hasRoomToSpread(static_cast<IBlockReader&>(world), pos)) {
                return;
            }

            // 向下延伸到空气
            BlockState newState = state;

            // 随机移除一些水平连接
            for (Direction horizDir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
                const BooleanProperty* horizProp = _getPropertyFor(horizDir);
                if (horizProp != nullptr && random.nextBoolean()) {
                    if (!_canAttachTo(static_cast<IBlockReader&>(world), abovePos.offset(horizDir), Direction::Up)) {
                        newState = newState.with(*horizProp, false);
                    }
                }
            }

            // 检查是否至少有一个水平连接
            if (_hasHorizontalConnection(newState)) {
                world.setBlockState(abovePos, &newState, 2);
            }
        }
    } else if (direction == Direction::Down && pos.y > world::MIN_BUILD_HEIGHT) {
        // 向下延伸
        BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);

        if (belowState != nullptr && (belowState->isAir() || belowState->is(this))) {
            BlockState belowNewState = belowState->isAir() ? defaultState() : *belowState;

            // 随机复制水平连接
            belowNewState = _copyRandomHorizontalConnections(state, belowNewState, random);

            // 检查是否至少有一个水平连接
            if (_hasHorizontalConnection(belowNewState)) {
                world.setBlockState(belowPos, &belowNewState, 2);
            }
        }
    }
}

bool VineBlock::_hasRoomToSpread(IBlockReader& world, const BlockPos& pos) const
{
    // 检查9x3x9范围内的藤蔓数量，最多允许5个
    i32 vineCount = 5; // 从5开始倒数

    for (i32 dx = -4; dx <= 4; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                const BlockState* checkState = world.getBlockState(checkPos);
                if (checkState != nullptr && checkState->is(this)) {
                    --vineCount;
                    if (vineCount <= 0) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

bool VineBlock::_hasHorizontalConnection(const BlockState& state) const
{
    return state.get(BlockStateProperties::NORTH()) || state.get(BlockStateProperties::SOUTH()) ||
        state.get(BlockStateProperties::EAST()) || state.get(BlockStateProperties::WEST());
}

BlockState VineBlock::_copyRandomHorizontalConnections(
    const BlockState& source, const BlockState& target, math::IRandom& random) const
{

    BlockState result = target;

    const BooleanProperty* northProp = _getPropertyFor(Direction::North);
    const BooleanProperty* southProp = _getPropertyFor(Direction::South);
    const BooleanProperty* eastProp = _getPropertyFor(Direction::East);
    const BooleanProperty* westProp = _getPropertyFor(Direction::West);

    if (northProp != nullptr && random.nextBoolean() && source.get(*northProp)) {
        result = result.with(*northProp, true);
    }
    if (southProp != nullptr && random.nextBoolean() && source.get(*southProp)) {
        result = result.with(*southProp, true);
    }
    if (eastProp != nullptr && random.nextBoolean() && source.get(*eastProp)) {
        result = result.with(*eastProp, true);
    }
    if (westProp != nullptr && random.nextBoolean() && source.get(*westProp)) {
        result = result.with(*westProp, true);
    }

    return result;
}

const BooleanProperty* VineBlock::_getPropertyFor(Direction direction) const
{
    switch (direction) {
        case Direction::Up:
            return &BlockStateProperties::UP();
        case Direction::North:
            return &BlockStateProperties::NORTH();
        case Direction::South:
            return &BlockStateProperties::SOUTH();
        case Direction::East:
            return &BlockStateProperties::EAST();
        case Direction::West:
            return &BlockStateProperties::WEST();
        default:
            return nullptr;
    }
}

bool VineBlock::_canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    // 藤蔓可以附着在固体方块的侧面
    return adjState->isSolid();
}

i32 VineBlock::_getConnectionCount(const BlockState& state) const
{
    i32 count = 0;
    if (state.get(BlockStateProperties::UP())) count++;
    if (state.get(BlockStateProperties::NORTH())) count++;
    if (state.get(BlockStateProperties::SOUTH())) count++;
    if (state.get(BlockStateProperties::EAST())) count++;
    if (state.get(BlockStateProperties::WEST())) count++;
    return count;
}

} // namespace blocks
} // namespace mc
