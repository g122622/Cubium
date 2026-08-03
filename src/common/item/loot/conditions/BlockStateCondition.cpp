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

#include "common/item/loot/conditions/BlockStateCondition.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

BlockStateCondition::BlockStateCondition(const std::string& blockId)
    : m_blockId(blockId)
    , m_properties()
{}

BlockStateCondition::BlockStateCondition(const std::string& blockId, StatePropertiesPredicate properties)
    : m_blockId(blockId)
    , m_properties(std::move(properties))
{}

bool BlockStateCondition::test(LootContext& context) const
{
    // 从上下文获取 BLOCK_STATE 参数
    auto* blockState = context.get<BlockState>(LootParams::BLOCK_STATE);
    if (!blockState) {
        return false;
    }

    // 检查方块ID是否匹配
    if (blockState->blockLocation().toString() != m_blockId) {
        return false;
    }

    // 如果有属性匹配条件，检查属性
    if (!m_properties.isEmpty()) {
        return m_properties.matches(*blockState);
    }

    return true;
}

std::unique_ptr<LootCondition> BlockStateCondition::clone() const noexcept
{
    return std::make_unique<BlockStateCondition>(m_blockId, m_properties);
}

} // namespace loot
} // namespace mc
