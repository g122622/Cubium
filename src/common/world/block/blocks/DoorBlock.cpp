#include "world/block/blocks/DoorBlock.hpp"
#include "world/IWorld.hpp"
#include "world/World.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "entity/Player.hpp"
#include "item/BlockItemUseContext.hpp"
#include "util/Direction.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

DoorBlock::DoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron) {

    // 预计算所有8种形状
    // 关闭状态: 4个朝向
    // EAST关闭: x=0-3
    m_shapes[0] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
    // SOUTH关闭: z=0-3
    m_shapes[1] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f);
    // WEST关闭: x=13-16
    m_shapes[2] = VoxelShapes::cube(13.0f, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    // NORTH关闭: z=13-16
    m_shapes[3] = VoxelShapes::cube(0.0f, 0.0f, 13.0f, 16.0f, 16.0f, 16.0f);

    // 打开状态: 4个朝向（铰链在左）
    // EAST打开+左铰链 -> 北向
    m_shapes[4] = VoxelShapes::cube(0.0f, 0.0f, 13.0f, 16.0f, 16.0f, 16.0f);
    // SOUTH打开+左铰链 -> 西向
    m_shapes[5] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
    // WEST打开+左铰链 -> 南向
    m_shapes[6] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f);
    // NORTH打开+左铰链 -> 东向
    m_shapes[7] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
}

// ========== 方块状态 ==========

void DoorBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    container.add(BlockStateProperties::HALF());
    container.add(BlockStateProperties::HORIZONTAL_FACING());
    container.add(BlockStateProperties::OPEN());
    container.add(BlockStateProperties::HINGE());
    container.add(BlockStateProperties::POWERED());
}

const BlockState& DoorBlock::getDefaultState() const {
    return defaultState()
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::HINGE(), BlockStateProperties::DoorHinge::Left)
        .with(BlockStateProperties::POWERED(), false);
}

// ========== 放置和更新 ==========

BlockState DoorBlock::getStateForPlacement(BlockItemUseContext& context) {
    BlockPos pos = context.placementPos();

    // 检查上方是否有空间
    const BlockState* upState = context.getWorld().getBlockState(pos.x, pos.y + 1, pos.z);
    if (pos.getY() >= 255 || upState == nullptr || !upState->isAir()) {
        // 检查是否可替换
        const Material* mat = upState ? &upState->getMaterial() : nullptr;
        if (mat == nullptr || !mat->isReplaceable()) {
            return defaultState();
        }
    }

    // TODO: 实现红石信号检测
    // bool powered = world.isBlockPowered(pos) || world.isBlockPowered(pos.up());
    bool powered = false;

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection())
        .with(BlockStateProperties::HINGE(), calculateHingeSide(context))
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
}

void DoorBlock::onBlockPlacedBy(World& world, const BlockPos& pos, const BlockState& state) {
    // 在上方放置上半部分
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    BlockState upperState = state.with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockState(abovePos.x, abovePos.y, abovePos.z, &upperState, 3);
}

void DoorBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                 Block& neighborBlock, const BlockPos& neighborPos,
                                 bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos.x, pos.y, pos.z);
    if (statePtr == nullptr || statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;

    // 获取另一半的位置
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());
    BlockPos otherHalfPos = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockPos(pos.x, pos.y + 1, pos.z)
        : BlockPos(pos.x, pos.y - 1, pos.z);

    // TODO: 实现红石信号检测
    // bool powered = world.isBlockPowered(pos) || world.isBlockPowered(otherHalfPos);
    bool powered = false;
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (powered != wasPowered) {
        bool wasOpen = state.get(BlockStateProperties::OPEN());

        // 更新状态
        BlockState newState = state
            .with(BlockStateProperties::POWERED(), powered)
            .with(BlockStateProperties::OPEN(), powered);

        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        // 如果开关状态改变，播放音效
        if (wasOpen != powered) {
            // playSound需要World引用，这里跳过音效
            // 实际实现中应该通过事件系统播放
        }
    }
}

