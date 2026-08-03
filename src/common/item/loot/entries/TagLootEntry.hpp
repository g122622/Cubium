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
#include <functional>
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 标签掉落条目
 *
 * 从物品标签中选择物品生成。
 * expand=true 时为标签中的每个物品各生成一次；
 * expand=false 时从标签中随机选择一个物品。
 */
class TagLootEntry : public LootEntry {
public:
    /**
     * @brief 构造标签条目
     * @param tagId 标签ID（如 "minecraft:creeper_drop_music_discs"）
     * @param expand 是否展开为独立条目
     * @param weight 权重
     * @param quality 质量
     */
    TagLootEntry(const std::string& tagId, bool expand = false, i32 weight = 1, i32 quality = 0);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Tag; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    [[nodiscard]] const std::string& getTagId() const { return m_tagId; }
    [[nodiscard]] bool isExpand() const { return m_expand; }

    void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const override;

private:
    std::string m_tagId;
    bool m_expand;
};

} // namespace loot
} // namespace mc
