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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "common/world/gen/density/CaveDensityFunctions.hpp"
#include "common/core/Constants.hpp"
#include "common/world/gen/RandomState.hpp"

namespace mc::world::gen::density {

// ============================================================================
// 辅助函数：将数组转为 vector
// ============================================================================

template <size_t N>
static std::vector<f64> toVector(const f64 (&arr)[N])
{
    return std::vector<f64>(arr, arr + N);
}

// ============================================================================
// spaghetti_2d
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::spaghetti2d(const RandomState& rs, u64 seed)
{
    // MC 1.21 NoiseRouterData:
    // densityFunction  = noise(SPAGHETTI_2D_MODULATOR, 2.0, 1.0)
    // densityFunction1 = weirdScaledSampler(densityFunction, SPAGHETTI_2D, TYPE2)
    // densityFunction2 = mappedNoise(SPAGHETTI_2D_ELEVATION, 0.0, -8, 8.0)
    // densityFunction3 = cacheOnce(mappedNoise(SPAGHETTI_2D_THICKNESS, 2.0, 1.0, -0.6, -1.3))
    // densityFunction4 = abs(densityFunction2 + yClampedGradient(-64, 320, 8.0, -40.0))
    // densityFunction5 = cube(densityFunction4 + densityFunction3)
    // densityFunction6 = densityFunction1 + 0.083 * densityFunction3
    // result = max(densityFunction6, densityFunction5).clamp(-1.0, 1.0)
    auto modulator = factory::noise(
        rs, seed ^ 0xB0000001ULL, SPAGHETTI_2D_MODULATOR_OCTAVE, toVector(SPAGHETTI_2D_MODULATOR_AMPS), 2.0, 1.0);

    auto spag2d = factory::weirdScaledSampler(std::move(modulator),
        rs,
        seed ^ 0xB0000002ULL,
        SPAGHETTI_2D_OCTAVE,
        toVector(SPAGHETTI_2D_AMPS),
        WeirdScaledSamplerType::Type2);

    auto elevation = factory::mappedNoise(rs,
        seed ^ 0xB0000003ULL,
        SPAGHETTI_2D_ELEVATION_OCTAVE,
        toVector(SPAGHETTI_2D_ELEVATION_AMPS),
        1.0,
        0.0,
        -8.0,
        8.0);

    auto thickness = factory::cacheOnce(factory::mappedNoise(rs,
        seed ^ 0xB0000004ULL,
        SPAGHETTI_2D_THICKNESS_OCTAVE,
        toVector(SPAGHETTI_2D_THICKNESS_AMPS),
        2.0,
        1.0,
        -0.6,
        -1.3));

    auto yGradient = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 8.0, -40.0);
    auto df4 = factory::abs(factory::add(std::move(elevation), std::move(yGradient)));

    auto df5 = factory::cube(factory::add(std::move(df4), std::move(thickness)));

    auto thicknessCopy = factory::cacheOnce(factory::mappedNoise(rs,
        seed ^ 0xB0000004ULL,
        SPAGHETTI_2D_THICKNESS_OCTAVE,
        toVector(SPAGHETTI_2D_THICKNESS_AMPS),
        2.0,
        1.0,
        -0.6,
        -1.3));
    auto df6 = factory::add(std::move(spag2d), factory::mul(factory::constant(0.083), std::move(thicknessCopy)));

    return factory::clamp(factory::max(std::move(df6), std::move(df5)), -1.0, 1.0);
}

// ============================================================================
// spaghettiRoughness
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::spaghettiRoughness(const RandomState& rs, u64 seed)
{
    // MC 1.21:
    // roughness = noise(SPAGHETTI_ROUGHNESS)
    // modulator = mappedNoise(SPAGHETTI_ROUGHNESS_MODULATOR, 0.0, -0.1)
    // result = cacheOnce(modulator * (abs(roughness) + constant(-0.4)))
    auto roughness = factory::noise(
        rs, seed ^ 0xB0000005ULL, SPAGHETTI_ROUGHNESS_OCTAVE, toVector(SPAGHETTI_ROUGHNESS_AMPS), 1.0, 1.0);

    auto modulator = factory::mappedNoise(rs,
        seed ^ 0xB0000006ULL,
        SPAGHETTI_ROUGHNESS_MODULATOR_OCTAVE,
        toVector(SPAGHETTI_ROUGHNESS_MODULATOR_AMPS),
        1.0,
        1.0,
        0.0,
        -0.1);

    auto absRoughness = factory::abs(std::move(roughness));
    auto inner = factory::add(std::move(absRoughness), factory::constant(-0.4));
    auto product = factory::mul(std::move(modulator), std::move(inner));

    return factory::cacheOnce(std::move(product));
}

