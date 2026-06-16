/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the the following conditions:
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

#include "SimplexNoise.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::noise {

/**
 * @brief 多倍频 Simplex 噪声
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.synth.PerlinSimplexNoise
 * 将多个不同倍频的 SimplexNoise 叠加，产生分形噪声。
 *
 * 与 PerlinNoise 的区别：
 * - PerlinNoise 使用 ImprovedNoise (Perlin 噪声) 作为倍频层
 * - PerlinSimplexNoise 使用 SimplexNoise 作为倍频层
 * - PerlinSimplexNoise 只支持 2D 采样（getValue(x, y, useOffset)）
 * - PerlinSimplexNoise 的构造方式不同：负倍频从主噪声派生种子
 *
 * 用于 Biome 温度噪声（TEMPERATURE_NOISE, FROZEN_TEMPERATURE_NOISE, BIOME_INFO_NOISE）。
 */
class PerlinSimplexNoise {
public:
    /**
     * @brief 使用随机源和倍频索引列表构造
     *
     * 参考 MC 1.21.11: PerlinSimplexNoise(RandomSource, List<Integer>)
     *
     * @param rng 随机源（调用者应使用 LegacyRandomSource 或等效实现）
     * @param octaves 倍频索引列表（如 {-2, -1, 0} 或 {0}）
     */
    PerlinSimplexNoise(math::IRandom& rng, std::vector<i32> octaves);

    ~PerlinSimplexNoise() = default;

    PerlinSimplexNoise(const PerlinSimplexNoise&) = delete;
    PerlinSimplexNoise& operator=(const PerlinSimplexNoise&) = delete;
    PerlinSimplexNoise(PerlinSimplexNoise&&) noexcept = default;
    PerlinSimplexNoise& operator=(PerlinSimplexNoise&&) noexcept = default;

    /**
     * @brief 采样 2D 噪声值
     *
     * 参考 MC 1.21.11: PerlinSimplexNoise.getValue(double, double, boolean)
     *
     * @param x X 坐标
     * @param y Y 坐标（MC 中通常传入 z 坐标）
     * @param useOffset 是否使用各倍频层的随机偏移
     * @return 噪声值，范围约 [-1, 1]
     */
    [[nodiscard]] f64 getValue(f64 x, f64 y, bool useOffset) const;

    /**
     * @brief 获取第一个倍频层的 SimplexNoise（用于种子派生）
     *
     * 返回 noiseLevels 中最高倍频的 SimplexNoise，
     * 与 MC 的 PerlinSimplexNoise.firstNoise 等价。
     */
    [[nodiscard]] const SimplexNoise* firstNoise() const
    {
        if (m_noiseLevels.empty()) {
            return nullptr;
        }
        return m_noiseLevels[0].get();
    }

private:
    /// 倍频层 SimplexNoise 数组（索引 0 对应最低倍频）
    std::vector<std::unique_ptr<SimplexNoise>> m_noiseLevels;

    /// 最高频率输入因子 = 2^maxOctave
    f64 m_highestFreqInputFactor = 0.0;

    /// 最高频率值因子 = 1.0 / (2^totalOctaves - 1)
    f64 m_highestFreqValueFactor = 0.0;
};

} // namespace mc::world::gen::noise
