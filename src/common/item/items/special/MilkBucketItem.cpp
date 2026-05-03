#include "MilkBucketItem.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/UseAction.hpp"
#include "../../Items.hpp"

namespace mc {
namespace item {
namespace special {

MilkBucketItem::MilkBucketItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

i32 MilkBucketItem::getUseDuration(const ItemStack& stack) const {
    (void)stack;
    // MC 1.16.5: 牛奶桶饮用时间为 32 ticks
    return 32;
}

UseAction MilkBucketItem::getUseAction(const ItemStack& stack) const {
    (void)stack;
    // MC 1.16.5: 牛奶桶返回 Drink 动作
    return UseAction::Drink;
}

ItemActionResult MilkBucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    (void)world;

    // MC 1.16.5: 牛奶桶可以在任何时候饮用
    // 与食物不同，不需要检查饥饿值
    ItemStack stack = player.getHeldItem(hand);
    if (canEat(stack, player)) {
        // 设置活跃手，开始饮用
        player.setActiveHand(hand);
        return ItemActionResult::consume(stack);
    }

    return ItemActionResult::pass(stack);
}

ItemStack MilkBucketItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) {
    (void)world;

    // MC 1.16.5: 清除所有药水效果
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
            if (remaining > 0) {
                // TODO: 掉落物品到地面
                // player->spawnItem(new ItemStack(Items::BUCKET, remaining));
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

bool MilkBucketItem::canEat(const ItemStack& stack, const Player& player) const {
    (void)stack;
    (void)player;

    // MC 1.16.5: 牛奶桶可以在任何时候饮用
    // 即使没有药水效果也可以饮用（创造模式可以用来测试）
    return true;
}

} // namespace special
} // namespace item
} // namespace mc
