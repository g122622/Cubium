#include "RedstoneWireBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../IWorld.hpp"

namespace mc {

// EnumProperty Traits 实现 - 必须在 mc 命名空间
String EnumProperty<blocks::RedstoneSide>::Traits::toString(const blocks::RedstoneSide& value) {
    switch (value) {
        case blocks::RedstoneSide::None: return "none";
        case blocks::RedstoneSide::Side: return "side";
        case blocks::RedstoneSide::Up: return "up";
        default: return "none";
    }
}

Optional<blocks::RedstoneSide> EnumProperty<blocks::RedstoneSide>::Traits::fromName(StringView name) {
    if (name == "none") return blocks::RedstoneSide::None;
    if (name == "side") return blocks::RedstoneSide::Side;
    if (name == "up") return blocks::RedstoneSide::Up;
    return std::nullopt;
}

namespace blocks {

// 静态属性获取
const EnumProperty<RedstoneSide>& RedstoneWireBlock::NORTH_PROP() {
    static auto prop = EnumProperty<RedstoneSide>::create("north", {
        RedstoneSide::None, RedstoneSide::Side, RedstoneSide::Up
    });
    return *prop;
}

const EnumProperty<RedstoneSide>& RedstoneWireBlock::EAST_PROP() {
    static auto prop = EnumProperty<RedstoneSide>::create("east", {
        RedstoneSide::None, RedstoneSide::Side, RedstoneSide::Up
    });
    return *prop;
}

const EnumProperty<RedstoneSide>& RedstoneWireBlock::SOUTH_PROP() {
    static auto prop = EnumProperty<RedstoneSide>::create("south", {
        RedstoneSide::None, RedstoneSide::Side, RedstoneSide::Up
    });
    return *prop;
}

const EnumProperty<RedstoneSide>& RedstoneWireBlock::WEST_PROP() {
    static auto prop = EnumProperty<RedstoneSide>::create("west", {
        RedstoneSide::None, RedstoneSide::Side, RedstoneSide::Up
    });
    return *prop;
}

RedstoneWireBlock::RedstoneWireBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::POWER_0_15())
        .add(NORTH_PROP())
        .add(EAST_PROP())
        .add(SOUTH_PROP())
        .add(WEST_PROP())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::POWER_0_15(), 0)
        .with(NORTH_PROP(), RedstoneSide::None)
        .with(EAST_PROP(), RedstoneSide::None)
        .with(SOUTH_PROP(), RedstoneSide::None)
        .with(WEST_PROP(), RedstoneSide::None));
}

i32 RedstoneWireBlock::getPower(const BlockState& state) {
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState RedstoneWireBlock::withPower(BlockState state, i32 power) {
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

bool RedstoneWireBlock::isNormalCube(const BlockState& state) {
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstoneWireBlock::canConnectTo(const BlockState& state) {
    return state.getBlock().canProvidePower(state);
}

BlockState RedstoneWireBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 只有水平方向影响连接状态
    if (!Directions::isHorizontal(facing)) {
        return state;
    }

    // 计算新的连接状态
    RedstoneSide connection = getConnection(world, currentPos, facing);

    // 根据方向设置连接属性
    switch (facing) {
        case Direction::North:
            return state.with(NORTH_PROP(), connection);
        case Direction::East:
            return state.with(EAST_PROP(), connection);
        case Direction::South:
            return state.with(SOUTH_PROP(), connection);
        case Direction::West:
            return state.with(WEST_PROP(), connection);
        default:
            return state;
    }
}

void RedstoneWireBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    // 更新信号强度和连接状态
    updatePower(world, pos);
}

void RedstoneWireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    // 通知相邻方块更新
    notifyWireNeighbors(world, pos);
}

void RedstoneWireBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                         const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 调度更新
    world.scheduleBlockTick(pos, *this, 0, world::tick::TickPriority::High);
}

void RedstoneWireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 计算新的信号强度
    i32 oldPower = getPower(state);
    i32 newPower = calculateInputPower(world, pos, state);

    if (oldPower != newPower) {
        // 更新状态
        BlockState newState = withPower(state, newPower);
        newState = calculateConnections(world, pos, newState);

        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        // 通知相邻红石线更新
        notifyWireNeighbors(world, pos);
    }
}

