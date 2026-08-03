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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

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

RailShapeProperty::RailShapeProperty(const std::string& name, std::vector<RailShape> values)
    : EnumProperty<RailShape>(name, std::move(values))
{}

std::unique_ptr<RailShapeProperty> RailShapeProperty::create(const std::string& name)
{
    return std::unique_ptr<RailShapeProperty>(new RailShapeProperty(name));
}

std::unique_ptr<RailShapeProperty> RailShapeProperty::createStraight(const std::string& name)
{
    // vanilla 矿车铁轨（动力/探测/激活）shape 仅 6 值：南北、东西、4 个斜坡，不含弯轨。
    return std::unique_ptr<RailShapeProperty>(new RailShapeProperty(name,
        {RailShape::NorthSouth,
            RailShape::EastWest,
            RailShape::AscendingEast,
            RailShape::AscendingWest,
            RailShape::AscendingNorth,
            RailShape::AscendingSouth}));
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
    // 放置时只根据玩家朝向确定初始形状（南北或东西），不做邻居连接计算。
    // 邻居连接计算在方块放置后由 updatePostPlacement / neighborChanged 触发，
    // 因为此时该位置还不是铁轨，RailState 无法获取正确的方块状态。
    Direction facing = context.horizontalDirection();
    RailShape defaultShape = RailShape::EastWest;
    if (facing == Direction::North || facing == Direction::South) {
        defaultShape = RailShape::NorthSouth;
    }

    // 检测放置位置是否含水，如果位于水中则设置 WATERLOGGED=true
    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos());

    return withRailShape(defaultState(), defaultShape).with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

void AbstractRailBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 铁轨放置后立即重新计算连接形状。
    // getStateForPlacement 只根据玩家朝向返回初始形状，
    // 真正的邻居连接计算在此处通过 updateDir 触发。
    // 注意：MC Java 中此处使用 updateDir(world, pos, state, true)，
    // 第二个参数 true 表示初始放置，会强制更新世界并传播连接到相邻铁轨。
    (void)updateDir(world, pos, state, true);
}

BlockState AbstractRailBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 含水铁轨需要调度流体 tick，确保水流模拟正确运行
    if (state.hasProperty(BlockStateProperties::WATERLOGGED()) && state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

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
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 客户端不处理邻居更新
    if (world.isClientSide()) {
        return;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    // 确保该位置仍然是同类型的铁轨方块
    if (!state->is(this)) {
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
    // 与 MC 1.21.11 BaseRailBlock.canSurvive 一致：
    //   Block.canSupportRigidBlock(world, pos.below())
    return Block::canSupportRigidBlock(world, pos.down());
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

BlockState AbstractRailBlock::updateDir(IWorld& world, const BlockPos& pos, const BlockState& state, bool updateBlock)
{
    // 客户端直接返回原状态，不执行 RailState 计算
    if (world.isClientSide()) {
        return state;
    }
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
    // 与 MC 1.21.11 BaseRailBlock.shouldBeRemoved 一致：
    // - 下方方块不再提供 Rigid 支撑 → 移除
    // - 斜坡铁轨在上升方向的相邻方块不再提供 Rigid 支撑 → 移除
    if (!Block::canSupportRigidBlock(world, pos.down())) {
        return true;
    }

    // 获取铁轨方块并查询形状
    const Block* block = &state.owner();
    const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
    if (rail == nullptr) {
        return false;
    }
    const RailShape shape = rail->getRailShape(state);

    switch (shape) {
        case RailShape::AscendingEast:
            return !Block::canSupportRigidBlock(world, pos.east());
        case RailShape::AscendingWest:
            return !Block::canSupportRigidBlock(world, pos.west());
        case RailShape::AscendingNorth:
            return !Block::canSupportRigidBlock(world, pos.north());
        case RailShape::AscendingSouth:
            return !Block::canSupportRigidBlock(world, pos.south());
        default:
            return false;
    }
}

const fluid::FluidState* AbstractRailBlock::getFluidState(const BlockState& state) const
{
    // 如果铁轨含水，返回水的流体状态；否则返回默认（空）流体状态
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
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
