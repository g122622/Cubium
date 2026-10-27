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
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace village {
namespace trade {

MerchantOffer::MerchantOffer()
    : m_costA(ItemStack(), 1)
{}

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_costA(buyA, buyA.getCount())
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack buyB, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_costA(std::move(buyA), buyA.getCount())
    , m_costB(ItemCost(std::move(buyB), buyB.getCount()))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

MerchantOffer::MerchantOffer(ItemCost costA, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_costA(std::move(costA))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

MerchantOffer::MerchantOffer(
    ItemCost costA, std::optional<ItemCost> costB, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_costA(std::move(costA))
    , m_costB(std::move(costB))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{}

ItemStack MerchantOffer::getBuyA() const
{
    return m_costA.toStack();
}

std::optional<ItemStack> MerchantOffer::getBuyB() const
{
    if (!m_costB.has_value()) {
        return std::nullopt;
    }
    return m_costB->toStack();
}

i32 MerchantOffer::getBaseModifiedCostCount() const noexcept
{
    const i32 base = m_costA.count();
    // 需求按"基础数量的百分比"折算：floor(base * demand * priceMultiplier)
    const i32 demandBonus = std::max(0, static_cast<i32>(std::floor(base * m_demand * m_priceMultiplier)));
    const i32 maxStack = m_costA.templateStack().getMaxStackSize();
    return std::clamp(base + demandBonus, 1, std::max(1, maxStack));
}

i32 MerchantOffer::getAdjustedBuyPrice() const noexcept
{
    const i32 base = m_costA.count();
    const i32 demandBonus = std::max(0, static_cast<i32>(std::floor(base * m_demand * m_priceMultiplier)));
    const i32 maxStack = m_costA.templateStack().getMaxStackSize();
    return std::clamp(base + demandBonus + m_specialPrice, 1, std::max(1, maxStack));
}

bool MerchantOffer::canAccept(const ItemStack& offeredA, const ItemStack& offeredB) const
{
    if (isOutOfStock()) {
        return false;
    }

    // 第一支付：物品 + 组件谓词 + 需求修正后的数量
    if (!m_costA.test(offeredA) || offeredA.getCount() < getAdjustedBuyPrice()) {
        return false;
    }

    // 第二支付：数量固定，不受需求与特殊价格影响
    if (m_costB.has_value()) {
        if (!m_costB->test(offeredB)) {
            return false;
        }
    } else if (!offeredB.isEmpty()) {
        // 单支付槽交易不接受第二物品
        return false;
    }

    return true;
}

bool MerchantOffer::canAccept(const ItemStack& offered) const
{
    return canAccept(offered, ItemStack());
}

bool MerchantOffer::matchesInput(const ItemStack& offeredA, const ItemStack& offeredB) const
{
    if (!m_costA.testComponentwise(offeredA)) {
        return false;
    }
    if (m_costB.has_value()) {
        return m_costB->testComponentwise(offeredB);
    }
    return offeredB.isEmpty();
}

bool MerchantOffer::apply(Player& player, IMerchant& merchant)
{
    PlayerInventory& inventory = player.inventory();

    // 背包整包扣减按支付代价逐槽匹配，匹配语义与 take() 一致
    const i32 buyACount = getAdjustedBuyPrice();
    const i32 buyBCount = m_costB.has_value() ? m_costB->count() : 0;

    // 预检：第一支付数量是否足够
    i32 availableA = 0;
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        const ItemStack slotStack = inventory.getItem(slot);
        if (m_costA.test(slotStack)) {
            availableA += slotStack.getCount();
        }
    }
    if (availableA < buyACount) {
        return false;
    }

    // 预检：第二支付数量是否足够
    if (m_costB.has_value()) {
        i32 availableB = 0;
        for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
            const ItemStack slotStack = inventory.getItem(slot);
            if (m_costB->test(slotStack)) {
                availableB += slotStack.getCount();
            }
        }
        if (availableB < buyBCount) {
            return false;
        }
    }

    // 预检：卖出物品能否放入背包
    i32 sellCanFit = 0;
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        const ItemStack slotStack = inventory.getItem(slot);
        if (slotStack.isEmpty()) {
            sellCanFit += m_sell.getMaxStackSize();
        } else if (slotStack.canStackWith(m_sell)) {
            sellCanFit += slotStack.getMaxStackSize() - slotStack.getCount();
        }
    }
    if (sellCanFit < m_sell.getCount()) {
        return false;
    }

    // 扣除第一支付
    i32 remainingA = buyACount;
    for (i32 slot = 0; slot < inventory.getContainerSize() && remainingA > 0; ++slot) {
        const ItemStack slotStack = inventory.getItem(slot);
        if (m_costA.test(slotStack)) {
            const i32 toRemove = std::min(remainingA, slotStack.getCount());
            inventory.removeItem(slot, toRemove);
            remainingA -= toRemove;
        }
    }

    // 扣除第二支付
    if (m_costB.has_value()) {
        i32 remainingB = buyBCount;
        for (i32 slot = 0; slot < inventory.getContainerSize() && remainingB > 0; ++slot) {
            const ItemStack slotStack = inventory.getItem(slot);
            if (m_costB->test(slotStack)) {
                const i32 toRemove = std::min(remainingB, slotStack.getCount());
                inventory.removeItem(slot, toRemove);
                remainingB -= toRemove;
            }
        }
    }

    // 发放卖出物品
    ItemStack sellToAdd = m_sell;
    inventory.add(sellToAdd);

    // 只推进使用次数；经验统一由 notifyTrade -> rewardTradeXp 发放，避免重复
    increaseUses();

    return true;
}

void MerchantOffer::restock()
{
    m_uses = 0;
    ++m_restocksToday;
}

bool MerchantOffer::take(ItemStack& buyA, ItemStack& buyB)
{
    if (!canAccept(buyA, buyB)) {
        return false;
    }

    // 扣除第一支付（数量为需求修正后的值）
    buyA.shrink(getAdjustedBuyPrice());

    // 扣除第二支付（数量固定）
    if (m_costB.has_value()) {
        buyB.shrink(m_costB->count());
    }

    return true;
}

bool MerchantOffer::isDisabled() const
{
    return isOutOfStock() && m_restocksToday >= 2; // 每天最多补货2次
}

f32 MerchantOffer::getProgress() const noexcept
{
    if (m_maxUses <= 0) {
        return 0.0f;
    }
    return static_cast<f32>(m_uses) / static_cast<f32>(m_maxUses);
}

void MerchantOffer::swapCosts()
{
    if (!m_costB.has_value()) {
        return;
    }
    std::swap(m_costA, *m_costB);
}

void MerchantOffer::updateDemand()
{
    // demand = demand + uses - (maxUses - uses)
    m_demand = m_demand + m_uses - (m_maxUses - m_uses);
}

void MerchantOffer::serialize(nbt::tags::compound_tag& tag) const
{
    // 第一支付代价：物品（含 potion_contents / custom_data）落在 buyA 节点下，
    // 数量单独存 buyACount（原版 buy 节点的 count 字段在这里由物品堆的 Count 承载）
    nbt::tags::compound_tag buyATag;
    m_costA.templateStack().toNbt(buyATag);
    tag.value["buyA"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyATag));

    // 第二支付代价（可选）
    if (m_costB.has_value()) {
        nbt::tags::compound_tag buyBTag;
        m_costB->templateStack().toNbt(buyBTag);
        tag.value["buyB"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyBTag));
    }

    // 卖出物品
    nbt::tags::compound_tag sellTag;
    m_sell.toNbt(sellTag);
    tag.value["sell"] = std::make_unique<nbt::tags::compound_tag>(std::move(sellTag));

    // 基础数值字段
    tag.put("uses", static_cast<std::int32_t>(m_uses));
    tag.put("maxUses", static_cast<std::int32_t>(m_maxUses));
    tag.put("xp", static_cast<std::int32_t>(m_xp));
    tag.put("priceMultiplier", static_cast<f32>(m_priceMultiplier));
    tag.put("specialPrice", static_cast<std::int32_t>(m_specialPrice));
    tag.put("demand", static_cast<std::int32_t>(m_demand));
    tag.put("restocksToday", static_cast<std::int32_t>(m_restocksToday));
    tag.put("lastRestock", static_cast<std::int64_t>(m_lastRestockTime));
}

