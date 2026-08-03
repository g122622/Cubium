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

#include "EnchantedBookItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../enchantment/EnchantmentRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item::items {

// NBT tag constants
namespace {
constexpr const char* TAG_STORED_ENCHANTMENTS = "StoredEnchantments";
constexpr const char* TAG_ENCHANTMENT_ID = "id";
constexpr const char* TAG_ENCHANTMENT_LEVEL = "lvl";
} // namespace

EnchantedBookItem::EnchantedBookItem(ItemProperties properties)
    : Item(std::move(properties))
{}

std::vector<EnchantedBookItem::EnchantmentData> EnchantedBookItem::getEnchantments(const ItemStack& stack)
{
    std::vector<EnchantmentData> result;

    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr || !tag->contains(TAG_STORED_ENCHANTMENTS)) {
        return result;
    }

    const nlohmann::json& storedEnchantments = (*tag)[TAG_STORED_ENCHANTMENTS];
    if (!storedEnchantments.is_array()) {
        return result;
    }

    for (const auto& enchantTag : storedEnchantments) {
        if (!enchantTag.is_object()) {
            continue;
        }

        std::string id = enchantTag.value(TAG_ENCHANTMENT_ID, "");
        i32 level = enchantTag.value(TAG_ENCHANTMENT_LEVEL, 0);

        if (id.empty() || level <= 0) {
            continue;
        }

        const enchant::Enchantment* enchantment = enchant::EnchantmentRegistry::get(id);
        if (enchantment != nullptr) {
            result.push_back({enchantment, level});
        }
    }

    return result;
}

void EnchantedBookItem::addEnchantment(ItemStack& stack, const enchant::Enchantment& enchantment, i32 level)
{
    nlohmann::json& tag = stack.getOrCreateTag();

    // 获取或创建附魔列表
    if (!tag.contains(TAG_STORED_ENCHANTMENTS) || !tag[TAG_STORED_ENCHANTMENTS].is_array()) {
        tag[TAG_STORED_ENCHANTMENTS] = nlohmann::json::array();
    }

    nlohmann::json& storedEnchantments = tag[TAG_STORED_ENCHANTMENTS];

    // 获取附魔ID
    std::string idStr = enchantment.id();

    // 检查是否已有相同附魔
    for (auto& enchantTag : storedEnchantments) {
        if (!enchantTag.is_object()) {
            continue;
        }

        std::string existingId = enchantTag.value(TAG_ENCHANTMENT_ID, "");
        if (existingId == idStr) {
            // 已有相同附魔，升级到更高等级
            i32 existingLevel = enchantTag.value(TAG_ENCHANTMENT_LEVEL, 0);
            if (level > existingLevel) {
                enchantTag[TAG_ENCHANTMENT_LEVEL] = level;
            }
            return;
        }
    }

    // 添加新附魔
    nlohmann::json newEnchant = nlohmann::json::object();
    newEnchant[TAG_ENCHANTMENT_ID] = idStr;
    newEnchant[TAG_ENCHANTMENT_LEVEL] = level;
    storedEnchantments.push_back(newEnchant);
}

bool EnchantedBookItem::hasEnchantments(const ItemStack& stack)
{
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr || !tag->contains(TAG_STORED_ENCHANTMENTS)) {
        return false;
    }

    const nlohmann::json& storedEnchantments = (*tag)[TAG_STORED_ENCHANTMENTS];
    return storedEnchantments.is_array() && !storedEnchantments.empty();
}

size_t EnchantedBookItem::getEnchantmentCount(const ItemStack& stack)
{
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr || !tag->contains(TAG_STORED_ENCHANTMENTS)) {
        return 0;
    }

    const nlohmann::json& storedEnchantments = (*tag)[TAG_STORED_ENCHANTMENTS];
    if (!storedEnchantments.is_array()) {
        return 0;
    }

    return storedEnchantments.size();
}

} // namespace item::items
} // namespace mc
