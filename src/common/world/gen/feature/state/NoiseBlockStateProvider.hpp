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
#include "common/world/gen/noise/NormalNoise.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::state {

/**
 * @brief 噪声方块状态提供者
 *
 * 由快噪声值映射到 states 列表索引选取方块状态。
 */
class NoiseBlockStateProvider : public BlockStateProvider {
public:
    NoiseBlockStateProvider(u64 seed,
        f32 scale,
        std::unique_ptr<world::gen::noise::NormalNoise> noise,
        std::vector<const BlockState*> states);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    u64 m_seed;
    f32 m_scale;
    std::unique_ptr<world::gen::noise::NormalNoise> m_noise;
    std::vector<const BlockState*> m_states;
};

} // namespace mc::world::gen::feature::state
