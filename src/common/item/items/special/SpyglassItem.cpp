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

#include "SpyglassItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc {
namespace item::items {

SpyglassItem::SpyglassItem(ItemProperties properties)
    : Item(std::move(properties))
{}

i32 SpyglassItem::getUseDuration(const ItemStack& stack) const
{
    MC_UNUSED(stack);
    return USE_DURATION;
}

UseAction SpyglassItem::getUseAction(const ItemStack& stack) const
{
    MC_UNUSED(stack);
    return UseAction::Spyglass;
}

ItemActionResult SpyglassItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 对齐 vanilla SpyglassItem#use：播放使用音效后立即开始使用（不消耗物品）
    player.playSound(SoundEvents::ITEM_SPYGLASS_USE, 1.0f, 1.0f);
    player.setActiveHand(hand);
    return ItemActionResult::success(player.getHeldItem(hand));
}

ItemStack SpyglassItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    MC_UNUSED(world);
    // 对齐 vanilla finishUsingItem：退出望远镜视角
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living != nullptr) {
        living->playSound(SoundEvents::ITEM_SPYGLASS_STOP_USING, 1.0f, 1.0f);
    }
    return stack;
}

void SpyglassItem::onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft)
{
    MC_UNUSED(stack);
    MC_UNUSED(world);
    MC_UNUSED(timeLeft);
    // 对齐 vanilla releaseUsing：提前松开也退出望远镜视角
    entity.playSound(SoundEvents::ITEM_SPYGLASS_STOP_USING, 1.0f, 1.0f);
}

} // namespace item::items
} // namespace mc