// ============================================================================
// entrances
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::entrances(const RandomState& rs, u64 seed)
{
    // MC 1.21:
    // rarity = cacheOnce(noise(SPAGHETTI_3D_RARITY, 2.0, 1.0))
    // thickness = mappedNoise(SPAGHETTI_3D_THICKNESS, -0.065, -0.088)
    // spag3d1 = weirdScaledSampler(rarity, SPAGHETTI_3D_1, TYPE1)
    // spag3d2 = weirdScaledSampler(rarity, SPAGHETTI_3D_2, TYPE1)
    // spag3d = clamp(add(max(spag3d1, spag3d2), thickness), -1.0, 1.0)
    // roughness = getFunction(SPAGHETTI_ROUGHNESS_FUNCTION)
    // caveEntrance = noise(CAVE_ENTRANCE, 0.75, 0.5)
    // entranceTerm = add(caveEntrance, constant(0.37)) + yClampedGradient(-10, 30, 0.3, 0.0)
    // result = cacheOnce(min(entranceTerm, add(roughness, spag3d)))
    auto rarity = factory::cacheOnce(factory::noise(
        rs, seed ^ 0xB0000007ULL, SPAGHETTI_3D_RARITY_OCTAVE, toVector(SPAGHETTI_3D_RARITY_AMPS), 2.0, 1.0));

    auto thickness = factory::mappedNoise(rs,
        seed ^ 0xB0000008ULL,
        SPAGHETTI_3D_THICKNESS_OCTAVE,
        toVector(SPAGHETTI_3D_THICKNESS_AMPS),
        1.0,
        1.0,
        -0.065,
        -0.088);

    auto rarity2 = factory::cacheOnce(factory::noise(
        rs, seed ^ 0xB0000007ULL, SPAGHETTI_3D_RARITY_OCTAVE, toVector(SPAGHETTI_3D_RARITY_AMPS), 2.0, 1.0));

    auto spag3d1 = factory::weirdScaledSampler(std::move(rarity),
        rs,
        seed ^ 0xB0000009ULL,
        SPAGHETTI_3D_1_OCTAVE,
        toVector(SPAGHETTI_3D_1_AMPS),
        WeirdScaledSamplerType::Type1);

    auto spag3d2 = factory::weirdScaledSampler(std::move(rarity2),
        rs,
        seed ^ 0xB000000AULL,
        SPAGHETTI_3D_2_OCTAVE,
        toVector(SPAGHETTI_3D_2_AMPS),
        WeirdScaledSamplerType::Type1);

    auto spag3d = factory::clamp(
        factory::add(factory::max(std::move(spag3d1), std::move(spag3d2)), std::move(thickness)), -1.0, 1.0);

    auto roughness = spaghettiRoughness(rs, seed);

    auto caveEntrance =
        factory::noise(rs, seed ^ 0xB000000BULL, CAVE_ENTRANCE_OCTAVE, toVector(CAVE_ENTRANCE_AMPS), 0.75, 0.5);

    auto entranceTerm = factory::add(
        factory::add(std::move(caveEntrance), factory::constant(0.37)), factory::yClampedGradient(-10, 30, 0.3, 0.0));

    return factory::cacheOnce(
        factory::min(std::move(entranceTerm), factory::add(std::move(roughness), std::move(spag3d))));
}

