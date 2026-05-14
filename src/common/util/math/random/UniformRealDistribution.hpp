#pragma once

#include "../../../core/Types.hpp"
#include "IRandom.hpp"

namespace mc::math {

/**
 * @brief 均匀实数分布生成器
 *
 * 避免每次调用都创建 std::uniform_real_distribution。
 * 提供便捷的浮点数范围随机数生成。
 *
 * 使用方法：
 * @code
 * UniformRealDistribution dist(0.0f, 1.0f);
 *
 * // 使用已有随机数生成器
 * Random rng(seed);
 * f32 value = dist(rng);
 *
 * // 或者便捷方法（内部创建临时生成器）
 * f32 value = dist.generate();
 * @endcode
 */
class UniformRealDistribution {
public:
    /**
     * @brief 构造均匀实数分布
     * @param min 最小值（包含）
     * @param max 最大值（不包含）
     */
    explicit UniformRealDistribution(f32 min = 0.0f, f32 max = 1.0f)
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 使用指定的随机数生成器生成随机数
     * @param random 随机数生成器
     * @return [min, max) 范围内的随机浮点数
     */
    [[nodiscard]] f32 operator()(IRandom& random) const { return random.nextFloat(m_min, m_max); }

    /**
     * @brief 创建一个新的 Random 并生成随机数（便捷方法）
     * @return [min, max) 范围内的随机浮点数
     *
     * @note 不推荐在高频调用中使用，会创建新的生成器
     */
    [[nodiscard]] f32 generate() const;

    /**
     * @brief 设置最小值
     */
    void setMin(f32 min) { m_min = min; }

    /**
     * @brief 设置最大值
     */
    void setMax(f32 max) { m_max = max; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 min() const { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 max() const { return m_max; }

private:
    f32 m_min;
    f32 m_max;
};

} // namespace mc::math
