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

#include "SetCustomModelDataFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <algorithm>
#include <memory>

namespace mc {
namespace loot {

namespace {

/// 读取已有列表（缺失时为空数组）
nlohmann::json _readList(const nlohmann::json& data, const char* key)
{
    if (data.is_object() && data.contains(key) && data[key].is_array()) {
        return data[key];
    }
    return nlohmann::json::array();
}

/// 按列表操作组合（对应原版 ListOperation.apply）
template <typename T>
void _applyOperation(std::vector<T>& target, ListOperationKind kind, i32 offset, const std::vector<T>& values)
{
    switch (kind) {
        case ListOperationKind::ReplaceAll:
            target = values;
            break;
        case ListOperationKind::Append:
            target.insert(target.end(), values.begin(), values.end());
            break;
        case ListOperationKind::Insert: {
            const auto position = std::min<std::size_t>(static_cast<std::size_t>(std::max(offset, 0)), target.size());
            target.insert(target.begin() + static_cast<std::ptrdiff_t>(position), values.begin(), values.end());
            break;
        }
    }
}

} // namespace

SetCustomModelDataFunction::SetCustomModelDataFunction(std::optional<FloatListOperation> floats,
    std::optional<BoolListOperation> flags,
    std::optional<StringListOperation> strings,
    std::optional<ColorListOperation> colors)
    : m_floats(std::move(floats))
    , m_flags(std::move(flags))
    , m_strings(std::move(strings))
    , m_colors(std::move(colors))
{}

ItemStack SetCustomModelDataFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }
    nlohmann::json data = stack.getCustomModelData();

    if (m_floats.has_value() || m_colors.has_value()) {
        std::vector<f32> current;
        auto json = _readList(data, m_floats.has_value() ? "floats" : "colors");
        for (const auto& value : json) {
            current.push_back(value.get<f32>());
        }
        // floats 与 colors 结构相同，但类型不同，分别处理
        if (m_floats.has_value()) {
            std::vector<f32> produced;
            produced.reserve(m_floats->values.size());
            for (const auto& provider : m_floats->values) {
                produced.push_back(static_cast<f32>(provider.generateInt(context.getRandom())));
            }
            _applyOperation(current, m_floats->kind, m_floats->offset, produced);
            data["floats"] = current;
        } else {
            std::vector<f32> produced;
            produced.reserve(m_colors->values.size());
            for (const auto& provider : m_colors->values) {
                produced.push_back(static_cast<f32>(provider.generateInt(context.getRandom())));
            }
            _applyOperation(current, m_colors->kind, m_colors->offset, produced);
            data["colors"] = current;
        }
    }
    if (m_flags.has_value()) {
        std::vector<bool> current;
        for (const auto& value : _readList(data, "flags")) {
            current.push_back(value.get<bool>());
        }
        _applyOperation(current, m_flags->kind, m_flags->offset, m_flags->values);
        data["flags"] = current;
    }
    if (m_strings.has_value()) {
        std::vector<std::string> current;
        for (const auto& value : _readList(data, "strings")) {
            current.push_back(value.get<std::string>());
        }
        _applyOperation(current, m_strings->kind, m_strings->offset, m_strings->values);
        data["strings"] = current;
    }
    stack.setCustomModelData(std::move(data));
    return stack;
}

std::unique_ptr<LootFunction> SetCustomModelDataFunction::clone() const noexcept
{
    auto func = std::make_unique<SetCustomModelDataFunction>(m_floats, m_flags, m_strings, m_colors);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
