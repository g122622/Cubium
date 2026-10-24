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

#include "LootFunction.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 按物品谓词分派函数（minecraft:filtered）
 *
 * 对应原版 FilteredFunction：物品匹配 item_filter 时应用 on_pass，否则应用 on_fail。
 *
 * JSON：{ "item_filter": { "items": "minecraft:diamond_sword" }, "on_pass": {...}, "on_fail": {...} }
 */
class FilteredFunction : public LootFunction {
public:
    FilteredFunction(advancement::ItemPredicate filter,
        std::optional<std::unique_ptr<LootFunction>> onPass,
        std::optional<std::unique_ptr<LootFunction>> onFail);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "filtered";
    }

    [[nodiscard]] const advancement::ItemPredicate& getFilter() const
    {
        return m_filter;
    }

    [[nodiscard]] const std::optional<std::unique_ptr<LootFunction>>& getOnPass() const
    {
        return m_onPass;
    }

    [[nodiscard]] const std::optional<std::unique_ptr<LootFunction>>& getOnFail() const
    {
        return m_onFail;
    }

private:
    advancement::ItemPredicate m_filter;
    std::optional<std::unique_ptr<LootFunction>> m_onPass;
    std::optional<std::unique_ptr<LootFunction>> m_onFail;
};

} // namespace loot
} // namespace mc
