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

#include "EnchantmentPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

namespace {

/// 资源位置是否以 `#` 开头（原版 HolderSet 的标签写法）。
[[nodiscard]] bool isTagName(std::string_view name)
{
    return !name.empty() && name.front() == '#';
}

} // namespace

EnchantmentPredicate::EnchantmentPredicate(std::optional<ResourceLocation> enchantment, IntBounds levels)
    : m_enchantment(std::move(enchantment))
    , m_levels(std::move(levels))
{}

std::optional<i32> EnchantmentPredicate::_findLevel(const item::enchant::EnchantmentContainer& enchantments,
    const std::function<std::vector<std::string>(const ResourceLocation&)>& tagResolver) const
{
    if (m_enchantmentTag.has_value()) {
        const std::vector<std::string> members = tagResolver(m_enchantmentTag.value());
        for (const auto& [id, level] : enchantments.getAll()) {
            if (std::find(members.begin(), members.end(), id) != members.end()) {
                return level;
            }
        }
        return std::nullopt;
    }
    if (m_enchantment.has_value()) {
        const i32 level = enchantments.getLevel(m_enchantment.value().toString());
        return level > 0 ? std::optional<i32>(level) : std::nullopt;
    }
    return std::nullopt;
}

bool EnchantmentPredicate::test(const item::enchant::EnchantmentContainer& enchantments) const
{
    // `#tag` 写法在走不到标签体系的老调用方里一律视为「不匹配」，绝不退化成匹配任意附魔：
    // 放宽会把不满足条件的掉落放出来，比漏掉更糟。
    return test(enchantments, [](const ResourceLocation&) {
        return std::vector<std::string>{};
    });
}

bool EnchantmentPredicate::test(const item::enchant::EnchantmentContainer& enchantments,
    const std::function<std::vector<std::string>(const ResourceLocation&)>& tagResolver) const
{
    if (isAny()) {
        return true;
    }

    // 指定了具体附魔或附魔标签：容器里取其等级，再判断等级范围。
    if (m_enchantment.has_value() || m_enchantmentTag.has_value()) {
        const std::optional<i32> level = _findLevel(enchantments, tagResolver);
        return level.has_value() && m_levels.test(level.value());
    }

    // 没有指定附魔集合：容器里任意一个满足等级范围的附魔即可。
    if (m_levels.isUnbounded()) {
        return enchantments.getAll().size() > 0;
    }

    for (const auto& [id, level] : enchantments.getAll()) {
        MC_UNUSED(id);
        if (m_levels.test(level)) {
            return true;
        }
    }
    return false;
}

bool EnchantmentPredicate::isAny() const noexcept
{
    return !m_enchantment.has_value() && !m_enchantmentTag.has_value() && m_levels.isUnbounded();
}

Result<EnchantmentPredicate> EnchantmentPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EnchantmentPredicate{};
    }

    std::optional<ResourceLocation> enchantment;
    std::optional<ResourceLocation> enchantmentTag;
    IntBounds levels;

    // 解析 HolderSet<Enchantment> 的一个元素：`#tag` 是标签，其余是具体附魔。
    const auto assign = [&](const std::string& name) {
        if (isTagName(name)) {
            enchantmentTag = ResourceLocation(name.substr(1));
        } else {
            enchantment = ResourceLocation(name);
        }
    };

    if (json.is_object()) {
        // 原版字段名是 `enchantments`（HolderSet<Enchantment>），值可以是单个字符串
        // 也可以是一个数组。数据包里 `match_tool` 的附魔谓词全部用这个名字；
        // 早期只认 `enchantment`，导致所有精准采集/时运判定都落空。
        // `minecraft:enchantments` 是 DataComponentMatchers 的组件键名（见
        // ItemPredicate::fromJson 的 `predicates` 分支），这里同样接受，两条路径共用同一解析。
        const nlohmann::json* names = nullptr;
        if (json.contains("enchantments")) {
            names = &json["enchantments"];
        } else if (json.contains("minecraft:enchantments")) {
            names = &json["minecraft:enchantments"];
        } else if (json.contains("enchantment")) {
            names = &json["enchantment"];
        }
        if (names != nullptr) {
            if (names->is_string()) {
                assign(names->get<std::string>());
            } else if (names->is_array() && !names->empty() && (*names)[0].is_string()) {
                // 多元素写法：原版 HolderSet 的语义是「包含其中任意一个」，且元素本身可能是标签。
                // TODO(item-predicate-multi-enchantment): 只建模了一个集合元素，多元素写法当前
                //   只取第一个；数据包目前未使用多元素写法。
                assign((*names)[0].get<std::string>());
            }
        }
        if (json.contains("levels")) {
            levels = IntBounds::fromJson(json["levels"]);
        }
    }

    EnchantmentPredicate predicate(std::move(enchantment), std::move(levels));
    predicate.m_enchantmentTag = std::move(enchantmentTag);
    return predicate;
}

nlohmann::json EnchantmentPredicate::toJson() const
{
    if (isAny()) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_enchantment.has_value()) {
        json["enchantment"] = m_enchantment.value().toString();
    }
    if (m_enchantmentTag.has_value()) {
        json["enchantment"] = "#" + m_enchantmentTag.value().toString();
    }
    if (!m_levels.isUnbounded()) {
        json["levels"] = m_levels.toJson();
    }
    return json;
}

} // namespace mc::advancement
