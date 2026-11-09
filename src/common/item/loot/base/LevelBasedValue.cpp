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

#include "common/item/loot/base/LevelBasedValue.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mc {
namespace loot {

namespace {

// ============================================================================
// 各形态实现
// ============================================================================
//
// 每个形态只负责自己的求值；嵌套由成员表达式递归完成。

/// @brief 常量形态：与等级无关。
class ConstantValue final : public LevelBasedValue {
public:
    explicit ConstantValue(f32 value)
        : m_value(value)
    {}

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        (void)level;
        return m_value;
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<ConstantValue>(m_value);
    }

private:
    f32 m_value;
};

/// @brief 线性形态：base + perLevelAboveFirst * (level - 1)。
class LinearValue final : public LevelBasedValue {
public:
    LinearValue(f32 base, f32 perLevelAboveFirst)
        : m_base(base)
        , m_perLevelAboveFirst(perLevelAboveFirst)
    {}

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        return m_base + m_perLevelAboveFirst * static_cast<f32>(level - 1);
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<LinearValue>(m_base, m_perLevelAboveFirst);
    }

private:
    f32 m_base;
    f32 m_perLevelAboveFirst;
};

/// @brief 等级平方形态：level² + added。
class LevelsSquaredValue final : public LevelBasedValue {
public:
    explicit LevelsSquaredValue(f32 added)
        : m_added(added)
    {}

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        const f32 square = static_cast<f32>(level) * static_cast<f32>(level);
        return square + m_added;
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<LevelsSquaredValue>(m_added);
    }

private:
    f32 m_added;
};

/// @brief 钳制形态：把内部表达式的值限制在 [min, max]。
class ClampedValue final : public LevelBasedValue {
public:
    ClampedValue(std::unique_ptr<LevelBasedValue> value, f32 min, f32 max)
        : m_value(std::move(value))
        , m_min(min)
        , m_max(max)
    {
        MC_ASSERT_RELEASE(m_value != nullptr);
        // 原版钳制区间是 min..max，写反了会让结果恒等于某个边界，属于配置错误。
        MC_ASSERT_RELEASE_MSG(min <= max, "clamped LevelBasedValue requires min <= max");
    }

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        return std::clamp(m_value->calculate(level), m_min, m_max);
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<ClampedValue>(m_value->clone(), m_min, m_max);
    }

private:
    std::unique_ptr<LevelBasedValue> m_value;
    f32 m_min;
    f32 m_max;
};

/// @brief 分数形态：numerator / denominator，分母为 0 时返回 0。
class FractionValue final : public LevelBasedValue {
public:
    FractionValue(std::unique_ptr<LevelBasedValue> numerator, std::unique_ptr<LevelBasedValue> denominator)
        : m_numerator(std::move(numerator))
        , m_denominator(std::move(denominator))
    {
        MC_ASSERT_RELEASE(m_numerator != nullptr);
        MC_ASSERT_RELEASE(m_denominator != nullptr);
    }

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        const f32 denominator = m_denominator->calculate(level);
        // 原版对分母为 0 的处理是返回 0 而不是产生 inf/NaN：
        // 数据包里的 unbreaking 分母是 level+1，等级 0 时正好是 0。
        if (denominator == 0.0f) {
            return 0.0f;
        }
        return m_numerator->calculate(level) / denominator;
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<FractionValue>(m_numerator->clone(), m_denominator->clone());
    }

private:
    std::unique_ptr<LevelBasedValue> m_numerator;
    std::unique_ptr<LevelBasedValue> m_denominator;
};

/// @brief 指数形态：pow(base, power)。
class ExponentValue final : public LevelBasedValue {
public:
    ExponentValue(std::unique_ptr<LevelBasedValue> base, std::unique_ptr<LevelBasedValue> power)
        : m_base(std::move(base))
        , m_power(std::move(power))
    {
        MC_ASSERT_RELEASE(m_base != nullptr);
        MC_ASSERT_RELEASE(m_power != nullptr);
    }

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        return static_cast<f32>(std::pow(m_base->calculate(level), m_power->calculate(level)));
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<ExponentValue>(m_base->clone(), m_power->clone());
    }

private:
    std::unique_ptr<LevelBasedValue> m_base;
    std::unique_ptr<LevelBasedValue> m_power;
};

/// @brief 查表形态：等级在表长内取对应项，超出后走回退表达式。
class LookupValue final : public LevelBasedValue {
public:
    LookupValue(std::vector<f32> values, std::unique_ptr<LevelBasedValue> fallback)
        : m_values(std::move(values))
        , m_fallback(std::move(fallback))
    {
        MC_ASSERT_RELEASE(m_fallback != nullptr);
        MC_ASSERT_RELEASE_MSG(!m_values.empty(), "lookup LevelBasedValue requires at least one entry");
    }

    [[nodiscard]] f32 calculate(i32 level) const override
    {
        // 等级从 1 开始，下标 0 对应等级 1。等级 <= 0 时不查表，直接走回退，
        // 避免负下标（原版 Math 层会把 0 级当成越界）。
        if (level >= 1 && static_cast<size_t>(level) <= m_values.size()) {
            return m_values[static_cast<size_t>(level - 1)];
        }
        return m_fallback->calculate(level);
    }

    [[nodiscard]] std::unique_ptr<LevelBasedValue> clone() const override
    {
        return std::make_unique<LookupValue>(m_values, m_fallback->clone());
    }

private:
    std::vector<f32> m_values;
    std::unique_ptr<LevelBasedValue> m_fallback;
};

} // namespace

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<LevelBasedValue> LevelBasedValue::constant(f32 value)
{
    return std::make_unique<ConstantValue>(value);
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::linear(f32 base, f32 perLevelAboveFirst)
{
    return std::make_unique<LinearValue>(base, perLevelAboveFirst);
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::levelsSquared(f32 added)
{
    return std::make_unique<LevelsSquaredValue>(added);
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::clamped(
    std::unique_ptr<LevelBasedValue> value, f32 min, f32 max)
{
    MC_ASSERT_RELEASE(value != nullptr);
    return std::make_unique<ClampedValue>(std::move(value), min, max);
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::fraction(
    std::unique_ptr<LevelBasedValue> numerator, std::unique_ptr<LevelBasedValue> denominator)
{
    MC_ASSERT_RELEASE(numerator != nullptr);
    MC_ASSERT_RELEASE(denominator != nullptr);
    return std::make_unique<FractionValue>(std::move(numerator), std::move(denominator));
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::exponent(
    std::unique_ptr<LevelBasedValue> base, std::unique_ptr<LevelBasedValue> power)
{
    MC_ASSERT_RELEASE(base != nullptr);
    MC_ASSERT_RELEASE(power != nullptr);
    return std::make_unique<ExponentValue>(std::move(base), std::move(power));
}

std::unique_ptr<LevelBasedValue> LevelBasedValue::lookup(
    std::vector<f32> values, std::unique_ptr<LevelBasedValue> fallback)
{
    MC_ASSERT_RELEASE(fallback != nullptr);
    return std::make_unique<LookupValue>(std::move(values), std::move(fallback));
}

} // namespace loot
} // namespace mc