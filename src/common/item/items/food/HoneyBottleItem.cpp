#include "HoneyBottleItem.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../Items.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item::items {

HoneyBottleItem::HoneyBottleItem(const food::Food* food, ItemProperties properties)
    : FoodItem(food, std::move(properties))
{}

ItemStack HoneyBottleItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 调用父类方法处理基本的食用逻辑（恢复饥饿值、饱和度、效果等）
    ItemStack result = FoodItem::onItemUseFinish(stack, world, entity);

    // MC 1.16.5: 清除中毒效果
    // 参考: net.minecraft.item.HoneyBottleItem#onItemUseFinish
    if (auto* livingEntity = dynamic_cast<LivingEntity*>(&entity)) {
        livingEntity->removeEffect(entity::effect::EffectType::Poison);
    }

    // 如果物品堆为空，返回玻璃瓶
    if (result.isEmpty()) {
        if (Items::GLASS_BOTTLE != nullptr) {
            return ItemStack(Items::GLASS_BOTTLE, 1);
        }
        return ItemStack();
    }

    // 如果玩家不是创造模式，尝试添加玻璃瓶到背包
    // FoodItem的父类已经处理了容器物品逻辑，这里返回result即可
    return result;
}

i32 HoneyBottleItem::getUseDuration(const ItemStack& stack) const
{
    // MC 1.16.5: 蜂蜜瓶使用时间为40 ticks（2秒）
    (void)stack;
    return 40;
}

UseAction HoneyBottleItem::getUseAction(const ItemStack& stack) const
{
    // MC 1.16.5: 蜂蜜瓶使用 Drink 动作
    (void)stack;
    return UseAction::Drink;
}

} // namespace item::items
} // namespace mc
