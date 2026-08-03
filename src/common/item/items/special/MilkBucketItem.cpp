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

#include "MilkBucketItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc {
namespace item {
namespace special {

MilkBucketItem::MilkBucketItem(ItemProperties properties)
    : Item(std::move(properties))
{}

i32 MilkBucketItem::getUseDuration(const ItemStack& stack) const
{
    (void)stack;
    // 牛奶桶饮用时间为 32 ticks
    return 32;
}

UseAction MilkBucketItem::getUseAction(const ItemStack& stack) const
{
    (void)stack;
    // 牛奶桶返回 Drink 动作
    return UseAction::Drink;
}

ItemActionResult MilkBucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    (void)world;

    // 牛奶桶可以在任何时候饮用，与食物不同，不需要检查饥饿值
    ItemStack stack = player.getHeldItem(hand);
    if (canEat(stack, player)) {
        // 设置活跃手，开始饮用
        player.setActiveHand(hand);
        return ItemActionResult::consume(stack);
    }

    return ItemActionResult::pass(stack);
}

ItemStack MilkBucketItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    (void)world;

    // 清除所有药水效果
    LivingEntity* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity != nullptr) {
        livingEntity->removeAllEffects();
    }

    // 播放打嗝音效（仅玩家）
    Player* player = dynamic_cast<Player*>(&entity);
    if (player != nullptr) {
        player->playSound(SoundEvents::ENTITY_PLAYER_BURP, 0.5f, 1.0f);

        // 减少物品数量（创造模式不减）
        if (!player->isCreative()) {
            stack.shrink(1);
        }

        // 返回空桶
        if (Items::BUCKET != nullptr && !stack.isEmpty()) {
            ItemStack bucketStack(Items::BUCKET, 1);
            // 尝试添加到背包
            i32 remaining = player->inventory().add(bucketStack);
            // 如果有剩余，掉落到地面
            if (remaining > 0 && !bucketStack.isEmpty()) {
                // 使用 ItemDropHelper 掉落物品
                math::Random rng;
                ItemDropHelper::spawnItemAtEntity(player, bucketStack, 0.5f, rng);
            }
        }
    } else {
        stack.shrink(1);
    }

    // 如果物品堆已空，返回空桶
    if (stack.isEmpty() && Items::BUCKET != nullptr) {
        return ItemStack(Items::BUCKET, 1);
    }

    return stack;
}

bool MilkBucketItem::canEat(const ItemStack& stack, const Player& player) const
{
    (void)stack;
    (void)player;

    // 牛奶桶可以在任何时候饮用，即使没有药水效果也可以饮用
    return true;
}

} // namespace special
} // namespace item
} // namespace mc
