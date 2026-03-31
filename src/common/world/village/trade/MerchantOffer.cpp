#include "MerchantOffer.hpp"
#include "Merchant.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/PlayerInventory.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace village {
namespace trade {

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack sell,
                              i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_buyA(std::move(buyA))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{
}

MerchantOffer::MerchantOffer(ItemStack buyA, ItemStack buyB, ItemStack sell,
                              i32 maxUses, i32 xp, f32 priceMultiplier)
    : m_buyA(std::move(buyA))
    , m_buyB(std::move(buyB))
    , m_sell(std::move(sell))
    , m_maxUses(maxUses)
    , m_xp(xp)
    , m_priceMultiplier(priceMultiplier)
{
}

bool MerchantOffer::canAccept(const ItemStack& offeredA, const ItemStack& offeredB) const {
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

bool MerchantOffer::canAccept(const ItemStack& offered) const {
    return canAccept(offered, ItemStack());
}

bool MerchantOffer::apply(Player& player, IMerchant& merchant) {
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
        return false;  // 物品A不足
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
            return false;  // 物品B不足
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
        return false;  // 背包空间不足
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

void MerchantOffer::restock() {
    m_uses = 0;
    ++m_restocksToday;
}

bool MerchantOffer::isDisabled() const {
    return isOutOfStock() && m_restocksToday >= 2; // 每天最多补货2次
}

f32 MerchantOffer::getProgress() const {
    if (m_maxUses <= 0) return 0.0f;
    return static_cast<f32>(m_uses) / static_cast<f32>(m_maxUses);
}

void MerchantOffer::applyDemand(i32 demandBonus) {
    m_demand += demandBonus;
    // 需求影响价格：需求越高，价格越高
    // m_specialPrice 会根据需求调整
    m_specialPrice = static_cast<i32>(m_demand * m_priceMultiplier);
}

i32 MerchantOffer::getAdjustedBuyPrice() const {
    i32 basePrice = m_buyA.getCount();
    i32 adjusted = basePrice + m_specialPrice;
    return std::max(1, adjusted);
}

void MerchantOffer::serialize(nbt::tags::compound_tag& tag) const {
    // 序列化买入物品A
    // TODO: ItemStack序列化需要使用JSON或NBT格式
    // 暂时使用简化格式
    nbt::tags::compound_tag buyATag;
    // m_buyA.serialize(buyATag); // 需要ItemStack支持NBT序列化
    tag.value["BuyA"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyATag));

    // 序列化买入物品B（如果有）
    if (m_buyB.has_value()) {
        nbt::tags::compound_tag buyBTag;
        // m_buyB->serialize(buyBTag);
        tag.value["BuyB"] = std::make_unique<nbt::tags::compound_tag>(std::move(buyBTag));
    }

    // 序列化卖出物品
    nbt::tags::compound_tag sellTag;
    // m_sell.serialize(sellTag);
    tag.value["Sell"] = std::make_unique<nbt::tags::compound_tag>(std::move(sellTag));

    tag.put("Uses", static_cast<std::int32_t>(m_uses));
    tag.put("MaxUses", static_cast<std::int32_t>(m_maxUses));
    tag.put("Xp", static_cast<std::int32_t>(m_xp));
    tag.put("PriceMultiplier", static_cast<float>(m_priceMultiplier));
    tag.put("SpecialPrice", static_cast<std::int32_t>(m_specialPrice));
    tag.put("Demand", static_cast<std::int32_t>(m_demand));
    tag.put("RestocksToday", static_cast<std::int32_t>(m_restocksToday));
    tag.put("LastRestockTime", static_cast<std::int64_t>(m_lastRestockTime));
}

MerchantOffer MerchantOffer::deserialize(const nbt::tags::compound_tag& tag) {
    MerchantOffer offer;

    // TODO: ItemStack反序列化
    // 暂时创建空ItemStack
    offer.m_buyA = ItemStack();
    offer.m_sell = ItemStack();

    offer.m_uses = tag.get<nbt::tags::int_tag>("Uses");
    offer.m_maxUses = tag.get<nbt::tags::int_tag>("MaxUses");
    offer.m_xp = tag.get<nbt::tags::int_tag>("Xp");
    offer.m_priceMultiplier = tag.get<nbt::tags::float_tag>("PriceMultiplier");
    offer.m_specialPrice = tag.get<nbt::tags::int_tag>("SpecialPrice");
    offer.m_demand = tag.get<nbt::tags::int_tag>("Demand");
    offer.m_restocksToday = tag.get<nbt::tags::int_tag>("RestocksToday");
    offer.m_lastRestockTime = tag.get<nbt::tags::long_tag>("LastRestockTime");

    return offer;
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
