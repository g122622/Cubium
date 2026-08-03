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
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief Fortune bonus function
 *
 * Increases drop count based on fortune enchantment level.
 * See: net.minecraft.loot.functions.ApplyBonus
 *
 * Used for ore, crop, and other block loot tables.
 * MC 1.16.5 Fortune algorithm:
 * - Fortune I: 33% chance +1
 * - Fortune II: 25% chance +1, 25% chance +2
 * - Fortune III: 20% chance +1, 20% chance +2, 20% chance +3
 */
class ApplyBonusFunction : public LootFunction {
public:
    /**
     * @brief Bonus type
     *
     * See MC 1.16.5's three bonus formulas.
     */
    enum class BonusType : u8 {
        Uniform,  // Uniform distribution: count + random(0, bonusMultiplier * fortune)
        Binomial, // Binomial distribution: count + binomial(fortune + extra, probability)
        OreDrops  // Ore drops: count * random(1, fortune + 1)
    };

    /**
     * @brief Construct fortune bonus function
     * @param bonusType Bonus type
     * @param bonusMultiplier Multiplier for Uniform type (default 1)
     * @param extra Extra trials for Binomial type (default 1)
     * @param probability Success probability for Binomial type
     */
    explicit ApplyBonusFunction(
        BonusType bonusType = BonusType::OreDrops, i32 bonusMultiplier = 1, i32 extra = 1, f32 probability = 1.0f);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "apply_bonus"; }

    [[nodiscard]] BonusType getBonusType() const { return m_bonusType; }
    [[nodiscard]] i32 getBonusMultiplier() const { return m_bonusMultiplier; }
    [[nodiscard]] i32 getExtra() const { return m_extra; }
    [[nodiscard]] f32 getProbability() const { return m_probability; }

    /**
     * @brief Calculate fortune bonus count (ore drops algorithm)
     *
     * MC 1.16.5 OreDropsFormula:
     * if (fortune > 0) {
     *     int i = random.nextInt(fortune + 2) - 1;
     *     if (i < 0) i = 0;
     *     return baseCount * (i + 1);
     * } else {
     *     return baseCount;
     * }
     *
     * @param baseCount Base drop count
     * @param fortuneLevel Fortune level (0-3)
     * @param random Random number generator
     * @return Drop count
     */
    [[nodiscard]] static i32 calculateOreDrops(i32 baseCount, i32 fortuneLevel, math::Random& random);

    /**
     * @brief Calculate uniform distribution bonus
     *
     * MC 1.16.5 UniformBonusCountFormula:
     * count + random.nextInt(bonusMultiplier * fortune + 1)
     *
     * @param baseCount Base drop count
     * @param fortuneLevel Fortune level
     * @param bonusMultiplier Multiplier (default 1)
     * @param random Random number generator
     * @return Drop count
     */
    [[nodiscard]] static i32 calculateUniformBonus(
        i32 baseCount, i32 fortuneLevel, i32 bonusMultiplier, math::Random& random);

    /**
     * @brief Calculate binomial distribution bonus
     *
     * MC 1.16.5 BinomialWithBonusCountFormula:
     * for (int i = 0; i < fortune + extra; ++i) {
     *     if (random.nextFloat() < probability) ++count;
     * }
     *
     * @param baseCount Base drop count
     * @param fortuneLevel Fortune level
     * @param extra Extra trial count (default 1)
     * @param probability Success probability
     * @param random Random number generator
     * @return Drop count
     */
    [[nodiscard]] static i32 calculateBinomialBonus(
        i32 baseCount, i32 fortuneLevel, i32 extra, f32 probability, math::Random& random);

private:
    BonusType m_bonusType;
    i32 m_bonusMultiplier; // Used by Uniform type
    i32 m_extra;           // Used by Binomial type
    f32 m_probability;     // Used by Binomial type
};

} // namespace loot
} // namespace mc
