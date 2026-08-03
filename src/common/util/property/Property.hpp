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

#include "IProperty.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

namespace detail {
// 辅助类型特征：检测是否为 std::vector<bool>
template <typename T>
struct is_vector_bool : std::false_type {};

template <typename Alloc>
struct is_vector_bool<std::vector<bool, Alloc>> : std::true_type {};
} // namespace detail

/**
 * @brief 类型安全的属性模板基类
 *
 * @tparam T 属性值类型，必须支持比较操作
 *
 * 参考: net.minecraft.state.Property<T extends Comparable<T>>
 *
 * 用法示例:
 * @code
 * auto prop = BooleanProperty::create("lit");
 * bool value = state.get(prop);
 * const BlockState& newState = state.with(prop, true);
 * @endcode
 *
 * 注意:
 * - 属性创建后不应被修改
 * - 属性应该被复用，通常通过静态成员或预定义属性访问
 * - 属性比较时使用指针比较，确保使用同一个实例
 */
template <typename T>
class Property : public IProperty {
public:
    using ValueType = T;

    // 对于 std::vector<bool> 特化，返回值类型；其他类型返回引用
    using ValueReturnType = std::conditional_t<detail::is_vector_bool<std::vector<T>>::value, T, const T&>;

    /**
     * @brief 获取属性名称
     */
    [[nodiscard]] const std::string& name() const override { return m_name; }

    /**
     * @brief 获取允许的值的数量
     */
    [[nodiscard]] size_t valueCount() const override { return m_values.size(); }

    /**
     * @brief 获取所有允许的值
     */
    [[nodiscard]] const std::vector<T>& allowedValues() const { return m_values; }

    /**
     * @brief 查找值的索引
     * @param value 要查找的值
     * @return 索引，如果不存在返回nullopt
     */
    [[nodiscard]] std::optional<size_t> indexOf(const T& value) const
    {
        auto it = m_valueToIndex.find(value);
        if (it != m_valueToIndex.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief 获取指定索引的值
     * @param index 索引
     * @return 值，如果索引越界抛出异常
     * @note 对于 bool 类型返回值而非引用，因为 std::vector<bool> 特化
     */
    [[nodiscard]] ValueReturnType valueAt(size_t index) const
    {
        if (index >= m_values.size()) {
            throw std::out_of_range("Property value index out of range: " + std::to_string(index));
        }
        return m_values[index];
    }

    /**
     * @brief 将值转换为字符串表示
     * @param value 值
     * @return 字符串表示
     */
    [[nodiscard]] virtual std::string valueToString(const T& value) const = 0;

    /**
     * @brief 将值索引转换为字符串表示
     */
    [[nodiscard]] std::string valueToString(size_t index) const override { return valueToString(valueAt(index)); }

    /**
     * @brief 解析字符串为值
     * @param str 字符串
     * @return 解析后的值，失败返回nullopt
     */
    [[nodiscard]] virtual std::optional<T> parse(std::string_view str) const = 0;

    /**
     * @brief 解析字符串为值索引（实现IProperty接口）
     */
    [[nodiscard]] std::optional<size_t> parseValue(std::string_view str) const override
    {
        auto value = parse(str);
        if (value) {
            return indexOf(*value);
        }
        return std::nullopt;
    }

    /**
     * @brief 比较两个属性是否相等
     *
     * 属性相等的条件：
     * 1. 名称相同
     * 2. 类型相同
     * 3. 允许的值集合相同
     */
    [[nodiscard]] bool equals(const IProperty& other) const override
    {
        if (this == &other) return true;
        if (typeName() != other.typeName()) return false;
        if (name() != other.name()) return false;

        // 尝试转换并比较值集合
        const auto* typedOther = dynamic_cast<const Property<T>*>(&other);
        if (!typedOther) return false;

        return m_values == typedOther->m_values;
    }

protected:
    /**
     * @brief 构造属性
     * @param name 属性名称
     * @param values 允许的值列表
     */
    Property(std::string name, std::vector<T> values)
        : m_name(std::move(name))
        , m_values(std::move(values))
    {
        // 构建值到索引的映射
        for (size_t i = 0; i < m_values.size(); ++i) {
            m_valueToIndex[m_values[i]] = i;
        }
    }

    std::string m_name;
    std::vector<T> m_values;
    std::unordered_map<T, size_t> m_valueToIndex;
};

} // namespace mc
