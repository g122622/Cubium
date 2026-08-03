/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do this, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MerchantContainerMenu.hpp"

#include "MerchantResultSlot.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/inventory/container/MerchantContainer.hpp"
#include "common/sound/SoundEvents.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc {

MerchantContainerMenu::MerchantContainerMenu(ContainerId id, PlayerInventory* playerInventory, IMerchant& merchant)
    : AbstractContainerMenu(id, playerInventory)
    , m_merchant(merchant)
    , m_tradeContainer(std::make_unique<MerchantContainer>(merchant))
{
    _initSlots(playerInventory);
}

void MerchantContainerMenu::_initSlots(PlayerInventory* playerInventory)
{
    // 支付槽1
    addSlot(std::make_unique<Slot>(m_tradeContainer.get(), MerchantContainer::SLOT_BUY_A, PAYMENT1_X, ROW_Y));

    // 支付槽2
    addSlot(std::make_unique<Slot>(m_tradeContainer.get(), MerchantContainer::SLOT_BUY_B, PAYMENT2_X, ROW_Y));

    // 结果槽（交易结果，只能取不能放）
    addSlot(std::make_unique<MerchantResultSlot>(
        *playerInventory->getPlayer(), m_merchant, *m_tradeContainer, MerchantContainer::SLOT_RESULT, RESULT_X, ROW_Y));

    // 玩家主背包 (3x9)
    addPlayerInventorySlots(PLAYER_INV_X, PLAYER_INV_Y);

    // 玩家快捷栏 (1x9)
    addPlayerHotbarSlots(PLAYER_INV_X, PLAYER_INV_Y + 58);
}

bool MerchantContainerMenu::stillValid(const Player& player) const
{
    return m_merchant.stillValid(player);
}

void MerchantContainerMenu::slotsChanged(IInventory* inventory)
{
    m_tradeContainer->updateSellItem();
    AbstractContainerMenu::slotsChanged(inventory);
}

void MerchantContainerMenu::removed(Player& player)
{
    AbstractContainerMenu::removed(player);

    // 关闭交易
    m_merchant.stopTrading();

    // 将支付槽中的物品返回给玩家
    if (!m_merchant.isClientSide()) {
        ItemStack itemA = m_tradeContainer->removeItemNoUpdate(MerchantContainer::SLOT_BUY_A);
        if (!itemA.isEmpty()) {
            player.inventory().addItem(itemA);
        }

        ItemStack itemB = m_tradeContainer->removeItemNoUpdate(MerchantContainer::SLOT_BUY_B);
        if (!itemB.isEmpty()) {
            player.inventory().addItem(itemB);
        }
    }
}

bool MerchantContainerMenu::canMergeSlot(const ItemStack& stack, const Slot& slot) const
{
    (void)stack;
    // 结果槽不允许Shift+点击合并
    return slot.getIndex() != SLOT_RESULT;
}

void MerchantContainerMenu::setSelectionHint(i32 hint)
{
    m_tradeContainer->setSelectionHint(hint);
}

MerchantOffers& MerchantContainerMenu::getOffers() const
{
    return m_merchant.getOffers();
}

void MerchantContainerMenu::setOffers(MerchantOffers offers)
{
    m_merchant.overrideOffers(std::move(offers));
}

i32 MerchantContainerMenu::getTraderXp() const
{
    return m_merchant.getVillagerXp();
}

i32 MerchantContainerMenu::getFutureXp() const
{
    return m_tradeContainer->getFutureXp();
}

void MerchantContainerMenu::setXp(i32 xp)
{
    m_merchant.overrideXp(xp);
}

