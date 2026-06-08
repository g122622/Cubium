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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::world::gen::noise {

/**
 * @brief 2D Simplex 噪声
 *
 * 参考 MC 1.21.11: SimplexNoise.java
 * 用于 EndIslands 密度函数中的岛屿检测噪声。
 *
 * 种子初始化使用 LegacyRandomSource，通过 consumeCount(17292) 前进随机序列，
 * 然后用同一随机源构建 SimplexNoise 的排列表和偏移。
 */
class SimplexNoise {
public:
    /**
     * @brief 从随机源构造 SimplexNoise
     *
     * 参考 MC 1.21.11: SimplexNoise(RandomSource)
     * 初始化排列表和随机偏移。
     *
     * @param rng 随机源（调用者应先 consumeCount(17292) 用于 EndIslands）
     */
    explicit SimplexNoise(math::Random& rng);

    /**
     * @brief 采样 2D Simplex 噪声值
     *
     * 参考 MC 1.21.11: SimplexNoise.getValue(double, double)
     * 输出范围大约为 [-1, 1]。
     */
    [[nodiscard]] f64 getValue(f64 x, f64 y) const;

    /**
     * @brief 采样 3D Simplex 噪声值
     *
     * 参考 MC 1.21.11: SimplexNoise.getValue(double, double, double)
     */
    [[nodiscard]] f64 getValue(f64 x, f64 y, f64 z) const;

private:
    [[nodiscard]] i32 p(i32 index) const;
    [[nodiscard]] static f64 dot(const i32 grad[3], f64 x, f64 y, f64 z);
    [[nodiscard]] f64 getCornerNoise3D(i32 hash, f64 x, f64 y, f64 z, f64 radius) const;

    static constexpr int GRADIENT[16][3] = {{1, 1, 0},
        {-1, 1, 0},
        {1, -1, 0},
        {-1, -1, 0},
        {1, 0, 1},
        {-1, 0, 1},
        {1, 0, -1},
        {-1, 0, -1},
        {0, 1, 1},
        {0, -1, 1},
        {0, 1, -1},
        {0, -1, -1},
        {1, 1, 0},
        {0, -1, 1},
        {-1, 1, 0},
        {0, -1, -1}};

    static constexpr f64 SQRT_3 = 1.7320508075688772;
    static constexpr f64 F2 = 0.5 * (SQRT_3 - 1.0);
    static constexpr f64 G2 = (3.0 - SQRT_3) / 6.0;

    i32 m_p[512];
    f64 m_xOffset = 0.0;
    f64 m_yOffset = 0.0;
    f64 m_zOffset = 0.0;
};

} // namespace mc::world::gen::noise
