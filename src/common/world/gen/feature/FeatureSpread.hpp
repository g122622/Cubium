#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"

namespace mc {

/**
 * @brief 特征扩散配置
 *
 * 用于定义特征生成时的范围扩散。
 * 包含基础值和随机扩散值。
 *
 * 参考: net.minecraft.world.gen.feature.FeatureSpread
 */
class FeatureSpread {
public:
    /**
     * @brief 构造固定值扩散
     * @param value 固定值
     */
    static FeatureSpread fixed(i32 value) { return FeatureSpread(value, 0); }

    /**
     * @brief 构造随机扩散
     * @param base 基础值
     * @param spread 扩散范围（0到spread的随机值）
     */
    static FeatureSpread spread(i32 base, i32 spread) { return FeatureSpread(base, spread); }

    /**
     * @brief 默认构造（值为0）
     */
    FeatureSpread()
        : m_base(0)
        , m_spread(0)
    {}

    /**
     * @brief 构造扩散配置
     * @param base 基础值
     * @param spread 扩散范围
     */
    FeatureSpread(i32 base, i32 spread)
        : m_base(base)
        , m_spread(spread)
    {}

    /**
     * @brief 获取随机值
     * @param random 随机数生成器
     * @return base + random(0, spread)
     */
    [[nodiscard]] i32 get(math::Random& random) const;

    /**
     * @brief 获取基础值
     */
    [[nodiscard]] i32 base() const noexcept { return m_base; }

    /**
     * @brief 获取扩散范围
     */
    [[nodiscard]] i32 spread() const noexcept { return m_spread; }

private:
    i32 m_base;
    i32 m_spread;
};

} // namespace mc
