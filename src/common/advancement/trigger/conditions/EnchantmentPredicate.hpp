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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
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
     * @brief 默认构造（匹配任意附魔）
     */
    EnchantmentPredicate() = default;

    /**
     * @brief 构造附魔谓词
     * @param enchantment 附魔ID（可选，为空则匹配任意附魔类型）
     * @param levels 附魔等级范围
     */
    EnchantmentPredicate(std::optional<ResourceLocation> enchantment, IntBounds levels);

    /**
     * @brief 检查附魔容器中是否存在匹配的附魔
     * @param enchantments 附魔容器
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const item::enchant::EnchantmentContainer& enchantments) const;

    /**
     * @brief 检查是否匹配任意附魔
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     */
    static Result<EnchantmentPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getEnchantment() const noexcept { return m_enchantment; }
    [[nodiscard]] const IntBounds& getLevels() const noexcept { return m_levels; }

private:
    std::optional<ResourceLocation> m_enchantment; ///< 附魔ID
    IntBounds m_levels;                            ///< 附魔等级范围
};

} // namespace mc::advancement
