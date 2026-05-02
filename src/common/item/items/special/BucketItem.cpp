#include "BucketItem.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/IBucketPickupHandler.hpp"
#include "../../../world/block/ILiquidContainer.hpp"
#include "../../../world/block/IWaterLoggable.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidState.hpp"
#include "../../../world/fluid/FluidRegistry.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../../world/block/blocks/LiquidBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../core/ItemStack.hpp"
#include "../../context/BlockItemUseContext.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../util/Direction.hpp"
#include "../../../core/Constants.hpp"

namespace mc {

BucketItem::BucketItem(
    fluid::Fluid* containedFluid,
    const ItemProperties& properties)
    : Item(properties)
    , m_containedFluid(containedFluid) {
}

ActionResultType BucketItem::onItemUse(ItemUseContext& context) {
    IWorld& world = context.getWorld();
    BlockPos pos = context.blockPos();
    Direction face = context.getFace();
    Player* player = context.getPlayer();
    ItemStack& stack = context.getItemStackMut();

    if (world.isRemote()) {
        return ActionResultType::Success;
    }

    // 计算目标位置（点击面的另一侧）
    BlockPos targetPos = pos.offset(face);

    // 空桶：从方块中取出流体
    if (m_containedFluid == nullptr) {
        const BlockState* blockState = world.getBlockState(pos);
        if (blockState == nullptr) {
            return ActionResultType::Fail;
        }

        Block* block = Block::getBlock(blockState->blockId());
        if (block == nullptr) {
            return ActionResultType::Fail;
        }

        // 检查方块是否实现了 IBucketPickupHandler
        auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(block);
        if (pickupHandler != nullptr) {
            fluid::Fluid* pickedFluid = pickupHandler->pickupFluid(world, pos, *blockState);
            if (pickedFluid != nullptr) {
                // 播放取水音效
                // TODO: world.playSound(player, pos, SoundEvents.ITEM_BUCKET_FILL, SoundCategory.BLOCKS, 1.0F, 1.0F);

                // 非创造模式下替换物品
                if (player == nullptr || !player->isCreative()) {
                    BucketItem* filledBucket = getFilledBucket(*pickedFluid);
                    if (filledBucket != nullptr) {
                        stack.shrink(1);
                        if (player != nullptr) {
                            player->addItem(filledBucket->getDefaultInstance());
                        }
                    }
                }
                return ActionResultType::Success;
            }
        }
        return ActionResultType::Fail;
    }

    // 装满的桶：放置流体
    const BlockState* targetState = world.getBlockState(targetPos);
    if (targetState == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查目标方块是否可以容纳流体
    Block* targetBlock = Block::getBlock(targetState->blockId());
    if (targetBlock != nullptr) {
        auto* liquidContainer = dynamic_cast<ILiquidContainer*>(targetBlock);
        if (liquidContainer != nullptr) {
            // 检查是否可以容纳该流体
            if (liquidContainer->canContainFluid(world, targetPos, *targetState, *m_containedFluid)) {
                // 获取流体状态（静止状态）
                const fluid::FluidState& fluidState = m_containedFluid->defaultState();
                if (liquidContainer->receiveFluid(world, targetPos, targetState, fluidState)) {
                    // 播放倒水音效
                    // TODO: world.playSound(player, targetPos, SoundEvents.ITEM_BUCKET_EMPTY, SoundCategory.BLOCKS, 1.0F, 1.0F);

                    // 非创造模式下替换为空桶
                    if (player == nullptr || !player->isCreative()) {
                        BucketItem* emptyBucket = getEmptyBucket();
                        if (emptyBucket != nullptr) {
                            stack.shrink(1);
                            if (player != nullptr) {
                                player->addItem(emptyBucket->getDefaultInstance());
                            }
                        }
                    }
                    return ActionResultType::Success;
                }
            }
        }
    }

    // 尝试直接放置流体方块
    if (tryPlaceContainedLiquid(player, world, targetPos, BlockRaycastResult())) {
        // 非创造模式下替换为空桶
        if (player == nullptr || !player->isCreative()) {
            BucketItem* emptyBucket = getEmptyBucket();
            if (emptyBucket != nullptr) {
                stack.shrink(1);
                if (player != nullptr) {
                    player->addItem(emptyBucket->getDefaultInstance());
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult BucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    if (world.isRemote()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    // 对于装满的桶，尝试放置流体
    if (m_containedFluid != nullptr) {
        // 射线检测目标位置
        // 这里简化处理，实际应该通过玩家的射线检测
        return ItemActionResult::pass(player.getHeldItem(hand));
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool BucketItem::tryPlaceContainedLiquid(
    Player* player,
    IWorld& world,
    const BlockPos& pos,
    const BlockRaycastResult& hit) {
    MC_UNUSED(player);
    MC_UNUSED(hit);

    if (m_containedFluid == nullptr) {
        return false;
    }

    // 检查目标位置是否可以放置流体
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr) {
        return false;
    }

    // 如果是空气或可替换方块，放置流体方块
    Block* currentBlock = Block::getBlock(currentState->blockId());
    if (currentBlock == nullptr || currentBlock->isAir(*currentState)) {
        // 获取流体对应的方块状态
        const BlockState* fluidBlockState = m_containedFluid->getBlockState();
        if (fluidBlockState == nullptr) {
            // 对于水或岩浆，使用对应的液体方块
            if (m_containedFluid->isIn(fluid::FluidTags::WATER())) {
                fluidBlockState = VanillaBlocks::getState(VanillaBlocks::WATER);
            } else if (m_containedFluid->isIn(fluid::FluidTags::LAVA())) {
                fluidBlockState = VanillaBlocks::getState(VanillaBlocks::LAVA);
            }
        }

        if (fluidBlockState != nullptr) {
            world.setBlockState(pos, fluidBlockState, 3);

            // 调度流体 tick
            m_containedFluid->tick(world, pos, m_containedFluid->defaultState());

            return true;
        }
    }

    // 检查是否可以替换
    const Material& material = currentState->owner().material();
    if (material.isReplaceable() && !material.isLiquid()) {
        const BlockState* fluidBlockState = m_containedFluid->getBlockState();
        if (fluidBlockState == nullptr) {
            if (m_containedFluid->isIn(fluid::FluidTags::WATER())) {
                fluidBlockState = VanillaBlocks::getState(VanillaBlocks::WATER);
            } else if (m_containedFluid->isIn(fluid::FluidTags::LAVA())) {
                fluidBlockState = VanillaBlocks::getState(VanillaBlocks::LAVA);
            }
        }

        if (fluidBlockState != nullptr) {
            world.setBlockState(pos, fluidBlockState, 3);
            return true;
        }
    }

    return false;
}

bool BucketItem::canBlockContainFluid(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) const {
    if (m_containedFluid == nullptr) {
        return false;
    }

    Block* block = Block::getBlock(state.blockId());
    if (block == nullptr) {
        return false;
    }

    auto* liquidContainer = dynamic_cast<ILiquidContainer*>(block);
    if (liquidContainer == nullptr) {
        return false;
    }

    return liquidContainer->canContainFluid(world, pos, state, *m_containedFluid);
}

BucketItem* BucketItem::getFilledBucket(fluid::Fluid& fluid) {
    // TODO: 通过物品注册表查找对应的桶
    // 目前返回 nullptr，需要在 Items.hpp 中注册桶物品后实现
    MC_UNUSED(fluid);
    return nullptr;
}

BucketItem* BucketItem::getEmptyBucket() {
    // TODO: 通过物品注册表查找空桶
    // 目前返回 nullptr，需要在 Items.hpp 中注册后实现
    return nullptr;
}

} // namespace mc
