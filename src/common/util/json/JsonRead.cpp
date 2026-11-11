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

#include <cmath>
#include <cstdint>
#include <istream>
#include <iterator>
#include <limits>
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

/// 取数组元素指针；越界或不是数组返回 nullptr
const nlohmann::json* _arrayElement(const nlohmann::json& value, std::size_t index)
{
    if (!value.is_array() || index >= value.size()) {
        return nullptr;
    }
    return &value[index];
}

Error _indexOutOfRange(std::size_t index)
{
    return Error(ErrorCode::InvalidData, "JSON: array index " + std::to_string(index) + " is out of range");
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
    // 先按 i64 取，再自行判定是否落在 int32 内：nlohmann 的 get<i32>() 越界会抛 out_of_range
    const auto wide = getInt64(value);
    if (wide.failed()) {
        return wide.error();
    }

    const i64 number = wide.value();
    if (number < static_cast<i64>(std::numeric_limits<i32>::min()) ||
        number > static_cast<i64>(std::numeric_limits<i32>::max())) {
        return Error(ErrorCode::InvalidData, "JSON: integer is out of int32 range");
    }

    return static_cast<i32>(number);
}

Result<i32> getInt32(const nlohmann::json& value, std::string_view field)
{
    // 非负整数字面量被 nlohmann 解析为 number_unsigned，故这里必须用 is_number_integer() 而非类型相等判断
    const auto* field_value = findField(value, field);
    if (field_value == nullptr) {
        return _missingField(field);
    }
    if (!field_value->is_number_integer()) {
        return _typeMismatch(field, "an integer");
    }
    return getInt32(*field_value);
}

Result<i64> getInt64(const nlohmann::json& value)
{
    // is_number_integer() 对无符号整数同样为真，而 get<i64>() 在超出 int64 时会抛 out_of_range
    if (value.is_number_unsigned()) {
        const u64 number = value.get<u64>();
        if (number > static_cast<u64>(std::numeric_limits<i64>::max())) {
            return Error(ErrorCode::InvalidData, "JSON: integer is out of int64 range");
        }
        return static_cast<i64>(number);
    }

    if (!value.is_number_integer()) {
        return Error(ErrorCode::InvalidData, "JSON: value must be an integer");
    }

    return value.get<i64>();
}

Result<i64> getInt64(const nlohmann::json& value, std::string_view field)
{
    const auto* field_value = findField(value, field);
    if (field_value == nullptr) {
        return _missingField(field);
    }
    if (!field_value->is_number_integer()) {
        return _typeMismatch(field, "an integer");
    }
    return getInt64(*field_value);
}

Result<f32> getFloat32(const nlohmann::json& value)
{
    // 先用 f64 承接，再判定能否安全收敛到 f32，避免数值溢出成 inf
    const auto wide = getFloat64(value);
    if (wide.failed()) {
        return wide.error();
    }

    const f64 number = wide.value();
    if (std::abs(number) > static_cast<f64>(std::numeric_limits<f32>::max())) {
        return Error(ErrorCode::InvalidData, "JSON: number is out of float32 range");
    }

    return static_cast<f32>(number);
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

Result<i32> getInt32At(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    return getInt32(*element);
}

Result<f32> getFloat32At(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    return getFloat32(*element);
}

Result<f64> getFloat64At(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    return getFloat64(*element);
}

Result<bool> getBoolAt(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    return getBool(*element);
}

Result<std::string> getStringAt(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    return getString(*element);
}

Result<const nlohmann::json&> getObjectAt(const nlohmann::json& value, std::size_t index)
{
    const auto* element = _arrayElement(value, index);
    if (element == nullptr) {
        return _indexOutOfRange(index);
    }
    if (!element->is_object()) {
        return Error(ErrorCode::InvalidData, "JSON: array element must be an object");
    }
    return *element;
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
