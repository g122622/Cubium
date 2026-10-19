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

#pragma once

#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"
#include "common/world/attribute/EnvironmentAttributeValueJson.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 为 JSON 解析准备一个值类型实例
 *
 * 可默认构造的类型直接构造；不可默认构造的值对象（如 Activity）由类型自身提供的
 * 占位构造完成——解析器随后整体赋值。
 */
template <typename Value>
[[nodiscard]] Value constructJsonValue()
{
    if constexpr (std::is_default_constructible_v<Value>) {
        return Value{};
    } else {
        // 不可默认构造的值对象（如 Activity）：以空键构造占位，解析器随后整体赋值
        return Value{std::string()};
    }
}

template <typename Value>
[[nodiscard]] std::function<Result<void>(AttributeModifierOperation operation, const void* jsonArgument, void* map)>
makeJsonArgumentApplier(const EnvironmentAttribute<Value>& attribute)
{
    // 经函数指针取得特化定义（见 EnvironmentAttributeValueJson.cpp）：直接调用模板会让编译器
    // 在调用点按主模板实例化，特化永远不生效。
    using ReaderFn = std::function<bool(const void* json, Value& out)> (*)();
    static constexpr ReaderFn READER_FACTORY = &valueJsonReader<Value>;
    if (READER_FACTORY == nullptr) {
        return {};
    }

    // 值类型未必可默认构造（如 Activity）：解析器直接构造最终参数
    return [&attribute, reader = READER_FACTORY()](
               AttributeModifierOperation operation, const void* jsonArgument, void* map) -> Result<void> {
        Value argument = constructJsonValue<Value>();
        if (!reader(jsonArgument, argument)) {
            return Error(ErrorCode::InvalidData, "Invalid JSON argument for environment attribute");
        }
        static_cast<EnvironmentAttributeMap::Builder*>(map)->modify(attribute, operation, std::move(argument));
        return Result<void>::ok();
    };
}

} // namespace attribute
} // namespace world
} // namespace mc
