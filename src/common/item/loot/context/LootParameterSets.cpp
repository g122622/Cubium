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
#include "common/item/loot/context/LootParameterSet.hpp"

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
    set.addOptional(LootParams::TOOL);
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

LootParameterSet selector()
{
    LootParameterSet set(LootParameterSet::Type::Selector);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addRequired(LootParams::BLOCK_POS);
    return set;
}

LootParameterSet archaeology()
{
    // 对齐 MC 1.21.11 LootContextParamSets.ARCHAEOLOGY：
    //   required = {ORIGIN}, optional = {THIS_ENTITY, TOOL}
    // 本项目使用 BLOCK_POS 代替 ORIGIN（项目约定，见 ChestBoatEntity.cpp:441）。
    // 此外还将 BLOCK_ENTITY、LUCK 设为可选参数，方便考古战利品表函数引用。
    LootParameterSet set(LootParameterSet::Type::Archaeology);
    set.addRequired(LootParams::BLOCK_POS);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::TOOL);
    set.addOptional(LootParams::BLOCK_ENTITY);
    set.addOptional(LootParams::LUCK);
    return set;
}

} // namespace LootParameterSets

} // namespace loot
} // namespace mc
