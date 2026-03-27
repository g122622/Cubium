#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockState.hpp"
#include "world/IWorld.hpp"
#include "world/World.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "entity/Player.hpp"
#include "item/ItemStack.hpp"
#include "item/ItemUseContext.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 常量 ==========

namespace {
    // VoxelShape 碰撞箱
    constexpr f32 CHEST_WIDTH = 14.0f;
    constexpr f32 CHEST_HEIGHT = 14.0f;
    constexpr f32 CHEST_DEPTH = 14.0f;
    constexpr f32 CHEST_OFFSET = 1.0f;
}

// ========== 构造函数 ==========

ChestBlock::ChestBlock(const BlockProperties& properties)
    : Block(properties) {
}

// ========== 方块状态 ==========

void ChestBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    container.add(BlockStateProperties::HORIZONTAL_FACING());
    container.add(BlockStateProperties::CHEST_TYPE());
    container.add(BlockStateProperties::WATERLOGGED());
}

const BlockState& ChestBlock::getDefaultState() const {
    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single)
        .with(BlockStateProperties::WATERLOGGED(), false);
}

// ========== 放置和更新 ==========

BlockState ChestBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.getHorizontalFacing().getOpposite();

    // 默认为单箱
    BlockStateProperties::ChestType chestType = BlockStateProperties::ChestType::Single;

    // 检查是否可以与相邻箱子合并
    // 检查四个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = context.getPos().offset(dir);
        BlockState neighborState = context.getWorld().getBlockState(neighborPos);

        if (neighborState.getBlock() == this) {
            Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

            // 只有朝向相同且是单箱的才能合并
            if (neighborFacing == facing && neighborType == BlockStateProperties::ChestType::Single) {
                // 根据相对位置确定LEFT或RIGHT
                if (dir == facing.rotateYCCW()) {
                    // 玩家面对的左边有箱子，当前箱子是RIGHT
                    chestType = BlockStateProperties::ChestType::Right;
                } else if (dir == facing.rotateY()) {
                    // 玩家面对的右边有箱子，当前箱子是LEFT
                    chestType = BlockStateProperties::ChestType::Left;
                }
                break;
            }
        }
    }

    // 检查水logged状态
    bool waterlogged = context.getWorld().getFluidState(context.getPos()).getType() != FluidType::Empty;

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::CHEST_TYPE(), chestType)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState ChestBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& neighborState,
    IWorld& world,
    const BlockPos& pos,
    const BlockPos& neighborPos
) {
    // 处理水logged状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        // TODO: 调度流体tick
    }

    // 检查是否与相邻箱子连接/断开
    if (facing.getAxis().isHorizontal()) {
        BlockStateProperties::ChestType currentType = state.get(BlockStateProperties::CHEST_TYPE());

        if (neighborState.getBlock() == this) {
            Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

            // 检查是否可以连接
            if (currentType == BlockStateProperties::ChestType::Single &&
                neighborType != BlockStateProperties::ChestType::Single &&
                currentFacing == neighborFacing) {

                Direction connectedDir = getConnectedDirection(neighborState);
                if (connectedDir == facing.getOpposite()) {
                    // 连接到相邻箱子
                    return state.with(BlockStateProperties::CHEST_TYPE(),
                        neighborType == BlockStateProperties::ChestType::Left
                            ? BlockStateProperties::ChestType::Right
                            : BlockStateProperties::ChestType::Left);
                }
            }
        } else {
            // 检查是否需要断开连接
            Direction connectedDir = getConnectedDirection(state);
            if (connectedDir != Direction::None && connectedDir == facing) {
                return state.with(BlockStateProperties::CHEST_TYPE(),
                    BlockStateProperties::ChestType::Single);
            }
        }
    }

    return state;
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> ChestBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::ChestEntity>(pos);
}

// ========== 交互 ==========

