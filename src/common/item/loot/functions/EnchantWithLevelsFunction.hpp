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
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief Enchant with levels function
 *
 * Randomly enchants an item.
 * See: net.minecraft.loot.functions.EnchantWithLevels
 *
 * Used for generating enchanted books, equipment, etc.
 */
class EnchantWithLevelsFunction : public LootFunction {
public:
    /**
     * @brief Construct enchant with levels function
     * @param levels Enchantment level range
     * @param treasure Whether to include treasure enchantments
     */
    explicit EnchantWithLevelsFunction(const RandomValueRange& levels, bool treasure = false) noexcept;

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "enchant_with_levels"; }

    [[nodiscard]] const RandomValueRange& getLevels() const noexcept { return m_levels; }
    [[nodiscard]] bool isTreasure() const noexcept { return m_treasure; }

private:
    RandomValueRange m_levels;
    bool m_treasure;
};

} // namespace loot
} // namespace mc
