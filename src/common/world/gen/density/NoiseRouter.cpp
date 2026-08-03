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

#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::density {

NoiseRouter::NoiseRouter(std::unique_ptr<DensityFunction> barrierNoise,
    std::unique_ptr<DensityFunction> fluidLevelFloodednessNoise,
    std::unique_ptr<DensityFunction> fluidLevelSpreadNoise,
    std::unique_ptr<DensityFunction> lavaNoise,
    std::unique_ptr<DensityFunction> temperature,
    std::unique_ptr<DensityFunction> vegetation,
    std::unique_ptr<DensityFunction> continents,
    std::unique_ptr<DensityFunction> erosion,
    std::unique_ptr<DensityFunction> depth,
    std::unique_ptr<DensityFunction> ridges,
    std::unique_ptr<DensityFunction> preliminarySurfaceLevel,
    std::unique_ptr<DensityFunction> finalDensity,
    std::unique_ptr<DensityFunction> veinToggle,
    std::unique_ptr<DensityFunction> veinRidged,
    std::unique_ptr<DensityFunction> veinGap)
    : m_barrierNoise(std::move(barrierNoise))
    , m_fluidLevelFloodednessNoise(std::move(fluidLevelFloodednessNoise))
    , m_fluidLevelSpreadNoise(std::move(fluidLevelSpreadNoise))
    , m_lavaNoise(std::move(lavaNoise))
    , m_temperature(std::move(temperature))
    , m_vegetation(std::move(vegetation))
    , m_continents(std::move(continents))
    , m_erosion(std::move(erosion))
    , m_depth(std::move(depth))
    , m_ridges(std::move(ridges))
    , m_preliminarySurfaceLevel(std::move(preliminarySurfaceLevel))
    , m_finalDensity(std::move(finalDensity))
    , m_veinToggle(std::move(veinToggle))
    , m_veinRidged(std::move(veinRidged))
    , m_veinGap(std::move(veinGap))
{
    MC_ASSERT_RELEASE(m_barrierNoise != nullptr);
    MC_ASSERT_RELEASE(m_fluidLevelFloodednessNoise != nullptr);
    MC_ASSERT_RELEASE(m_fluidLevelSpreadNoise != nullptr);
    MC_ASSERT_RELEASE(m_lavaNoise != nullptr);
    MC_ASSERT_RELEASE(m_temperature != nullptr);
    MC_ASSERT_RELEASE(m_vegetation != nullptr);
    MC_ASSERT_RELEASE(m_continents != nullptr);
    MC_ASSERT_RELEASE(m_erosion != nullptr);
    MC_ASSERT_RELEASE(m_depth != nullptr);
    MC_ASSERT_RELEASE(m_ridges != nullptr);
    MC_ASSERT_RELEASE(m_preliminarySurfaceLevel != nullptr);
    MC_ASSERT_RELEASE(m_finalDensity != nullptr);
    MC_ASSERT_RELEASE(m_veinToggle != nullptr);
    MC_ASSERT_RELEASE(m_veinRidged != nullptr);
    MC_ASSERT_RELEASE(m_veinGap != nullptr);
}

mc::world::biome::climate::Sampler NoiseRouter::createClimateSampler() const
{
    return mc::world::biome::climate::Sampler(
        *m_temperature, *m_vegetation, *m_continents, *m_erosion, *m_depth, *m_ridges);
}

void NoiseRouter::mapAll(DensityFunction::Visitor& visitor)
{
    DensityFunction::applyInPlace(m_barrierNoise, visitor);
    DensityFunction::applyInPlace(m_fluidLevelFloodednessNoise, visitor);
    DensityFunction::applyInPlace(m_fluidLevelSpreadNoise, visitor);
    DensityFunction::applyInPlace(m_lavaNoise, visitor);
    DensityFunction::applyInPlace(m_temperature, visitor);
    DensityFunction::applyInPlace(m_vegetation, visitor);
    DensityFunction::applyInPlace(m_continents, visitor);
    DensityFunction::applyInPlace(m_erosion, visitor);
    DensityFunction::applyInPlace(m_depth, visitor);
    DensityFunction::applyInPlace(m_ridges, visitor);
    DensityFunction::applyInPlace(m_preliminarySurfaceLevel, visitor);
    DensityFunction::applyInPlace(m_finalDensity, visitor);
    DensityFunction::applyInPlace(m_veinToggle, visitor);
    DensityFunction::applyInPlace(m_veinRidged, visitor);
    DensityFunction::applyInPlace(m_veinGap, visitor);
}

std::unique_ptr<DensityFunction> NoiseRouter::extractFinalDensity()
{
    return std::move(m_finalDensity);
}

void NoiseRouter::replaceFinalDensity(std::unique_ptr<DensityFunction> density)
{
    MC_ASSERT_RELEASE(density != nullptr);
    m_finalDensity = std::move(density);
}

} // namespace mc::world::gen::density
