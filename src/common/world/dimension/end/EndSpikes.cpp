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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "EndSpikes.hpp"

#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace mc {

std::vector<EndSpike> generateSpikes(u64 worldSeed)
{
    std::vector<EndSpike> spikes;

    // 10根柱子，使用种子随机打乱高度/半径索引
    math::Random rng(worldSeed);
    u64 cacheKey = rng.nextLong() & 65535ULL;
    math::Random shuffleRng(static_cast<u64>(cacheKey));

    // 创建索引 0-9 并打乱
    std::vector<i32> indices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(indices.begin(), indices.end(), std::mt19937(static_cast<u32>(cacheKey)));

    // 生成10根柱子
    for (i32 i = 0; i < 10; ++i) {
        // 角度计算：均匀分布在圆周上
        f64 angle = 2.0 * (-mc::math::PI_DOUBLE + (mc::math::PI_DOUBLE / 10.0) * static_cast<f64>(i));

        // 使用半径 42 围绕中心分布
        i32 x = static_cast<i32>(std::floor(42.0 * std::cos(angle)));
        i32 z = static_cast<i32>(std::floor(42.0 * std::sin(angle)));

        i32 idx = indices[i];
        // 半径范围：2-5（根据索引计算）
        i32 radius = 2 + idx / 3;
        // 高度范围：76-103（根据索引计算）
        i32 height = 76 + idx * 3;
        // 某些柱子有铁栏杆笼子保护
        bool guarded = (idx == 1 || idx == 2);

        spikes.emplace_back(x, z, radius, height, guarded);
    }

    return spikes;
}

} // namespace mc