// ============================================================================
// noodle
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::noodle(const RandomState& rs, u64 seed, i32 minY, i32 maxY)
{
    // MC 1.21:
    // Y = yClampedGradient(minY*2, maxY*2, minY*2, maxY*2)
    // noodleToggle = yLimitedInterpolatable(Y, noise(NOODLE, 1.0, 1.0), -60, 320, -1)
    // noodleThickness = yLimitedInterpolatable(Y, mappedNoise(NOODLE_THICKNESS, 1.0, 1.0, -0.05, -0.1), -60, 320, 0)
    // noodleRidgeA = yLimitedInterpolatable(Y, noise(NOODLE_RIDGE_A, 2.6667, 2.6667), -60, 320, 0)
    // noodleRidgeB = yLimitedInterpolatable(Y, noise(NOODLE_RIDGE_B, 2.6667, 2.6667), -60, 320, 0)
    // noodleRidge = 1.5 * max(abs(noodleRidgeA), abs(noodleRidgeB))
    // result = rangeChoice(noodleToggle, -1e6, 0, constant(64), add(noodleThickness, noodleRidge))
    auto yFunc = factory::yClampedGradient(minY * 2, maxY * 2, static_cast<f64>(minY * 2), static_cast<f64>(maxY * 2));

    auto noodleToggle = TerrainProvider::yLimitedInterpolatable(
        factory::yClampedGradient(minY * 2, maxY * 2, static_cast<f64>(minY * 2), static_cast<f64>(maxY * 2)),
        factory::noise(rs, seed ^ 0xB000000CULL, NOODLE_OCTAVE, toVector(NOODLE_AMPS), 1.0, 1.0),
        -60,
        320,
        -1.0);

    auto noodleThickness = TerrainProvider::yLimitedInterpolatable(
        factory::yClampedGradient(minY * 2, maxY * 2, static_cast<f64>(minY * 2), static_cast<f64>(maxY * 2)),
        factory::mappedNoise(
            rs, seed ^ 0xB000000DULL, NOODLE_THICKNESS_OCTAVE, toVector(NOODLE_THICKNESS_AMPS), 1.0, 1.0, -0.05, -0.1),
        -60,
        320,
        0.0);

    auto noodleRidgeA = TerrainProvider::yLimitedInterpolatable(
        factory::yClampedGradient(minY * 2, maxY * 2, static_cast<f64>(minY * 2), static_cast<f64>(maxY * 2)),
        factory::noise(rs,
            seed ^ 0xB000000EULL,
            NOODLE_RIDGE_A_OCTAVE,
            toVector(NOODLE_RIDGE_A_AMPS),
            2.6666666666666665,
            2.6666666666666665),
        -60,
        320,
        0.0);

    auto noodleRidgeB = TerrainProvider::yLimitedInterpolatable(
        factory::yClampedGradient(minY * 2, maxY * 2, static_cast<f64>(minY * 2), static_cast<f64>(maxY * 2)),
        factory::noise(rs,
            seed ^ 0xB000000FULL,
            NOODLE_RIDGE_B_OCTAVE,
            toVector(NOODLE_RIDGE_B_AMPS),
            2.6666666666666665,
            2.6666666666666665),
        -60,
        320,
        0.0);

    auto noodleRidge = factory::mul(factory::constant(1.5),
        factory::max(factory::abs(std::move(noodleRidgeA)), factory::abs(std::move(noodleRidgeB))));

    return factory::rangeChoice(std::move(noodleToggle),
        -1000000.0,
        0.0,
        factory::constant(64.0),
        factory::add(std::move(noodleThickness), std::move(noodleRidge)));
}

// ============================================================================
// pillars
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::pillars(const RandomState& rs, u64 seed)
{
    // MC 1.21:
    // pillarNoise = noise(PILLAR, 25.0, 0.3)
    // pillarRareness = mappedNoise(PILLAR_RARENESS, 0.0, -2.0)
    // pillarThickness = mappedNoise(PILLAR_THICKNESS, 0.0, 1.1)
    // combined = add(mul(pillarNoise, constant(2.0)), pillarRareness)
    // result = cacheOnce(mul(combined, cube(pillarThickness)))
    auto pillarNoise = factory::noise(rs, seed ^ 0xB0000010ULL, PILLAR_OCTAVE, toVector(PILLAR_AMPS), 25.0, 0.3);

    auto pillarRareness = factory::mappedNoise(
        rs, seed ^ 0xB0000011ULL, PILLAR_RARENESS_OCTAVE, toVector(PILLAR_RARENESS_AMPS), 1.0, 1.0, 0.0, -2.0);

    auto pillarThickness = factory::mappedNoise(
        rs, seed ^ 0xB0000012ULL, PILLAR_THICKNESS_OCTAVE, toVector(PILLAR_THICKNESS_AMPS), 1.0, 1.0, 0.0, 1.1);

    auto combined =
        factory::add(factory::mul(std::move(pillarNoise), factory::constant(2.0)), std::move(pillarRareness));

    return factory::cacheOnce(factory::mul(std::move(combined), factory::cube(std::move(pillarThickness))));
}

// ============================================================================
// underground
// ============================================================================

std::unique_ptr<DensityFunction> CaveDensityFunctions::underground(const RandomState& rs, u64 seed, i32 minY, i32 maxY)
{
    auto spag2d = spaghetti2d(rs, seed);
    auto roughnessFunc = spaghettiRoughness(rs, seed);
    auto entrancesFunc = entrances(rs, seed);

    auto spaghettiWithRoughness = factory::add(std::move(spag2d), std::move(roughnessFunc));
    auto minEntrances = std::move(entrancesFunc);

    return factory::min(std::move(minEntrances), std::move(spaghettiWithRoughness));
}

} // namespace mc::world::gen::density
