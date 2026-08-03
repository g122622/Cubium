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

#include "ChestBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 常量 ==========

namespace {
// VoxelShape 碰撞箱
constexpr f32 CHEST_WIDTH = 14.0f;
constexpr f32 CHEST_HEIGHT = 14.0f;
constexpr f32 CHEST_DEPTH = 14.0f;
constexpr f32 CHEST_OFFSET = 1.0f;
} // namespace

// ========== 构造函数 ==========

ChestBlock::ChestBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::CHEST_TYPE())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ========== 放置和更新 ==========

BlockState ChestBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = Directions::opposite(context.horizontalDirection());

    // 默认为单箱
    BlockStateProperties::ChestType chestType = BlockStateProperties::ChestType::Single;

    // 检查是否可以与相邻箱子合并
    // 检查四个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = context.placementPos().offset(dir);
        const BlockState* neighborStatePtr = context.getWorld().getBlockState(neighborPos);

        if (neighborStatePtr != nullptr && chestCanConnectTo(*neighborStatePtr)) {
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

    // 检查水logged状态
    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::CHEST_TYPE(), chestType)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState ChestBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& neighborState,
    IWorld& world,
    const BlockPos& pos,
    const BlockPos& neighborPos)
{
    // 处理水logged状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, pos);
    }

    // 检查是否与相邻箱子连接/断开
    if (Directions::isHorizontal(facing)) {
        BlockStateProperties::ChestType currentType = state.get(BlockStateProperties::CHEST_TYPE());

        if (chestCanConnectTo(neighborState)) {
            Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            Direction neighborFacing = neighborState.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockStateProperties::ChestType neighborType = neighborState.get(BlockStateProperties::CHEST_TYPE());

            // 检查是否可以连接
            if (currentType == BlockStateProperties::ChestType::Single &&
                neighborType != BlockStateProperties::ChestType::Single && currentFacing == neighborFacing) {

                Direction connectedDir = getConnectedDirection(neighborState);
                if (connectedDir == Directions::opposite(facing)) {
                    // 连接到相邻箱子
                    return state.with(BlockStateProperties::CHEST_TYPE(),
                        neighborType == BlockStateProperties::ChestType::Left ? BlockStateProperties::ChestType::Right
                                                                              : BlockStateProperties::ChestType::Left);
                }
            }
        } else {
            // 检查是否需要断开连接
            Direction connectedDir = getConnectedDirection(state);
            if (connectedDir != Direction::None && connectedDir == facing) {
                return state.with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
            }
        }
    }

    return state;
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> ChestBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::ChestEntity>(pos);
}

// ========== 交互 ==========

BlockActionResult ChestBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 检查是否被阻挡
    if (isBlocked(world, pos) || isCatSittingOn(world, pos)) {
        return ActionResultType::Success;
    }

    // 获取方块实体（支持普通箱子和陷阱箱）
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity ||
        (blockEntity->getType() != BlockEntityType::Chest && blockEntity->getType() != BlockEntityType::TrappedChest)) {
        return ActionResultType::Pass;
    }

    auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);

    // 双箱时，检查另一半是否也被阻挡
    if (chest->isDoubleChest(world)) {
        blockentity::ChestEntity* connected = chest->getConnectedChest(world);
        if (connected != nullptr) {
            // 获取另一半的位置来检查阻挡和猫
            BlockPos connectedPos = connected->getPos();
            if (isBlocked(world, connectedPos) || isCatSittingOn(world, connectedPos)) {
                return ActionResultType::Success;
            }
        }
    }

    // 检查是否可以打开
    if (!chest->canOpen(&player, player.getHeldItem(hand))) {
        // 播放锁定音效
        world.playSound(
            ResourceLocation("minecraft:block.chest.locked"), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        return ActionResultType::Success;
    }

    // 打开箱子GUI
    // 单个箱子是27格 (3行)，使用 Generic9x3 类型
    if (world.openContainer(ContainerType::Generic9x3, pos, player)) {
        chest->openContainer(&player);
        // 陷阱箱子与普通箱子使用不同的统计
        if (getBlockEntityType() == BlockEntityType::TrappedChest) {
            player.awardCustomStat(ResourceLocation(stats::TRIGGER_TRAPPED_CHEST), 1);
        } else {
            player.awardCustomStat(ResourceLocation(stats::OPEN_CHEST), 1);
        }

        // 打开箱子时激怒附近能看到玩家的猪灵
        entity::PiglinAi::angerNearbyPiglins(world, player, true);

        // 双箱时，同步增加另一半的打开计数
        // 参考 MC Java: CompoundContainer.startOpen() 会转发到两个半箱
        if (chest->isDoubleChest(world)) {
            blockentity::ChestEntity* connected = chest->getConnectedChest(world);
            if (connected != nullptr) {
                connected->openContainer(&player);
            }
        }

        return ActionResultType::Consume;
    }

    return world.asServerWorld() == nullptr ? ActionResultType::Success : ActionResultType::Pass;
}

// ========== 移除处理 ==========

void ChestBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 箱子被移除时需要掉落其内容物
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr &&
        (blockEntity->getType() == BlockEntityType::Chest || blockEntity->getType() == BlockEntityType::TrappedChest)) {
        auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);

        // 收集箱子中的所有物品
        std::vector<ItemStack> drops;
        IInventory* inventory = chest->getInventory();
        for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
            ItemStack stack = inventory->getItem(i);
            if (!stack.isEmpty()) {
                drops.push_back(stack);
            }
        }

        // 在方块位置掉落所有物品
        if (!drops.empty() && !world.isClientSide()) {
            math::Random rng;
            ItemDropHelper::spawnItemEntities(&world, pos, drops, rng);
        }

        // 清空物品
        chest->clearContainer();
        MC_UNUSED(state);
    }

    // 调用基类处理
    Block::onBlockRemoved(world, pos, state);
}

// ========== 红石 ==========

i32 ChestBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity ||
        (blockEntity->getType() != BlockEntityType::Chest && blockEntity->getType() != BlockEntityType::TrappedChest)) {
        return 0;
    }

    auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
    return chest->getComparatorSignal(world);
}

// ========== 旋转和镜像 ==========

const BlockState& ChestBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 旋转箱子的朝向（HORIZONTAL_FACING）
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& ChestBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 镜像箱子的朝向，并交换左/右类型
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::ChestType chestType = state.get(BlockStateProperties::CHEST_TYPE());

    // 镜像朝向
    Direction newFacing = facing;
    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    // 镜像时交换左/右类型
    BlockStateProperties::ChestType newType = chestType;
    if (chestType == BlockStateProperties::ChestType::Left) {
        newType = BlockStateProperties::ChestType::Right;
    } else if (chestType == BlockStateProperties::ChestType::Right) {
        newType = BlockStateProperties::ChestType::Left;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing)
        .with(BlockStateProperties::CHEST_TYPE(), newType);
}

// ========== 静态工具方法 ==========

Direction ChestBlock::getConnectedDirection(const BlockState& state)
{
    BlockStateProperties::ChestType type = state.get(BlockStateProperties::CHEST_TYPE());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    switch (type) {
        case BlockStateProperties::ChestType::Left:
            return Directions::rotateY(facing); // 左箱子向右连接
        case BlockStateProperties::ChestType::Right:
            return Directions::rotateYCCW(facing); // 右箱子向左连接
        default:
            return Direction::None; // 单箱无连接
    }
}

bool ChestBlock::isBlocked(IWorld& world, const BlockPos& pos)
{
    // 检查上方位置是否有不透明方块阻挡箱子打开
    BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false; // 上方为空气，不阻挡
    }
    // 检查方块是否是不透明的固体方块
    return aboveState->hasOpaqueCollisionShape();
}

bool ChestBlock::isCatSittingOn(IWorld& world, const BlockPos& pos)
{
    const AxisAlignedBB catBox(static_cast<f32>(pos.x),
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

// ========== 双箱连接 ==========

bool ChestBlock::chestCanConnectTo(const BlockState& neighborState) const
{
    // 默认实现：邻居方块类型与当前方块一致时可以连接
    return &neighborState.getBlock() == this;
}

// ========== 开合音效 ==========

const ResourceLocation& ChestBlock::getOpenSound() const
{
    // 默认返回普通箱子的开箱音效；铜箱子子类重写此方法返回对应氧化等级的声音事件
    return SoundEvents::BLOCK_CHEST_OPEN;
}

const ResourceLocation& ChestBlock::getCloseSound() const
{
    // 默认返回普通箱子的关箱音效；铜箱子子类重写此方法返回对应氧化等级的声音事件
    return SoundEvents::BLOCK_CHEST_CLOSE;
}

// ========== 受保护方法 ==========

void ChestBlock::combineChests(const BlockState& state, IWorld& world, const BlockPos& pos, Direction facing)
{
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
    if (neighborStatePtr != nullptr && chestCanConnectTo(*neighborStatePtr)) {
        BlockState neighborState = *neighborStatePtr;
        BlockStateProperties::ChestType neighborType = newType == BlockStateProperties::ChestType::Left
            ? BlockStateProperties::ChestType::Right
            : BlockStateProperties::ChestType::Left;
        world.setBlockState(neighborPos, &neighborState.with(BlockStateProperties::CHEST_TYPE(), neighborType), 3);
    }
}

bool ChestBlock::canCombineWithChestAt(
    IWorld& world, const BlockPos& pos, Direction facing, Direction expectedFacing) const
{
    BlockPos neighborPos = pos.offset(facing);
    const BlockState* neighborStatePtr = world.getBlockState(neighborPos);

    if (neighborStatePtr == nullptr || !chestCanConnectTo(*neighborStatePtr)) {
        return false;
    }

    Direction neighborFacing = neighborStatePtr->get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::ChestType neighborType = neighborStatePtr->get(BlockStateProperties::CHEST_TYPE());

    return neighborFacing == expectedFacing && neighborType == BlockStateProperties::ChestType::Single;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* ChestBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
