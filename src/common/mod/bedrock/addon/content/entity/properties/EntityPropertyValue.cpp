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

#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyValue.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::mod::bedrock::addon {
Result<void> limitEntityPropertyNumber(const std::string& name, nlohmann::json& value, const nlohmann::json& definition)
{
    const auto& type = definition["type"];
    if (value.is_number() && (type == "int" || type == "float")) {
        double number = value.get<double>();
        if (!std::isfinite(number)) {
            return Error(ErrorCode::InvalidData, "Entity event property must be finite: " + name);
        }
        // JSON 事件赋值将数值限制在声明范围，脚本 setProperty 则报告越界。
        double minimum = type == "int" ? static_cast<double>(std::numeric_limits<mc::i32>::min())
                                       : -static_cast<double>(std::numeric_limits<mc::f32>::max());
        double maximum = type == "int" ? static_cast<double>(std::numeric_limits<mc::i32>::max())
                                       : static_cast<double>(std::numeric_limits<mc::f32>::max());
        if (definition.contains("range")) {
            const auto& range = definition["range"];
            minimum = range[0].get<double>();
            maximum = range[1].get<double>();
        }
        number = std::clamp(number, minimum, maximum);
        if (type == "int") {
            number = std::trunc(number);
        }
        value = number;
    }
    return Result<void>::ok();
}

Result<nlohmann::json> castMolangPropertyValue(
    const std::string& name, const nlohmann::json& definition, const MolangValue& value)
{
    nlohmann::json result;
    if (const auto* number = std::get_if<mc::f32>(&value)) {
        result = definition["type"] == "bool" ? nlohmann::json(*number != 0) : nlohmann::json(*number);
    } else {
        result = std::get<std::string>(value);
    }
    MC_TRY(limitEntityPropertyNumber(name, result, definition));
    return result;
}
} // namespace mc::mod::bedrock::addon
