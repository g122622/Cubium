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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <optional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

/**
 * @brief 范围谓词基类
 *
 * 用于表示数值范围条件，如 [min, max]。
 *
 * @tparam T 数值类型（i32, f32, i64等）
 */
template <typename T>
class MinMaxBounds {
public:
    using ValueType = T;

    /**
     * @brief 默认构造（无边界限制）
     */
    MinMaxBounds() = default;

    /**
     * @brief 构造范围
     * @param min 最小值（包含）
     * @param max 最大值（包含）
     */
    MinMaxBounds(std::optional<T> min, std::optional<T> max)
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 检查值是否在范围内
     * @param value 要检查的值
     * @return 是否在范围内
     */
    [[nodiscard]] bool test(T value) const noexcept
    {
        if (m_min.has_value() && value < m_min.value()) {
            return false;
        }
        if (m_max.has_value() && value > m_max.value()) {
            return false;
        }
        return true;
    }

    /**
     * @brief 检查平方值是否在范围内（用于距离计算）
     * @param value 平方值
     * @return 是否在范围内
     */
    [[nodiscard]] bool testSquared(T value) const noexcept
    {
        if (m_min.has_value() && value < m_min.value() * m_min.value()) {
            return false;
        }
        if (m_max.has_value() && value > m_max.value() * m_max.value()) {
            return false;
        }
        return true;
    }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] std::optional<T> getMin() const noexcept { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] std::optional<T> getMax() const noexcept { return m_max; }

    /**
     * @brief 是否为无边界（任何值都满足）
     */
    [[nodiscard]] bool isUnbounded() const noexcept { return !m_min.has_value() && !m_max.has_value(); }

    /**
     * @brief 从JSON解析
     * @param json JSON对象
     * @return 范围谓词
     */
    static MinMaxBounds<T> fromJson(const nlohmann::json& json)
    {
        std::optional<T> min;
        std::optional<T> max;

        if (json.is_object()) {
            if (json.contains("min")) {
                min = json["min"].get<T>();
            }
            if (json.contains("max")) {
                max = json["max"].get<T>();
            }
        } else if (json.is_number()) {
            // 单个数值表示精确匹配
            T value = json.get<T>();
            min = value;
            max = value;
        }

        return MinMaxBounds<T>(min, max);
    }

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const
    {
        nlohmann::json json;
        if (m_min.has_value() && m_max.has_value() && m_min.value() == m_max.value()) {
            // 精确匹配，用单个数值表示
            return m_min.value();
        }
        if (m_min.has_value()) {
            json["min"] = m_min.value();
        }
        if (m_max.has_value()) {
            json["max"] = m_max.value();
        }
        return json;
    }

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建无边界范围
     */
    static MinMaxBounds<T> unbounded() { return MinMaxBounds<T>(); }

    /**
     * @brief 创建精确匹配
     * @param value 精确值
     */
    static MinMaxBounds<T> exactly(T value) { return MinMaxBounds<T>(value, value); }

    /**
     * @brief 创建至少范围
     * @param min 最小值
     */
    static MinMaxBounds<T> atLeast(T min) { return MinMaxBounds<T>(min, std::nullopt); }

    /**
     * @brief 创建至多范围
     * @param max 最大值
     */
    static MinMaxBounds<T> atMost(T max) { return MinMaxBounds<T>(std::nullopt, max); }

    /**
     * @brief 创建范围
     * @param min 最小值
     * @param max 最大值
     */
    static MinMaxBounds<T> between(T min, T max) { return MinMaxBounds<T>(min, max); }

private:
    std::optional<T> m_min;
    std::optional<T> m_max;
};

// ========== 常用类型别名 ==========

using IntBounds = MinMaxBounds<i32>;
using FloatBounds = MinMaxBounds<f32>;
using LongBounds = MinMaxBounds<i64>;
using DoubleBounds = MinMaxBounds<f64>;

} // namespace mc::advancement
