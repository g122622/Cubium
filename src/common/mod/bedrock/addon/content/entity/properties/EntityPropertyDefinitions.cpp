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

#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyDefinitions.hpp"
#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyValue.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include <algorithm>

namespace mc::mod::bedrock::addon {
Result<EntityPropertyDefinitions> EntityPropertyDefinitions::compile(const nlohmann::json& definitions)
{
    if (!definitions.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity property definitions must be an object");
    }
    EntityPropertyDefinitions result;
    result.m_literalSchema = definitions;
    for (auto& [name, definition] : result.m_literalSchema.items()) {
        if (!definition.is_object() || !definition.contains("type") || !definition["type"].is_string() ||
            !definition.contains("default")) {
            return Error(ErrorCode::InvalidData, "Invalid entity property declaration: " + name);
        }
        const auto type = definition["type"].get<std::string>();
        auto& value = definition["default"];
        if (!value.is_string()) {
            continue;
        }
        if (type == "enum" && definition.contains("values") && definition["values"].is_array() &&
            std::find(definition["values"].begin(), definition["values"].end(), value) != definition["values"].end()) {
            continue;
        }
        if (type != "bool" && type != "int" && type != "float" && type != "enum") {
            continue;
        }
        auto expression = MolangExpression::compile(value.get_ref<const std::string&>());
        if (expression.failed()) {
            return expression.error();
        }
        if (expression.value().usesPropertyQueries()) {
            // 默认表达式计算时实体属性尚未建立，不能读取其他属性或依赖定义顺序。
            return Error(ErrorCode::Unsupported, "Entity property defaults cannot query entity properties: " + name);
        }
        result.m_defaults.emplace(name, std::move(expression.value()));
        if (type == "bool") {
            value = false;
        } else if (type == "enum") {
            if (!definition.contains("values") || !definition["values"].is_array() || definition["values"].empty()) {
                return Error(ErrorCode::InvalidData, "Enum property default requires a valid values list: " + name);
            }
            value = definition["values"][0];
        } else {
            value = 0;
            const auto range = definition.find("range");
            if (range != definition.end() && range->is_array() && range->size() == 2 && (*range)[0].is_number()) {
                value = (*range)[0];
            }
        }
    }
    EntityAddonState validator;
    MC_TRY(validator.defineProperties(result.m_literalSchema));
    return result;
}

const nlohmann::json& EntityPropertyDefinitions::literalSchema() const
{
    return m_literalSchema;
}

Result<nlohmann::json> EntityPropertyDefinitions::instantiate(mc::math::IRandom& random) const
{
    nlohmann::json definitions = m_literalSchema;
    MolangEvaluationContext context{random, [](std::string_view) -> std::optional<MolangValue> {
                                        return std::nullopt;
                                    }};
    for (const auto& [name, expression] : m_defaults) {
        auto value = expression.evaluate(context);
        if (value.failed()) {
            return value.error();
        }
        auto cast = castMolangPropertyValue(name, definitions[name], value.value());
        if (cast.failed()) {
            return cast.error();
        }
        definitions[name]["default"] = std::move(cast.value());
    }
    EntityAddonState validator;
    MC_TRY(validator.defineProperties(definitions));
    return definitions;
}
} // namespace mc::mod::bedrock::addon
