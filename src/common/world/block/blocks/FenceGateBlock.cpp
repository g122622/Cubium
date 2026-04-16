#include "FenceGateBlock.hpp"
#include "../../IWorld.hpp"
#include "../VanillaBlocks.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FenceGateBlock::FenceGateBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::OPEN())
        .add(BlockStateProperties::IN_WALL())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::IN_WALL(), false)
        .with(BlockStateProperties::POWERED(), false));

    // 关闭状态碰撞形状：2像素厚（根据朝向旋转）
    // 像素坐标：关闭时为 (0, 0, 7) -> (16, 16, 9) 或旋转后
    m_closedShape = VoxelShapes::cube(0.0f, 0.0f, 0.4375f, 1.0f, 1.0f, 0.5625f);

    // 打开状态形状：无碰撞
    m_openShape = VoxelShapes::empty();

    // 墙内关闭状态碰撞形状：稍低（0-13像素高）
    m_inWallClosedShape = VoxelShapes::cube(0.0f, 0.0f, 0.4375f, 1.0f, 0.8125f, 0.5625f);

    // 预计算各朝向的形状
    // NORTH: Z轴中心
    m_closedShapes[static_cast<size_t>(Direction::North)] = m_closedShape;
    m_openShapes[static_cast<size_t>(Direction::North)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::North)] = m_inWallClosedShape;

    // SOUTH: Z轴中心（相同）
    m_closedShapes[static_cast<size_t>(Direction::South)] = m_closedShape;
    m_openShapes[static_cast<size_t>(Direction::South)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::South)] = m_inWallClosedShape;

    // EAST: X轴中心
    m_closedShapes[static_cast<size_t>(Direction::East)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f);
    m_openShapes[static_cast<size_t>(Direction::East)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::East)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 0.8125f, 1.0f);

    // WEST: X轴中心（相同）
    m_closedShapes[static_cast<size_t>(Direction::West)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f);
    m_openShapes[static_cast<size_t>(Direction::West)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::West)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 0.8125f, 1.0f);
}

// ========== 放置和更新 ==========

BlockState FenceGateBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 获取水平朝向
    Direction facing = context.horizontalDirection();

    // 检查是否在墙内
    bool inWall = isWall(context.getWorld(), context.placementPos(), facing);

    // TODO: 实现红石信号检测
    // bool powered = world.isBlockPowered(pos);
    bool powered = false;

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::IN_WALL(), inWall)
        .with(BlockStateProperties::POWERED(), powered);
}

void FenceGateBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                      Block& neighborBlock, const BlockPos& neighborPos,
                                      bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;

    // TODO: 实现红石信号检测
    // bool powered = world.isBlockPowered(pos);
    bool powered = false;
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (powered != wasPowered) {
        bool wasOpen = state.get(BlockStateProperties::OPEN());

        // 更新状态
        BlockState newState = state
            .with(BlockStateProperties::POWERED(), powered)
            .with(BlockStateProperties::OPEN(), powered);

        world.setBlockState(pos, &newState, 2);

        // 如果开关状态改变，播放音效
        if (wasOpen != powered) {
            // playSound需要World引用，这里跳过音效
        }
    }
}

BlockState FenceGateBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查是否在墙内
    Direction gateFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool inWall = isWall(world, currentPos, gateFacing);
    bool currentInWall = state.get(BlockStateProperties::IN_WALL());

    if (inWall != currentInWall) {
        return state.with(BlockStateProperties::IN_WALL(), inWall);
    }

    return state;
}

// ========== 交互 ==========

ActionResultType FenceGateBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 切换开关状态
    bool wasOpen = state.get(BlockStateProperties::OPEN());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    BlockState newState = state.with(BlockStateProperties::OPEN(), !wasOpen);

    world.setBlockState(pos, &newState, 10);

    // 播放音效
    playSound(world, pos, !wasOpen);

    return ActionResultType::Success;
}

// ========== 形状 ==========

const CollisionShape& FenceGateBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    bool inWall = state.get(BlockStateProperties::IN_WALL());

    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    if (open) {
        return m_openShapes[index];
    } else if (inWall) {
        return m_inWallClosedShapes[index];
    } else {
        return m_closedShapes[index];
    }
}

const CollisionShape& FenceGateBlock::getCollisionShape(const BlockState& state) const {
    bool open = state.get(BlockStateProperties::OPEN());

    if (open) {
        return VoxelShapes::empty();
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool inWall = state.get(BlockStateProperties::IN_WALL());

    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    if (inWall) {
        return m_inWallClosedShapes[index];
    } else {
        return m_closedShapes[index];
    }
}

const CollisionShape& FenceGateBlock::getOcclusionShape(const BlockState& state) const {
    bool open = state.get(BlockStateProperties::OPEN());

    if (open) {
        return VoxelShapes::empty();
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    return m_closedShapes[index];
}

// ========== 旋转和镜像 ==========

const BlockState& FenceGateBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& FenceGateBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);

    // 直接返回 rotate 的结果，不要创建局部副本
    return rotate(state, rotation);
}

// ========== 静态工具方法 ==========

bool FenceGateBlock::isOpen(const BlockState& state) {
    return state.get(BlockStateProperties::OPEN());
}

// ========== 私有方法 ==========

bool FenceGateBlock::isWall(const IWorld& world, const BlockPos& pos, Direction facing) const {
    // 检查栅栏门两侧是否有墙
    // 栅栏门的朝向是门的"面"方向，墙应该在门的左右两侧

    Direction leftDir;
    Direction rightDir;

    // 根据栅栏门朝向确定左右方向
    switch (facing) {
        case Direction::North:
            leftDir = Direction::West;
            rightDir = Direction::East;
            break;
        case Direction::South:
            leftDir = Direction::East;
            rightDir = Direction::West;
            break;
        case Direction::East:
            leftDir = Direction::North;
            rightDir = Direction::South;
            break;
        case Direction::West:
            leftDir = Direction::South;
            rightDir = Direction::North;
            break;
        default:
            return false;
    }

    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));

    const BlockState* leftState = world.getBlockState(leftPos);
    const BlockState* rightState = world.getBlockState(rightPos);

    // 检查是否为墙方块
    // TODO: 检查是否为实际的墙方块类型
    // 目前简化为检查是否为固体方块
    bool leftWall = leftState != nullptr && leftState->isSolid();
    bool rightWall = rightState != nullptr && rightState->isSolid();

    return leftWall && rightWall;
}

void FenceGateBlock::playSound(IWorld& world, const BlockPos& pos, bool isOpening) {
    // TODO: 实现音效系统
    // int soundId = isOpening ? 1003 : 1004; // 栅栏门开/关音效
    // world.playEvent(nullptr, soundId, pos, 0);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isOpening);
}

} // namespace blocks
} // namespace mc
