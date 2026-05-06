/**
 * @file RandomRanges.hpp
 * @brief 随机值范围工具类
 *
 * 提供不同类型的随机值生成器，用于掉落表、实体属性等场景。
 * 参考: net.minecraft.loot.RandomValueRange, BinomialRange, ConstantRange
 */

#pragma once

#include "common/core/Types.hpp"
#include "Random.hpp"

namespace mc {
namespace math {

/**
 * @brief 随机值范围
 *
 * 用于掉落表中的数量范围。
 * 参考: net.minecraft.loot.RandomValueRange
 */
class RandomValueRange {
public:
    RandomValueRange() : m_min(0.0f), m_max(0.0f) {}
    RandomValueRange(f32 value) : m_min(value), m_max(value) {}
    RandomValueRange(f32 min, f32 max) : m_min(min), m_max(max) {}

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 getMin() const { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 getMax() const { return m_max; }

    /**
     * @brief 生成随机浮点数
     */
    [[nodiscard]] f32 generateFloat(Random& random) const {
        if (m_min == m_max) {
            return m_min;
        }
        return random.nextFloat(m_min, m_max);
    }

    /**
     * @brief 生成随机整数
     */
    [[nodiscard]] i32 generateInt(Random& random) const {
        if (m_min == m_max) {
            return static_cast<i32>(m_min);
        }
        return random.nextInt(static_cast<i32>(m_min), static_cast<i32>(m_max));
    }

    /**
     * @brief 是否为固定值
     */
    [[nodiscard]] bool isFixed() const { return m_min == m_max; }

    bool operator==(const RandomValueRange& other) const {
        return m_min == other.m_min && m_max == other.m_max;
    }

    bool operator!=(const RandomValueRange& other) const {
        return !(*this == other);
    }

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
class BinomialRange {
public:
    BinomialRange(i32 n, f32 p) : m_n(n), m_p(p) {}

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
    [[nodiscard]] i32 generateInt(Random& random) const;

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
class ConstantRange {
public:
    explicit ConstantRange(i32 value) : m_value(value) {}

    /**
     * @brief 获取值
     */
    [[nodiscard]] i32 getValue() const { return m_value; }

    /**
     * @brief 生成随机整数（固定值）
     */
    [[nodiscard]] i32 generateInt(Random& /*random*/) const { return m_value; }

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
