#include "AbstractRailBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// RailShapeProperty
// ============================================================================

RailShapeProperty::RailShapeProperty(const String& name)
    : EnumProperty<RailShape>(name, {
        RailShape::NorthSouth,
        RailShape::EastWest,
        RailShape::AscendingEast,
        RailShape::AscendingWest,
        RailShape::AscendingNorth,
        RailShape::AscendingSouth,
        RailShape::SouthEast,
        RailShape::SouthWest,
        RailShape::NorthWest,
        RailShape::NorthEast
    })
{
}

std::unique_ptr<RailShapeProperty> RailShapeProperty::create(const String& name) {
    return std::unique_ptr<RailShapeProperty>(new RailShapeProperty(name));
}

// ============================================================================
// AbstractRailBlock
// ============================================================================

AbstractRailBlock::AbstractRailBlock(const BlockProperties& properties, bool isPowered)
    : Block(properties)
    , m_isPowered(isPowered)
{
    // 初始化形状
    // 直轨形状：平坦的条状
    m_shapes[static_cast<size_t>(RailShape::NorthSouth)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::EastWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);

    // 斜轨形状：一端抬升
    m_shapes[static_cast<size_t>(RailShape::AscendingEast)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingNorth)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::AscendingSouth)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);

    // 弯轨形状
    m_shapes[static_cast<size_t>(RailShape::SouthEast)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::SouthWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::NorthWest)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    m_shapes[static_cast<size_t>(RailShape::NorthEast)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
}

BlockState AbstractRailBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 根据相邻铁轨计算初始形状
    BlockPos pos = context.placementPos();
    const IWorld& world = context.getWorld();

    // 简化实现：默认东西方向
    RailShape shape = RailShape::EastWest;

    // 根据玩家朝向调整
    Direction facing = context.horizontalDirection();
    if (facing == Direction::North || facing == Direction::South) {
        shape = RailShape::NorthSouth;
    }

    return withRailShape(defaultState(), shape);
}

BlockState AbstractRailBlock::updatePostPlacement(
    const BlockState& state,
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

    // 重新计算铁轨形状
    RailShape newShape = calculateRailShape(world, currentPos, state);
    return withRailShape(state, newShape);
}

bool AbstractRailBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查下方是否有固体方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 铁轨需要放在固体方块上
    return belowState->isSolid();
}

const CollisionShape& AbstractRailBlock::getShape(const BlockState& state) const {
    RailShape shape = getRailShape(state);
    size_t index = static_cast<size_t>(shape);
    if (index < m_shapes.size()) {
        return m_shapes[index];
    }
    return m_shapes[0];
}

RailShape AbstractRailBlock::calculateRailShape(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    // 检查四个方向的铁轨连接
    bool north = isRailAt(static_cast<IBlockReader&>(world), pos.offset(Direction::North));
    bool south = isRailAt(static_cast<IBlockReader&>(world), pos.offset(Direction::South));
    bool east = isRailAt(static_cast<IBlockReader&>(world), pos.offset(Direction::East));
    bool west = isRailAt(static_cast<IBlockReader&>(world), pos.offset(Direction::West));

    // 检查斜坡
    bool northUp = canAscendTo(static_cast<IBlockReader&>(world), pos, Direction::North);
    bool southUp = canAscendTo(static_cast<IBlockReader&>(world), pos, Direction::South);
    bool eastUp = canAscendTo(static_cast<IBlockReader&>(world), pos, Direction::East);
    bool westUp = canAscendTo(static_cast<IBlockReader&>(world), pos, Direction::West);

    // 计算连接数
    int connections = (north ? 1 : 0) + (south ? 1 : 0) + (east ? 1 : 0) + (west ? 1 : 0);

    // 优先处理斜坡
    if (northUp) {
        return RailShape::AscendingNorth;
    }
    if (southUp) {
        return RailShape::AscendingSouth;
    }
    if (eastUp) {
        return RailShape::AscendingEast;
    }
    if (westUp) {
        return RailShape::AscendingWest;
    }

    // 根据连接数确定形状
    if (connections == 0) {
        // 无连接，保持当前方向或默认
        RailShape currentShape = getRailShape(state);
        if (currentShape == RailShape::NorthSouth || currentShape == RailShape::EastWest) {
            return currentShape;
        }
        return RailShape::NorthSouth;
    }

    if (connections == 1) {
        // 单连接，变成直轨
        if (north || south) {
            return RailShape::NorthSouth;
        }
        return RailShape::EastWest;
    }

    if (connections == 2) {
        // 双连接
        if (north && south) {
            return RailShape::NorthSouth;
        }
        if (east && west) {
            return RailShape::EastWest;
        }
        // 弯轨
        if (north && east) {
            return RailShape::NorthEast;
        }
        if (north && west) {
            return RailShape::NorthWest;
        }
        if (south && east) {
            return RailShape::SouthEast;
        }
        if (south && west) {
            return RailShape::SouthWest;
        }
    }

    // 三连接或四连接：保持当前形状
    // 实际MC中会更复杂，这里简化处理
    return getRailShape(state);
}

bool AbstractRailBlock::isRailAt(IBlockReader& world, const BlockPos& pos) const {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    // 检查方块是否为铁轨类型
    const Block* block = &state->owner();
    const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
    if (rail == nullptr) {
        return false;
    }

    // 使用该铁轨的属性检查方法
    return rail->hasRailShapeProperty(*state);
}

bool AbstractRailBlock::canAscendTo(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    // 检查目标位置是否有铁轨
    BlockPos targetPos = pos.offset(direction);
    const BlockState* targetState = world.getBlockState(targetPos.x, targetPos.y, targetPos.z);

    if (targetState == nullptr) {
        return false;
    }

    // 检查目标位置是否为铁轨类型
    const Block* block = &targetState->owner();
    const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
    if (rail == nullptr) {
        return false;
    }

    // 检查目标位置上方是否有空间（矿车需要通过）
    BlockPos abovePos(targetPos.x, targetPos.y + 1, targetPos.z);
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

    // 上方需要是空气或非固体方块
    return aboveState == nullptr || !aboveState->isSolid();
}

} // namespace blocks
} // namespace mc

// ============================================================================
// EnumProperty<RailShape> Traits 实现 - 必须在 mc 命名空间
// ============================================================================

namespace mc {

String EnumProperty<blocks::RailShape>::Traits::toString(const blocks::RailShape& value) {
    static const char* names[] = {
        "north_south",
        "east_west",
        "ascending_east",
        "ascending_west",
        "ascending_north",
        "ascending_south",
        "south_east",
        "south_west",
        "north_west",
        "north_east"
    };
    return names[static_cast<size_t>(value)];
}

Optional<blocks::RailShape> EnumProperty<blocks::RailShape>::Traits::fromName(StringView name) {
    static const std::unordered_map<String, blocks::RailShape> map = {
        {"north_south", blocks::RailShape::NorthSouth},
        {"east_west", blocks::RailShape::EastWest},
        {"ascending_east", blocks::RailShape::AscendingEast},
        {"ascending_west", blocks::RailShape::AscendingWest},
        {"ascending_north", blocks::RailShape::AscendingNorth},
        {"ascending_south", blocks::RailShape::AscendingSouth},
        {"south_east", blocks::RailShape::SouthEast},
        {"south_west", blocks::RailShape::SouthWest},
        {"north_west", blocks::RailShape::NorthWest},
        {"north_east", blocks::RailShape::NorthEast}
    };
    auto it = map.find(String(name));
    return it != map.end() ? Optional<blocks::RailShape>(it->second) : std::nullopt;
}

} // namespace mc
