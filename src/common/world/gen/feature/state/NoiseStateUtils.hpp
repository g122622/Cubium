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
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include <string>
#include <vector>

namespace mc::world::gen::feature::state {

/// 闭区间范围（仅用于 DualNoiseProvider.variety，1..64）。
struct InclusiveRange {
    i32 minInclusive;
    i32 maxInclusive;
};

namespace noise_state_utils {

/**
 * @brief 噪声驱动提供者的噪声采样值
 *
 * 对应 NoiseBasedStateProvider.getNoiseValue(pos, scale) = noise.getValue(pos*scale, ...)。
 */
[[nodiscard]] inline f64 getNoiseValue(const world::gen::noise::NormalNoise& noise, f32 scale, i32 x, i32 y, i32 z)
{
    const f64 s = static_cast<f64>(scale);
    return noise.getValue(static_cast<f64>(x) * s, static_cast<f64>(y) * s, static_cast<f64>(z) * s);
}

/**
 * @brief 由噪声值从状态列表中选取一个状态
 *
 * 对应 NoiseProvider.getRandomState(list, noiseValue)：
 * clamp((1+d0)/2, 0, 0.9999) * size 取索引。
 */
[[nodiscard]] inline const BlockState* getRandomStateByNoise(
    const std::vector<const BlockState*>& states, f64 noiseValue)
{
    const f64 d0 = math::clamp((1.0 + noiseValue) / 2.0, 0.0, 0.9999);
    const size_t idx = static_cast<size_t>(d0 * static_cast<f64>(states.size()));
    return states[idx < states.size() ? idx : states.size() - 1];
}

/**
 * @brief 按名查找方块状态上的 IntegerProperty
 *
 * 对应 RandomizedIntStateProvider.findProperty：先在方块的 stateContainer 中按名取
 * IProperty，再 dynamic_cast 为 IntegerProperty。
 */
[[nodiscard]] inline const IntegerProperty* findIntegerProperty(const BlockState& state, const std::string& name)
{
    const IProperty* prop = state.getBlock().stateContainer().getProperty(name);
    return prop != nullptr ? dynamic_cast<const IntegerProperty*>(prop) : nullptr;
}

} // namespace noise_state_utils

} // namespace mc::world::gen::feature::state
