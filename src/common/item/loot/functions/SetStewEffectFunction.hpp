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
#include "common/util/math/random/RandomRanges.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief Set stew effect function
 *
 * Sets status effects on suspicious stew.
 * See: net.minecraft.loot.functions.SetStewEffect
 *
 * Used for generating suspicious stew with specific effects.
 */
class SetStewEffectFunction : public LootFunction {
public:
    /**
     * @brief Effect definition
     */
    struct EffectEntry {
        std::string effectId;      // Effect ID
        RandomValueRange duration; // Duration in seconds
    };

    SetStewEffectFunction() = default;

    /**
     * @brief Add an effect
     */
    void addEffect(const std::string& effectId, const RandomValueRange& duration);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] std::string getType() const override { return "set_stew_effect"; }

    [[nodiscard]] const std::vector<EffectEntry>& getEffects() const { return m_effects; }

private:
    std::vector<EffectEntry> m_effects;
};

} // namespace loot
} // namespace mc
