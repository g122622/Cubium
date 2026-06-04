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
#include "common/util/math/random/Random.hpp"

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
    static FeatureSpread fixed(i32 value) noexcept { return FeatureSpread(value, 0); }

    /**
     * @brief 构造随机扩散
     * @param base 基础值
     * @param spread 扩散范围（0到spread的随机值）
     */
    static FeatureSpread spread(i32 base, i32 spread) noexcept { return FeatureSpread(base, spread); }

    /**
     * @brief 默认构造（值为0）
     */
    FeatureSpread() noexcept
        : m_base(0)
        , m_spread(0)
    {}

    /**
     * @brief 构造扩散配置
     * @param base 基础值
     * @param spread 扩散范围
     */
    FeatureSpread(i32 base, i32 spread) noexcept
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
