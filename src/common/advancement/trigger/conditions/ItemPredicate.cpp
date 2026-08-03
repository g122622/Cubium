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
 * IMPLIED, INCLUDING ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "ItemPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/conditions/EnchantmentPredicate.hpp"
#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

ItemPredicate::ItemPredicate(std::optional<ResourceLocation> item,
    IntBounds count,
    IntBounds durability,
    std::optional<ResourceLocation> potion,
    std::vector<EnchantmentPredicate> enchantments,
    std::vector<EnchantmentPredicate> storedEnchantments,
    NBTPredicate nbt)
    : m_item(std::move(item))
    , m_count(std::move(count))
    , m_durability(std::move(durability))
    , m_potion(std::move(potion))
    , m_enchantments(std::move(enchantments))
    , m_storedEnchantments(std::move(storedEnchantments))
    , m_nbt(std::move(nbt))
{
    _updateIsAny();
}

ItemPredicate::ItemPredicate(std::optional<ResourceLocation> item,
    std::optional<ResourceLocation> tag,
    IntBounds count,
    IntBounds durability,
    std::optional<ResourceLocation> potion,
    std::vector<EnchantmentPredicate> enchantments,
    std::vector<EnchantmentPredicate> storedEnchantments,
    NBTPredicate nbt)
    : m_item(std::move(item))
    , m_tag(std::move(tag))
    , m_count(std::move(count))
    , m_durability(std::move(durability))
    , m_potion(std::move(potion))
    , m_enchantments(std::move(enchantments))
    , m_storedEnchantments(std::move(storedEnchantments))
    , m_nbt(std::move(nbt))
{
    _updateIsAny();
}

bool ItemPredicate::test(const ItemStack& stack) const
{
    if (m_isAny) {
        return true;
    }

    if (stack.isEmpty()) {
        return false;
    }

    // 检查物品ID
    if (m_item.has_value()) {
        const Item* item = stack.getItem();
        if (item == nullptr) {
            return false;
        }
        if (item->itemLocation() != m_item.value()) {
            return false;
        }
    }

    // 检查物品标签
    if (m_tag.has_value()) {
        item::tag::ItemTag* tag = item::tag::ItemTags::getTag(m_tag.value());
        if (tag == nullptr) {
            // 未知标签不匹配任何物品
            return false;
        }
        if (!tag->contains(stack)) {
            return false;
        }
    }

    // 检查数量
    if (!m_count.isUnbounded() && !m_count.test(stack.getCount())) {
        return false;
    }

    // 检查耐久
    if (!m_durability.isUnbounded()) {
        i32 durability = stack.getMaxDamage() - stack.getDamage();
        if (!m_durability.test(durability)) {
            return false;
        }
    }

    // 检查药水类型
    if (m_potion.has_value()) {
        const potion::Potion* actualPotion = potion::PotionUtils::getPotion(stack);
        if (actualPotion == nullptr) {
            return false;
        }
        if (actualPotion->id() != m_potion.value()) {
            return false;
        }
    }

    // 检查附魔
    if (!_testEnchantments(m_enchantments, stack.getEnchantments())) {
        return false;
    }

    // 检查存储附魔（附魔书的 StoredEnchantments）
    if (!m_storedEnchantments.empty()) {
        // 构建附魔书的存储附魔容器
        item::enchant::EnchantmentContainer storedContainer;
        auto storedData = item::items::EnchantedBookItem::getEnchantments(stack);
        for (const auto& data : storedData) {
            if (data.enchantment != nullptr) {
                storedContainer.set(data.enchantment->id(), data.level);
            }
        }
        if (!_testEnchantments(m_storedEnchantments, storedContainer)) {
            return false;
        }
    }

    // 检查NBT
    if (!m_nbt.test(stack)) {
        return false;
    }

    return true;
}

bool ItemPredicate::isAny() const noexcept
{
    return m_isAny;
}

