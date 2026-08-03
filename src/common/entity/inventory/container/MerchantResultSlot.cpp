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

#include "MerchantResultSlot.hpp"

#include "MerchantContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include <algorithm>

namespace mc {

MerchantResultSlot::MerchantResultSlot(
    Player& player, IMerchant& merchant, MerchantContainer& container, i32 slotIndex, i32 x, i32 y)
    : Slot(&container, slotIndex, x, y)
    , m_player(player)
    , m_merchant(merchant)
    , m_container(container)
{}

bool MerchantResultSlot::mayPlace(const ItemStack& stack) const
{
    (void)stack;
    // 结果槽不允许放置物品
    return false;
}

ItemStack MerchantResultSlot::remove(i32 amount)
{
    if (hasItem()) {
        m_removeCount += std::min(amount, getItem().getCount());
    }
    return Slot::remove(amount);
}

ItemStack MerchantResultSlot::onTake(Player& player, ItemStack stack)
{
    (void)player;
    _checkTakeAchievements(stack);

    MerchantOffer* offer = m_container.getActiveOffer();
    if (offer != nullptr) {
        ItemStack buyA = m_container.getItem(MerchantContainer::SLOT_BUY_A);
        ItemStack buyB = m_container.getItem(MerchantContainer::SLOT_BUY_B);

        // 尝试扣除支付物品（尝试两种顺序：先A后B，或先B后A）
        if (offer->take(buyA, buyB) || offer->take(buyB, buyA)) {
            // 交易成功，通知商民（内部已调用 rewardTradeXp 添加经验）
            m_merchant.notifyTrade(*offer);

            // 更新支付槽物品（扣除后的剩余数量）
            m_container.setItem(MerchantContainer::SLOT_BUY_A, buyA);
            m_container.setItem(MerchantContainer::SLOT_BUY_B, buyB);
        }
    }

    return stack;
}

void MerchantResultSlot::_checkTakeAchievements(ItemStack& stack)
{
    // 成就/进度触发和统计追踪通过 VillagerTradeEvent 在 notifyTrade() 中完成
    (void)stack;
    m_removeCount = 0;
}

} // namespace mc
