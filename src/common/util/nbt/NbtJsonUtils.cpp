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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "NbtJsonUtils.hpp"
#include "Nbt.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::nbt {

// ============================================================================
// nbtToJson 实现
// ============================================================================

namespace {

nlohmann::json nbtListToJsonImpl(const tags::list_tag& list);

nlohmann::json nbtToJsonImpl(const tags::tag& tag)
{
    using namespace tags;

    switch (tag.id()) {
        case TagId::Byte: {
            const auto& t = static_cast<const byte_tag&>(tag);
            return t.value;
        }
        case TagId::Short: {
            const auto& t = static_cast<const short_tag&>(tag);
            return t.value;
        }
        case TagId::Int: {
            const auto& t = static_cast<const int_tag&>(tag);
            return t.value;
        }
        case TagId::Long: {
            const auto& t = static_cast<const long_tag&>(tag);
            return t.value;
        }
        case TagId::Float: {
            const auto& t = static_cast<const float_tag&>(tag);
            return static_cast<double>(t.value);
        }
        case TagId::Double: {
            const auto& t = static_cast<const double_tag&>(tag);
            return t.value;
        }
        case TagId::ByteArray: {
            const auto& t = static_cast<const bytearray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case TagId::String: {
            const auto& t = static_cast<const string_tag&>(tag);
            return t.value;
        }
        case TagId::List: {
            const auto& t = static_cast<const list_tag&>(tag);
            return nbtListToJsonImpl(t);
        }
        case TagId::Compound: {
            const auto& t = static_cast<const compound_tag&>(tag);
            nlohmann::json result = nlohmann::json::object();
            for (const auto& [key, value] : t.value) {
                if (value) {
                    result[key] = nbtToJsonImpl(*value);
                }
            }
            return result;
        }
        case TagId::IntArray: {
            const auto& t = static_cast<const intarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case TagId::LongArray: {
            const auto& t = static_cast<const longarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case TagId::End:
        default:
            return nullptr;
    }
}

nlohmann::json nbtListToJsonImpl(const tags::list_tag& list)
{
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        auto elem = list[i];
        if (elem) {
            result.push_back(nbtToJsonImpl(*elem));
        }
    }
    return result;
}

} // anonymous namespace

nlohmann::json nbtToJson(const tags::tag& tag)
{
    return nbtToJsonImpl(tag);
}

// ============================================================================
// jsonToNbt 实现
// ============================================================================

namespace {

/**
 * @brief 根据 JSON 值推断最合适的整数 NBT 标签类型
 *
 * MC Java 的 NBT 体系中，整数默认为 Int，但 JSON 中没有类型信息，
 * 因此需要根据值的大小范围推断最合适的类型：
 * - [-128, 127] -> byte_tag
 * - [-32768, 32767] -> short_tag
 * - [-2147483648, 2147483647] -> int_tag
 * - 超出 int 范围 -> long_tag
 */
std::unique_ptr<tags::tag> jsonNumberToNbtTag(const nlohmann::json& json)
{
    using namespace tags;

    // 尝试作为整数处理
    if (json.is_number_integer()) {
        auto val = json.get<i64>();
        if (val >= static_cast<i64>(static_cast<i8>(-128)) && val <= static_cast<i64>(static_cast<i8>(127))) {
            return std::make_unique<byte_tag>(static_cast<i8>(val));
        }
        if (val >= static_cast<i64>(static_cast<i16>(-32768)) && val <= static_cast<i64>(static_cast<i16>(32767))) {
            return std::make_unique<short_tag>(static_cast<i16>(val));
        }
        if (val >= static_cast<i64>(static_cast<i32>(-2147483648)) &&
            val <= static_cast<i64>(static_cast<i32>(2147483647))) {
            return std::make_unique<int_tag>(static_cast<i32>(val));
        }
        return std::make_unique<long_tag>(val);
    }

    // 浮点数
    if (json.is_number_float()) {
        auto val = json.get<f64>();
        // 检查是否可以用 float 精确表示
        if (static_cast<f64>(static_cast<f32>(val)) == val) {
            return std::make_unique<float_tag>(static_cast<f32>(val));
        }
        return std::make_unique<double_tag>(val);
    }

    // 兜底：作为 int 处理
    return std::make_unique<int_tag>(json.get<i32>());
}

std::unique_ptr<tags::tag> jsonValueToNbtTag(const nlohmann::json& json);

std::unique_ptr<tags::compound_tag> jsonObjectToNbt(const nlohmann::json& json)
{
    using namespace tags;
    auto result = std::make_unique<compound_tag>();
    for (const auto& [key, value] : json.items()) {
        if (value.is_object()) {
            auto child = jsonObjectToNbt(value);
            result->value.emplace(key, std::move(child));
        } else if (value.is_array()) {
            auto child = jsonValueToNbtTag(value);
            result->value.emplace(key, std::move(child));
        } else {
            auto child = jsonValueToNbtTag(value);
            result->value.emplace(key, std::move(child));
        }
    }
    return result;
}

std::unique_ptr<tags::tag> jsonArrayToNbt(const nlohmann::json& json)
{
    using namespace tags;

    if (json.empty()) {
        // 空数组 -> 空 list（end_list_tag）
        return std::make_unique<end_list_tag>();
    }

    // 检查第一个元素的类型来决定列表类型
    const auto& first = json[0];

    if (first.is_object()) {
        // compound_list_tag
        auto list = std::make_unique<compound_list_tag>();
        for (const auto& elem : json) {
            if (!elem.is_object()) {
                // 类型不一致，使用 tag_list_tag
                auto tagList = std::make_unique<tag_list_tag>(TagId::Compound);
                for (size_t i = 0; i < list->value.size(); ++i) {
                    tagList->value.push_back(std::make_unique<compound_tag>(std::move(list->value[i])));
                }
                for (size_t i = 0; i < json.size(); ++i) {
                    if (json[i].is_object()) {
                        auto child = jsonObjectToNbt(json[i]);
                        tagList->value.push_back(std::move(child));
                    } else {
                        auto child = jsonValueToNbtTag(json[i]);
                        tagList->value.push_back(std::move(child));
                    }
                }
                return tagList;
            }
            auto child = jsonObjectToNbt(elem);
            list->value.push_back(std::move(*child));
        }
        return list;
    }

    if (first.is_string()) {
        auto list = std::make_unique<string_list_tag>();
        for (const auto& elem : json) {
            if (!elem.is_string()) {
                // 类型不一致，回退到 tag_list_tag
                auto tagList = std::make_unique<tag_list_tag>(TagId::String);
                for (const auto& e : json) {
                    auto child = jsonValueToNbtTag(e);
                    tagList->value.push_back(std::move(child));
                }
                return tagList;
            }
            list->value.push_back(elem.get<std::string>());
        }
        return list;
    }

    if (first.is_number()) {
        // 根据数值类型推断列表类型
        bool allInteger = true;
        bool allFloat = true;
        for (const auto& elem : json) {
            if (!elem.is_number()) {
                allInteger = false;
                allFloat = false;
                break;
            }
            if (elem.is_number_float()) {
                allInteger = false;
            } else {
                allFloat = false;
            }
        }

        if (allInteger) {
            auto list = std::make_unique<int_list_tag>();
            for (const auto& elem : json) {
                list->value.push_back(elem.get<i32>());
            }
            return list;
        }

        if (allFloat) {
            auto list = std::make_unique<double_list_tag>();
            for (const auto& elem : json) {
                list->value.push_back(elem.get<f64>());
            }
            return list;
        }

        // 混合数值，使用 tag_list_tag
        auto tagList = std::make_unique<tag_list_tag>(TagId::Int);
        for (const auto& elem : json) {
            auto child = jsonNumberToNbtTag(elem);
            tagList->value.push_back(std::move(child));
        }
        return tagList;
    }

    // 混合类型，使用 tag_list_tag
    auto tagList = std::make_unique<tag_list_tag>(TagId::End);
    for (const auto& elem : json) {
        auto child = jsonValueToNbtTag(elem);
        tagList->value.push_back(std::move(child));
    }
    return tagList;
}

std::unique_ptr<tags::tag> jsonValueToNbtTag(const nlohmann::json& json)
{
    using namespace tags;

    if (json.is_null()) {
        return std::make_unique<tags::compound_tag>();
    }
    if (json.is_object()) {
        return jsonObjectToNbt(json);
    }
    if (json.is_array()) {
        return jsonArrayToNbt(json);
    }
    if (json.is_string()) {
        return std::make_unique<string_tag>(json.get<std::string>());
    }
    if (json.is_number()) {
        return jsonNumberToNbtTag(json);
    }
    if (json.is_boolean()) {
        return std::make_unique<byte_tag>(json.get<bool>() ? static_cast<i8>(1) : static_cast<i8>(0));
    }

    return std::make_unique<compound_tag>();
}

} // anonymous namespace

std::unique_ptr<tags::compound_tag> jsonToNbt(const nlohmann::json& json)
{
    if (json.is_null() || !json.is_object()) {
        return nullptr;
    }

    if (json.empty()) {
        return std::make_unique<tags::compound_tag>();
    }

    return jsonObjectToNbt(json);
}

// ============================================================================
// parseMojangson 实现
// ============================================================================

std::unique_ptr<tags::compound_tag> parseMojangson(const std::string& mojangsonString)
{
    if (mojangsonString.empty()) {
        return nullptr;
    }

    try {
        std::istringstream iss(mojangsonString);
        iss >> contexts::mojangson;
        auto result = tags::compound_tag::read(iss);
        if (!result || iss.fail()) {
            return nullptr;
        }
        return result;
    }
    catch (const std::exception&) {
        return nullptr;
    }
}

} // namespace mc::nbt