ActionResult ChestBlock::onBlockActivated(
    const BlockState& state,
    World& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit
) {
    // 检查是否被阻挡
    if (isBlocked(world, pos)) {
        return ActionResult::Success;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Chest) {
        return ActionResult::Pass;
    }

    auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);

    // 检查是否可以打开
    if (!chest->canOpen(&player, player.getHeldItem(hand))) {
        // 播放锁定音效
        // TODO: world.playSound(player, pos, SoundEvents.BLOCK_CHEST_LOCKED, SoundCategory::BLOCKS, 1.0f, 1.0f);
        return ActionResult::Success;
    }

    // 打开箱子GUI
    // TODO: player.openContainer(state.getContainer(world, pos));

    // 增加打开计数
    chest->openContainer();

    return ActionResult::Consume;
}

// ========== 红石 ==========

i32 ChestBlock::getComparatorInputOverride(
    const BlockState& state,
    World& world,
    const BlockPos& pos
) const {
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Chest) {
        return 0;
    }

    auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
    return chest->getComparatorSignal(world);
}

// ========== 静态工具方法 ==========

Direction ChestBlock::getConnectedDirection(const BlockState& state) {
    BlockStateProperties::ChestType type = state.get(BlockStateProperties::CHEST_TYPE());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    switch (type) {
        case BlockStateProperties::ChestType::Left:
            return facing.rotateY();  // 左箱子向右连接
        case BlockStateProperties::ChestType::Right:
            return facing.rotateYCCW();  // 右箱子向左连接
        default:
            return Direction::None;  // 单箱无连接
    }
}

bool ChestBlock::isBlocked(IWorld& world, const BlockPos& pos) {
    return isBlocked(world, pos.up());
}

bool ChestBlock::isCatSittingOn(IWorld& world, const BlockPos& pos) {
    // TODO: 检查是否有猫坐在箱子上
    // List<CatEntity> cats = world.getEntitiesWithinAABB(CatEntity.class,
    //     new AxisAlignedBB(pos.getX(), pos.getY() + 1, pos.getZ(),
    //                       pos.getX() + 1, pos.getY() + 2, pos.getZ() + 1));
    // for (CatEntity cat : cats) {
    //     if (cat.isSitting()) return true;
    // }
    (void)world;
    (void)pos;
    return false;
}

// ========== 受保护方法 ==========

void ChestBlock::combineChests(
    const BlockState& state,
    World& world,
    const BlockPos& pos,
    Direction facing
) {
    // 更新当前箱子的类型
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    BlockStateProperties::ChestType newType = BlockStateProperties::ChestType::Single;
    if (facing == currentFacing.rotateYCCW()) {
        newType = BlockStateProperties::ChestType::Right;
    } else if (facing == currentFacing.rotateY()) {
        newType = BlockStateProperties::ChestType::Left;
    }

    world.setBlockState(pos, state.with(BlockStateProperties::CHEST_TYPE(), newType), 3);

    // 更新相邻箱子的类型
    BlockPos neighborPos = pos.offset(facing);
    BlockState neighborState = world.getBlockState(neighborPos);

    if (neighborState.getBlock() == this) {
        BlockStateProperties::ChestType neighborType =
            newType == BlockStateProperties::ChestType::Left
                ? BlockStateProperties::ChestType::Right
                : BlockStateProperties::ChestType::Left;
        world.setBlockState(neighborPos,
            neighborState.with(BlockStateProperties::CHEST_TYPE(), neighborType), 3);
    }
}

bool ChestBlock::canCombineWithChestAt(
    IWorld& world,
    const BlockPos& pos,
    Direction facing,
    Direction expectedFacing
) const {
    BlockPos neighborPos = pos.offset(facing);
    BlockState neighborState = world.getBlockState(neighborPos);

    if (neighborState.getBlock() != this) {
        return false;
    }

    Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

    return neighborFacing == expectedFacing &&
           neighborType == BlockStateProperties::ChestType::Single;
}

} // namespace blocks
} // namespace mc
