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

#include "EnchantmentContainer.hpp"
#include "EnchantmentRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// EnchantmentInstance 实现
// ============================================================================

const Enchantment* EnchantmentInstance::getEnchantment() const
{
    return EnchantmentRegistry::get(enchantmentId);
}

// ============================================================================
// EnchantmentContainer 实现
// ============================================================================

i32 EnchantmentContainer::getLevel(std::string_view enchantmentId) const
{
    for (const auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            return instance.level;
        }
    }
    return 0;
}

bool EnchantmentContainer::has(std::string_view enchantmentId) const
{
    for (const auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            return true;
        }
    }
    return false;
}

bool EnchantmentContainer::hasType(EnchantmentType type) const
{
    for (const auto& instance : m_enchantments) {
        const Enchantment* enchantment = instance.getEnchantment();
        if (enchantment && enchantment->type() == type) {
            return true;
        }
    }
    return false;
}

void EnchantmentContainer::set(std::string_view enchantmentId, i32 level)
{
    // 原版 ItemEnchantments.Mutable.set：level <= 0 表示删除，而不是写入 0 级。
    // 若不删除，"移除附魔"会留下一条 level=0 的条目，导致 has() 仍返回 true。
    if (level <= 0) {
        remove(enchantmentId);
        return;
    }

    // 原版用 Math.min(level, 255) 截断。附魔等级会写进 NBT 的 byte 字段，
    // 超过 255 而不截断会在存盘时被截成负数。
    const i32 clamped = std::min(level, MAX_ENCHANTMENT_LEVEL);

    for (auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            instance.level = clamped;
            return;
        }
    }

    m_enchantments.emplace_back(std::string(enchantmentId), clamped);
}

void EnchantmentContainer::upgrade(std::string_view enchantmentId, i32 level)
{
    // 原版 Mutable.upgrade：level <= 0 时直接返回，既不新建也不清除。
    if (level <= 0) {
        return;
    }
    // 先截断再比较：否则传入 300 时会存成 255，但若已有 255 也不该再动。
    const i32 clamped = std::min(level, MAX_ENCHANTMENT_LEVEL);
    const i32 current = getLevel(enchantmentId);
    if (clamped > current) {
        set(enchantmentId, clamped);
    }
}

bool EnchantmentContainer::remove(std::string_view enchantmentId)
{
    for (auto it = m_enchantments.begin(); it != m_enchantments.end(); ++it) {
        if (it->enchantmentId == enchantmentId) {
            m_enchantments.erase(it);
            return true;
        }
    }
    return false;
}

bool EnchantmentContainer::canAdd(std::string_view enchantmentId) const
{
    const Enchantment* newEnchantment = EnchantmentRegistry::get(std::string(enchantmentId));
    if (!newEnchantment) {
        return false;
    }

    // 检查与现有附魔的兼容性
    for (const auto& instance : m_enchantments) {
        const Enchantment* existing = instance.getEnchantment();
        if (existing && !newEnchantment->isCompatibleWith(*existing)) {
            return false;
        }
    }

    return true;
}

nlohmann::json EnchantmentContainer::toJson() const
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto& instance : m_enchantments) {
        nlohmann::json enchJson;
        enchJson["id"] = instance.enchantmentId;
        enchJson["lvl"] = instance.level;
        json.push_back(enchJson);
    }
    return json;
}

Result<EnchantmentContainer> EnchantmentContainer::fromJson(const nlohmann::json& json)
{
    EnchantmentContainer container;

    if (!json.is_array()) {
        return container;
    }

    for (const auto& enchJson : json) {
        if (!enchJson.is_object()) {
            continue;
        }
        if (!enchJson.contains("id") || !enchJson["id"].is_string()) {
            continue;
        }

        std::string id = enchJson["id"].get<std::string>();
        i32 level = 1;
        if (enchJson.contains("lvl") && enchJson["lvl"].is_number()) {
            level = enchJson["lvl"].get<i32>();
        }

        container.m_enchantments.emplace_back(id, level);
    }

    return container;
}

std::unique_ptr<nbt::tags::list_tag> EnchantmentContainer::toNbt() const
{
    auto list = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& instance : m_enchantments) {
        nbt::tags::compound_tag enchTag;
        enchTag.put("id", instance.enchantmentId);
        enchTag.put("lvl", static_cast<i16>(instance.level));
        list->value.push_back(std::move(enchTag));
    }
    return list;
}

EnchantmentContainer EnchantmentContainer::fromNbt(const nbt::tags::list_tag& list)
{
    EnchantmentContainer container;

    // 检查列表类型
    if (list.element_id() != nbt::TagId::Compound) {
        return container;
    }

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(list);
    for (const auto& enchTag : compoundList.value) {
        // 获取附魔ID
        auto it = enchTag.value.find("id");
        if (it == enchTag.value.end() || it->second->id() != nbt::TagId::String) {
            continue;
        }
        std::string id = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;

        // 获取附魔等级
        i32 level = 1;
        it = enchTag.value.find("lvl");
        if (it != enchTag.value.end()) {
            if (it->second->id() == nbt::TagId::Short) {
                level = dynamic_cast<const nbt::tags::short_tag&>(*it->second).value;
            } else if (it->second->id() == nbt::TagId::Int) {
                level = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
            } else if (it->second->id() == nbt::TagId::Byte) {
                level = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
            }
        }

        container.m_enchantments.emplace_back(id, level);
    }

    return container;
}

} // namespace enchant
} // namespace item
} // namespace mc
