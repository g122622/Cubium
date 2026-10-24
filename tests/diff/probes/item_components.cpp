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

/**
 * @file item_components.cpp
 * @brief 差分能力：物品的耐久、稀有度与附魔能力。
 *
 * `list`：全部物品 ID
 * `run` ：{ "item": "minecraft:diamond_sword" } → maxDamage / rarity / enchantable
 *
 * 这三项是服务端判定的输入（损坏、名字颜色与战利品品质、附魔台等级），
 * 都是纯数据，手抄出错的概率远高于逻辑写错。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        s_initialized = true;
    }
}

/** 稀有度名：与原版 Rarity 的枚举名（小写）一致。 */
const char* rarityText(ItemRarity rarity)
{
    switch (rarity) {
        case ItemRarity::Common:
            return "common";
        case ItemRarity::Uncommon:
            return "uncommon";
        case ItemRarity::Rare:
            return "rare";
        case ItemRarity::Epic:
            return "epic";
    }
    return "unknown";
}

void writeList(FILE* out)
{
    ensureItems();
    std::vector<std::string> names;
    ItemRegistry::instance().forEachItem([&](Item& item) {
        names.push_back(item.itemLocation().toString());
    });
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("item").get<std::string>();

    Item* item = ItemRegistry::instance().getItem(ResourceLocation(requested));
    if (item == nullptr) {
        return json{{"item", requested}, {"missing", true}}.dump();
    }

    return json{{"item", requested},
        {"maxDamage", item->maxDamage()},
        {"rarity", rarityText(item->rarity())},
        {"enchantable", item->enchantable()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_components", &writeList, &runLine)

} // namespace mc::diff