ItemStack MerchantContainerMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;
    ItemStack result;
    Slot* slot = getSlot(slotIndex);

    if (slot == nullptr || !slot->hasItem()) {
        return result;
    }

    ItemStack slotStack = slot->getItem();
    result = slotStack.copy();

    if (slotIndex == SLOT_RESULT) {
        // 从结果槽移到玩家背包
        if (!moveItemToRange(slotStack, INV_SLOT_START, HOTBAR_SLOT_END, true)) {
            return ItemStack();
        }
        slot->onCrafting(slotStack, result.getCount());
        _playTradeSound();
    } else if (slotIndex == SLOT_PAYMENT_1 || slotIndex == SLOT_PAYMENT_2) {
        // 从支付槽移到玩家背包
        if (!moveItemToRange(slotStack, INV_SLOT_START, HOTBAR_SLOT_END, false)) {
            return ItemStack();
        }
    } else if (slotIndex >= INV_SLOT_START && slotIndex < INV_SLOT_END) {
        // 从主背包移到快捷栏
        if (!moveItemToRange(slotStack, HOTBAR_SLOT_START, HOTBAR_SLOT_END, false)) {
            return ItemStack();
        }
    } else if (slotIndex >= HOTBAR_SLOT_START && slotIndex < HOTBAR_SLOT_END) {
        // 从快捷栏移到主背包
        if (!moveItemToRange(slotStack, INV_SLOT_START, INV_SLOT_END, false)) {
            return ItemStack();
        }
    }

    if (slotStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }

    if (slotStack.getCount() == result.getCount()) {
        return ItemStack();
    }

    slot->onTake(player, slotStack);
    return result;
}

void MerchantContainerMenu::tryMoveItems(i32 offerIndex)
{
    if (offerIndex < 0) {
        return;
    }

    auto& offers = getOffers();
    if (offerIndex >= static_cast<i32>(offers.size())) {
        return;
    }

    // 如果支付槽已有物品，先尝试移回背包
    ItemStack itemA = m_tradeContainer->getItem(MerchantContainer::SLOT_BUY_A);
    if (!itemA.isEmpty()) {
        if (!moveItemToRange(itemA, INV_SLOT_START, HOTBAR_SLOT_END, true)) {
            return;
        }
        m_tradeContainer->setItem(MerchantContainer::SLOT_BUY_A, itemA);
    }

    ItemStack itemB = m_tradeContainer->getItem(MerchantContainer::SLOT_BUY_B);
    if (!itemB.isEmpty()) {
        if (!moveItemToRange(itemB, INV_SLOT_START, HOTBAR_SLOT_END, true)) {
            return;
        }
        m_tradeContainer->setItem(MerchantContainer::SLOT_BUY_B, itemB);
    }

    // 如果支付槽为空，从背包中自动填充对应物品
    if (m_tradeContainer->getItem(MerchantContainer::SLOT_BUY_A).isEmpty() &&
        m_tradeContainer->getItem(MerchantContainer::SLOT_BUY_B).isEmpty()) {
        MerchantOffer& offer = *offers.getOffer(offerIndex);
        _moveFromInventoryToPaymentSlot(MerchantContainer::SLOT_BUY_A, offer.getBuyA());

        if (offer.getBuyB().has_value()) {
            _moveFromInventoryToPaymentSlot(MerchantContainer::SLOT_BUY_B, offer.getBuyB().value());
        }
    }
}

void MerchantContainerMenu::_moveFromInventoryToPaymentSlot(i32 paymentSlot, const ItemStack& targetStack)
{
    if (targetStack.isEmpty()) {
        return;
    }

    for (i32 i = INV_SLOT_START; i < HOTBAR_SLOT_END; ++i) {
        Slot* slot = getSlot(i);
        if (slot == nullptr) {
            continue;
        }

        ItemStack invStack = slot->getItem();
        if (invStack.isEmpty() || !invStack.isSameItem(targetStack)) {
            continue;
        }

        ItemStack paymentStack = m_tradeContainer->getItem(paymentSlot);
        if (!paymentStack.isEmpty() && !paymentStack.isSameItem(invStack)) {
            continue;
        }

        i32 maxStackSize = invStack.getMaxStackSize();
        i32 currentCount = paymentStack.isEmpty() ? 0 : paymentStack.getCount();
        i32 transferCount = std::min(maxStackSize - currentCount, invStack.getCount());

        ItemStack newPaymentStack = invStack.copy();
        newPaymentStack.setCount(currentCount + transferCount);
        invStack.shrink(transferCount);

        m_tradeContainer->setItem(paymentSlot, std::move(newPaymentStack));

        if (newPaymentStack.getCount() >= maxStackSize) {
            break;
        }
    }
}

void MerchantContainerMenu::_playTradeSound()
{
    if (!m_merchant.isClientSide()) {
        Entity* entity = m_merchant.asEntity();
        if (entity != nullptr) {
            entity->playSound(SoundEvents::ENTITY_VILLAGER_TRADE, 1.0f, 1.0f);
        }
    }
}

} // namespace mc
