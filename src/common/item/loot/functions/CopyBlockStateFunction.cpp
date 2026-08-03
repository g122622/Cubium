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

#include "CopyBlockStateFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

CopyBlockStateFunction::CopyBlockStateFunction(const std::string& blockId, const std::vector<std::string>& properties)
    : m_blockId(blockId)
    , m_properties(properties)
{}

ItemStack CopyBlockStateFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 从 LootContext 获取 BlockState
    auto* blockState = context.get<BlockState>(LootParams::BLOCK_STATE);
    if (blockState == nullptr) {
        return stack;
    }

    // 验证方块 ID 是否匹配
    const std::string actualBlockId = blockState->getBlock().blockLocation().toString();
    if (!m_blockId.empty() && actualBlockId != m_blockId) {
        return stack;
    }

    // 获取或创建 BlockStateTag 子标签
    nlohmann::json& blockStateTag = stack.getOrCreateChildTag("BlockStateTag");

    // 遍历 BlockState 的所有属性
    // 获取方块的属性名称映射（通过 StateContainer）
    const auto& blockProperties = blockState->getBlock().stateContainer().properties();

    for (const auto& [propName, prop] : blockProperties) {
        // 如果指定了属性列表，只复制指定的属性
        if (!m_properties.empty()) {
            bool found = false;
            for (const auto& wanted : m_properties) {
                if (wanted == propName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                continue;
            }
        }

        // 获取属性值
        const auto valueIndex = blockState->getValueIndex(*prop);
        if (valueIndex.has_value()) {
            // 获取属性值的字符串表示
            std::string valueStr = prop->valueToString(*valueIndex);
            blockStateTag[propName] = valueStr;
        }
    }

    return stack;
}

std::unique_ptr<LootFunction> CopyBlockStateFunction::clone() const noexcept
{
    auto func = std::make_unique<CopyBlockStateFunction>(m_blockId, m_properties);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
