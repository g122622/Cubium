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

#pragma once

#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::item::enchant {
class EnchantmentContainer;
}

namespace mc::advancement {

/**
 * @brief 附魔谓词
 *
 * 用于匹配附魔的条件谓词，检查附魔类型和等级。
 * 对应 MC Java 的 EnchantmentPredicate。
 */
class EnchantmentPredicate {
public:
    /**
     * @brief 附魔集合条目
     *
     * 原版 `enchantments` 字段是 `HolderSet<Enchantment>`，每个元素可以是具体附魔 ID，
     * 也可以是 `#tag` 形式的附魔标签；语义是"容器中包含其中任意一个即可"。
     */
    struct Entry {
        ResourceLocation id; ///< 附魔 ID，或标签名（不含 `#` 前缀）
        bool isTag;          ///< true 表示 id 是标签名
    };

    /**
     * @brief 默认构造（匹配任意附魔）
     */
    EnchantmentPredicate() = default;

    /**
     * @brief 构造附魔谓词
     * @param entries 附魔集合条目（为空则匹配任意附魔类型）
     * @param levels 附魔等级范围
     */
    EnchantmentPredicate(std::vector<Entry> entries, IntBounds levels);

    /**
     * @brief 检查附魔容器中是否存在匹配的附魔
     *
     * 等价于传一个「永远解析不到成员」的标签解析器：`#tag` 形式的谓词在这里
     * 无法判断，一律不匹配。需要完整语义的调用方请用带 tagResolver 的重载。
     *
     * @param enchantments 附魔容器
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const item::enchant::EnchantmentContainer& enchantments) const;

    /**
     * @brief 检查附魔容器中是否存在匹配的附魔（支持附魔标签）
     *
     * 原版的 `enchantments` 字段是 `HolderSet<Enchantment>`，既可以写具体附魔，
     * 也可以写 `#minecraft:smelts_loot` 这样的标签。此前只解析具体附魔，标签写法
     * 被当成一个普通 ID 去查等级，恒不匹配——「烧死的动物掉熟肉」因此永远不触发。
     *
     * @param enchantments 附魔容器（物品上的附魔）
     * @param tagResolver 标签名 → 成员附魔 ID；标签未知时返回空
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const item::enchant::EnchantmentContainer& enchantments,
        const std::function<std::vector<std::string>(const ResourceLocation&)>& tagResolver) const;

    /**
     * @brief 检查是否匹配任意附魔
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     *
     * `#name` 形式的取值解析成附魔标签，其余按附魔 ID 解析。
     */
    static Result<EnchantmentPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========
    // ========== Getters ==========

    /// 附魔集合的全部条目（空表示不限定附魔集合）。
    [[nodiscard]] const std::vector<Entry>& getEntries() const noexcept
    {
        return m_entries;
    }

    [[nodiscard]] const IntBounds& getLevels() const noexcept
    {
        return m_levels;
    }

private:
    /// 在容器的附魔集合中查找与本谓词条目匹配的等级；返回首个命中的等级。
    [[nodiscard]] std::optional<i32> _findLevel(const item::enchant::EnchantmentContainer& enchantments,
        const std::function<std::vector<std::string>(const ResourceLocation&)>& tagResolver) const;

    std::vector<Entry> m_entries; ///< 附魔集合（空表示不限定）
    IntBounds m_levels;           ///< 附魔等级范围
};

} // namespace mc::advancement
