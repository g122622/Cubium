#include "ChestBlock.hpp"
#include "../BlockRegistry.hpp"
#include "../Block.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/storage/ChestEntity.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/entities/passive/tamable/CatEntity.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../fluid/Fluid.hpp"
#include "../../fluid/FluidRegistry.hpp"
#include "../../fluid/FluidTags.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 常量 ==========

namespace {
    // VoxelShape 碰撞箱
    constexpr f32 CHEST_WIDTH = 14.0f;
    constexpr f32 CHEST_HEIGHT = 14.0f;
    constexpr f32 CHEST_DEPTH = 14.0f;
    constexpr f32 CHEST_OFFSET = 1.0f;

    void scheduleWaterTick(mc::IWorld& world, const mc::BlockPos& pos) {
        mc::fluid::Fluid* waterFluid = mc::fluid::FluidRegistry::instance().getFluid(mc::fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.scheduleFluidTick(pos, *waterFluid, waterFluid->getTickDelay(world));
    }
}

// ========== 构造函数 ==========

ChestBlock::ChestBlock(const BlockProperties& properties)
    : Block(properties) {
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::CHEST_TYPE())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single)
        .with(BlockStateProperties::WATERLOGGED(), false));
}

// ========== 放置和更新 ==========

BlockState ChestBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = Directions::opposite(context.horizontalDirection());

    // 默认为单箱
    BlockStateProperties::ChestType chestType = BlockStateProperties::ChestType::Single;

    // 检查是否可以与相邻箱子合并
    // 检查四个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = context.placementPos().offset(dir);
        const BlockState* neighborStatePtr = context.getWorld().getBlockState(neighborPos);

        if (neighborStatePtr != nullptr && &neighborStatePtr->getBlock() == this) {
            const BlockState& neighborState = *neighborStatePtr;
            Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

            // 只有朝向相同且是单箱的才能合并
            if (neighborFacing == facing && neighborType == BlockStateProperties::ChestType::Single) {
                // 根据相对位置确定LEFT或RIGHT
                if (dir == Directions::rotateYCCW(facing)) {
                    // 玩家面对的左边有箱子，当前箱子是RIGHT
                    chestType = BlockStateProperties::ChestType::Right;
                } else if (dir == Directions::rotateY(facing)) {
                    // 玩家面对的右边有箱子，当前箱子是LEFT
                    chestType = BlockStateProperties::ChestType::Left;
                }
                break;
            }
        }
    }

    // 检查水logged状态 - 暂时不支持，默认false
    bool waterlogged = false;

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
        scheduleWaterTick(world, pos);
    }

    // 检查是否与相邻箱子连接/断开
    if (Directions::isHorizontal(facing)) {
        BlockStateProperties::ChestType currentType = state.get(BlockStateProperties::CHEST_TYPE());

        if (&neighborState.getBlock() == this) {
            Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

            // 检查是否可以连接
            if (currentType == BlockStateProperties::ChestType::Single &&
                neighborType != BlockStateProperties::ChestType::Single &&
                currentFacing == neighborFacing) {

                Direction connectedDir = getConnectedDirection(neighborState);
                if (connectedDir == Directions::opposite(facing)) {
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

ActionResultType ChestBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit
) {
    // 检查是否被阻挡
    if (isBlocked(world, pos) || isCatSittingOn(world, pos)) {
        return ActionResultType::Success;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Chest) {
        return ActionResultType::Pass;
    }

    auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);

    // 检查是否可以打开
    if (!chest->canOpen(&player, player.getHeldItem(hand))) {
        // 播放锁定音效
        world.playSound(ResourceLocation("minecraft:block.chest.locked"), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        return ActionResultType::Success;
    }

    // 打开箱子GUI
    if (world.openContainer(ContainerType::Chest, pos, player)) {
        chest->openContainer();
        return ActionResultType::Consume;
    }

    return world.asServerWorld() == nullptr ? ActionResultType::Success : ActionResultType::Pass;
}

// ========== 红石 ==========

i32 ChestBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
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
            return Directions::rotateY(facing);  // 左箱子向右连接
        case BlockStateProperties::ChestType::Right:
            return Directions::rotateYCCW(facing);  // 右箱子向左连接
        default:
            return Direction::None;  // 单箱无连接
    }
}

bool ChestBlock::isBlocked(IWorld& world, const BlockPos& pos) {
    // 检查上方位置是否有不透明方块阻挡箱子打开
    BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false;  // 上方为空气，不阻挡
    }
    // 检查方块是否是不透明的固体方块
    return aboveState->hasOpaqueCollisionShape();
}

bool ChestBlock::isCatSittingOn(IWorld& world, const BlockPos& pos) {
    const AxisAlignedBB catBox(
        static_cast<f32>(pos.x),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 2),
        static_cast<f32>(pos.z + 1));

    for (Entity* entity : world.getEntitiesInAABB(catBox)) {
        auto* cat = dynamic_cast<CatEntity*>(entity);
        if (cat != nullptr && cat->isSitting()) {
            return true;
        }
    }

    return false;
}

// ========== 受保护方法 ==========

void ChestBlock::combineChests(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction facing
) {
    // 更新当前箱子的类型
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    BlockStateProperties::ChestType newType = BlockStateProperties::ChestType::Single;
    if (facing == Directions::rotateYCCW(currentFacing)) {
        newType = BlockStateProperties::ChestType::Right;
    } else if (facing == Directions::rotateY(currentFacing)) {
        newType = BlockStateProperties::ChestType::Left;
    }

    world.setBlockState(pos, &state.with(BlockStateProperties::CHEST_TYPE(), newType), 3);

    // 更新相邻箱子的类型
    BlockPos neighborPos = pos.offset(facing);
    const BlockState* neighborStatePtr = world.getBlockState(neighborPos);
    if (neighborStatePtr != nullptr && &neighborStatePtr->getBlock() == this) {
        BlockState neighborState = *neighborStatePtr;
        BlockStateProperties::ChestType neighborType =
            newType == BlockStateProperties::ChestType::Left
                ? BlockStateProperties::ChestType::Right
                : BlockStateProperties::ChestType::Left;
        world.setBlockState(neighborPos,
            &neighborState.with(BlockStateProperties::CHEST_TYPE(), neighborType), 3);
    }
}

bool ChestBlock::canCombineWithChestAt(
    IWorld& world,
    const BlockPos& pos,
    Direction facing,
    Direction expectedFacing
) const {
    BlockPos neighborPos = pos.offset(facing);
    const BlockState* neighborStatePtr = world.getBlockState(neighborPos);

    if (neighborStatePtr == nullptr || &neighborStatePtr->getBlock() != this) {
        return false;
    }

    Direction neighborFacing = neighborStatePtr->get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::ChestType neighborType = neighborStatePtr->get(BlockStateProperties::CHEST_TYPE());

    return neighborFacing == expectedFacing &&
           neighborType == BlockStateProperties::ChestType::Single;
}

} // namespace blocks
} // namespace mc
