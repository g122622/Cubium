#include "RedstoneWireBlock.hpp"
#include "RedstoneDiodeBlock.hpp"
#include "ObserverBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../BlockRegistry.hpp"
#include "../../VanillaBlocks.hpp"
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

std::optional<blocks::RedstoneSide> EnumProperty<blocks::RedstoneSide>::Traits::fromName(StringView name) {
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
    // 基础检查：如果方块可以输出红石信号，则可以连接
    return state.getBlock().canProvidePower(state);
}

bool RedstoneWireBlock::canConnectTo(const BlockState& state, Direction side) {
    const Block& block = state.getBlock();

    // 红石线总是可以连接到其他红石线
    if (state.is(VanillaBlocks::REDSTONE_WIRE)) {
        return true;
    }

    // 检查中继器 - 只有朝向正确时才连接
    if (state.is(VanillaBlocks::REDSTONE_REPEATER) || state.is(VanillaBlocks::REDSTONE_COMPARATOR)) {
        Direction facing = RedstoneDiodeBlock::getFacing(state);
        // 中继器/比较器的输出端朝向我们时才连接
        return side == facing;
    }

    // 检查观察者 - 只有观察者的输出端朝向我们时才连接
    if (state.is(VanillaBlocks::OBSERVER)) {
        Direction facing = ObserverBlock::getFacing(state);
        // 观察者的输出端朝向我们时才连接
        return side == facing;
    }

    // 其他方块：检查 canProvidePower 和 canConnectRedstone
    if (block.canProvidePower(state)) {
        return true;
    }

    // 调用方块的 canConnectRedstone 方法
    return block.canConnectRedstone(state, side);
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

        world.setBlockState(pos, &newState, 2);

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

    // 如果暂时禁用信号输出，返回0
    if (!m_canProvidePower) {
        return 0;
    }

    // MC Java: 红石线不向下输出信号 (side != Direction.DOWN)
    if (side == Direction::Down) {
        return 0;
    }

    // MC Java: 只有该方向有连接时才输出信号
    // 获取该方向对应的连接属性
    RedstoneSide connection = RedstoneSide::None;
    switch (side) {
        case Direction::North:
            connection = state.get(NORTH_PROP());
            break;
        case Direction::East:
            connection = state.get(EAST_PROP());
            break;
        case Direction::South:
            connection = state.get(SOUTH_PROP());
            break;
        case Direction::West:
            connection = state.get(WEST_PROP());
            break;
        case Direction::Up:
            // 向上总是可以输出（如果有信号）
            return getPower(state);
        default:
            return 0;
    }

    // 只有该方向有连接时才输出信号
    if (connection == RedstoneSide::None) {
        return 0;
    }

    return getPower(state);
}

i32 RedstoneWireBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // MC Java: 红石线的 getStrongPower 委托给 getWeakPower
    // 这使得红石线可以充能相邻的实体方块
    return getWeakPower(state, world, pos, side);
}

bool RedstoneWireBlock::updatePower(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->is(this)) {
        return false;
    }

    i32 oldPower = getPower(*state);
    i32 newPower = calculateInputPower(world, pos, *state);

    if (oldPower != newPower) {
        BlockState newState = withPower(*state, newPower);
        newState = calculateConnections(world, pos, newState);
        world.setBlockState(pos, &newState, 2);

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
    const BlockState* neighborState = world.getBlockState(neighborPos);

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
        const BlockState* upState = world.getBlockState(upPos);
        if (upState && upState->is(this)) {
            return RedstoneSide::Up;
        }
    } else {
        // 相邻不是实体方块，检查其下方是否有红石线
        BlockPos downPos = neighborPos.down();
        const BlockState* downState = world.getBlockState(downPos);
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
        const BlockState* neighborState = world.getBlockState(neighborPos);

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
            const BlockState* neighborState = world.getBlockState(neighborPos);

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
                const BlockState* upState = world.getBlockState(upPos);
                if (upState && upState->is(this)) {
                    i32 wirePower = getPower(*upState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            } else {
                // 检查向下连接
                BlockPos downPos = neighborPos.down();
                const BlockState* downState = world.getBlockState(downPos);
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
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->is(this)) {
        return 0;
    }
    return getPower(*state);
}

void RedstoneWireBlock::notifyWireNeighbors(IWorld& world, const BlockPos& pos) {
    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 更新相邻红石线的信号
    updatePower(world, pos);
}

ActionResultType RedstoneWireBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // MC 1.16.5: 右键点击可以在十字连接和点状连接之间切换
    // 只有当玩家可以编辑时才允许切换
    // 检查是否是十字连接或点状连接模式
    bool isCross = isCrossConnection(state);
    bool isDot = isDotConnection(state);

    if (isCross || isDot) {
        // 切换模式：十字 -> 点状，点状 -> 十字
        BlockState newState = isCross ? createDotState(state) : createCrossState(state);
        newState = calculateConnections(world, pos, newState);

        if (newState != state) {
            world.setBlockState(pos, &newState, 3);

            // 通知对角邻居更新
            notifyDiagonalNeighbors(world, pos, state, newState);
            return ActionResultType::Success;
        }
    }

    return ActionResultType::Pass;
}

