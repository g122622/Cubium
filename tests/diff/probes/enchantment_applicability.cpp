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
 * @file enchantment_applicability.cpp
 * @brief 差分能力：附魔能附到哪些物品上。
 *
 * `list`：全部附魔 ID（排序）
 * `run` ：{ "enchantment": "minecraft:sharpness" }
 *         → { "enchantment": "...", "supportedItems": ["minecraft:iron_sword", ...] }
 *
 * 对应原版 `Enchantment#isSupportedItem`，它直接读数据包的 `supported_items`
 * 物品标签引用（如 `#minecraft:enchantable/sharp_weapon`）。
 *
 * Cubium 的附魔定义是硬编码的，没有 `supported_items` 字段，改用
 * `EnchantmentType` 枚举**归纳**原版标签，再由
 * `Enchantment::canApplyToItem` 按物品类型判定。本能力就是校验这层归纳是否与
 * 原版标签逐项吻合——「铁镐能不能附力量」这类问题，靠读代码很难发现，
 * 但枚举与标签的差异一定会在某个物品上暴露。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
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

void ensureEnchantments()
{
    ensureItems();
    if (!item::enchant::EnchantmentRegistry::isInitialized()) {
        item::enchant::EnchantmentRegistry::initialize();
    }
}

/** 全部物品 ID（排序后缓存一次）。 */
const std::vector<std::string>& allItemIds()
{
    static const auto ids = [] {
        std::vector<std::string> names;
        ItemRegistry::instance().forEachItem([&](Item& item) {
            names.push_back(item.itemLocation().toString());
        });
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        return names;
    }();
    return ids;
}

void writeList(FILE* out)
{
    ensureEnchantments();
    std::vector<std::string> names;
    for (const auto& [id, enchantment] : item::enchant::EnchantmentRegistry::all()) {
        names.push_back(id);
    }
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEnchantments();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("enchantment").get<std::string>();

    const item::enchant::Enchantment* enchantment = item::enchant::EnchantmentRegistry::get(requested);
    if (enchantment == nullptr) {
        return json{{"enchantment", requested}, {"missing", true}}.dump();
    }

    // 逐个物品问「这个附魔能不能附上去」，等价于原版 `isSupportedItem(stack)`。
    // 比的是最终集合，而不是标签名——正是因为两侧的中间表示完全不同。
    std::vector<std::string> supported;
    for (const auto& itemId : allItemIds()) {
        const Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
        if (item == nullptr) {
            continue;
        }
        const ItemStack stack(*item, 1);
        if (enchantment->canApplyToItem(stack)) {
            supported.push_back(itemId);
        }
    }
    // 集合语义：排序后输出。
    std::sort(supported.begin(), supported.end());

    return json{{"enchantment", requested}, {"supportedItems", std::move(supported)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_applicability", &writeList, &runLine)

} // namespace mc::diff
