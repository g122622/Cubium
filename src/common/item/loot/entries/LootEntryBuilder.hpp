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

#include "LootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/conditions/LootConditions.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/item/loot/functions/LootFunctions.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落条目构建器
 *
 * 用于流式构建各种类型的掉落条目（物品、空条目、掉落表引用、标签、动态条目）。
 */
class LootEntryBuilder {
public:
    LootEntryBuilder() = default;

    // 移动构造和赋值（unique_ptr 成员需要）
    LootEntryBuilder(LootEntryBuilder&&) noexcept = default;
    LootEntryBuilder& operator=(LootEntryBuilder&&) noexcept = default;

    // 禁止拷贝（unique_ptr 成员不可拷贝）
    LootEntryBuilder(const LootEntryBuilder&) = delete;
    LootEntryBuilder& operator=(const LootEntryBuilder&) = delete;

    /**
     * @brief 设置权重
     */
    LootEntryBuilder& weight(i32 w)
    {
        m_weight = w;
        return *this;
    }

    /**
     * @brief 设置质量
     */
    LootEntryBuilder& quality(i32 q)
    {
        m_quality = q;
        return *this;
    }

    /**
     * @brief 构建物品条目
     */
    static LootEntryBuilder item(const std::string& itemId);

    /**
     * @brief 构建空条目
     */
    static LootEntryBuilder empty();

    /**
     * @brief 构建掉落表引用
     */
    static LootEntryBuilder table(const std::string& tableId);

    /**
     * @brief 构建标签条目
     */
    static LootEntryBuilder tag(const std::string& tagId, bool expand = false);

    /**
     * @brief 构建动态条目
     */
    static LootEntryBuilder dynamic_(const std::string& name);

    /**
     * @brief 设置数量
     */
    LootEntryBuilder& count(f32 min, f32 max);

    /**
     * @brief 设置固定数量
     */
    LootEntryBuilder& count(i32 value);

    /**
     * @brief 添加条件
     */
    LootEntryBuilder& condition(std::unique_ptr<LootCondition> cond)
    {
        m_conditions.push_back(std::move(cond));
        return *this;
    }

    /**
     * @brief 添加函数
     */
    LootEntryBuilder& function(std::unique_ptr<LootFunction> func)
    {
        m_functions.push_back(std::move(func));
        return *this;
    }

    /**
     * @brief 构建条目
     */
    [[nodiscard]] std::unique_ptr<LootEntry> build() const;

private:
    std::string m_itemId;
    std::string m_tableId;
    std::string m_tagId;
    std::string m_dynamicName;
    bool m_expand = false;
    LootEntryType m_type = LootEntryType::Empty;
    RandomValueRange m_count{1.0f, 1.0f};
    i32 m_weight = 1;
    i32 m_quality = 0;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
};

} // namespace loot
} // namespace mc
