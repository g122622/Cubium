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

#include "common/mod/bedrock/addon/content/base/properties/AddonPropertyValue.hpp"

#include <cmath>

namespace mc::mod::bedrock::addon {
namespace {
constexpr std::size_t _MAX_DYNAMIC_STRING_BYTES = 32767;
} // namespace

nlohmann::json encodeAddonPropertyValue(const nlohmann::json& value)
{
    // JSON 布尔写入 NBT 后表现为 Byte，独立类型标记用于恢复原始脚本类型。
    const char* kind = value.is_boolean() ? "boolean"
        : value.is_string()               ? "string"
        : value.is_number()               ? "number"
                                          : "vector";
    return {{"kind", kind}, {"value", value}};
}

Result<nlohmann::json> decodeAddonPropertyValue(const nlohmann::json& encoded)
{
    if (!encoded.is_object() || !encoded.contains("kind") || !encoded["kind"].is_string() ||
        !encoded.contains("value")) {
        return Error(ErrorCode::InvalidData, "Invalid addon property encoding");
    }

    const auto kind = encoded["kind"].get<std::string>();
    const auto& value = encoded["value"];
    if (kind == "boolean") {
        if (value.is_boolean()) {
            return value;
        }
        if (value.is_number_integer() && (value == 0 || value == 1)) {
            return nlohmann::json(value.get<int>() != 0);
        }
    } else if ((kind == "number" && value.is_number()) || (kind == "string" && value.is_string()) ||
        (kind == "vector" && value.is_object())) {
        return value;
    }

    return Error(ErrorCode::InvalidData, "Addon property type does not match its encoding");
}

Result<void> validateDynamicPropertyValue(const nlohmann::json& value)
{
    if (value.is_null() || value.is_boolean()) {
        return Result<void>::ok();
    }
    if (value.is_string() && value.get_ref<const std::string&>().size() <= _MAX_DYNAMIC_STRING_BYTES) {
        return Result<void>::ok();
    }
    if (value.is_number() && std::isfinite(value.get<double>())) {
        return Result<void>::ok();
    }
    if (value.is_object() && value.size() == 3) {
        for (const char* coordinate : {"x", "y", "z"}) {
            if (!value.contains(coordinate) || !value[coordinate].is_number() ||
                !std::isfinite(value[coordinate].get<double>())) {
                return Error(ErrorCode::InvalidArgument, "Dynamic property requires finite Vector3 coordinates");
            }
        }
        return Result<void>::ok();
    }

    return Error(ErrorCode::InvalidArgument, "Unsupported dynamic property value or oversized string");
}

} // namespace mc::mod::bedrock::addon
