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

#include "NoiseThresholdBlockStateProvider.hpp"

#include "common/world/gen/feature/state/NoiseStateUtils.hpp"

#include <utility>

namespace mc::world::gen::feature::state {

NoiseThresholdBlockStateProvider::NoiseThresholdBlockStateProvider(u64 seed,
    f32 scale,
    std::unique_ptr<world::gen::noise::NormalNoise> noise,
    f32 threshold,
    f32 highChance,
    const BlockState* defaultState,
    std::vector<const BlockState*> lowStates,
    std::vector<const BlockState*> highStates)
    : m_seed(seed)
    , m_scale(scale)
    , m_noise(std::move(noise))
    , m_threshold(threshold)
    , m_highChance(highChance)
    , m_defaultState(defaultState)
    , m_lowStates(std::move(lowStates))
    , m_highStates(std::move(highStates))
{}

const BlockState* NoiseThresholdBlockStateProvider::getState(
    const IWorld& /*world*/, math::IRandom& random, i32 x, i32 y, i32 z) const
{
    if (m_noise == nullptr) {
        return nullptr;
    }
    const f64 d0 = noise_state_utils::getNoiseValue(*m_noise, m_scale, x, y, z);
    if (d0 < static_cast<f64>(m_threshold)) {
        return m_lowStates[static_cast<size_t>(random.nextInt(static_cast<i32>(m_lowStates.size())))];
    }
    if (random.nextFloat() < m_highChance) {
        return m_highStates[static_cast<size_t>(random.nextInt(static_cast<i32>(m_highStates.size())))];
    }
    return m_defaultState;
}

std::unique_ptr<BlockStateProvider> NoiseThresholdBlockStateProvider::clone() const
{
    // NormalNoise 不可拷贝，但可由 seed + 参数重建（NormalNoise 提供 clone()）。
    return std::make_unique<NoiseThresholdBlockStateProvider>(m_seed,
        m_scale,
        m_noise ? m_noise->clone() : nullptr,
        m_threshold,
        m_highChance,
        m_defaultState,
        m_lowStates,
        m_highStates);
}

} // namespace mc::world::gen::feature::state
