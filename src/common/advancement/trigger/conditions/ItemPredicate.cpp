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

#include "ItemPredicate.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

ItemPredicate::ItemPredicate(std::optional<ResourceLocation> item,
    std::optional<i32> count,
    IntBounds durability,
    std::optional<ResourceLocation> potion,
    const nbt::tags::compound_tag* nbt)
    : m_item(std::move(item))
    , m_count(count)
    , m_durability(std::move(durability))
    , m_potion(std::move(potion))
    , m_isAny(!m_item.has_value() && !m_count.has_value() && m_durability.isUnbounded() && !m_potion.has_value())
{
    MC_UNUSED(nbt);
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

    // 检查数量
    if (m_count.has_value() && stack.getCount() != m_count.value()) {
        return false;
    }

    // 检查耐久
    if (!m_durability.isUnbounded()) {
        i32 durability = stack.getMaxDamage() - stack.getDamage();
        if (!m_durability.test(durability)) {
            return false;
        }
    }

    // [TODO 阶段3+4：触发器完善] 检查药水、NBT、附魔等

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
    std::optional<i32> count;
    IntBounds durability;
    std::optional<ResourceLocation> potion;

    if (json.contains("item")) {
        item = ResourceLocation(json["item"].get<std::string>());
    }

    if (json.contains("count")) {
        if (json["count"].is_number()) {
            count = json["count"].get<i32>();
        } else {
            durability = IntBounds::fromJson(json["count"]);
        }
    }

    if (json.contains("durability")) {
        durability = IntBounds::fromJson(json["durability"]);
    }

    if (json.contains("potion")) {
        potion = ResourceLocation(json["potion"].get<std::string>());
    }

    // [TODO 阶段3+4：触发器完善] 解析 nbt, enchantments, stored_enchantments 等

    return ItemPredicate(std::move(item), count, std::move(durability), std::move(potion), nullptr);
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
    if (m_count.has_value()) {
        json["count"] = m_count.value();
    }
    if (!m_durability.isUnbounded()) {
        json["durability"] = m_durability.toJson();
    }
    if (m_potion.has_value()) {
        json["potion"] = m_potion.value().toString();
    }
    return json;
}

} // namespace mc::advancement
