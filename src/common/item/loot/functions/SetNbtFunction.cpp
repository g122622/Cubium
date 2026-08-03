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

#include "SetNbtFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

SetNbtFunction::SetNbtFunction(const std::string& nbtString)
    : m_nbtString(nbtString)
{}

ItemStack SetNbtFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);

    if (stack.isEmpty() || m_nbtString.empty()) {
        return stack;
    }

    // 使用 Mojangson 格式解析 NBT 字符串
    auto parsedTag = nbt::parseMojangson(m_nbtString);
    if (!parsedTag) {
        return stack;
    }

    // 将 NBT 转换为 JSON 并合并到 ItemStack
    nlohmann::json jsonTag = nbt::nbtToJson(*parsedTag);
    if (jsonTag.is_object() && !jsonTag.empty()) {
        stack.mergeTag(jsonTag);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetNbtFunction::clone() const noexcept
{
    auto func = std::make_unique<SetNbtFunction>(m_nbtString);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
