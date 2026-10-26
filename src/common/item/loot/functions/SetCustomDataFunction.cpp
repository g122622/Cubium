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

#include "SetCustomDataFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

SetCustomDataFunction::SetCustomDataFunction(const std::string& nbtString, std::optional<nlohmann::json> parsedTag)
    : m_nbtString(nbtString)
    , m_parsedTag(std::move(parsedTag))
{}

Result<std::unique_ptr<SetCustomDataFunction>> SetCustomDataFunction::create(const std::string& nbtString)
{
    // 加载期即解析（对应 vanilla 在 codec 阶段用 TagParser 解析 tag）：非法文本必须让整张战利品表加载失败，
    // 而不是在掉落时才静默丢弃。
    auto parsedTag = nbt::parseMojangson(nbtString);
    if (!parsedTag) {
        return Error(ErrorCode::InvalidData, "set_custom_data: malformed tag '" + nbtString + "'");
    }
    return std::make_unique<SetCustomDataFunction>(nbtString, nbt::nbtToJson(*parsedTag));
}

ItemStack SetCustomDataFunction::run(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);

    if (stack.isEmpty() || !m_parsedTag.has_value()) {
        return stack;
    }

    // 合并加载期解析好的自定义数据（对象且非空时生效）
    if (m_parsedTag->is_object() && !m_parsedTag->empty()) {
        stack.mergeTag(*m_parsedTag);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetCustomDataFunction::clone() const noexcept
{
    auto func = std::make_unique<SetCustomDataFunction>(m_nbtString, m_parsedTag);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
