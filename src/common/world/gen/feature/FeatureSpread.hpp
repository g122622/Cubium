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
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>

namespace mc {

/**
 * @brief 特征扩散配置
 *
 * 用于定义特征生成时的范围扩散。
 *
 * MC 1.21.11 起 FoliagePlacer 的 radius/offset 为 IntProvider（而非旧的
 * FeatureSpread{base,spread}）。本类兼容两种表示：
 * - 旧的 {base, spread}（get() = base + random[0..spread]）；
 * - 新的任意 IntProvider（get() 委托 provider->sample()）。
 * 当 provider 非空时以 IntProvider 为准。
 *
 * 参考: net.minecraft.world.gen.feature.FeatureSpread / 1.21 FoliagePlacer.radius
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
     * @brief 由 IntProvider 构造（MC 1.21 radius/offset 语义）
     */
    static FeatureSpread of(std::unique_ptr<world::gen::valueprovider::IntProvider> provider)
    {
        FeatureSpread fs(0, 0);
        fs.m_provider = std::move(provider);
        return fs;
    }

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

    FeatureSpread(FeatureSpread&&) noexcept = default;
    FeatureSpread& operator=(FeatureSpread&&) noexcept = default;
    FeatureSpread(const FeatureSpread& other)
        : m_base(other.m_base)
        , m_spread(other.m_spread)
        , m_provider(other.m_provider ? other.m_provider->clone() : nullptr)
    {}
    FeatureSpread& operator=(const FeatureSpread& other)
    {
        if (this != &other) {
            m_base = other.m_base;
            m_spread = other.m_spread;
            m_provider = other.m_provider ? other.m_provider->clone() : nullptr;
        }
        return *this;
    }

    /**
     * @brief 获取随机值
     * @param random 随机数生成器
     * @return 持有 IntProvider 时 provider->sample()；否则 base + random(0, spread)
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

    /**
     * @brief 是否持有 IntProvider（MC 1.21 语义）
     */
    [[nodiscard]] bool hasProvider() const noexcept { return m_provider != nullptr; }

private:
    i32 m_base;
    i32 m_spread;
    std::unique_ptr<world::gen::valueprovider::IntProvider> m_provider;
};

} // namespace mc
