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

#include "LootContext.hpp"
#include "LootConditions.hpp"
#include "LootTable.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <chrono>

namespace mc {
namespace loot {

// 预定义掉落参数
namespace LootParams {
const LootParameter<Entity> THIS_ENTITY("this_entity");
const LootParameter<Player> KILLER_PLAYER("killer_player");
const LootParameter<Entity> KILLER_ENTITY("killer_entity");
const LootParameter<Entity> DIRECT_KILLER("direct_killer");
const LootParameter<DamageSource> DAMAGE_SOURCE("damage_source");
const LootParameter<f32> LUCK("luck");

// 方块相关参数
const LootParameter<BlockState> BLOCK_STATE("block_state");
const LootParameter<BlockPos> BLOCK_POS("block_pos");
const LootParameter<ItemStack> TOOL("tool");
const LootParameter<BlockEntity> BLOCK_ENTITY("block_entity");

// 附魔等级参数
const LootParameter<i32> FORTUNE_LEVEL("fortune_level");
const LootParameter<i32> SILK_TOUCH_LEVEL("silk_touch_level");

// 爆炸相关参数
const LootParameter<f32> EXPLOSION_RADIUS("explosion_radius");

// 钓鱼相关参数
const LootParameter<bool> IS_IN_OPEN_WATER("is_in_open_water");
} // namespace LootParams

// ============================================================================
// LootParameterSet
// ============================================================================

bool LootParameterSet::contains(const std::string& paramId) const
{
    for (const auto& id : m_requiredParams) {
        if (id == paramId) return true;
    }
    for (const auto& id : m_optionalParams) {
        if (id == paramId) return true;
    }
    return false;
}

bool LootParameterSet::validate(const std::vector<std::string>& providedParams) const
{
    // 检查所有必需参数
    for (const auto& required : m_requiredParams) {
        bool found = false;
        for (const auto& provided : providedParams) {
            if (required == provided) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// LootContext
// ============================================================================

LootContext::LootContext(IWorld& world, math::Random& random)
    : m_world(world)
    , m_random(random)
{}

const LootTable* LootContext::getLootTable(const std::string& id) const
{
    if (m_lootTableResolver) {
        return m_lootTableResolver(id);
    }
    return nullptr;
}

bool LootContext::pushLootTable(const LootTable* table)
{
    // 检查是否已经在访问栈中（循环引用）
    for (const auto* visited : m_visitedTables) {
        if (visited == table) {
            return false; // 循环引用
        }
    }
    m_visitedTables.push_back(table);
    return true;
}

void LootContext::popLootTable(const LootTable* table)
{
    if (!m_visitedTables.empty() && m_visitedTables.back() == table) {
        m_visitedTables.pop_back();
    }
}

// ============================================================================
// LootContextBuilder
// ============================================================================

LootContextBuilder::LootContextBuilder(IWorld& world)
    : m_world(world)
{}

LootContextBuilder& LootContextBuilder::withRandom(math::Random& random)
{
    m_random = &random;
    return *this;
}

LootContextBuilder& LootContextBuilder::withSeed(u64 seed)
{
    m_seed = seed;
    m_hasSeed = true;
    return *this;
}

LootContextBuilder& LootContextBuilder::withLuck(f32 luck)
{
    m_luck = luck;
    return *this;
}

LootContextBuilder& LootContextBuilder::withLootingModifier(i32 level)
{
    m_lootingModifier = level;
    return *this;
}

std::unique_ptr<LootContext> LootContextBuilder::build(const LootParameterSet& /* paramSet */)
{
    // 创建随机数生成器
    static thread_local math::Random defaultRandom(0);

    math::Random* randomToUse = m_random;
    if (randomToUse == nullptr) {
        if (m_hasSeed) {
            defaultRandom.setSeed(m_seed);
        } else {
            defaultRandom.setSeed(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
        }
        randomToUse = &defaultRandom;
    }

    auto context = std::make_unique<LootContext>(m_world, *randomToUse);

    // 设置幸运值
    context->setLuck(m_luck);
    context->setLootingModifier(m_lootingModifier);

    // 复制参数
    for (const auto& [id, value] : m_params) {
        context->m_params[id] = value;
    }

    // 转移拥有所有权的值
    for (auto& ownedPtr : m_ownedValues) {
        context->m_ownedValues.push_back(std::move(ownedPtr));
    }

    // 设置掉落表解析器
    if (m_lootTableResolver) {
        context->setLootTableResolver(std::move(m_lootTableResolver));
    }

    return context;
}

} // namespace loot
} // namespace mc
