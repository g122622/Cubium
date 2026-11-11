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

#include "JsonRead.hpp"

#include <istream>
#include <iterator>
#include <string>

namespace mc::util::json {

namespace {

Error _missingField(std::string_view field)
{
    return Error(ErrorCode::InvalidData, "JSON: missing required field '" + std::string(field) + "'");
}

Error _typeMismatch(std::string_view field, const char* expectedType)
{
    return Error(ErrorCode::InvalidData, "JSON: field '" + std::string(field) + "' must be " + expectedType);
}

/// 取字段指针；不存在或不是指定类型时给出对应错误
const nlohmann::json* _requireType(const nlohmann::json& value,
    std::string_view field,
    nlohmann::json::value_t expected,
    const char* expectedType,
    Error& error)
{
    const auto* field_value = findField(value, field);
    if (field_value == nullptr) {
        error = _missingField(field);
        return nullptr;
    }
    if (field_value->type() != expected) {
        error = _typeMismatch(field, expectedType);
        return nullptr;
    }
    return field_value;
}

} // namespace

bool hasField(const nlohmann::json& value, std::string_view field)
{
    // find 不抛异常（operator[] / at 在键缺失时会抛或插入）
    return value.is_object() && value.find(field) != value.end();
}

const nlohmann::json* findField(const nlohmann::json& value, std::string_view field)
{
    if (!value.is_object()) {
        return nullptr;
    }
    const auto iterator = value.find(field);
    if (iterator == value.end()) {
        return nullptr;
    }
    return &(*iterator);
}

Result<nlohmann::json> parse(std::string_view text)
{
    // allow_exceptions=false：非法文本返回 discarded 值而不是抛 parse_error
    nlohmann::json parsed = nlohmann::json::parse(text.begin(), text.end(), nullptr, false);
    if (parsed.is_discarded()) {
        return Error(ErrorCode::InvalidData, "JSON: failed to parse document");
    }
    return parsed;
}

Result<nlohmann::json> parse(std::istream& input)
{
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return parse(text);
}

Result<i32> getInt32(const nlohmann::json& value)
{
    if (!value.is_number_integer()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be an integer");
    }
    return value.get<i64>();
}

Result<i32> getInt32(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::number_integer, "an integer", error);
    if (field_value == nullptr) {
        return error;
    }
    return getInt32(*field_value);
}

Result<i64> getInt64(const nlohmann::json& value)
{
    if (!value.is_number_integer()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be an integer");
    }
    return value.get<i64>();
}

Result<i64> getInt64(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::number_integer, "an integer", error);
    if (field_value == nullptr) {
        return error;
    }
    return getInt64(*field_value);
}

Result<f32> getFloat32(const nlohmann::json& value)
{
    if (!value.is_number()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be a number");
    }
    return value.get<f64>();
}

Result<f32> getFloat32(const nlohmann::json& value, std::string_view field)
{
    const auto* field_value = findField(value, field);
    if (field_value == nullptr) {
        return _missingField(field);
    }
    if (!field_value->is_number()) {
        return _typeMismatch(field, "a number");
    }
    return getFloat32(*field_value);
}

Result<f64> getFloat64(const nlohmann::json& value)
{
    if (!value.is_number()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be a number");
    }
    return value.get<f64>();
}

Result<f64> getFloat64(const nlohmann::json& value, std::string_view field)
{
    const auto* field_value = findField(value, field);
    if (field_value == nullptr) {
        return _missingField(field);
    }
    if (!field_value->is_number()) {
        return _typeMismatch(field, "a number");
    }
    return getFloat64(*field_value);
}

Result<bool> getBool(const nlohmann::json& value)
{
    if (!value.is_boolean()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be a boolean");
    }
    return value.get<bool>();
}

Result<bool> getBool(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::boolean, "a boolean", error);
    if (field_value == nullptr) {
        return error;
    }
    return getBool(*field_value);
}

Result<std::string> getString(const nlohmann::json& value)
{
    if (!value.is_string()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be a string");
    }
    return value.get<std::string>();
}

Result<std::string> getString(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::string, "a string", error);
    if (field_value == nullptr) {
        return error;
    }
    return getString(*field_value);
}

Result<const nlohmann::json&> getObject(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::object, "an object", error);
    if (field_value == nullptr) {
        return error;
    }
    return *field_value;
}

Result<const nlohmann::json&> getArray(const nlohmann::json& value, std::string_view field)
{
    Error error{ErrorCode::Success};
    const auto* field_value = _requireType(value, field, nlohmann::json::value_t::array, "an array", error);
    if (field_value == nullptr) {
        return error;
    }
    return *field_value;
}

} // namespace mc::util::json
