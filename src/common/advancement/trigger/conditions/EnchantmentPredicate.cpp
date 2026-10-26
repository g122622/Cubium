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

#include "EnchantmentPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

EnchantmentPredicate::EnchantmentPredicate(std::optional<ResourceLocation> enchantment, IntBounds levels)
    : m_enchantment(std::move(enchantment))
    , m_levels(std::move(levels))
{}

bool EnchantmentPredicate::test(const item::enchant::EnchantmentContainer& enchantments) const
{
    if (isAny()) {
        return true;
    }

    // 如果指定了附魔ID，检查容器中是否存在该附魔且等级在范围内
    if (m_enchantment.has_value()) {
        i32 level = enchantments.getLevel(m_enchantment.value().toString());
        if (level <= 0) {
            // 没有该附魔
            return false;
        }
        return m_levels.test(level);
    }

    // 如果没有指定附魔ID，检查容器中是否存在任意附魔满足等级范围
    if (m_levels.isUnbounded()) {
        // 任意等级范围 + 任意附魔类型 = 只要有任何附魔就匹配
        return enchantments.getAll().size() > 0;
    }

    // 有等级范围但没有指定附魔类型，检查是否有任意附魔满足等级
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
    return !m_enchantment.has_value() && m_levels.isUnbounded();
}

Result<EnchantmentPredicate> EnchantmentPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EnchantmentPredicate{};
    }

    std::optional<ResourceLocation> enchantment;
    IntBounds levels;

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
                enchantment = ResourceLocation(names->get<std::string>());
            } else if (names->is_array() && !names->empty() && (*names)[0].is_string()) {
                // 多附魔写法：原版是 HolderSet<Enchantment>，语义为「包含其中任意一个」。
                // TODO(item-predicate-multi-enchantment): 本项目的 EnchantmentPredicate 只
                //   建模单个附魔，多元素写法当前只取第一个；数据包目前未使用多元素写法。
                enchantment = ResourceLocation((*names)[0].get<std::string>());
            }
        }
        if (json.contains("levels")) {
            levels = IntBounds::fromJson(json["levels"]);
        }
    }

    return EnchantmentPredicate(std::move(enchantment), std::move(levels));
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
    if (!m_levels.isUnbounded()) {
        json["levels"] = m_levels.toJson();
    }
    return json;
}

} // namespace mc::advancement
