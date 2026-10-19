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

#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistryDataPack.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace attribute {
namespace {

/**
 * @brief 数据包中的操作名 → 修改器操作标识
 *
 * 与 vanilla `AttributeModifier.OperationId.CODEC` 的序列化名一致。
 */
std::optional<AttributeModifierOperation> _operationByName(std::string_view name)
{
    static constexpr std::pair<std::string_view, AttributeModifierOperation> OPERATIONS[] = {
        {"override", AttributeModifierOperation::Override},
        {"alpha_blend", AttributeModifierOperation::AlphaBlend},
        {"add", AttributeModifierOperation::Add},
        {"subtract", AttributeModifierOperation::Subtract},
        {"multiply", AttributeModifierOperation::Multiply},
        {"blend_to_gray", AttributeModifierOperation::BlendToGray},
        {"minimum", AttributeModifierOperation::Minimum},
        {"maximum", AttributeModifierOperation::Maximum},
        {"and", AttributeModifierOperation::And},
        {"nand", AttributeModifierOperation::Nand},
        {"or", AttributeModifierOperation::Or},
        {"nor", AttributeModifierOperation::Nor},
        {"xor", AttributeModifierOperation::Xor},
        {"xnor", AttributeModifierOperation::Xnor},
    };
    for (const auto& [operationName, operation] : OPERATIONS) {
        if (operationName == name) {
            return operation;
        }
    }
    return std::nullopt;
}

} // namespace

Result<void> EnvironmentAttributeMap::Builder::setFromJson(std::string_view name, const nlohmann::json& rawArgument)
{
    const void* const attribute = EnvironmentAttributeRegistry::find(name);
    if (attribute == nullptr) {
        return Error(ErrorCode::NotFound, "Unknown environment attribute: " + std::string(name));
    }

    const std::vector<EnvironmentAttributeEntry>& registered = EnvironmentAttributeRegistry::attributes();
    const auto entryIt = std::find_if(
        registered.begin(), registered.end(), [attribute](const EnvironmentAttributeEntry& entry) {
            return entry.attribute == attribute;
        });
    MC_ASSERT_RELEASE_MSG(entryIt != registered.end(), "EnvironmentAttributeMap: attribute missing from registry");
    const EnvironmentAttributeEntry& entry = *entryIt;

    // 两种写法：直接给出参数（覆写），或 `{"modifier": ..., "argument": ...}`
    const bool hasModifier = rawArgument.is_object() && rawArgument.contains("modifier");
    const nlohmann::json& parameter = hasModifier ? rawArgument.at("argument") : rawArgument;

    AttributeModifierOperation operation = AttributeModifierOperation::Override;
    if (hasModifier) {
        const nlohmann::json& modifier = rawArgument.at("modifier");
        if (!modifier.is_string()) {
            return Error(ErrorCode::InvalidData, "Attribute modifier must be a string: " + entry.name);
        }
        const std::string modifierName = modifier.get<std::string>();
        const std::optional<AttributeModifierOperation> parsed = _operationByName(modifierName);
        if (!parsed.has_value()) {
            return Error(ErrorCode::InvalidData, "Unknown attribute modifier '" + modifierName + "'");
        }
        operation = *parsed;
    }

    if (!entry.allowModifier(operation)) {
        return Error(ErrorCode::InvalidData,
            "Modifier '" + std::string(attributeModifierOperationName(operation)) + "' is not allowed for attribute "
                + entry.name);
    }
    if (!entry.applyJsonArgument) {
        return Error(ErrorCode::InvalidData, "Attribute value type has no JSON representation: " + entry.name);
    }

    // 参数按属性值类型解析；覆写直接取参数，其余修改器以属性默认值为基础应用。
    // 与程序内 `modify` 走同一入口，保证数据包条目与内建条目的求值语义完全一致。
    return entry.applyJsonArgument(operation, &parameter, this);
}

} // namespace attribute
} // namespace world
} // namespace mc