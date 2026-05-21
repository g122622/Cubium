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

#include "LootParameterSets.hpp"
#include "LootParams.hpp"

namespace mc {
namespace loot {

// ============================================================================
// LootParameterSets - 预定义参数集合
// ============================================================================

namespace LootParameterSets {

LootParameterSet empty()
{
    return LootParameterSet(LootParameterSet::Type::Empty);
}

LootParameterSet block()
{
    LootParameterSet set(LootParameterSet::Type::Block);
    set.addRequired(LootParams::BLOCK_STATE);
    set.addRequired(LootParams::BLOCK_POS);
    set.addRequired(LootParams::TOOL);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::BLOCK_ENTITY);
    set.addOptional(LootParams::FORTUNE_LEVEL);
    set.addOptional(LootParams::SILK_TOUCH_LEVEL);
    return set;
}

LootParameterSet chest()
{
    LootParameterSet set(LootParameterSet::Type::Chest);
    set.addRequired(LootParams::BLOCK_POS);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::BLOCK_ENTITY);
    return set;
}

LootParameterSet entity()
{
    LootParameterSet set(LootParameterSet::Type::Entity);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::KILLER_ENTITY);
    set.addOptional(LootParams::KILLER_PLAYER);
    set.addOptional(LootParams::DIRECT_KILLER);
    set.addOptional(LootParams::DAMAGE_SOURCE);
    set.addOptional(LootParams::LOOTING_MODIFIER);
    return set;
}

LootParameterSet fishing()
{
    LootParameterSet set(LootParameterSet::Type::Fishing);
    set.addRequired(LootParams::BLOCK_POS);
    set.addRequired(LootParams::TOOL);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::IS_IN_OPEN_WATER);
    set.addOptional(LootParams::LOOTING_MODIFIER);
    return set;
}

LootParameterSet gift()
{
    LootParameterSet set(LootParameterSet::Type::Gift);
    set.addRequired(LootParams::BLOCK_POS);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::KILLER_PLAYER);
    return set;
}

LootParameterSet barter()
{
    LootParameterSet set(LootParameterSet::Type::Barter);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::KILLER_PLAYER);
    return set;
}

LootParameterSet generic()
{
    return LootParameterSet(LootParameterSet::Type::Generic);
}

} // namespace LootParameterSets

} // namespace loot
} // namespace mc
