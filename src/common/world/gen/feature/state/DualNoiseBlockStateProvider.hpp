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
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/NoiseStateUtils.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::state {

/**
 * @brief 双噪声方块状态提供者
 *
 * 由慢噪声值决定候选状态数量 i（variety 范围内），构造 i 个候选状态列表，
 * 再由快噪声从候选列表中索引选取。
 */
class DualNoiseBlockStateProvider : public BlockStateProvider {
public:
    DualNoiseBlockStateProvider(u64 seed,
        f32 scale,
        std::unique_ptr<world::gen::noise::NormalNoise> noise,
        InclusiveRange variety,
        f32 slowScale,
        std::unique_ptr<world::gen::noise::NormalNoise> slowNoise,
        std::vector<const BlockState*> states);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    u64 m_seed;
    f32 m_scale;
    std::unique_ptr<world::gen::noise::NormalNoise> m_noise;
    InclusiveRange m_variety;
    f32 m_slowScale;
    std::unique_ptr<world::gen::noise::NormalNoise> m_slowNoise;
    std::vector<const BlockState*> m_states;
};

} // namespace mc::world::gen::feature::state
