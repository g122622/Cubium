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

#include "common/item/core/ItemRarity.hpp"

namespace mc {

namespace {

struct RarityEntry {
    ItemRarity rarity;
    const char* name;
};

constexpr RarityEntry kRarityEntries[] = {
    {ItemRarity::Common, "common"},
    {ItemRarity::Uncommon, "uncommon"},
    {ItemRarity::Rare, "rare"},
    {ItemRarity::Epic, "epic"},
};

} // namespace

std::optional<std::string_view> rarityName(ItemRarity rarity) noexcept
{
    for (const auto& entry : kRarityEntries) {
        if (entry.rarity == rarity) {
            return std::string_view(entry.name);
        }
    }
    return std::nullopt;
}

std::optional<ItemRarity> rarityFromName(std::string_view name) noexcept
{
    for (const auto& entry : kRarityEntries) {
        if (entry.name == name) {
            return entry.rarity;
        }
    }
    return std::nullopt;
}

ItemRarity rarityFromId(i32 id) noexcept
{
    for (const auto& entry : kRarityEntries) {
        if (rarityId(entry.rarity) == id) {
            return entry.rarity;
        }
    }
    // 对齐 vanilla ByIdMap.OutOfBoundsStrategy.ZERO：越界回落到 Common
    return ItemRarity::Common;
}

} // namespace mc
