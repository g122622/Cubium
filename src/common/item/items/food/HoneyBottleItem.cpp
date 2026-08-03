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

#include "HoneyBottleItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/food/Food.hpp"
#include "common/item/items/food/FoodItem.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc {
namespace item::items {

HoneyBottleItem::HoneyBottleItem(const food::Food* food, ItemProperties properties)
    : FoodItem(food, std::move(properties))
{}

ItemStack HoneyBottleItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 调用父类方法处理基本的食用逻辑（恢复饥饿值、饱和度、效果等）
    ItemStack result = FoodItem::onItemUseFinish(stack, world, entity);

    // 清除中毒效果
    if (auto* livingEntity = dynamic_cast<LivingEntity*>(&entity)) {
        livingEntity->removeEffect(entity::effect::EffectType::Poison);
    }

    // 如果物品堆为空，返回玻璃瓶
    if (result.isEmpty()) {
        return ItemStack(Items::GLASS_BOTTLE, 1);
    }

    // 如果玩家不是创造模式，尝试添加玻璃瓶到背包
    // FoodItem的父类已经处理了容器物品逻辑，这里返回result即可
    return result;
}

i32 HoneyBottleItem::getUseDuration(const ItemStack& stack) const
{
    // 蜂蜜瓶使用时间为40 ticks（2秒）
    (void)stack;
    return 40;
}

UseAction HoneyBottleItem::getUseAction(const ItemStack& stack) const
{
    // 蜂蜜瓶使用 Drink 动作
    (void)stack;
    return UseAction::Drink;
}

} // namespace item::items
} // namespace mc