BlockState DoorBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());

    // 检查是否是另一半方向上的更新
    if (facing.getAxis() == Axis::Y) {
        bool isLower = half == BlockStateProperties::DoubleBlockHalf::Lower;
        bool isUpDirection = facing == Direction::Up;

        if (isLower == isUpDirection) {
            // 检查另一半是否是同类型门
            if (facingState.getBlock() == this &&
                facingState.get(BlockStateProperties::HALF()) != half) {
                // 同步状态
                return state
                    .with(BlockStateProperties::HORIZONTAL_FACING(), facingState.get(BlockStateProperties::HORIZONTAL_FACING()))
                    .with(BlockStateProperties::OPEN(), facingState.get(BlockStateProperties::OPEN()))
                    .with(BlockStateProperties::HINGE(), facingState.get(BlockStateProperties::HINGE()))
                    .with(BlockStateProperties::POWERED(), facingState.get(BlockStateProperties::POWERED()));
            } else {
                // 另一半不存在，变成空气
                return VanillaBlocks::AIR->defaultState();
            }
        }
    }

    // 下半部分检查支撑
    if (half == BlockStateProperties::DoubleBlockHalf::Lower && facing == Direction::Down) {
        BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
        if (belowState == nullptr || !belowState->isSolidSide(world, belowPos, Direction::Up)) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

bool DoorBlock::isValidPosition(
    const BlockState& state,
    IWorldReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());

    if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
        // 下半部分需要下方有支撑
        const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
        return belowState != nullptr && belowState->isSolidSide(world, BlockPos(pos.x, pos.y - 1, pos.z), Direction::Up);
    } else {
        // 上半部分需要下方是同类型门的下半部分
        const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
        return belowState != nullptr &&
               belowState->getBlock() == this &&
               belowState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
    }
}

// ========== 交互 ==========

ActionResult DoorBlock::onBlockActivated(
    const BlockState& state,
    World& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 铁门不能手动开关
    if (m_isIron) {
        return ActionResult::PASS;
    }

    // 切换开关状态
    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState = state.with(BlockStateProperties::OPEN(), !wasOpen);

    world.setBlockState(pos.x, pos.y, pos.z, &newState, 10);

    // 播放音效
    playSound(world, pos, !wasOpen);

    return ActionResult::SUCCESS;
}

void DoorBlock::toggleDoor(World& world, const BlockPos& pos, bool open) {
    const BlockState* statePtr = world.getBlockState(pos.x, pos.y, pos.z);
    if (statePtr == nullptr || statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    if (state.get(BlockStateProperties::OPEN()) != open) {
        world.setBlockState(pos.x, pos.y, pos.z, &state.with(BlockStateProperties::OPEN(), open), 10);
        playSound(world, pos, open);
    }
}

// ========== 形状 ==========

const CollisionShape& DoorBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    bool hingeRight = state.get(BlockStateProperties::HINGE()) == BlockStateProperties::DoorHinge::Right;

    size_t index = getShapeIndex(facing, open, hingeRight);
    return m_shapes[index];
}

const CollisionShape& DoorBlock::getCollisionShape(const BlockState& state) const {
    // 门的碰撞形状与渲染形状相同
    return getShape(state);
}

// ========== 旋转和镜像 ==========

const BlockState& DoorBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& DoorBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    // 先旋转
    Rotation rotation = mirrorToRotation(mirror, state.get(BlockStateProperties::HORIZONTAL_FACING()));
    BlockState rotated = rotate(state, rotation);

    // 然后翻转铰链
    BlockStateProperties::DoorHinge hinge = rotated.get(BlockStateProperties::HINGE());
    BlockStateProperties::DoorHinge newHinge = (hinge == BlockStateProperties::DoorHinge::Left)
        ? BlockStateProperties::DoorHinge::Right
        : BlockStateProperties::DoorHinge::Left;

    return rotated.with(BlockStateProperties::HINGE(), newHinge);
}

// ========== 静态工具方法 ==========

bool DoorBlock::isOpen(const BlockState& state) {
    return state.get(BlockStateProperties::OPEN());
}

bool DoorBlock::isWooden(const BlockState& state) {
    const Block* block = &state.owner();
    if (auto* doorBlock = dynamic_cast<const DoorBlock*>(block)) {
        const Material& mat = doorBlock->material();
        return mat == Material::WOOD || mat == Material::NETHER_WOOD;
    }
    return false;
}

// ========== 私有方法 ==========

