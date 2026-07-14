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

#include "world/biome/climate/Sampler.hpp"
#include "world/biome/climate/SpawnFinder.hpp"
#include "world/gen/density/DensityFunction.hpp"

namespace mc::world::biome::climate {

Sampler::Sampler(const mc::world::gen::density::DensityFunction& temperature,
    const mc::world::gen::density::DensityFunction& humidity,
    const mc::world::gen::density::DensityFunction& continentalness,
    const mc::world::gen::density::DensityFunction& erosion,
    const mc::world::gen::density::DensityFunction& depth,
    const mc::world::gen::density::DensityFunction& weirdness)
    : m_temperature(&temperature)
    , m_humidity(&humidity)
    , m_continentalness(&continentalness)
    , m_erosion(&erosion)
    , m_depth(&depth)
    , m_weirdness(&weirdness)
{}

TargetPoint Sampler::sample(i32 quartX, i32 quartY, i32 quartZ) const
{
    // quart 坐标转换为方块坐标
    const i32 blockX = quartX << 2;
    const i32 blockY = quartY << 2;
    const i32 blockZ = quartZ << 2;

    return TargetPoint::fromFloats(static_cast<f32>(m_temperature->compute(blockX, blockY, blockZ)),
        static_cast<f32>(m_humidity->compute(blockX, blockY, blockZ)),
        static_cast<f32>(m_continentalness->compute(blockX, blockY, blockZ)),
        static_cast<f32>(m_erosion->compute(blockX, blockY, blockZ)),
        static_cast<f32>(m_depth->compute(blockX, blockY, blockZ)),
        static_cast<f32>(m_weirdness->compute(blockX, blockY, blockZ)));
}

BlockPos Sampler::findSpawnPosition() const
{
    if (m_spawnTarget.empty()) {
        return BlockPos(0, 0, 0);
    }
    return SpawnFinder::findSpawnPosition(m_spawnTarget, *this);
}

} // namespace mc::world::biome::climate
