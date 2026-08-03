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

#include "MerchantContainer.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include <utility>

namespace mc {

MerchantContainer::MerchantContainer(IMerchant& merchant)
    : m_merchant(merchant)
    , m_items(CONTAINER_SIZE)
{}

bool MerchantContainer::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack MerchantContainer::getItem(i32 slot) const
{
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return ItemStack();
    }
    return m_items[slot];
}

ItemStack MerchantContainer::removeItem(i32 slot, i32 count)
{
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return ItemStack();
    }

    ItemStack& stack = m_items[slot];

    // 结果槽：移除全部数量
    if (slot == SLOT_RESULT && !stack.isEmpty()) {
        return stack.split(stack.getCount());
    }

    ItemStack result = stack.split(count);
    if (!result.isEmpty() && _isPaymentSlot(slot)) {
        updateSellItem();
    }
    return result;
}

ItemStack MerchantContainer::removeItemNoUpdate(i32 slot)
{
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return ItemStack();
    }

    ItemStack result = std::move(m_items[slot]);
    m_items[slot] = ItemStack();
    return result;
}

void MerchantContainer::setItem(i32 slot, const ItemStack& stack)
{
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return;
    }

    m_items[slot] = std::move(stack);
    if (_isPaymentSlot(slot)) {
        updateSellItem();
    }
}

void MerchantContainer::clear()
{
    for (auto& item : m_items) {
        item = ItemStack();
    }
}

void MerchantContainer::setChanged()
{
    updateSellItem();
}

bool MerchantContainer::isUsableByPlayer(const Player& player) const
{
    // 检查交易对象是否为该玩家
    const Player* tradingPlayer = m_merchant.getTradingPlayer();
    return tradingPlayer == &player;
}

bool MerchantContainer::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    (void)stack;
    // 只允许放入支付槽，不允许放入结果槽
    return _isPaymentSlot(slot);
}

// ========== 交易特定方法 ==========

void MerchantContainer::updateSellItem()
{
    m_activeOffer = nullptr;

    ItemStack buyA;
    ItemStack buyB;

    // 确定主要和次要支付物品
    if (m_items[SLOT_BUY_A].isEmpty()) {
        buyA = m_items[SLOT_BUY_B];
        buyB = ItemStack();
    } else {
        buyA = m_items[SLOT_BUY_A];
        buyB = m_items[SLOT_BUY_B];
    }

    if (buyA.isEmpty()) {
        m_items[SLOT_RESULT] = ItemStack();
        m_futureXp = 0;
        return;
    }

    auto& offers = m_merchant.getOffers();
    if (offers.empty()) {
        m_items[SLOT_RESULT] = ItemStack();
        m_futureXp = 0;
        return;
    }

    // 尝试按原始顺序匹配交易
    MerchantOffer* offer = offers.getOfferFor(buyA, buyB, m_selectionHint);

    // 如果按原始顺序无匹配，或匹配到的交易已售罄，尝试交换支付槽顺序
    if (offer == nullptr || offer->isOutOfStock()) {
        m_activeOffer = offer;
        offer = offers.getOfferFor(buyB, buyA, m_selectionHint);
    }

    if (offer != nullptr && !offer->isOutOfStock()) {
        m_activeOffer = offer;
        m_items[SLOT_RESULT] = offer->getSell().copy();
        m_futureXp = offer->getXp();
    } else {
        m_items[SLOT_RESULT] = ItemStack();
        m_futureXp = 0;
    }

    // 通知商民交易更新（用于播放声音）
    m_merchant.notifyTradeUpdated(m_items[SLOT_RESULT]);
}

MerchantOffer* MerchantContainer::getActiveOffer()
{
    return m_activeOffer;
}

const MerchantOffer* MerchantContainer::getActiveOffer() const
{
    return m_activeOffer;
}

void MerchantContainer::setSelectionHint(i32 hint)
{
    m_selectionHint = hint;
    updateSellItem();
}

bool MerchantContainer::_isPaymentSlot(i32 slot)
{
    return slot == SLOT_BUY_A || slot == SLOT_BUY_B;
}

} // namespace mc
