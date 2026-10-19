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

#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/AttributeRange.hpp"
#include "common/world/attribute/AttributeType.hpp"

#include <functional>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 环境属性
 *
 * MC 1.21.11 class EnvironmentAttribute<Value>。
 * 定义一个环境属性的默认值、值范围、同步/位置/空间插值标志与属性类型。
 *
 * 属性实例必须是静态对象：环境属性系统以属性地址为键索引各属性，
 * 而属性是否计入网络同步也与地址集合相关。
 */
template <typename Value>
class EnvironmentAttribute {
public:
    /// 该属性在注册表中的名字（`minecraft:<path>`）
    using NameProvider = std::function<const std::string&()>;

    EnvironmentAttribute(NameProvider name,
        AttributeType<Value> type,
        Value defaultValue,
        AttributeRange<Value> valueRange,
        bool isSyncable,
        bool isPositional,
        bool isSpatiallyInterpolated)
        : m_name(std::move(name))
        , m_type(std::move(type))
        , m_defaultValue(std::move(defaultValue))
        , m_valueRange(std::move(valueRange))
        , m_isSyncable(isSyncable)
        , m_isPositional(isPositional)
        , m_isSpatiallyInterpolated(isSpatiallyInterpolated)
    {}

    /**
     * @brief 属性的注册名
     *
     * 惰性解析：构造时属性尚未登记进注册表，只有在注册完成后调用才有效。
     */
    [[nodiscard]] const std::string& name() const { return m_name(); }

    const AttributeType<Value>& type() const noexcept { return m_type; }
    const Value& defaultValue() const noexcept { return m_defaultValue; }
    bool isSyncable() const noexcept { return m_isSyncable; }
    bool isPositional() const noexcept { return m_isPositional; }
    bool isSpatiallyInterpolated() const noexcept { return m_isSpatiallyInterpolated; }

    Value sanitizeValue(Value value) const { return m_valueRange.sanitize(std::move(value)); }

    /**
     * @brief 环境属性构建器
     *
     * 默认值在构造时传入：Value 未必具备默认构造函数（如 Activity 为值对象类型），
     * 因此不提供无参构造。
     */
    class Builder {
    public:
        Builder(NameProvider name, AttributeType<Value> type, Value defaultValue)
            : m_name(std::move(name))
            , m_type(std::move(type))
            , m_defaultValue(std::move(defaultValue))
        {}

        Builder& valueRange(AttributeRange<Value> range)
        {
            m_valueRange = std::move(range);
            return *this;
        }

        Builder& syncable()
        {
            m_isSyncable = true;
            return *this;
        }

        Builder& notPositional()
        {
            m_isPositional = false;
            return *this;
        }

        Builder& spatiallyInterpolated()
        {
            m_isSpatiallyInterpolated = true;
            return *this;
        }

        EnvironmentAttribute<Value> build()
        {
            return EnvironmentAttribute<Value>(std::move(m_name),
                std::move(m_type),
                std::move(m_defaultValue),
                std::move(m_valueRange),
                m_isSyncable,
                m_isPositional,
                m_isSpatiallyInterpolated);
        }

    private:
        NameProvider m_name;
        AttributeType<Value> m_type;
        Value m_defaultValue;
        AttributeRange<Value> m_valueRange = AttributeRange<Value>::any();
        bool m_isSyncable = false;
        bool m_isPositional = true;
        bool m_isSpatiallyInterpolated = false;
    };

    static Builder builder(NameProvider name, AttributeType<Value> type, Value defaultValue)
    {
        return Builder(std::move(name), std::move(type), std::move(defaultValue));
    }

private:
    NameProvider m_name;
    AttributeType<Value> m_type;
    Value m_defaultValue;
    AttributeRange<Value> m_valueRange;
    bool m_isSyncable;
    bool m_isPositional;
    bool m_isSpatiallyInterpolated;
};

} // namespace attribute
} // namespace world
} // namespace mc