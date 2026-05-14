/**
 * @file RandomRanges.hpp
 * @brief 随机值范围工具类
 *
 * 提供不同类型的随机值生成器，用于掉落表、实体属性等场景。
 * 参考: net.minecraft.loot.RandomValueRange, BinomialRange, ConstantRange
 */

#pragma once

#include "Random.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace math {

/**
 * @brief 随机范围接口
 *
 * 所有随机范围类型的基类接口，用于多态解析。
 */
class IRandomRange {
public:
    virtual ~IRandomRange() = default;

    /**
     * @brief 生成随机整数
     */
    [[nodiscard]] virtual i32 generateInt(Random& random) const = 0;

    /**
     * @brief 生成随机浮点数
     */
    [[nodiscard]] virtual f32 generateFloat(Random& random) const = 0;

    /**
     * @brief 是否为固定值
     */
    [[nodiscard]] virtual bool isFixed() const = 0;

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] virtual f32 getMin() const = 0;

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] virtual f32 getMax() const = 0;
};

/**
 * @brief 随机值范围
 *
 * 用于掉落表中的数量范围。
 * 参考: net.minecraft.loot.RandomValueRange
 */
class RandomValueRange : public IRandomRange {
public:
    RandomValueRange()
        : m_min(0.0f)
        , m_max(0.0f)
    {}
    RandomValueRange(f32 value)
        : m_min(value)
        , m_max(value)
    {}
    RandomValueRange(f32 min, f32 max)
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 getMin() const override { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 getMax() const override { return m_max; }

    /**
     * @brief 生成随机浮点数
     */
    [[nodiscard]] f32 generateFloat(Random& random) const override
    {
        if (m_min == m_max) {
            return m_min;
        }
        return random.nextFloat(m_min, m_max);
    }

    /**
     * @brief 生成随机整数
     */
    [[nodiscard]] i32 generateInt(Random& random) const override
    {
        if (m_min == m_max) {
            return static_cast<i32>(m_min);
        }
        return random.nextInt(static_cast<i32>(m_min), static_cast<i32>(m_max));
    }

    /**
     * @brief 是否为固定值
     */
    [[nodiscard]] bool isFixed() const override { return m_min == m_max; }

    bool operator==(const RandomValueRange& other) const { return m_min == other.m_min && m_max == other.m_max; }

    bool operator!=(const RandomValueRange& other) const { return !(*this == other); }

private:
    f32 m_min;
    f32 m_max;
};

/**
 * @brief 二项分布范围
 *
 * 使用二项分布生成随机值。
 * 参考: net.minecraft.loot.BinomialRange
 */
class BinomialRange : public IRandomRange {
public:
    BinomialRange(i32 n, f32 p)
        : m_n(n)
        , m_p(p)
    {}

    /**
     * @brief 获取试验次数
     */
    [[nodiscard]] i32 getN() const { return m_n; }

    /**
     * @brief 获取成功概率
     */
    [[nodiscard]] f32 getP() const { return m_p; }

    /**
     * @brief 生成随机整数
     *
     * 使用二项分布：进行n次试验，每次有p的概率成功，返回成功的次数。
     */
    [[nodiscard]] i32 generateInt(Random& random) const override;

    /**
     * @brief 生成随机浮点数
     */
    [[nodiscard]] f32 generateFloat(Random& random) const override { return static_cast<f32>(generateInt(random)); }

    /**
     * @brief 是否为固定值
     */
    [[nodiscard]] bool isFixed() const override { return m_n == 0 || m_p <= 0.0f || m_p >= 1.0f; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 getMin() const override { return 0.0f; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 getMax() const override { return static_cast<f32>(m_n); }

private:
    i32 m_n;
    f32 m_p;
};

/**
 * @brief 固定值范围
 *
 * 总是返回固定值。
 * 参考: net.minecraft.loot.ConstantRange
 */
class ConstantRange : public IRandomRange {
public:
    explicit ConstantRange(i32 value)
        : m_value(value)
    {}

    /**
     * @brief 获取值
     */
    [[nodiscard]] i32 getValue() const { return m_value; }

    /**
     * @brief 生成随机整数（固定值）
     */
    [[nodiscard]] i32 generateInt(Random& /*random*/) const override { return m_value; }

    /**
     * @brief 生成随机浮点数（固定值）
     */
    [[nodiscard]] f32 generateFloat(Random& /*random*/) const override { return static_cast<f32>(m_value); }

    /**
     * @brief 是否为固定值
     */
    [[nodiscard]] bool isFixed() const override { return true; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 getMin() const override { return static_cast<f32>(m_value); }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 getMax() const override { return static_cast<f32>(m_value); }

private:
    i32 m_value;
};

} // namespace math

// ============================================================================
// 向后兼容别名（保留在 loot 命名空间）
// ============================================================================

namespace loot {
// 使用 math 命名空间中的类
using RandomValueRange = math::RandomValueRange;
using BinomialRange = math::BinomialRange;
using ConstantRange = math::ConstantRange;
} // namespace loot

} // namespace mc
