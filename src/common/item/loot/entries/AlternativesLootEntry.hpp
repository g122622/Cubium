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
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 替代条目
 *
 * 尝试多个条目，直到一个成功。
 */
class AlternativesLootEntry : public LootEntry {
public:
    AlternativesLootEntry() noexcept = default;
    explicit AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children);

    [[nodiscard]] LootEntryType getType() const noexcept override { return LootEntryType::Alternatives; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    void addChild(std::unique_ptr<LootEntry> child);

    void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const override;

private:
    std::vector<std::unique_ptr<LootEntry>> m_children;
};

} // namespace loot
} // namespace mc
