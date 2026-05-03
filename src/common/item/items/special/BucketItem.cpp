#include "BucketItem.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/IBucketPickupHandler.hpp"
#include "../../../world/block/ILiquidContainer.hpp"
#include "../../../world/block/IWaterLoggable.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidRegistry.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../../world/block/blocks/LiquidBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../core/ItemStack.hpp"
#include "../../context/BlockItemUseContext.hpp"
#include "../../core/ItemRegistry.hpp"
#include "../../Items.hpp"
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

    if (world.isClientSide()) {
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
                            ItemStack filledStack = filledBucket->getDefaultInstance();
                            player->inventory().add(filledStack);
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
                fluid::FluidState fluidState = m_containedFluid->defaultState();
                if (liquidContainer->receiveFluid(world, targetPos, *targetState, fluidState)) {
                    // 播放倒水音效
                    // TODO: world.playSound(player, targetPos, SoundEvents.ITEM_BUCKET_EMPTY, SoundCategory.BLOCKS, 1.0F, 1.0F);

                    // 非创造模式下替换为空桶
                    if (player == nullptr || !player->isCreative()) {
                        BucketItem* emptyBucket = getEmptyBucket();
                        if (emptyBucket != nullptr) {
                            stack.shrink(1);
                            if (player != nullptr) {
                                ItemStack emptyStack = emptyBucket->getDefaultInstance();
                                player->inventory().add(emptyStack);
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
                    ItemStack emptyStack = emptyBucket->getDefaultInstance();
                    player->inventory().add(emptyStack);
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult BucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    if (world.isClientSide()) {
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
        fluid::FluidState fluidState = m_containedFluid->defaultState();
        const BlockState* fluidBlockState = fluidState.getBlockState();
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

            // 调度流体 tick（需要非const引用）
            fluid::FluidState mutableFluidState = m_containedFluid->defaultState();
            m_containedFluid->tick(world, pos, mutableFluidState);

            return true;
        }
    }

    // 检查是否可以替换
    const Material& material = currentState->owner().material();
    if (material.isReplaceable() && !material.isLiquid()) {
        fluid::FluidState fluidState = m_containedFluid->defaultState();
        const BlockState* fluidBlockState = fluidState.getBlockState();
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
    // 参考 MC 1.16.5: fluid.getFilledBucket()
    // 使用 FluidTags 判断流体类型，返回对应的桶物品
    if (fluid::FluidTags::WATER().contains(fluid)) {
        MC_ASSERT_RELEASE(Items::WATER_BUCKET != nullptr);
        return static_cast<BucketItem*>(Items::WATER_BUCKET);
    }
    if (fluid::FluidTags::LAVA().contains(fluid)) {
        MC_ASSERT_RELEASE(Items::LAVA_BUCKET != nullptr);
        return static_cast<BucketItem*>(Items::LAVA_BUCKET);
    }
    return nullptr;
}

BucketItem* BucketItem::getEmptyBucket() {
    MC_ASSERT_RELEASE(Items::BUCKET != nullptr);
    return static_cast<BucketItem*>(Items::BUCKET);
}

} // namespace mc