Result<ItemPredicate> ItemPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return ItemPredicate{};
    }

    std::optional<ResourceLocation> item;
    std::optional<ResourceLocation> tag;
    IntBounds count;
    IntBounds durability;
    std::optional<ResourceLocation> potion;
    std::vector<EnchantmentPredicate> enchantments;
    std::vector<EnchantmentPredicate> storedEnchantments;
    NBTPredicate nbt;

    // 支持简写格式：直接传字符串表示物品ID
    if (json.is_string()) {
        item = ResourceLocation(json.get<std::string>());
        return ItemPredicate(std::move(item),
            std::move(tag),
            std::move(count),
            std::move(durability),
            std::move(potion),
            std::move(enchantments),
            std::move(storedEnchantments),
            std::move(nbt));
    }

    if (json.contains("item")) {
        item = ResourceLocation(json["item"].get<std::string>());
    }

    if (json.contains("tag")) {
        tag = ResourceLocation(json["tag"].get<std::string>());
    }

    if (json.contains("count")) {
        count = IntBounds::fromJson(json["count"]);
    }

    if (json.contains("durability")) {
        durability = IntBounds::fromJson(json["durability"]);
    }

    if (json.contains("potion")) {
        potion = ResourceLocation(json["potion"].get<std::string>());
    }

    // 解析附魔谓词列表
    if (json.contains("enchantments") && json["enchantments"].is_array()) {
        for (const auto& enchJson : json["enchantments"]) {
            auto result = EnchantmentPredicate::fromJson(enchJson);
            if (result.success()) {
                enchantments.push_back(std::move(result.value()));
            }
        }
    }

    // 解析存储附魔谓词列表（附魔书）
    if (json.contains("stored_enchantments") && json["stored_enchantments"].is_array()) {
        for (const auto& enchJson : json["stored_enchantments"]) {
            auto result = EnchantmentPredicate::fromJson(enchJson);
            if (result.success()) {
                storedEnchantments.push_back(std::move(result.value()));
            }
        }
    }

    // 解析NBT谓词
    if (json.contains("nbt")) {
        auto result = NBTPredicate::fromJson(json["nbt"]);
        if (result.success()) {
            nbt = std::move(result.value());
        }
    }

    return ItemPredicate(std::move(item),
        std::move(tag),
        std::move(count),
        std::move(durability),
        std::move(potion),
        std::move(enchantments),
        std::move(storedEnchantments),
        std::move(nbt));
}

nlohmann::json ItemPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_item.has_value()) {
        json["item"] = m_item.value().toString();
    }
    if (m_tag.has_value()) {
        json["tag"] = m_tag.value().toString();
    }
    if (!m_count.isUnbounded()) {
        json["count"] = m_count.toJson();
    }
    if (!m_durability.isUnbounded()) {
        json["durability"] = m_durability.toJson();
    }
    if (m_potion.has_value()) {
        json["potion"] = m_potion.value().toString();
    }
    if (!m_enchantments.empty()) {
        nlohmann::json enchArray = nlohmann::json::array();
        for (const auto& ench : m_enchantments) {
            enchArray.push_back(ench.toJson());
        }
        json["enchantments"] = enchArray;
    }
    if (!m_storedEnchantments.empty()) {
        nlohmann::json storedArray = nlohmann::json::array();
        for (const auto& ench : m_storedEnchantments) {
            storedArray.push_back(ench.toJson());
        }
        json["stored_enchantments"] = storedArray;
    }
    if (!m_nbt.isAny()) {
        json["nbt"] = m_nbt.toJson();
    }
    return json;
}

void ItemPredicate::_updateIsAny()
{
    m_isAny = !m_item.has_value() && !m_tag.has_value() && m_count.isUnbounded() && m_durability.isUnbounded() &&
        !m_potion.has_value() && m_enchantments.empty() && m_storedEnchantments.empty() && m_nbt.isAny();
}

bool ItemPredicate::_testEnchantments(
    const std::vector<EnchantmentPredicate>& predicates, const item::enchant::EnchantmentContainer& enchantments)
{
    // 所有谓词都必须匹配（AND 语义）
    // 每个谓词检查附魔容器中是否存在满足条件的附魔
    for (const auto& pred : predicates) {
        if (!pred.test(enchantments)) {
            return false;
        }
    }
    return true;
}

} // namespace mc::advancement