BlockStateProperties::DoorHinge DoorBlock::calculateHingeSide(BlockItemUseContext& context) {
    const IWorld& reader = context.getWorld();
    BlockPos pos = context.placementPos();
    Direction facing = context.horizontalDirection();

    // 获取左右方向
    Direction leftDir = Directions::rotateYCCW(facing);
    Direction rightDir = Directions::rotateY(facing);

    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    BlockPos leftUpPos(leftPos.x, leftPos.y + 1, leftPos.z);
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));
    BlockPos rightUpPos(rightPos.x, rightPos.y + 1, rightPos.z);

    const BlockState* leftState = reader.getBlockState(leftPos.x, leftPos.y, leftPos.z);
    const BlockState* leftUpState = reader.getBlockState(leftUpPos.x, leftUpPos.y, leftUpPos.z);
    const BlockState* rightState = reader.getBlockState(rightPos.x, rightPos.y, rightPos.z);
    const BlockState* rightUpState = reader.getBlockState(rightUpPos.x, rightUpPos.y, rightUpPos.z);

    // 计算左右两侧的遮挡情况
    i32 score = 0;
    if (leftState != nullptr && leftState->hasOpaqueCollisionShape()) score--;
    if (leftUpState != nullptr && leftUpState->hasOpaqueCollisionShape()) score--;
    if (rightState != nullptr && rightState->hasOpaqueCollisionShape()) score++;
    if (rightUpState != nullptr && rightUpState->hasOpaqueCollisionShape()) score++;

    // 检查相邻门的位置
    bool hasLeftDoor = leftState != nullptr &&
                       leftState->getBlock() == this &&
                       leftState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
    bool hasRightDoor = rightState != nullptr &&
                        rightState->getBlock() == this &&
                        rightState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;

    if ((!hasLeftDoor || hasRightDoor) && score <= 0) {
        if ((!hasRightDoor || hasLeftDoor) && score >= 0) {
            // 根据点击位置决定
            i32 dx = Directions::xOffset(facing);
            i32 dz = Directions::zOffset(facing);
            f32 hitX = context.getHitX() - static_cast<f32>(pos.getX());
            f32 hitZ = context.getHitZ() - static_cast<f32>(pos.getZ());

            if ((dx >= 0 || !(hitZ < 0.5f)) &&
                (dx <= 0 || !(hitZ > 0.5f)) &&
                (dz >= 0 || !(hitX > 0.5f)) &&
                (dz <= 0 || !(hitX < 0.5f))) {
                return BlockStateProperties::DoorHinge::Left;
            }
            return BlockStateProperties::DoorHinge::Right;
        }
        return BlockStateProperties::DoorHinge::Left;
    }
    return BlockStateProperties::DoorHinge::Right;
}

void DoorBlock::playSound(World& world, const BlockPos& pos, bool isOpening) {
    // TODO: 实现音效系统
    // int soundId = isOpening ? getOpenSound() : getCloseSound();
    // world.playEvent(nullptr, soundId, pos, 0);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isOpening);
}

i32 DoorBlock::getOpenSound() const {
    // 1011 = 铁门打开, 1005 = 木门打开
    return m_isIron ? 1011 : 1005;
}

i32 DoorBlock::getCloseSound() const {
    // 1012 = 铁门关闭, 1006 = 木门关闭
    return m_isIron ? 1012 : 1006;
}

size_t DoorBlock::getShapeIndex(Direction facing, bool open, bool hingeRight) {
    // 关闭状态: 根据朝向选择形状 (索引 0-3)
    // EAST=0, SOUTH=1, WEST=2, NORTH=3
    if (!open) {
        switch (facing) {
            case Direction::East:  return 0;
            case Direction::South: return 1;
            case Direction::West:  return 2;
            case Direction::North: return 3;
            default: return 0;
        }
    }

    // 打开状态: 根据铰链位置计算实际朝向 (索引 4-7)
    // 左铰链: 门向逆时针方向打开 (门扇在铰链的右边)
    // 右铰链: 门向顺时针方向打开 (门扇在铰链的左边)
    //
    // 形状定义:
    // m_shapes[4] = NORTH形状(z=13-16) - EAST门+左铰链打开
    // m_shapes[5] = WEST形状(x=0-3) - SOUTH门+左铰链打开
    // m_shapes[6] = SOUTH形状(z=0-3) - WEST门+左铰链打开
    // m_shapes[7] = EAST形状(x=0-3) - NORTH门+左铰链打开

    if (hingeRight) {
        // 右铰链: 门的开口方向与左铰链相反
        switch (facing) {
            case Direction::East:  return 5;  // 门向右开，在西侧
            case Direction::South: return 6;  // 门向右开，在南侧
            case Direction::West:  return 7;  // 门向右开，在东侧
            case Direction::North: return 4;  // 门向右开，在北侧
            default: return 4;
        }
    } else {
        // 左铰链
        switch (facing) {
            case Direction::East:  return 4;  // 门向左开，在北侧
            case Direction::South: return 5;  // 门向左开，在西侧
            case Direction::West:  return 6;  // 门向左开，在南侧
            case Direction::North: return 7;  // 门向左开，在东侧
            default: return 4;
        }
    }
}

} // namespace blocks
} // namespace mc
