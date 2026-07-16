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

#include "NoiseBlockStateProvider.hpp"

#include "common/world/gen/feature/state/NoiseStateUtils.hpp"

#include <utility>

namespace mc::world::gen::feature::state {

NoiseBlockStateProvider::NoiseBlockStateProvider(
    u64 seed, f32 scale, std::unique_ptr<world::gen::noise::NormalNoise> noise, std::vector<const BlockState*> states)
    : m_seed(seed)
    , m_scale(scale)
    , m_noise(std::move(noise))
    , m_states(std::move(states))
{}

const BlockState* NoiseBlockStateProvider::getState(
    const IWorld& /*world*/, math::IRandom& /*random*/, i32 x, i32 y, i32 z) const
{
    if (m_noise == nullptr || m_states.empty()) {
        return nullptr;
    }
    const f64 noiseValue = noise_state_utils::getNoiseValue(*m_noise, m_scale, x, y, z);
    return noise_state_utils::getRandomStateByNoise(m_states, noiseValue);
}

std::unique_ptr<BlockStateProvider> NoiseBlockStateProvider::clone() const
{
    return std::make_unique<NoiseBlockStateProvider>(m_seed, m_scale, m_noise ? m_noise->clone() : nullptr, m_states);
}

} // namespace mc::world::gen::feature::state
