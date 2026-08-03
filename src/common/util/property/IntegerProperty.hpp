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

#include "Property.hpp"
#include "common/core/Types.hpp"
#include "common/util/property/IProperty.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

/**
 * @brief 整数属性
 *
 * 表示一个整数范围的方块状态属性，如 age, power, level 等。
 *
 * 参考: net.minecraft.state.IntegerProperty
 *
 * 用法示例:
 * @code
 * // 创建0-15的整数属性（如红石信号强度）
 * auto power = IntegerProperty::create("power", 0, 15);
 *
 * // 获取值
 * i32 powerLevel = state.get(*power);
 *
 * // 设置值
 * const BlockState& newState = state.with(*power, 10);
 * @endcode
 *
 * 注意:
 * - 最小值必须 >= 0
 * - 最大值必须 > 最小值
 * - 值的数量不能太大，以免状态空间爆炸
 */
class IntegerProperty : public Property<i32> {
public:
    /**
     * @brief 创建整数属性
     * @param name 属性名称
     * @param min 最小值（包含）
     * @param max 最大值（包含）
     * @return 属性实例
     * @throws std::invalid_argument 如果 min < 0 或 max <= min
     */
    [[nodiscard]] static std::unique_ptr<IntegerProperty> create(const std::string& name, i32 min, i32 max)
    {
        if (min < 0) {
            throw std::invalid_argument("Min value of " + name + " must be 0 or greater");
        }
        if (max <= min) {
            throw std::invalid_argument(
                "Max value of " + name + " must be greater than min (" + std::to_string(min) + ")");
        }
        return std::unique_ptr<IntegerProperty>(new IntegerProperty(name, min, max));
    }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] i32 minValue() const noexcept { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] i32 maxValue() const noexcept { return m_max; }

    /**
     * @brief 将整数值转换为字符串
     */
    [[nodiscard]] std::string valueToString(const i32& value) const override { return std::to_string(value); }

    /**
     * @brief 解析字符串为整数值
     */
    [[nodiscard]] std::optional<i32> parse(std::string_view str) const override
    {
        try {
            size_t pos = 0;
            i32 value = std::stoi(std::string(str), &pos);
            if (pos != str.length()) {
                return std::nullopt;
            }
            // 检查值是否在允许范围内
            if (value < m_min || value > m_max) {
                return std::nullopt;
            }
            return value;
        }
        catch (const std::exception&) {
            return std::nullopt;
        }
    }

    /**
     * @brief 计算哈希值
     */
    [[nodiscard]] size_t hashCode() const override
    {
        size_t h = std::hash<std::string>{}(m_name);
        h ^= (std::hash<std::string>{}("IntegerProperty") << 1);
        h ^= (std::hash<i32>{}(m_min) << 2);
        h ^= (std::hash<i32>{}(m_max) << 3);
        return h;
    }

    /**
     * @brief 比较两个属性是否相等
     */
    [[nodiscard]] bool equals(const IProperty& other) const override
    {
        if (!Property<i32>::equals(other)) return false;
        const auto* intOther = dynamic_cast<const IntegerProperty*>(&other);
        if (!intOther) return false;
        return m_min == intOther->m_min && m_max == intOther->m_max;
    }

    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] const char* typeName() const override { return "IntegerProperty"; }

private:
    i32 m_min;
    i32 m_max;

    IntegerProperty(const std::string& name, i32 min, i32 max)
        : Property<i32>(name, generateValues(min, max))
        , m_min(min)
        , m_max(max)
    {}

    static std::vector<i32> generateValues(i32 min, i32 max)
    {
        std::vector<i32> values;
        values.reserve(static_cast<size_t>(max - min + 1));
        for (i32 i = min; i <= max; ++i) {
            values.push_back(i);
        }
        return values;
    }
};

} // namespace mc
