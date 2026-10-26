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
//
// 全部对齐 MC 1.21.11 `LootContextParamSets`：必需键集合即数据包 JSON 里
// `"type"` 所声明参数集的必需键。原版在 `LootParams.Builder#create(ContextKeySet)`
// 里对必需键做硬校验，缺键直接抛异常——这里保持同样的语义，否则「某张表要求
// 哪些参数」这件事在两侧会得到不同答案。
//
// 少数本项目独有的键（BLOCK_POS）
// 仍然登记为**可选**：原版不存在对应上下文键（时运/掠夺从工具与攻击者的附魔读），
// 而本项目的调用方仍在传它们。

namespace LootParameterSets {

LootParameterSet empty()
{
    return LootParameterSet(LootParameterSet::Type::Empty);
}

LootParameterSet block()
{
    // 1.21.11 BLOCK: required {BLOCK_STATE, ORIGIN, TOOL}, optional {THIS_ENTITY, BLOCK_ENTITY, EXPLOSION_RADIUS}
    LootParameterSet set(LootParameterSet::Type::Block);
    set.addRequired(LootParams::BLOCK_STATE);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::TOOL);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::BLOCK_ENTITY);
    set.addOptional(LootParams::EXPLOSION_RADIUS);
    set.addOptional(LootParams::BLOCK_POS);
    return set;
}

LootParameterSet chest()
{
    // 1.21.11 CHEST: required {ORIGIN}, optional {THIS_ENTITY}
    LootParameterSet set(LootParameterSet::Type::Chest);
    set.addRequired(LootParams::ORIGIN);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::BLOCK_POS);
    set.addOptional(LootParams::BLOCK_ENTITY);
    return set;
}

LootParameterSet entity()
{
    // 1.21.11 ENTITY:
    //   required {THIS_ENTITY, ORIGIN, DAMAGE_SOURCE}
    //   optional {ATTACKING_ENTITY, DIRECT_ATTACKING_ENTITY, LAST_DAMAGE_PLAYER}
    LootParameterSet set(LootParameterSet::Type::Entity);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::DAMAGE_SOURCE);
    set.addOptional(LootParams::KILLER_ENTITY);
    set.addOptional(LootParams::DIRECT_KILLER);
    set.addOptional(LootParams::KILLER_PLAYER);
    return set;
}

LootParameterSet fishing()
{
    // 1.21.11 FISHING: required {ORIGIN, TOOL}, optional {THIS_ENTITY}
    LootParameterSet set(LootParameterSet::Type::Fishing);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::TOOL);
    set.addOptional(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::IS_IN_OPEN_WATER);
    set.addOptional(LootParams::BLOCK_POS);
    return set;
}

LootParameterSet gift()
{
    // 1.21.11 GIFT: required {ORIGIN, THIS_ENTITY}
    LootParameterSet set(LootParameterSet::Type::Gift);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::KILLER_PLAYER);
    set.addOptional(LootParams::BLOCK_POS);
    return set;
}

LootParameterSet barter()
{
    // 1.21.11 PIGLIN_BARTER: required {THIS_ENTITY}
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
    // 1.21.11 SELECTOR: required {ORIGIN, THIS_ENTITY}
    LootParameterSet set(LootParameterSet::Type::Selector);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addOptional(LootParams::BLOCK_POS);
    return set;
}

LootParameterSet archaeology()
{
    // 1.21.11 ARCHAEOLOGY: required {ORIGIN, THIS_ENTITY, TOOL}
    LootParameterSet set(LootParameterSet::Type::Archaeology);
    set.addRequired(LootParams::ORIGIN);
    set.addRequired(LootParams::THIS_ENTITY);
    set.addRequired(LootParams::TOOL);
    set.addOptional(LootParams::BLOCK_POS);
    set.addOptional(LootParams::BLOCK_ENTITY);
    set.addOptional(LootParams::LUCK);
    return set;
}

} // namespace LootParameterSets

} // namespace loot
} // namespace mc