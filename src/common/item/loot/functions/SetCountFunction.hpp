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

#include <memory>
#include "LootFunction.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace loot {

/**
 * @brief Set count function
 *
 * Sets the item stack count.
 * See: net.minecraft.loot.functions.SetCount
 *
 * Can set a fixed count or random range.
 * Extra count affected by luck can be set via the quality parameter.
 */
class SetCountFunction : public LootFunction {
public:
    /**
     * @brief Construct set count function
     * @param count Count range
     * @param add Whether to add on top of existing count (default false, replace)
     */
    explicit SetCountFunction(const RandomValueRange& count, bool add = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] std::string getType() const override { return "set_count"; }

    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }
    [[nodiscard]] bool isAdd() const { return m_add; }

private:
    RandomValueRange m_count;
    bool m_add;
};

} // namespace loot
} // namespace mc
