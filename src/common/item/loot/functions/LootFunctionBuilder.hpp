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

#include "ApplyBonusFunction.hpp"
#include "CopyBlockStateFunction.hpp"
#include "CopyNbtFunction.hpp"
#include "EnchantRandomlyFunction.hpp"
#include "EnchantWithLevelsFunction.hpp"
#include "ExplorationMapFunction.hpp"
#include "ExplosionDecayFunction.hpp"
#include "FillPlayerHeadFunction.hpp"
#include "FurnaceSmeltFunction.hpp"
#include "LimitCountFunction.hpp"
#include "LootFunction.hpp"
#include "LootingEnchantBonusFunction.hpp"
#include "SetAttributesFunction.hpp"
#include "SetContentsFunction.hpp"
#include "SetCountFunction.hpp"
#include "SetDamageFunction.hpp"
#include "SetLootTableFunction.hpp"
#include "SetLoreFunction.hpp"
#include "SetNameFunction.hpp"
#include "SetNbtFunction.hpp"
#include "SetStewEffectFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/functions/CopyNameFunction.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief Loot function builder
 *
 * Provides a fluent interface for building loot functions.
 */
class LootFunctionBuilder {
public:
    LootFunctionBuilder() = default;

    // ========== Static factory methods ==========

    /**
     * @brief Create a set count function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setCount(const RandomValueRange& count, bool add = false);

    /**
     * @brief Create a set count function (fixed value)
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setCount(i32 count, bool add = false);

    /**
     * @brief Create a fortune bonus function
     * @param bonusType Bonus type
     * @param bonusMultiplier Multiplier for Uniform type (default 1)
     * @param extra Extra trials for Binomial type (default 1)
     * @param probability Success probability for Binomial type
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> applyBonus(
        ApplyBonusFunction::BonusType bonusType = ApplyBonusFunction::BonusType::OreDrops,
        i32 bonusMultiplier = 1,
        i32 extra = 1,
        f32 probability = 1.0f);

    /**
     * @brief Create a looting enchant bonus function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> lootingEnchantBonus(
        const RandomValueRange& count = RandomValueRange(0.0f, 1.0f), i32 limit = 0);

    /**
     * @brief Create a set damage function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setDamage(const RandomValueRange& durability, bool add = false);

    /**
     * @brief Create a set name function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setName(const std::string& name, bool replace = true);

    /**
     * @brief Create a set lore function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setLore(
        const std::vector<std::string>& lore, bool replace = true);

    /**
     * @brief Create a limit count function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> limitCount(i32 min = -1, i32 max = -1);

    /**
     * @brief Create a furnace smelt function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> furnaceSmelt();

    /**
     * @brief Create an enchant with levels function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> enchantWithLevels(
        const RandomValueRange& levels, bool treasure = false);

    /**
     * @brief Create an enchant randomly function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> enchantRandomly(
        const std::vector<std::string>& enchantments = {}, bool treasure = false);

    /**
     * @brief Create an explosion decay function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> explosionDecay();

    /**
     * @brief Create a set NBT function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setNbt(const std::string& nbtString);

    /**
     * @brief Create a copy name function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyName(CopyNameFunction::Source source);

    /**
     * @brief Create a copy block state function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyBlockState(const std::string& blockId);

    /**
     * @brief Create a copy NBT function
     * @param source Data source
     * @return Created loot function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyNbt(CopyNbtFunction::Source source);

    /**
     * @brief Create a fill player head function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> fillPlayerHead();

    /**
     * @brief Create a set attributes function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setAttributes();

    /**
     * @brief Create a set contents function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setContents();

    /**
     * @brief Create a set loot table function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setLootTable(const std::string& lootTableId);

    /**
     * @brief Create an exploration map function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> explorationMap();

    /**
     * @brief Create a set stew effect function
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setStewEffect();
};

} // namespace loot
} // namespace mc
