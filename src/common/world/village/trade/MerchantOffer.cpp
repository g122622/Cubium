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

#include "MerchantOffer.hpp"
#include "Merchant.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace village {
namespace trade {

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_buyA(std::move(buyA))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack buyB, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_buyA(std::move(buyA))
    , m_buyB(std::move(buyB))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

bool MerchantOffer::canAccept(const ItemStack& offeredA, const ItemStack& offeredB) const
{
    if (isOutOfStock()) {
        return false;
    }

    // 检查第一物品
    if (!offeredA.canStackWith(m_buyA) || offeredA.getCount() < getAdjustedBuyPrice()) {
        return false;
    }

    // 检查第二物品（如果需要）
    if (m_buyB.has_value()) {
        if (!offeredB.canStackWith(*m_buyB) || offeredB.getCount() < m_buyB->getCount()) {
            return false;
        }
    }

    return true;
}

bool MerchantOffer::canAccept(const ItemStack& offered) const
{
    return canAccept(offered, ItemStack());
}

bool MerchantOffer::apply(Player& player, IMerchant& merchant)
{
    // 检查是否已售罄
    if (isOutOfStock()) {
        return false;
    }

    // 获取玩家背包
    PlayerInventory& inventory = player.inventory();

    // 获取调整后的买入价格
    i32 buyACount = getAdjustedBuyPrice();
    i32 buyBCount = m_buyB.has_value() ? m_buyB->getCount() : 0;

    // 检查玩家是否有足够的物品
    // 先检查买入物品A
    i32 buyAAvailable = 0;
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack slotStack = inventory.getItem(slot);
        if (slotStack.canStackWith(m_buyA)) {
            buyAAvailable += slotStack.getCount();
        }
    }
    if (buyAAvailable < buyACount) {
        return false; // 物品A不足
    }

    // 检查买入物品B（如果需要）
    if (m_buyB.has_value()) {
        i32 buyBAvailable = 0;
        for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
            ItemStack slotStack = inventory.getItem(slot);
            if (slotStack.canStackWith(*m_buyB)) {
                buyBAvailable += slotStack.getCount();
            }
        }
        if (buyBAvailable < buyBCount) {
            return false; // 物品B不足
        }
    }

    // 检查卖出物品是否可以放入背包
    ItemStack sellCopy = m_sell;
    i32 sellCount = m_sell.getCount();

    // 计算背包可以容纳多少卖出物品
    i32 sellCanFit = 0;
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack slotStack = inventory.getItem(slot);
        if (slotStack.isEmpty()) {
            // 空槽位可以放满
            sellCanFit += m_sell.getMaxStackSize();
        } else if (slotStack.canStackWith(m_sell)) {
            // 相同物品可以合并
            sellCanFit += slotStack.getMaxStackSize() - slotStack.getCount();
        }
    }
    if (sellCanFit < sellCount) {
        return false; // 背包空间不足
    }

    // 执行交易
    // 1. 从玩家背包扣除买入物品A
    i32 remainingA = buyACount;
    for (i32 slot = 0; slot < inventory.getContainerSize() && remainingA > 0; ++slot) {
        ItemStack slotStack = inventory.getItem(slot);
        if (slotStack.canStackWith(m_buyA)) {
            i32 toRemove = std::min(remainingA, slotStack.getCount());
            inventory.removeItem(slot, toRemove);
            remainingA -= toRemove;
        }
    }

    // 2. 从玩家背包扣除买入物品B（如果需要）
    if (m_buyB.has_value()) {
        i32 remainingB = buyBCount;
        for (i32 slot = 0; slot < inventory.getContainerSize() && remainingB > 0; ++slot) {
            ItemStack slotStack = inventory.getItem(slot);
            if (slotStack.canStackWith(*m_buyB)) {
                i32 toRemove = std::min(remainingB, slotStack.getCount());
                inventory.removeItem(slot, toRemove);
                remainingB -= toRemove;
            }
        }
    }

    // 3. 向玩家背包添加卖出物品
    ItemStack sellToAdd = m_sell;
    inventory.add(sellToAdd);

    // 4. 增加使用次数
    increaseUses();

    // 5. 给商人增加经验
    merchant.addExperience(m_xp);

    return true;
}

void MerchantOffer::restock()
{
    m_uses = 0;
    ++m_restocksToday;
}

bool MerchantOffer::take(ItemStack& buyA, ItemStack& buyB)
{
    // 检查是否满足交易条件
    if (!canAccept(buyA, buyB)) {
        return false;
    }

    // 扣除第一物品
    buyA.shrink(getAdjustedBuyPrice());

    // 扣除第二物品（如果有）
    if (m_buyB.has_value() && !buyB.isEmpty()) {
        buyB.shrink(m_buyB->getCount());
    }

    return true;
}

bool MerchantOffer::isDisabled() const
{
    return isOutOfStock() && m_restocksToday >= 2; // 每天最多补货2次
}

f32 MerchantOffer::getProgress() const noexcept
{
    if (m_maxUses <= 0) return 0.0f;
    return static_cast<f32>(m_uses) / static_cast<f32>(m_maxUses);
}

void MerchantOffer::applyDemand(i32 demandBonus)
{
    m_demand += demandBonus;
    // 需求影响价格：需求越高，价格越高
    // m_specialPrice 会根据需求调整
    m_specialPrice = static_cast<i32>(m_demand * m_priceMultiplier);
}

