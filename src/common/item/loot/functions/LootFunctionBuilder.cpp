/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "LootFunctionBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/item/loot/functions/ApplyBonusFunction.hpp"
#include "common/item/loot/functions/CopyBlockStateFunction.hpp"
#include "common/item/loot/functions/CopyNameFunction.hpp"
#include "common/item/loot/functions/CopyNbtFunction.hpp"
#include "common/item/loot/functions/EnchantRandomlyFunction.hpp"
#include "common/item/loot/functions/EnchantWithLevelsFunction.hpp"
#include "common/item/loot/functions/ExplorationMapFunction.hpp"
#include "common/item/loot/functions/ExplosionDecayFunction.hpp"
#include "common/item/loot/functions/FillPlayerHeadFunction.hpp"
#include "common/item/loot/functions/FurnaceSmeltFunction.hpp"
#include "common/item/loot/functions/LimitCountFunction.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/item/loot/functions/LootingEnchantBonusFunction.hpp"
#include "common/item/loot/functions/SetAttributesFunction.hpp"
#include "common/item/loot/functions/SetContentsFunction.hpp"
#include "common/item/loot/functions/SetCountFunction.hpp"
#include "common/item/loot/functions/SetDamageFunction.hpp"
#include "common/item/loot/functions/SetLootTableFunction.hpp"
#include "common/item/loot/functions/SetLoreFunction.hpp"
#include "common/item/loot/functions/SetNameFunction.hpp"
#include "common/item/loot/functions/SetNbtFunction.hpp"
#include "common/item/loot/functions/SetStewEffectFunction.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace loot {

std::unique_ptr<LootFunction> LootFunctionBuilder::setCount(const RandomValueRange& count, bool add)
{
    return std::make_unique<SetCountFunction>(count, add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setCount(i32 count, bool add)
{
    return std::make_unique<SetCountFunction>(RandomValueRange(static_cast<f32>(count)), add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::applyBonus(
    ApplyBonusFunction::BonusType bonusType, i32 bonusMultiplier, i32 extra, f32 probability)
{
    return std::make_unique<ApplyBonusFunction>(bonusType, bonusMultiplier, extra, probability);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::lootingEnchantBonus(const RandomValueRange& count, i32 limit)
{
    return std::make_unique<LootingEnchantBonusFunction>(count, limit);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setDamage(const RandomValueRange& durability, bool add)
{
    return std::make_unique<SetDamageFunction>(durability, add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setName(const std::string& name, bool replace)
{
    return std::make_unique<SetNameFunction>(name, replace);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setLore(const std::vector<std::string>& lore, bool replace)
{
    return std::make_unique<SetLoreFunction>(lore, replace);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::limitCount(i32 min, i32 max)
{
    return std::make_unique<LimitCountFunction>(min, max);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::furnaceSmelt()
{
    return std::make_unique<FurnaceSmeltFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::enchantWithLevels(const RandomValueRange& levels, bool treasure)
{
    return std::make_unique<EnchantWithLevelsFunction>(levels, treasure);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::enchantRandomly(
    const std::vector<std::string>& enchantments, bool treasure)
{
    return std::make_unique<EnchantRandomlyFunction>(enchantments, treasure);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::explosionDecay()
{
    return std::make_unique<ExplosionDecayFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setNbt(const std::string& nbtString)
{
    return std::make_unique<SetNbtFunction>(nbtString);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyName(CopyNameFunction::Source source)
{
    return std::make_unique<CopyNameFunction>(source);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyBlockState(const std::string& blockId)
{
    return std::make_unique<CopyBlockStateFunction>(blockId);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyNbt(CopyNbtFunction::Source source)
{
    return std::make_unique<CopyNbtFunction>(source);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::fillPlayerHead()
{
    return std::make_unique<FillPlayerHeadFunction>(CopyNameFunction::Source::KillerPlayer);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setAttributes()
{
    return std::make_unique<SetAttributesFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setContents()
{
    return std::make_unique<SetContentsFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setLootTable(const std::string& lootTableId)
{
    return std::make_unique<SetLootTableFunction>(lootTableId);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::explorationMap()
{
    return std::make_unique<ExplorationMapFunction>(
        ExplorationMapFunction::Destination::BuriedTreasure, std::nullopt, 2, 50, true);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setStewEffect()
{
    return std::make_unique<SetStewEffectFunction>();
}

} // namespace loot
} // namespace mc
