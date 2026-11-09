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

#include "common/core/Types.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 以「附魔等级」为自变量的数值表达式
 *
 * 对应原版 `LevelBasedValue` 接口及其七个实现。数据包在多处用它表达
 * "数值随等级怎么变"，例如战利品条件的 `enchanted_chance`、附魔效果的
 * `value`。各形态可以嵌套（clamped 包住 fraction、exponent 的底数与指数
 * 本身也可以是表达式）。
 *
 * 使用方式：
 * @code
 * auto value = LevelBasedValue::linear(0.1f, 0.05f);
 * f32 chance = value->calculate(level);
 * @endcode
 */
class LevelBasedValue {
public:
    virtual ~LevelBasedValue() = default;

    /**
     * @brief 按等级求值
     *
     * @param level 附魔等级（原版约定从 1 开始；等级 0 也会传入，由各形态自行处理）
     * @return 求值结果
     */
    [[nodiscard]] virtual f32 calculate(i32 level) const = 0;

    /**
     * @brief 复制自身
     *
     * 表达式是树形结构，深拷贝才能安全共享。
     */
    [[nodiscard]] virtual std::unique_ptr<LevelBasedValue> clone() const = 0;

    // ========== 各形态的工厂方法 ==========

    /// @brief 常量：恒为 `value`，与等级无关。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> constant(f32 value);

    /// @brief 线性：`base + perLevelAboveFirst * (level - 1)`。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> linear(f32 base, f32 perLevelAboveFirst);

    /// @brief 等级平方：`level² + added`。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> levelsSquared(f32 added);

    /// @brief 钳制：把 `value` 的求值结果限制在 `[min, max]`。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> clamped(
        std::unique_ptr<LevelBasedValue> value, f32 min, f32 max);

    /// @brief 分数：`numerator / denominator`；分母为 0 时返回 0。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> fraction(
        std::unique_ptr<LevelBasedValue> numerator, std::unique_ptr<LevelBasedValue> denominator);

    /// @brief 指数：`pow(base, power)`。
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> exponent(
        std::unique_ptr<LevelBasedValue> base, std::unique_ptr<LevelBasedValue> power);

    /**
     * @brief 查表：`level <= values.size()` 时取 `values[level - 1]`，否则用 `fallback`。
     *
     * @param values 逐级取值表（下标 0 对应等级 1）
     * @param fallback 超出表长时的回退表达式
     */
    [[nodiscard]] static std::unique_ptr<LevelBasedValue> lookup(
        std::vector<f32> values, std::unique_ptr<LevelBasedValue> fallback);
};

} // namespace loot
} // namespace mc