i32 RedstoneWireBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 如果暂时禁用信号输出，返回0
    if (!m_canProvidePower) {
        return 0;
    }

    // 红石线不向上输出信号
    if (side == Direction::Up) {
        return 0;
    }

    return getPower(state);
}

bool RedstoneWireBlock::updatePower(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state || !state->is(this)) {
        return false;
    }

    i32 oldPower = getPower(*state);
    i32 newPower = calculateInputPower(world, pos, *state);

    if (oldPower != newPower) {
        BlockState newState = withPower(*state, newPower);
        newState = calculateConnections(world, pos, newState);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        // 通知相邻红石线更新
        notifyWireNeighbors(world, pos);
        return true;
    }

    return false;
}

BlockState RedstoneWireBlock::calculateConnections(IWorld& world,
                                                     const BlockPos& pos,
                                                     const BlockState& state) const {
    BlockState result = state;

    // 计算四个方向的连接状态
    result = result.with(NORTH_PROP(), getConnection(world, pos, Direction::North));
    result = result.with(EAST_PROP(), getConnection(world, pos, Direction::East));
    result = result.with(SOUTH_PROP(), getConnection(world, pos, Direction::South));
    result = result.with(WEST_PROP(), getConnection(world, pos, Direction::West));

    return result;
}

RedstoneSide RedstoneWireBlock::getConnection(IWorld& world,
                                               const BlockPos& pos,
                                               Direction direction) const {
    BlockPos neighborPos = pos.offset(direction);
    const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

    if (!neighborState || neighborState->isAir()) {
        return RedstoneSide::None;
    }

    // 检查相邻方块是否可以连接红石
    if (canConnectTo(*neighborState)) {
        return RedstoneSide::Side;
    }

    // 检查向上连接
    if (isNormalCube(*neighborState)) {
        // 相邻是实体方块，检查其上方是否有红石线
        BlockPos upPos = neighborPos.up();
        const BlockState* upState = world.getBlockState(upPos.x, upPos.y, upPos.z);
        if (upState && upState->is(this)) {
            return RedstoneSide::Up;
        }
    } else {
        // 相邻不是实体方块，检查其下方是否有红石线
        BlockPos downPos = neighborPos.down();
        const BlockState* downState = world.getBlockState(downPos.x, downPos.y, downPos.z);
        if (downState && downState->is(this)) {
            return RedstoneSide::Side;
        }
    }

    return RedstoneSide::None;
}

i32 RedstoneWireBlock::calculateInputPower(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    MC_UNUSED(state);

    i32 maxPower = 0;

    // 防止循环依赖
    bool prevCanProvidePower = m_canProvidePower;
    m_canProvidePower = false;

    // 1. 从相邻方块获取强信号
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 获取强信号
        if (neighborBlock.canProvidePower(*neighborState)) {
            Direction oppositeDir = Directions::opposite(dir);
            i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
            if (strongPower > maxPower) {
                maxPower = strongPower;
            }
        }
    }

    // 2. 从相邻红石线获取信号（衰减1）
    if (maxPower < 15) {
        for (Direction dir : Directions::horizontal()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

            if (!neighborState) {
                continue;
            }

            // 检查是否是红石线
            if (neighborState->is(this)) {
                i32 wirePower = getPower(*neighborState) - 1;
                if (wirePower > maxPower) {
                    maxPower = wirePower;
                }
            }

            // 检查向上连接
            if (isNormalCube(*neighborState)) {
                BlockPos upPos = neighborPos.up();
                const BlockState* upState = world.getBlockState(upPos.x, upPos.y, upPos.z);
                if (upState && upState->is(this)) {
                    i32 wirePower = getPower(*upState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            } else {
                // 检查向下连接
                BlockPos downPos = neighborPos.down();
                const BlockState* downState = world.getBlockState(downPos.x, downPos.y, downPos.z);
                if (downState && downState->is(this)) {
                    i32 wirePower = getPower(*downState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            }
        }
    }

    m_canProvidePower = prevCanProvidePower;
    return maxPower;
}

i32 RedstoneWireBlock::getWirePower(IWorld& world, const BlockPos& pos) const {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state || !state->is(this)) {
        return 0;
    }
    return getPower(*state);
}

void RedstoneWireBlock::notifyWireNeighbors(IWorld& world, const BlockPos& pos) {
    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 更新相邻红石线的信号
    updatePower(world, pos);
}

} // namespace blocks
} // namespace mc
