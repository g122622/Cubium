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

#include "BucketItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBucketPickupHandler.hpp"
#include "common/world/block/ILiquidContainer.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {

BucketItem::BucketItem(fluid::Fluid* containedFluid, const ItemProperties& properties)
    : Item(properties)
    , m_containedFluid(containedFluid)
{}

ActionResultType BucketItem::onItemUse(ItemUseContext& context)
{
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
            // 首先尝试流体拾取（水、岩浆等）
            fluid::Fluid* pickedFluid = pickupHandler->pickupFluid(world, pos, *blockState);
            if (pickedFluid != nullptr) {
                // 播放取水音效
                Vector3 soundPos(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
                world.playSound(SoundEvents::ITEM_BUCKET_FILL, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);

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
                } else if (player != nullptr) {
                    // 创造模式下仍需触发事件，但不需要给物品
                    BucketItem* filledBucket = getFilledBucket(*pickedFluid);
                    if (filledBucket != nullptr) {
                        ItemStack filledStack = filledBucket->getDefaultInstance();
                        world.onFilledBucket(player->id(), filledStack);
                    }
                }

                // 非创造模式下触发事件
                if (player != nullptr && !player->isCreative()) {
                    BucketItem* filledBucket = getFilledBucket(*pickedFluid);
                    if (filledBucket != nullptr) {
                        ItemStack filledStack = filledBucket->getDefaultInstance();
                        world.onFilledBucket(player->id(), filledStack);
                    }
                }

                return ActionResultType::Success;
            }

            // 如果流体拾取返回 nullptr，尝试非流体拾取（细雪等）
            const Item* pickedItem = pickupHandler->pickupItem(world, pos, *blockState);
            if (pickedItem != nullptr) {
                // 播放拾取音效（使用方块指定的音效或默认音效）
                Vector3 soundPos(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
                const ResourceLocation* pickupSound = pickupHandler->getPickupSound(world, pos, *blockState);
                if (pickupSound != nullptr) {
                    world.playSound(*pickupSound, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);
                } else {
                    world.playSound(SoundEvents::ITEM_BUCKET_FILL, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);
                }

                // 非创造模式下替换物品
                if (player == nullptr || !player->isCreative()) {
                    stack.shrink(1);
                    if (player != nullptr) {
                        ItemStack pickedStack(pickedItem, 1);
                        player->inventory().add(pickedStack);
                    }
                } else if (player != nullptr) {
                    // 创造模式下仍需触发事件，但不需要给物品
                    ItemStack pickedStack(pickedItem, 1);
                    world.onFilledBucket(player->id(), pickedStack);
                }

                // 非创造模式下触发事件
                if (player != nullptr && !player->isCreative()) {
                    ItemStack pickedStack(pickedItem, 1);
                    world.onFilledBucket(player->id(), pickedStack);
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
                    Vector3 soundPos(static_cast<f32>(targetPos.x) + 0.5f,
                        static_cast<f32>(targetPos.y) + 0.5f,
                        static_cast<f32>(targetPos.z) + 0.5f);
                    world.playSound(SoundEvents::ITEM_BUCKET_EMPTY, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);

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

ItemActionResult BucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
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
    Player* player, IWorld& world, const BlockPos& pos, const BlockRaycastResult& hit)
{
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

            // 通过世界调度器调度流体 tick，而非直接调用
            world.tickManager().scheduleFluidTick(pos, *m_containedFluid, m_containedFluid->getTickDelay(world));

            return true;
        }
    }

    // 检查是否可被流体替换（对应 MC Java 的 blockstate.canBeReplaced(fluidContent)）
    // canBeReplacedByFluid() = canBeReplaced() || !isSolid()，覆盖可替换方块和非固体方块
    if (currentState->canBeReplacedByFluid()) {
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

bool BucketItem::canBlockContainFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
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

BucketItem* BucketItem::getFilledBucket(fluid::Fluid& fluid)
{
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

BucketItem* BucketItem::getEmptyBucket()
{
    MC_ASSERT_RELEASE(Items::BUCKET != nullptr);
    return static_cast<BucketItem*>(Items::BUCKET);
}

bool BucketItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // 只有空桶可以挤奶
    if (m_containedFluid != nullptr) {
        return false;
    }

    // 只有牛（包括哞菇）可以被挤奶
    auto* cow = dynamic_cast<CowEntity*>(&target);
    if (cow == nullptr) {
        return false;
    }

    // 幼年牛不能被挤奶
    if (cow->isChild()) {
        return false;
    }

    // 播放挤奶音效
    cow->playSound(SoundEvents::ENTITY_COW_MILK, 1.0f, 1.0f);

    // 处理物品转换
    // 非创造模式：减少空桶，添加牛奶桶
    if (!player.isCreative()) {
        stack.shrink(1);

        // 创建牛奶桶
        if (Items::MILK_BUCKET != nullptr) {
            ItemStack milkBucket(Items::MILK_BUCKET, 1);

            // 如果空桶用完了，直接返回牛奶桶
            if (stack.isEmpty()) {
                // 需要通过某种方式设置玩家手持物品
                // 这里返回 true 表示交互成功，实际物品替换由调用方处理
                player.inventory().add(milkBucket);
            } else {
                // 空桶还有剩余，尝试将牛奶桶添加到背包
                player.inventory().add(milkBucket);
            }
        }
    }

    return true;
}

} // namespace mc
