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

#include "SetLootTableFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

SetLootTableFunction::SetLootTableFunction(const std::string& lootTableId, u64 seed)
    : m_lootTableId(lootTableId)
    , m_seed(seed)
{}

ItemStack SetLootTableFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);

    if (stack.isEmpty() || m_lootTableId.empty()) {
        return stack;
    }

    // 将掉落表信息写入 BlockEntityTag 子标签
    // 结构: {BlockEntityTag: {LootTable: "minecraft:blocks/chest", LootTableSeed: 12345L}}
    nlohmann::json& blockEntityTag = stack.getOrCreateChildTag("BlockEntityTag");

    // 设置掉落表 ID
    blockEntityTag["LootTable"] = m_lootTableId;

    // 如果有种子，也设置种子（种子为 0 时不存储）
    if (m_seed != 0) {
        // JSON 数字类型自动处理整数
        blockEntityTag["LootTableSeed"] = static_cast<i64>(m_seed);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetLootTableFunction::clone() const noexcept
{
    auto func = std::make_unique<SetLootTableFunction>(m_lootTableId, m_seed);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
