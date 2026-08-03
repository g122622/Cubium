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

#include "DualNoiseBlockStateProvider.hpp"

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/NoiseStateUtils.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::feature::state {

DualNoiseBlockStateProvider::DualNoiseBlockStateProvider(u64 seed,
    f32 scale,
    std::unique_ptr<world::gen::noise::NormalNoise> noise,
    InclusiveRange variety,
    f32 slowScale,
    std::unique_ptr<world::gen::noise::NormalNoise> slowNoise,
    std::vector<const BlockState*> states)
    : m_seed(seed)
    , m_scale(scale)
    , m_noise(std::move(noise))
    , m_variety(variety)
    , m_slowScale(slowScale)
    , m_slowNoise(std::move(slowNoise))
    , m_states(std::move(states))
{}

const BlockState* DualNoiseBlockStateProvider::getState(
    const IWorld& /*world*/, math::IRandom& /*random*/, i32 x, i32 y, i32 z) const
{
    if (m_slowNoise == nullptr || m_noise == nullptr || m_states.empty()) {
        return nullptr;
    }
    // 慢噪声决定候选数量 i = clampedMap(slowNoiseValue, -1, 1, variety.min, variety.max+1)
    const f64 slowValue = noise_state_utils::getNoiseValue(*m_slowNoise, m_slowScale, x, y, z);
    const i32 i = static_cast<i32>(math::clampedMap(
        slowValue, -1.0, 1.0, static_cast<f64>(m_variety.minInclusive), static_cast<f64>(m_variety.maxInclusive + 1)));
    // 构造 i 个候选状态，每个用偏移位置的慢噪声值从 states 选取。
    std::vector<const BlockState*> list;
    list.reserve(static_cast<size_t>(i > 0 ? i : 0));
    for (i32 j = 0; j < i; ++j) {
        const BlockPos offsetPos(x + j * 54545, y, z + j * 34234);
        const f64 nv =
            noise_state_utils::getNoiseValue(*m_slowNoise, m_slowScale, offsetPos.x, offsetPos.y, offsetPos.z);
        list.push_back(noise_state_utils::getRandomStateByNoise(m_states, nv));
    }
    if (list.empty()) {
        return m_states[0];
    }
    // 用快噪声从候选列表索引选取。
    const f64 noiseValue = noise_state_utils::getNoiseValue(*m_noise, m_scale, x, y, z);
    return noise_state_utils::getRandomStateByNoise(list, noiseValue);
}

std::unique_ptr<BlockStateProvider> DualNoiseBlockStateProvider::clone() const
{
    return std::make_unique<DualNoiseBlockStateProvider>(m_seed,
        m_scale,
        m_noise ? m_noise->clone() : nullptr,
        m_variety,
        m_slowScale,
        m_slowNoise ? m_slowNoise->clone() : nullptr,
        m_states);
}

} // namespace mc::world::gen::feature::state
