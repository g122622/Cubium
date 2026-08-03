/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PowderSnowBucketItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"

namespace mc {
namespace item {

PowderSnowBucketItem::PowderSnowBucketItem(const ItemProperties& properties)
    : Item(properties)
{}

ActionResultType PowderSnowBucketItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.getWorld();
    Player* player = context.getPlayer();

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 水下使用时返回 Consume（不允许水下放置）
    // 对应 MC Java 的 SolidBucketItem 继承 BlockItem，水下放置失败后
    // 通过 CONSUMABLE 数据组件返回 CONSUME 的涌现行为
    if (player != nullptr && player->isInWater()) {
        return ActionResultType::Consume;
    }

    // 计算放置位置（点击面的另一侧）
    BlockPos targetPos = context.blockPos().offset(context.face());

    // 尝试放置细雪方块
    if (emptyContents(player, world, targetPos)) {
        // 放置成功，消耗细雪桶并返回空桶
        if (player == nullptr || !player->isCreative()) {
            context.getItemStackMut().shrink(1);
            if (player != nullptr) {
                _returnEmptyBucket(*player, context.getItemStackMut());
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

bool PowderSnowBucketItem::emptyContents(Player* player, IWorld& world, const BlockPos& pos) const
{
    // 检查目标位置的方块状态
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr) {
        return false;
    }

    // 只能在空方块处放置（对应 MC Java 的 isEmptyBlock）
    Block* currentBlock = Block::getBlock(currentState->blockId());
    if (currentBlock != nullptr && !currentBlock->isAir(*currentState)) {
        return false;
    }

    // 放置细雪方块
    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    if (powderSnowState == nullptr) {
        return false;
    }

    world.setBlockState(pos, powderSnowState, 3);

    // 触发方块放置游戏事件
    world.gameEvent(gameevent::GameEvents::BLOCK_PLACE,
        pos,
        gameevent::GameEvent::Context(static_cast<const Entity*>(player), powderSnowState));

    // 播放细雪桶倒空音效
    Vector3 soundPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
    world.playSound(SoundEvents::ITEM_BUCKET_EMPTY_POWDER_SNOW, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);

    return true;
}

void PowderSnowBucketItem::_returnEmptyBucket(Player& player, ItemStack& stack) const
{
    if (Items::BUCKET == nullptr) {
        return;
    }

    // 如果物品堆已空，直接替换为空桶
    if (stack.isEmpty()) {
        stack = ItemStack(Items::BUCKET, 1);
        return;
    }

    // 否则尝试将空桶添加到背包
    ItemStack bucketStack(Items::BUCKET, 1);
    i32 remaining = player.inventory().add(bucketStack);

    // 如果背包满了，掉落到地面
    if (remaining > 0 && !bucketStack.isEmpty()) {
        math::Random rng;
        ItemDropHelper::spawnItemAtEntity(&player, bucketStack, 0.5f, rng);
    }
}

} // namespace item
} // namespace mc
