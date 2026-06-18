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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "AbstractRailBlock.hpp"
#include "RailState.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// RailShapeProperty
// ============================================================================

RailShapeProperty::RailShapeProperty(const std::string& name)
    : EnumProperty<RailShape>(name,
          {RailShape::NorthSouth,
              RailShape::EastWest,
              RailShape::AscendingEast,
              RailShape::AscendingWest,
              RailShape::AscendingNorth,
              RailShape::AscendingSouth,
              RailShape::SouthEast,
              RailShape::SouthWest,
              RailShape::NorthWest,
              RailShape::NorthEast})
{}

std::unique_ptr<RailShapeProperty> RailShapeProperty::create(const std::string& name)
{
    return std::unique_ptr<RailShapeProperty>(new RailShapeProperty(name));
}

// ============================================================================
// AbstractRailBlock
// ============================================================================

AbstractRailBlock::AbstractRailBlock(const BlockProperties& properties, bool isStraight, bool isPowered)
    : Block(properties)
    , m_isStraight(isStraight)
    , m_isPowered(isPowered)
{
    // 初始化形状
    // 直轨形状：平坦的条状
    m_shapes[static_cast<size_t>(RailShape::NorthSouth)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::EastWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);

    // 斜轨形状：一端抬升
    m_shapes[static_cast<size_t>(RailShape::AscendingEast)] =
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingWest)] =
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingNorth)] =
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingSouth)] =
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);

    // 弯轨形状
    m_shapes[static_cast<size_t>(RailShape::SouthEast)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::SouthWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::NorthWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::NorthEast)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
}

BlockState AbstractRailBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据相邻铁轨计算初始形状
    // 使用 RailState 进行完整的连接计算
    BlockPos pos = context.placementPos();
    IWorld& world = context.getWorld();

    // 先根据玩家朝向确定默认形状
    Direction facing = context.horizontalDirection();
    RailShape defaultShape = RailShape::EastWest;
    if (facing == Direction::North || facing == Direction::South) {
        defaultShape = RailShape::NorthSouth;
    }

    BlockState initialState = withRailShape(defaultState(), defaultShape);

    // 通过 RailState 计算完整连接
    RailState railState(world, pos, *this, initialState);
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);
    return railState.place(hasPower, false, defaultShape);
}

BlockState AbstractRailBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查是否还能放置
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        // 方块应该掉落
        return state;
    }

    // 检查斜坡铁轨是否仍有支撑
    if (shouldBeRemoved(state, blockReader, currentPos)) {
        return state;
    }

    // 通过 RailState 重新计算铁轨形状
    bool hasPower = world::redstone::RedstonePower::isPowered(world, currentPos);
    RailState railState(world, currentPos, *this, state);
    return railState.place(hasPower, false, getRailShape(state));
}

void AbstractRailBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    // 检查是否仍然有效放置
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(*state, blockReader, pos)) {
        // 铁轨下方无支撑，移除方块（掉落物由 onBlockRemoved 处理）
        world.setBlockState(pos.x, pos.y, pos.z, nullptr, 3);
        return;
    }

    // 检查斜坡铁轨支撑
    if (shouldBeRemoved(*state, blockReader, pos)) {
        world.setBlockState(pos.x, pos.y, pos.z, nullptr, 3);
        return;
    }

    // 更新铁轨状态
    updateState(world, pos, *state, neighborBlock);
}

bool AbstractRailBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查下方是否有固体方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 铁轨需要放在固体方块上
    return belowState->isSolid();
}

const CollisionShape& AbstractRailBlock::getShape(const BlockState& state) const
{
    RailShape shape = getRailShape(state);
    size_t index = static_cast<size_t>(shape);
    if (index < m_shapes.size()) {
        return m_shapes[index];
    }
    return m_shapes[0];
}

BlockState AbstractRailBlock::updateDir(
    IWorld& world, const BlockPos& pos, const BlockState& state, bool updateBlock)
{
    RailState railState(world, pos, *this, state);
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);
    return railState.place(hasPower, updateBlock, getRailShape(state));
}

void AbstractRailBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state, Block& neighborBlock)
{
    MC_UNUSED(neighborBlock);

    // 基类实现：重新计算铁轨方向
    (void)updateDir(world, pos, state, false);

    // 如果是动力铁轨类型，还需要传播更新
    if (m_isPowered) {
        world.updateNeighbors(pos, *this);
    }
}

bool AbstractRailBlock::shouldBeRemoved(const BlockState& state, IBlockReader& world, const BlockPos& pos)
{
    // 获取铁轨方块并查询形状
    const Block* block = &state.owner();
    const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
    if (rail == nullptr) {
        return false;
    }
    RailShape shape = rail->getRailShape(state);

    // 斜坡铁轨需要在其上升方向上方有支撑方块
    switch (shape) {
        case RailShape::AscendingNorth: {
            BlockPos supportPos = pos.north().up();
            const BlockState* supportState = world.getBlockState(supportPos);
            return supportState == nullptr || !supportState->isSolid();
        }
        case RailShape::AscendingSouth: {
            BlockPos supportPos = pos.south().up();
            const BlockState* supportState = world.getBlockState(supportPos);
            return supportState == nullptr || !supportState->isSolid();
        }
        case RailShape::AscendingEast: {
            BlockPos supportPos = pos.east().up();
            const BlockState* supportState = world.getBlockState(supportPos);
            return supportState == nullptr || !supportState->isSolid();
        }
        case RailShape::AscendingWest: {
            BlockPos supportPos = pos.west().up();
            const BlockState* supportState = world.getBlockState(supportPos);
            return supportState == nullptr || !supportState->isSolid();
        }
        default:
            return false;
    }
}

} // namespace blocks
} // namespace mc

// ============================================================================
// EnumProperty<RailShape> Traits 实现 - 必须在 mc 命名空间
// ============================================================================

namespace mc {

std::string EnumProperty<blocks::RailShape>::Traits::toString(const blocks::RailShape& value)
{
    static const char* names[] = {"north_south",
        "east_west",
        "ascending_east",
        "ascending_west",
        "ascending_north",
        "ascending_south",
        "south_east",
        "south_west",
        "north_west",
        "north_east"};
    return names[static_cast<size_t>(value)];
}

std::optional<blocks::RailShape> EnumProperty<blocks::RailShape>::Traits::fromName(std::string_view name)
{
    static const std::unordered_map<std::string, blocks::RailShape> map = {
        {"north_south", blocks::RailShape::NorthSouth},
        {"east_west", blocks::RailShape::EastWest},
        {"ascending_east", blocks::RailShape::AscendingEast},
        {"ascending_west", blocks::RailShape::AscendingWest},
        {"ascending_north", blocks::RailShape::AscendingNorth},
        {"ascending_south", blocks::RailShape::AscendingSouth},
        {"south_east", blocks::RailShape::SouthEast},
        {"south_west", blocks::RailShape::SouthWest},
        {"north_west", blocks::RailShape::NorthWest},
        {"north_east", blocks::RailShape::NorthEast}};
    auto it = map.find(std::string(name));
    return it != map.end() ? std::optional<blocks::RailShape>(it->second) : std::nullopt;
}

} // namespace mc
