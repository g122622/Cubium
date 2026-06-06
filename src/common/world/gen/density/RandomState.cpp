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

#include "RandomState.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"

namespace mc::world::gen::density {

RandomState::RandomState(u64 seed,
    NoiseRouter router,
    std::unique_ptr<surface::SurfaceRule> surfaceRule,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    i32 noiseMinY,
    i32 noiseHeight)
    : m_seed(seed)
    , m_router(std::move(router))
    , m_climateSampler(m_router.createClimateSampler())
    , m_positionalRandom(_createPositionalRandom(seed))
    , m_aquiferRandom(m_positionalRandom.fromHashOf("aquifer")->forkPositional())
    , m_oreRandom(m_positionalRandom.fromHashOf("ore")->forkPositional())
    , m_defaultBlock(defaultBlock)
    , m_defaultFluid(defaultFluid)
    , m_seaLevel(seaLevel)
    , m_noiseMinY(noiseMinY)
    , m_noiseHeight(noiseHeight)
{
    // 创建 SurfaceSystem（如果有表面规则）
    if (surfaceRule) {
        m_surfaceSystem = std::make_unique<surface::SurfaceSystem>(
            std::move(surfaceRule), defaultBlock, defaultFluid, seaLevel, noiseMinY, noiseHeight, seed);
    }
}

math::PositionalRandomFactory RandomState::_createPositionalRandom(u64 seed)
{
    math::Random seedRng(seed);
    auto xoroshiro = std::make_unique<math::Xoroshiro128ppRandom>(
        static_cast<u64>(seedRng.nextLong()), static_cast<u64>(seedRng.nextLong()));
    return xoroshiro->forkPositional();
}

RandomState::~RandomState() = default;

RandomState::RandomState(RandomState&&) noexcept = default;
RandomState& RandomState::operator=(RandomState&&) noexcept = default;

} // namespace mc::world::gen::density