MerchantOffer MerchantOffer::deserialize(const nbt::tags::compound_tag& tag)
{
    MerchantOffer offer;

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

    // 读取支付代价：物品堆的 Count 即代价数量，逐组件谓词随物品堆数据一起还原
    auto readCost = [&tag](const char* key) -> std::optional<ItemCost> {
        auto it = tag.value.find(key);
        if (it == tag.value.end() || it->second->id() != nbt::TagId::Compound) {
            return std::nullopt;
        }
        auto& stackTag = dynamic_cast<const nbt::tags::compound_tag&>(*it->second);
        auto stackResult = ItemStack::fromNbt(stackTag);
        if (!stackResult.success()) {
            return std::nullopt;
        }
        ItemStack stack = stackResult.value();
        if (stack.isEmpty()) {
            return std::nullopt;
        }
        ItemCost cost(std::move(stack));
        // potion_contents 谓词随 potionId 一起还原，避免"任意水瓶"匹配到"治疗药水"
        const std::string& potionId = cost.templateStack().getPotionId();
        if (!potionId.empty()) {
            cost.withPotion(potionId);
        }
        return cost;
    };

    if (auto costA = readCost("buyA"); costA.has_value()) {
        offer.m_costA = std::move(*costA);
    }

    if (auto costB = readCost("buyB"); costB.has_value()) {
        offer.m_costB = std::move(*costB);
    }

    // 卖出物品
    auto sellIt = tag.value.find("sell");
    if (sellIt != tag.value.end() && sellIt->second->id() == nbt::TagId::Compound) {
        auto& sellTag = dynamic_cast<const nbt::tags::compound_tag&>(*sellIt->second);
        auto sellResult = ItemStack::fromNbt(sellTag);
        if (sellResult.success()) {
            offer.m_sell = sellResult.value();
        }
    }

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
