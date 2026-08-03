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
 * @brief Looting enchant bonus function
 *
 * Increases drop count based on looting enchantment level.
 * See: net.minecraft.loot.functions.LootingEnchantBonus
 *
 * Formula: count + random(0, lootingLevel) * lootingMultiplier
 * Or using random range: count + random(min, max) per looting level
 *
 * Used for mob drops (e.g. rotten flesh, bones, etc.).
 */
class LootingEnchantBonusFunction : public LootFunction {
public:
    /**
     * @brief Construct looting bonus function
     * @param count Extra count range
     * @param limit Maximum count limit (0 means unlimited)
     */
    explicit LootingEnchantBonusFunction(const RandomValueRange& count = RandomValueRange(0.0f, 1.0f), i32 limit = 0);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "looting_enchant"; }

    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }
    [[nodiscard]] i32 getLimit() const { return m_limit; }

private:
    RandomValueRange m_count;
    i32 m_limit; // 0 means unlimited
};

} // namespace loot
} // namespace mc