void MerchantOffer::updateDemand()
{
    // MC原版逻辑：demand = demand + uses - (maxUses - uses)
    // 即 demand += 2 * uses - maxUses
    // 使用次数超过一半时需求增加（价格上涨），反之需求减少（价格下降）
    m_demand = m_demand + m_uses - (m_maxUses - m_uses);
    m_specialPrice = static_cast<i32>(m_demand * m_priceMultiplier);
}

i32 MerchantOffer::getAdjustedBuyPrice() const noexcept
{
    i32 basePrice = m_buyA.getCount();
    i32 adjusted = basePrice + m_specialPrice;
    return std::max(1, adjusted);
}

void MerchantOffer::serialize(nbt::tags::compound_tag& tag) const
{
    // 序列化买入物品A
    nbt::tags::compound_tag buyATag;
    m_buyA.toNbt(buyATag);
    tag.value["buy"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyATag));

    // 序列化买入物品B（如果有）
    if (m_buyB.has_value()) {
        nbt::tags::compound_tag buyBTag;
        m_buyB->toNbt(buyBTag);
        tag.value["buyB"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyBTag));
    }

    // 序列化卖出物品
    nbt::tags::compound_tag sellTag;
    m_sell.toNbt(sellTag);
    tag.value["sell"] = std::make_unique<nbt::tags::compound_tag>(std::move(sellTag));

    // 序列化基础数值字段
    tag.put("uses", static_cast<std::int32_t>(m_uses));
    tag.put("maxUses", static_cast<std::int32_t>(m_maxUses));
    tag.put("xp", static_cast<std::int32_t>(m_xp));
    tag.put("priceMultiplier", static_cast<float>(m_priceMultiplier));
    tag.put("specialPrice", static_cast<std::int32_t>(m_specialPrice));
    tag.put("demand", static_cast<std::int32_t>(m_demand));
    tag.put("restocksToday", static_cast<std::int32_t>(m_restocksToday));
    tag.put("lastRestock", static_cast<std::int64_t>(m_lastRestockTime));
}

MerchantOffer MerchantOffer::deserialize(const nbt::tags::compound_tag& tag)
{
    MerchantOffer offer;

    // 反序列化买入物品A
    auto buyAIt = tag.value.find("buy");
    if (buyAIt != tag.value.end() && buyAIt->second->id() == nbt::TagId::Compound) {
        auto& buyATag = dynamic_cast<const nbt::tags::compound_tag&>(*buyAIt->second);
        auto buyAResult = ItemStack::fromNbt(buyATag);
        if (buyAResult.success()) {
            offer.m_buyA = buyAResult.value();
        }
    }

    // 反序列化买入物品B（可选）
    auto buyBIt = tag.value.find("buyB");
    if (buyBIt != tag.value.end() && buyBIt->second->id() == nbt::TagId::Compound) {
        auto& buyBTag = dynamic_cast<const nbt::tags::compound_tag&>(*buyBIt->second);
        auto buyBResult = ItemStack::fromNbt(buyBTag);
        if (buyBResult.success()) {
            offer.m_buyB = buyBResult.value();
        }
    }

    // 反序列化卖出物品
    auto sellIt = tag.value.find("sell");
    if (sellIt != tag.value.end() && sellIt->second->id() == nbt::TagId::Compound) {
        auto& sellTag = dynamic_cast<const nbt::tags::compound_tag&>(*sellIt->second);
        auto sellResult = ItemStack::fromNbt(sellTag);
        if (sellResult.success()) {
            offer.m_sell = sellResult.value();
        }
    }

    // 反序列化数值字段
    auto getOptionalInt = [&tag](const std::string& key, i32 defaultValue) -> i32 {
        auto it = tag.value.find(key);
        if (it != tag.value.end()) {
            if (it->second->id() == nbt::TagId::Int) {
                return dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
            }
        }
        return defaultValue;
    };

    auto getOptionalFloat = [&tag](const std::string& key, f32 defaultValue) -> f32 {
        auto it = tag.value.find(key);
        if (it != tag.value.end()) {
            if (it->second->id() == nbt::TagId::Float) {
                return dynamic_cast<const nbt::tags::float_tag&>(*it->second).value;
            } else if (it->second->id() == nbt::TagId::Double) {
                return static_cast<f32>(dynamic_cast<const nbt::tags::double_tag&>(*it->second).value);
            }
        }
        return defaultValue;
    };

    auto getOptionalLong = [&tag](const std::string& key, i64 defaultValue) -> i64 {
        auto it = tag.value.find(key);
        if (it != tag.value.end()) {
            if (it->second->id() == nbt::TagId::Long) {
                return dynamic_cast<const nbt::tags::long_tag&>(*it->second).value;
            }
        }
        return defaultValue;
    };

    offer.m_uses = getOptionalInt("uses", 0);
    offer.m_maxUses = getOptionalInt("maxUses", 12);
    offer.m_xp = getOptionalInt("xp", 0);
    offer.m_priceMultiplier = getOptionalFloat("priceMultiplier", 1.0f);
    offer.m_specialPrice = getOptionalInt("specialPrice", 0);
    offer.m_demand = getOptionalInt("demand", 0);
    offer.m_restocksToday = getOptionalInt("restocksToday", 0);
    offer.m_lastRestockTime = getOptionalLong("lastRestock", 0);

    return offer;
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