bool RedstoneWireBlock::isCrossConnection(const BlockState& state) const {
    // MC 1.16.5: func_235555_m_ - 检查四个方向是否都有连接
    return state.get(NORTH_PROP()) != RedstoneSide::None &&
           state.get(SOUTH_PROP()) != RedstoneSide::None &&
           state.get(EAST_PROP()) != RedstoneSide::None &&
           state.get(WEST_PROP()) != RedstoneSide::None;
}

bool RedstoneWireBlock::isDotConnection(const BlockState& state) const {
    // MC 1.16.5: func_235556_n_ - 检查四个方向是否都没有连接
    return state.get(NORTH_PROP()) == RedstoneSide::None &&
           state.get(SOUTH_PROP()) == RedstoneSide::None &&
           state.get(EAST_PROP()) == RedstoneSide::None &&
           state.get(WEST_PROP()) == RedstoneSide::None;
}

BlockState RedstoneWireBlock::createDotState(const BlockState& state) const {
    // 创建点状连接状态（所有方向都无连接）
    return state.with(NORTH_PROP(), RedstoneSide::None)
                .with(SOUTH_PROP(), RedstoneSide::None)
                .with(EAST_PROP(), RedstoneSide::None)
                .with(WEST_PROP(), RedstoneSide::None);
}

BlockState RedstoneWireBlock::createCrossState(const BlockState& state) const {
    // 创建十字连接状态（所有方向都有 Side 连接）
    return state.with(NORTH_PROP(), RedstoneSide::Side)
                .with(SOUTH_PROP(), RedstoneSide::Side)
                .with(EAST_PROP(), RedstoneSide::Side)
                .with(WEST_PROP(), RedstoneSide::Side);
}

void RedstoneWireBlock::notifyDiagonalNeighbors(IWorld& world, const BlockPos& pos,
                                                  const BlockState& oldState,
                                                  const BlockState& newState) {
    // MC 1.16.5: updateDiagonalNeighbors
    // 当连接状态改变时，通知对角方向的方块更新
    for (Direction dir : Directions::horizontal()) {
        RedstoneSide oldConnection = RedstoneSide::None;
        RedstoneSide newConnection = RedstoneSide::None;

        switch (dir) {
            case Direction::North:
                oldConnection = oldState.get(NORTH_PROP());
                newConnection = newState.get(NORTH_PROP());
                break;
            case Direction::South:
                oldConnection = oldState.get(SOUTH_PROP());
                newConnection = newState.get(SOUTH_PROP());
                break;
            case Direction::East:
                oldConnection = oldState.get(EAST_PROP());
                newConnection = newState.get(EAST_PROP());
                break;
            case Direction::West:
                oldConnection = oldState.get(WEST_PROP());
                newConnection = newState.get(WEST_PROP());
                break;
            default:
                break;
        }

        // 如果连接状态发生变化，通知对角邻居
        bool oldIsConnected = (oldConnection != RedstoneSide::None);
        bool newIsConnected = (newConnection != RedstoneSide::None);

        if (oldIsConnected != newIsConnected) {
            BlockPos neighborPos = pos.offset(dir);

            // 通知对角方向的方块
            BlockPos diagDownPos = neighborPos.down();
            const BlockState* diagDownState = world.getBlockState(diagDownPos);
            if (diagDownState && !diagDownState->isAir()) {
                Block& diagBlock = const_cast<Block&>(diagDownState->getBlock());
                diagBlock.neighborChanged(world, diagDownPos, *this, pos, false);
            }

            BlockPos diagUpPos = neighborPos.up();
            const BlockState* diagUpState = world.getBlockState(diagUpPos);
            if (diagUpState && !diagUpState->isAir()) {
                Block& diagBlock = const_cast<Block&>(diagUpState->getBlock());
                diagBlock.neighborChanged(world, diagUpPos, *this, pos, false);
            }
        }
    }
}

} // namespace blocks
} // namespace mc
