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
 * IMPLIED, INCLUDING BUT BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::command {

/**
 * @brief 单坐标分量
 *
 * 表示命令坐标中的一个分量，可以是绝对值或相对值（~ 前缀）。
 *
 * 对应 MC Java 的 WorldCoordinate (原名为 CoordinateArgument)。
 *
 * 示例:
 * - `100`  → WorldCoordinate(false, 100.0)   — 绝对坐标
 * - `~5`   → WorldCoordinate(true, 5.0)      — 相对坐标，偏移 +5
 * - `~`    → WorldCoordinate(true, 0.0)      — 相对坐标，偏移 0（即当前位置）
 */
class WorldCoordinate {
public:
    WorldCoordinate() = default;

    /**
     * @brief 构造坐标分量
     * @param relative 是否为相对坐标（~ 前缀）
     * @param value 坐标值（绝对值或偏移值）
     */
    WorldCoordinate(bool relative, f64 value)
        : m_relative(relative)
        , m_value(value)
    {}

    /**
     * @brief 是否为相对坐标
     */
    [[nodiscard]] bool isRelative() const noexcept { return m_relative; }

    /**
     * @brief 获取原始值
     */
    [[nodiscard]] f64 value() const noexcept { return m_value; }

    /**
     * @brief 根据基准值计算最终坐标
     * @param baseValue 基准值（相对坐标时为命令源位置，绝对坐标时忽略）
     * @return 最终坐标值
     *
     * 相对坐标: 返回 m_value + baseValue
     * 绝对坐标: 返回 m_value
     */
    [[nodiscard]] f64 get(f64 baseValue) const noexcept { return m_relative ? m_value + baseValue : m_value; }

    /**
     * @brief 获取整数值（用于方块坐标）
     * @param baseValue 基准值
     * @return 取整后的坐标值
     */
    [[nodiscard]] i32 getInt(f64 baseValue) const noexcept { return static_cast<i32>(std::floor(get(baseValue))); }

private:
    bool m_relative = false;
    f64 m_value = 0.0;
};

} // namespace mc::command
