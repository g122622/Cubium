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

#include <bitset>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>

namespace mc {

/**
 * @brief 枚举集合类
 *
 * 用于存储和操作枚举值的集合，基于 std::bitset 实现。
 * 模板参数 E 必须是枚举类型，且枚举值从 0 开始连续。
 *
 * @tparam E 枚举类型
 */
template <typename E>
class EnumSet {
public:
    static_assert(std::is_enum_v<E>, "EnumSet requires an enum type");

    using EnumType = E;
    using UnderlyingType = std::underlying_type_t<E>;
    static constexpr size_t Size = static_cast<size_t>(E::Count);

    /**
     * @brief 默认构造，创建空集合
     */
    EnumSet()
        : m_bits{}
    {}

    /**
     * @brief 从初始化列表构造
     */
    EnumSet(std::initializer_list<E> values)
        : m_bits{}
    {
        for (E value : values) {
            set(value);
        }
    }

    /**
     * @brief 设置一个枚举值
     */
    EnumSet& set(E value)
    {
        m_bits.set(static_cast<size_t>(value));
        return *this;
    }

    /**
     * @brief 清除一个枚举值
     */
    EnumSet& reset(E value)
    {
        m_bits.reset(static_cast<size_t>(value));
        return *this;
    }

    /**
     * @brief 切换一个枚举值
     */
    EnumSet& flip(E value)
    {
        m_bits.flip(static_cast<size_t>(value));
        return *this;
    }

    /**
     * @brief 检查是否包含某个枚举值
     */
    [[nodiscard]] bool test(E value) const { return m_bits.test(static_cast<size_t>(value)); }

    /**
     * @brief 检查是否包含某个枚举值（运算符版本）
     */
    [[nodiscard]] bool operator[](E value) const { return m_bits[static_cast<size_t>(value)]; }

    /**
     * @brief 清空所有值
     */
    EnumSet& clear()
    {
        m_bits.reset();
        return *this;
    }

    /**
     * @brief 设置所有值
     */
    EnumSet& setAll()
    {
        m_bits.set();
        return *this;
    }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return m_bits.none(); }

    /**
     * @brief 检查是否有任何值
     */
    [[nodiscard]] bool any() const { return m_bits.any(); }

    /**
     * @brief 检查是否包含所有值
     */
    [[nodiscard]] bool all() const { return m_bits.all(); }

    /**
     * @brief 计算值的数量
     */
    [[nodiscard]] size_t count() const { return m_bits.count(); }

    /**
     * @brief 获取大小
     */
    [[nodiscard]] constexpr size_t size() const { return Size; }

    /**
     * @brief 并集运算
     */
    EnumSet& operator|=(const EnumSet& other)
    {
        m_bits |= other.m_bits;
        return *this;
    }

    /**
     * @brief 交集运算
     */
    EnumSet& operator&=(const EnumSet& other)
    {
        m_bits &= other.m_bits;
        return *this;
    }

    /**
     * @brief 差集运算
     */
    EnumSet& operator-=(const EnumSet& other)
    {
        m_bits &= ~other.m_bits;
        return *this;
    }

    /**
     * @brief 异或运算
     */
    EnumSet& operator^=(const EnumSet& other)
    {
        m_bits ^= other.m_bits;
        return *this;
    }

    /**
     * @brief 并集
     */
    [[nodiscard]] EnumSet operator|(const EnumSet& other) const
    {
        EnumSet result = *this;
        result |= other;
        return result;
    }

    /**
     * @brief 交集
     */
    [[nodiscard]] EnumSet operator&(const EnumSet& other) const
    {
        EnumSet result = *this;
        result &= other;
        return result;
    }

    /**
     * @brief 差集
     */
    [[nodiscard]] EnumSet operator-(const EnumSet& other) const
    {
        EnumSet result = *this;
        result -= other;
        return result;
    }

    /**
     * @brief 异或
     */
    [[nodiscard]] EnumSet operator^(const EnumSet& other) const
    {
        EnumSet result = *this;
        result ^= other;
        return result;
    }

    /**
     * @brief 取反
     */
    [[nodiscard]] EnumSet operator~() const
    {
        EnumSet result;
        result.m_bits = ~m_bits;
        return result;
    }

    /**
     * @brief 相等比较
     */
    [[nodiscard]] bool operator==(const EnumSet& other) const { return m_bits == other.m_bits; }

    /**
     * @brief 不等比较
     */
    [[nodiscard]] bool operator!=(const EnumSet& other) const { return m_bits != other.m_bits; }

    /**
     * @brief 检查是否与另一个集合有交集
     */
    [[nodiscard]] bool intersects(const EnumSet& other) const { return (m_bits & other.m_bits).any(); }

    /**
     * @brief 检查是否包含另一个集合
     */
    [[nodiscard]] bool contains(const EnumSet& other) const { return (m_bits & other.m_bits) == other.m_bits; }

    /**
     * @brief 遍历所有设置的枚举值
     * @tparam Func 可调用类型
     * @param func 对每个枚举值调用的函数
     */
    template <typename Func>
    void forEach(Func&& func) const
    {
        for (size_t i = 0; i < Size; ++i) {
            if (m_bits[i]) {
                func(static_cast<E>(i));
            }
        }
    }

    /**
     * @brief 转换为底层位集
     */
    [[nodiscard]] const std::bitset<Size>& bits() const { return m_bits; }

private:
    std::bitset<Size> m_bits;
};

} // namespace mc

// 为标准库提供 EnumSet 的哈希支持
namespace std {
template <typename E>
struct hash<mc::EnumSet<E>> {
    size_t operator()(const mc::EnumSet<E>& set) const noexcept
    {
        return std::hash<std::bitset<mc::EnumSet<E>::Size>>{}(set.bits());
    }
};
} // namespace std
