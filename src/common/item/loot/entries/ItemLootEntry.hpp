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
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <functional>
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 物品掉落条目
 *
 * 生成特定物品。
 * 参考: net.minecraft.loot.ItemLootEntry
 */
class ItemLootEntry : public LootEntry {
public:
    /**
     * @brief 构造物品条目
     * @param itemId 物品ID
     * @param count 数量范围
     * @param weight 权重
     * @param quality 质量
     */
    ItemLootEntry(const std::string& itemId, const RandomValueRange& count, i32 weight, i32 quality);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Item; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    [[nodiscard]] const std::string& getItemId() const { return m_itemId; }
    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }

    void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const override;

    /**
     * @brief 设置数量范围
     */
    void setCount(const RandomValueRange& count) { m_count = count; }

private:
    std::string m_itemId;
    RandomValueRange m_count;
};

} // namespace loot
} // namespace mc
