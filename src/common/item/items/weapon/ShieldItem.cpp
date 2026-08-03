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

#include "ShieldItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item {

// ========== 构造函数 ==========

ShieldItem::ShieldItem(const ItemProperties& properties)
    : Item(properties)
{}

// ========== Item 接口重写 ==========

i32 ShieldItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return MAX_USE_DURATION;
}

UseAction ShieldItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Block;
}

ItemActionResult ShieldItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand)
{
    ItemStack shieldStack = player.getHeldItem(hand);

    // 进入格挡状态
    player.setActiveHand(hand);
    return ItemActionResult::success(shieldStack);
}

// ========== 静态方法 ==========

bool ShieldItem::isShield(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return dynamic_cast<const ShieldItem*>(stack.getItem()) != nullptr;
}

} // namespace item
} // namespace mc
