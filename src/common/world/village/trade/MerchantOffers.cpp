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

#include "../../../util/nbt/Nbt.hpp"
#include "Merchant.hpp"
#include "MerchantOffer.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace village {
namespace trade {

// ========== MerchantOffers ==========

void MerchantOffers::addOffer(std::unique_ptr<MerchantOffer> offer)
{
    if (offer) {
        m_offers.push_back(std::move(offer));
    }
}

void MerchantOffers::removeOffer(size_t index)
{
    if (index < m_offers.size()) {
        m_offers.erase(m_offers.begin() + static_cast<ptrdiff_t>(index));
    }
}

MerchantOffer* MerchantOffers::getOffer(size_t index)
{
    if (index < m_offers.size()) {
        return m_offers[index].get();
    }
    return nullptr;
}

const MerchantOffer* MerchantOffers::getOffer(size_t index) const
{
    if (index < m_offers.size()) {
        return m_offers[index].get();
    }
    return nullptr;
}

void MerchantOffers::restockAll()
{
    for (auto& offer : m_offers) {
        if (offer) {
            offer->restock();
        }
    }
}

void MerchantOffers::updatePrices(f32 modifier)
{
    for (auto& offer : m_offers) {
        if (offer) {
            // 价格修正影响特殊价格
            i32 basePrice = offer->getBuyA().getCount();
            i32 adjusted = static_cast<i32>(basePrice * modifier);
            i32 specialPrice = adjusted - basePrice;
            offer->setSpecialPrice(specialPrice);
        }
    }
}

void MerchantOffers::serialize(nbt::tags::compound_tag& tag) const
{
    auto offersList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& offer : m_offers) {
        if (offer) {
            nbt::tags::compound_tag offerTag;
            offer->serialize(offerTag);
            offersList->value.push_back(std::move(offerTag));
        }
    }
    tag.value["Offers"] = std::move(offersList);
}

MerchantOffers MerchantOffers::deserialize(const nbt::tags::compound_tag& tag)
{
    MerchantOffers offers;

    auto offersIt = tag.value.find("Offers");
    if (offersIt != tag.value.end()) {
        auto* offersList = dynamic_cast<const nbt::tags::compound_list_tag*>(offersIt->second.get());
        if (offersList) {
            for (const auto& offerTag : offersList->value) {
                auto offer = std::make_unique<MerchantOffer>(MerchantOffer::deserialize(offerTag));
                offers.m_offers.push_back(std::move(offer));
            }
        }
    }

    return offers;
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
