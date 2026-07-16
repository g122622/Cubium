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

#include "common/world/gen/density/NoiseRouterData.hpp"
#include "common/core/Constants.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/CaveDensityFunctions.hpp"
#include "common/world/gen/density/TerrainProvider.hpp"

namespace mc::world::gen::density {

// ============================================================================
// 辅助：将数组转为 vector
// ============================================================================

template <size_t N>
static std::vector<f64> toVector(const f64 (&arr)[N])
{
    return std::vector<f64>(arr, arr + N);
}

// ============================================================================
// NoiseRouterData 实现
// ============================================================================

NoiseRouterData::ClimateFunctions NoiseRouterData::createOverworldClimate(
    const RandomState& rs, u64 seed, bool largeBiomes)
{
    ClimateFunctions climate;

    // 共享版：shiftA/shiftB/shiftedNoise2d 均从 RandomState 缓存获取 NormalNoise
    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto shiftX = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    const i32 tempOctave = largeBiomes ? TEMPERATURE_LARGE_FIRST_OCTAVE : TEMPERATURE_FIRST_OCTAVE;
    const auto tempAmps = largeBiomes ? toVector(TEMPERATURE_LARGE_AMPLITUDES) : toVector(TEMPERATURE_AMPLITUDES);

    const i32 vegOctave = largeBiomes ? VEGETATION_LARGE_FIRST_OCTAVE : VEGETATION_FIRST_OCTAVE;
    const auto vegAmps = largeBiomes ? toVector(VEGETATION_LARGE_AMPLITUDES) : toVector(VEGETATION_AMPLITUDES);

    const i32 contOctave = largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE;
    const auto contAmps =
        largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES);

    const i32 eroOctave = largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE;
    const auto eroAmps = largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES);

    climate.temperature = factory::shiftedNoise2d(
        rs, std::move(shiftX), std::move(shiftZ), 0.25, seed ^ 0x11111111ULL, tempOctave, tempAmps);

    auto shiftX2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.vegetation = factory::shiftedNoise2d(
        rs, std::move(shiftX2), std::move(shiftZ2), 0.25, seed ^ 0x22222222ULL, vegOctave, vegAmps);

    auto shiftX3 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ3 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.continents = factory::flatCacheMarker(factory::shiftedNoise2d(
        rs, std::move(shiftX3), std::move(shiftZ3), 0.25, seed ^ 0x33333333ULL, contOctave, contAmps));

    auto shiftX4 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ4 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.erosion = factory::flatCacheMarker(factory::shiftedNoise2d(
        rs, std::move(shiftX4), std::move(shiftZ4), 0.25, seed ^ 0x44444444ULL, eroOctave, eroAmps));

    climate.depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 1.5, -1.5);

    auto shiftX5 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ5 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.ridges = factory::flatCacheMarker(factory::shiftedNoise2d(rs,
        std::move(shiftX5),
        std::move(shiftZ5),
        0.25,
        seed ^ 0x55555555ULL,
        RIDGE_FIRST_OCTAVE,
        toVector(RIDGE_AMPLITUDES)));

    return climate;
}

std::unique_ptr<DensityFunction> NoiseRouterData::peaksAndValleys(std::unique_ptr<DensityFunction> ridges)
{
    // MC 1.21: mul(-3, add(abs(add(abs(ridges), -2/3)), -1/3))
    // 注意：不使用 squeeze，直接是 (abs(abs(ridges) - 2/3) - 1/3) * -3
    auto absRidges = factory::abs(std::move(ridges));
    auto shifted = factory::add(std::move(absRidges), factory::constant(-2.0 / 3.0));
    auto absShifted = factory::abs(std::move(shifted));
    auto sub = factory::add(std::move(absShifted), factory::constant(-1.0 / 3.0));
    return factory::mul(factory::constant(-3.0), std::move(sub));
}

// ============================================================================
// slide / postProcess / noNewCaves
// ============================================================================

std::unique_ptr<DensityFunction> NoiseRouterData::slide(std::unique_ptr<DensityFunction> input,
    i32 startY,
    i32 height,
    i32 topSlideFromTop,
    i32 topSlideToTop,
    f64 topSlideTarget,
    i32 bottomSlideFromBottom,
    i32 bottomSlideToBottom,
    f64 bottomSlideTarget)
{
    // MC 1.21: slide 函数在维度顶部和底部应用 Y 方向渐变
    // 顶部: Y 从 (startY+height-topSlideFromTop) 到 (startY+height-topSlideToTop) 从 1.0 渐变到 0.0
    // 底部: Y 从 (startY+bottomSlideFromBottom) 到 (startY+bottomSlideToBottom) 从 0.0 渐变到 1.0

    auto topGradient =
        factory::yClampedGradient(startY + height - topSlideFromTop, startY + height - topSlideToTop, 1.0, 0.0);
    auto afterTop = factory::lerp(std::move(topGradient), factory::constant(topSlideTarget), std::move(input));

    auto bottomGradient =
        factory::yClampedGradient(startY + bottomSlideFromBottom, startY + bottomSlideToBottom, 0.0, 1.0);
    return factory::lerp(std::move(bottomGradient), factory::constant(bottomSlideTarget), std::move(afterTop));
}

std::unique_ptr<DensityFunction> NoiseRouterData::slideOverworld(std::unique_ptr<DensityFunction> input, bool amplified)
{
    // MC 1.21.11: slideOverworld(amplified, input)
    //   slide(input, -64, 384,
    //         amplified ? 16 : 80,   // topSlideFromTop
    //         amplified ? 0  : 64,   // topSlideToTop
    //         -0.078125,             // topSlideTarget
    //         0, 24,                 // bottomSlideFromBottom, bottomSlideToBottom
    //         amplified ? 0.4 : 0.1171875)  // bottomSlideTarget
    return slide(std::move(input),
        world::MIN_BUILD_HEIGHT,
        world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT,
        amplified ? 16 : 80,
        amplified ? 0 : 64,
        -0.078125,
        0,
        24,
        amplified ? 0.4 : 0.1171875);
}

std::unique_ptr<DensityFunction> NoiseRouterData::slideNetherLike(
    std::unique_ptr<DensityFunction> input, i32 startY, i32 height)
{
    // MC 1.21: nether slide 参数
    // topSlide: fromTop=24, toTop=0, target=0.9375
    // bottomSlide: fromBottom=-8, toBottom=24, target=2.5
    return slide(std::move(input), startY, height, 24, 0, 0.9375, -8, 24, 2.5);
}

std::unique_ptr<DensityFunction> NoiseRouterData::slideEndLike(
    std::unique_ptr<DensityFunction> input, i32 startY, i32 height)
{
    // MC 1.21: end slide 参数
    // topSlide: fromTop=72, toTop=-184, target=-23.4375
    // bottomSlide: fromBottom=4, toBottom=32, target=-0.234375
    return slide(std::move(input), startY, height, 72, -184, -23.4375, 4, 32, -0.234375);
}

std::unique_ptr<DensityFunction> NoiseRouterData::remap(
    std::unique_ptr<DensityFunction> input, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax)
{
    // MC 1.21.11: NoiseRouterData.remap
    //   d0 = (toMax - toMin) / (fromMax - fromMin)
    //   d1 = toMin - fromMin * d0
    //   result = add(mul(x, constant(d0)), constant(d1))
    const f64 d0 = (toMax - toMin) / (fromMax - fromMin);
    const f64 d1 = toMin - fromMin * d0;
    return factory::add(factory::mul(std::move(input), factory::constant(d0)), factory::constant(d1));
}

std::unique_ptr<DensityFunction> NoiseRouterData::offsetToDepth(std::unique_ptr<DensityFunction> offset)
{
    // MC 1.21.11: NoiseRouterData.offsetToDepth(x)
    //   = add(yClampedGradient(-64, 320, 1.5, -1.5), x)
    return factory::add(
        factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 1.5, -1.5), std::move(offset));
}

std::unique_ptr<DensityFunction> NoiseRouterData::preliminarySurfaceLevel(
    std::shared_ptr<DensityFunction> offset, std::shared_ptr<DensityFunction> factor, bool amplified)
{
    // MC 1.21.11: NoiseRouterData.preliminarySurfaceLevel(offset, factor, amplified)
    //   densityfunction  = cache2d(factor)
    //   densityfunction1 = cache2d(offset)
    //   densityfunction2 (upperBound) =
    //       remap(add(mul(0.2734375, factor.invert()), mul(-1.0, offset)), 1.5, -1.5, -64.0, 320.0)
    //       .clamp(-40.0, 320.0)
    //   densityfunction3 (density) =
    //       add(slideOverworld(amplified,
    //             add(noiseGradientDensity(factor, offsetToDepth(offset)), -0.703125).clamp(-64.0, 64.0)),
    //           -0.390625)
    //   return findTopSurface(density, upperBound, -64, cellHeight)
    //
    // 注意：Java 版使用 DensityFunction 引用共享（Holder），factor/offset 被 cache2d 包装后
    // 在 densityfunction2 和 densityfunction3 中各引用一次。C++ 使用 shared_ptr 实现共享。
    // cache2d 在 C++ 中对应 cache2DMarker（由 NoiseChunk::wrap() 替换为实际 Cache2D）。
    auto factorCached = factory::cache2DMarker(factory::sharedHolder(factor));
    auto offsetCached = factory::cache2DMarker(factory::sharedHolder(offset));

    // upperBound = remap(add(mul(0.2734375, factor.invert()), mul(-1.0, offset)), 1.5, -1.5, -64, 320).clamp(-40, 320)
    auto factorInverted = factory::invert(factory::sharedHolder(factor));
    auto term1 = factory::mul(factory::constant(0.2734375), std::move(factorInverted));
    auto term2 = factory::mul(factory::constant(-1.0), factory::sharedHolder(offset));
    auto upperRaw = remap(factory::add(std::move(term1), std::move(term2)), 1.5, -1.5, -64.0, 320.0);
    auto upperBound = factory::clamp(std::move(upperRaw), -40.0, 320.0);

    // density = add(slideOverworld(amplified,
    //                add(noiseGradientDensity(factorCached, offsetToDepth(offsetCached)), -0.703125).clamp(-64, 64)),
    //              -0.390625)
    auto depth = offsetToDepth(std::move(offsetCached));
    auto gradient = TerrainProvider::noiseGradientDensity(std::move(factorCached), std::move(depth));
    auto gradientWithOffset = factory::add(std::move(gradient), factory::constant(-0.703125));
    auto clampedGradient = factory::clamp(std::move(gradientWithOffset), -64.0, 64.0);
    auto slid = slideOverworld(std::move(clampedGradient), amplified);
    auto density = factory::add(std::move(slid), factory::constant(-0.390625));

    // cellHeight = NoiseSettings::OVERWORLD_NOISE_SETTINGS.getCellHeight()
    //            = sizeVertical * 4 = 2 * 4 = 8
    return factory::findTopSurface(std::move(density), std::move(upperBound), world::MIN_BUILD_HEIGHT, 8);
}

std::unique_ptr<DensityFunction> NoiseRouterData::postProcess(std::unique_ptr<DensityFunction> input)
{
    // MC 1.21: postProcess(x) = squeeze(interpolated(x) * 0.64)
    // 旧区块混合（blendDensity/Blender）已移除，本项目不兼容旧版存档，直接对输入做插值与缩放。
    auto interpolated = factory::interpolated(std::move(input));
    auto scaled = factory::mul(factory::constant(0.64), std::move(interpolated));
    return factory::squeeze(std::move(scaled));
}

NoiseRouter NoiseRouterData::noNewCaves(const RandomState& rs, u64 seed, std::unique_ptr<DensityFunction> finalDensity)
{
    auto processedDensity = postProcess(std::move(finalDensity));

    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto shiftX = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    auto temperature = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        std::move(shiftX),
        std::move(shiftZ),
        0.25,
        seed ^ 0x11111111ULL,
        TEMPERATURE_FIRST_OCTAVE,
        toVector(TEMPERATURE_AMPLITUDES)));

    auto shiftX2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    auto vegetation = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        std::move(shiftX2),
        std::move(shiftZ2),
        0.25,
        seed ^ 0x22222222ULL,
        VEGETATION_FIRST_OCTAVE,
        toVector(VEGETATION_AMPLITUDES)));

    return NoiseRouter(factory::constant(0.0), // barrierNoise
        factory::constant(0.0),                // fluidLevelFloodednessNoise
        factory::constant(0.0),                // fluidLevelSpreadNoise
        factory::constant(0.0),                // lavaNoise
        std::move(temperature),                // temperature
        std::move(vegetation),                 // vegetation
        factory::constant(0.0),                // continents
        factory::constant(0.0),                // erosion
        factory::constant(0.0),                // depth
        factory::constant(0.0),                // ridges
        factory::constant(0.0),                // preliminarySurfaceLevel
        std::move(processedDensity),           // finalDensity
        factory::constant(0.0),                // veinToggle
        factory::constant(0.0),                // veinRidged
        factory::constant(0.0));               // veinGap
}

NoiseRouter NoiseRouterData::overworld(const RandomState& rs, u64 seed, bool largeBiomes, bool amplified)
{
    // 共享版：所有 NormalNoise 叶子从 RandomState 缓存获取，跨区块复用。
    // 树拓扑与 overworld(seed) 完全一致，仅 m_noise 由 clone 变为 shared_ptr 共享。
    auto climate = createOverworldClimate(rs, seed, largeBiomes);

    // ========== 含水层噪声 ==========
    auto barrierNoise = factory::noise(rs, seed ^ 0xA5100001ULL, -3, {1.0}, 1.0, 0.5);
    auto fluidLevelFloodednessNoise = factory::noise(rs, seed ^ 0xA5100002ULL, -7, {1.0}, 1.0, 0.67);
    auto fluidLevelSpreadNoise = factory::noise(rs, seed ^ 0xA5100003ULL, -5, {1.0}, 1.0, 0.7142857142857143);
    auto lavaNoise = factory::noise(rs, seed ^ 0xA5100004ULL, -1, {1.0}, 1.0, 1.0);

    auto climateForRouter = createOverworldClimate(rs, seed, largeBiomes);

    auto ridgesPV = peaksAndValleys(std::move(climate.ridges));

    auto continentsShared = std::shared_ptr<DensityFunction>(std::move(climate.continents));
    auto erosionShared = std::shared_ptr<DensityFunction>(std::move(climate.erosion));
    auto ridgesPVShared = std::shared_ptr<DensityFunction>(std::move(ridgesPV));

    auto offsetSpline = TerrainProvider::overworldOffset(continentsShared, erosionShared, ridgesPVShared);
    auto offsetWithGlobal = factory::add(factory::constant(TerrainProvider::GLOBAL_OFFSET), std::move(offsetSpline));
    auto offset = TerrainProvider::splineWithBlending(std::move(offsetWithGlobal));
    // offset 同时被 depthPlusOffset 和 preliminarySurfaceLevel 引用，转为 shared_ptr
    auto offsetShared = std::shared_ptr<DensityFunction>(std::move(offset));

    auto continentsForDepthShared = std::shared_ptr<DensityFunction>(std::move(climateForRouter.continents));
    auto erosionForDepthShared = std::shared_ptr<DensityFunction>(std::move(climateForRouter.erosion));
    auto offsetForDepth =
        TerrainProvider::splineWithBlending(factory::add(factory::constant(TerrainProvider::GLOBAL_OFFSET),
            TerrainProvider::overworldOffset(continentsForDepthShared, erosionForDepthShared, ridgesPVShared)));
    climateForRouter.depth = factory::add(std::move(climateForRouter.depth), std::move(offsetForDepth));

    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto ridgesForFactor = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        0.25,
        seed ^ 0x55555555ULL,
        RIDGE_FIRST_OCTAVE,
        toVector(RIDGE_AMPLITUDES)));

    auto weirdnessShared = std::shared_ptr<DensityFunction>(std::move(ridgesForFactor));

    auto erosionForFactor = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        0.25,
        seed ^ 0x44444444ULL,
        largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE,
        largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES)));
    auto erosionForFactorShared = std::shared_ptr<DensityFunction>(std::move(erosionForFactor));

    auto continentsForFactor = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        0.25,
        seed ^ 0x33333333ULL,
        largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE,
        largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES)));
    auto continentsForFactorShared = std::shared_ptr<DensityFunction>(std::move(continentsForFactor));

    auto factorSpline = TerrainProvider::overworldFactor(
        continentsForFactorShared, erosionForFactorShared, weirdnessShared, ridgesPVShared);
    auto factor = TerrainProvider::splineWithBlending(std::move(factorSpline));
    // factor 同时被 slopedCheeseDensity 和 preliminarySurfaceLevel 引用，转为 shared_ptr
    auto factorShared = std::shared_ptr<DensityFunction>(std::move(factor));

    auto continentsForJagged = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        0.25,
        seed ^ 0x33333333ULL,
        largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE,
        largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES)));
    auto erosionForJagged = factory::cache2DMarker(factory::shiftedNoise2d(rs,
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftA(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        factory::flatCacheMarker(
            factory::cache2DMarker(factory::shiftB(rs, shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
        0.25,
        seed ^ 0x44444444ULL,
        largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE,
        largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES)));

    auto continentsForJaggedShared = std::shared_ptr<DensityFunction>(std::move(continentsForJagged));
    auto erosionForJaggedShared = std::shared_ptr<DensityFunction>(std::move(erosionForJagged));

    auto jaggednessSpline = TerrainProvider::overworldJaggedness(
        continentsForJaggedShared, erosionForJaggedShared, weirdnessShared, ridgesPVShared);
    auto jaggedness = TerrainProvider::splineWithBlending(std::move(jaggednessSpline));

    auto depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 1.5, -1.5);
    auto depthPlusOffset = factory::add(std::move(depth), factory::sharedHolder(offsetShared));

    auto jaggedNoise = factory::noise(rs,
        seed ^ 0x77777777ULL,
        -16,
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        1500.0,
        0.0);
    auto jaggedContribution = factory::mul(std::move(jaggedness), factory::halfNegative(std::move(jaggedNoise)));

    auto depthPlusOffsetPlusJagged = factory::add(std::move(depthPlusOffset), std::move(jaggedContribution));

    auto slopedCheeseDensity = TerrainProvider::noiseGradientDensity(
        factory::sharedHolder(factorShared), std::move(depthPlusOffsetPlusJagged));

    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.125, 80.0, 160.0, 8.0);

    auto slopedCheese = factory::add(std::move(slopedCheeseDensity), std::move(base3dNoise));

    auto slopedCheeseShared =
        std::shared_ptr<DensityFunction>(factory::cacheAllInCellMarker(factory::interpolated(std::move(slopedCheese))));

    // --- 洞穴密度 ---
    auto entrancesFunc = CaveDensityFunctions::entrances(rs, seed);
    auto pillarsFunc = CaveDensityFunctions::pillars(rs, seed);

    auto caveLayer = factory::mul(factory::constant(4.0),
        factory::square(factory::noise(rs,
            seed ^ 0xB0000013ULL,
            CaveDensityFunctions::CAVE_LAYER_OCTAVE,
            toVector(CaveDensityFunctions::CAVE_LAYER_AMPS),
            1.0,
            8.0)));

    auto caveCheese = factory::noise(rs,
        seed ^ 0xB0000014ULL,
        CaveDensityFunctions::CAVE_CHEESE_OCTAVE,
        toVector(CaveDensityFunctions::CAVE_CHEESE_AMPS),
        1.0,
        0.6666666666666666);

    auto caveCheeseFactor =
        factory::add(factory::clamp(factory::add(factory::constant(0.27), std::move(caveCheese)), -1.0, 1.0),
            factory::clamp(factory::add(factory::constant(1.5),
                               factory::mul(factory::constant(-0.64), factory::sharedHolder(slopedCheeseShared))),
                0.0,
                0.5));

    auto caveDensity = factory::add(std::move(caveLayer), std::move(caveCheeseFactor));

    auto spag2d = CaveDensityFunctions::spaghetti2d(rs, seed);
    auto spagRoughness = CaveDensityFunctions::spaghettiRoughness(rs, seed);

    auto minCaveDensityEntrances = factory::min(std::move(caveDensity), std::move(entrancesFunc));
    auto minSpaghettiRoughness =
        factory::min(std::move(minCaveDensityEntrances), factory::add(std::move(spag2d), std::move(spagRoughness)));
    auto pillarCheck = factory::rangeChoice(std::move(pillarsFunc),
        -1000000.0,
        0.03,
        factory::constant(-1000000.0),
        CaveDensityFunctions::pillars(rs, seed));
    auto underground = factory::max(std::move(minSpaghettiRoughness), std::move(pillarCheck));

    auto undergroundShared = std::shared_ptr<DensityFunction>(std::move(underground));

    // --- noodle ---
    auto noodle = CaveDensityFunctions::noodle(rs, seed, world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);

    auto entrancesForRange = CaveDensityFunctions::entrances(rs, seed);
    auto minSlopedCheese5Entrances = factory::min(
        factory::sharedHolder(slopedCheeseShared), factory::mul(factory::constant(5.0), std::move(entrancesForRange)));
    auto whenInRange = factory::min(std::move(minSlopedCheese5Entrances), factory::sharedHolder(undergroundShared));

    auto noCavesOrNoodle = factory::rangeChoice(factory::sharedHolder(slopedCheeseShared),
        -1000000.0,
        1.5625,
        std::move(whenInRange),
        factory::sharedHolder(undergroundShared));

    auto slid = slideOverworld(std::move(noCavesOrNoodle), amplified);
    auto processedDensity = postProcess(std::move(slid));

    auto finalDensity = factory::min(std::move(processedDensity), std::move(noodle));

    // ========== 矿脉噪声 ==========
    auto veinToggle = TerrainProvider::yLimitedInterpolatable(factory::yClampedGradient(world::MIN_BUILD_HEIGHT * 2,
                                                                  world::MAX_BUILD_HEIGHT * 2,
                                                                  static_cast<f64>(world::MIN_BUILD_HEIGHT * 2),
                                                                  static_cast<f64>(world::MAX_BUILD_HEIGHT * 2)),
        factory::noise(rs,
            seed ^ 0xC0000001ULL,
            CaveDensityFunctions::ORE_VEININESS_OCTAVE,
            toVector(CaveDensityFunctions::ORE_VEININESS_AMPS),
            1.5,
            1.5),
        -60,
        50,
        0.0);

    auto veinAYLimited = TerrainProvider::yLimitedInterpolatable(factory::yClampedGradient(world::MIN_BUILD_HEIGHT * 2,
                                                                     world::MAX_BUILD_HEIGHT * 2,
                                                                     static_cast<f64>(world::MIN_BUILD_HEIGHT * 2),
                                                                     static_cast<f64>(world::MAX_BUILD_HEIGHT * 2)),
        factory::noise(rs,
            seed ^ 0xC0000002ULL,
            CaveDensityFunctions::ORE_VEIN_A_OCTAVE,
            toVector(CaveDensityFunctions::ORE_VEIN_A_AMPS),
            4.0,
            4.0),
        -60,
        50,
        0.0);

    auto veinBYLimited = TerrainProvider::yLimitedInterpolatable(factory::yClampedGradient(world::MIN_BUILD_HEIGHT * 2,
                                                                     world::MAX_BUILD_HEIGHT * 2,
                                                                     static_cast<f64>(world::MIN_BUILD_HEIGHT * 2),
                                                                     static_cast<f64>(world::MAX_BUILD_HEIGHT * 2)),
        factory::noise(rs,
            seed ^ 0xC0000003ULL,
            CaveDensityFunctions::ORE_VEIN_B_OCTAVE,
            toVector(CaveDensityFunctions::ORE_VEIN_B_AMPS),
            4.0,
            4.0),
        -60,
        50,
        0.0);

    auto veinRidged = factory::add(factory::constant(-0.08),
        factory::max(factory::abs(std::move(veinAYLimited)), factory::abs(std::move(veinBYLimited))));

    auto veinGap = factory::noise(rs,
        seed ^ 0xC0000004ULL,
        CaveDensityFunctions::ORE_GAP_OCTAVE,
        toVector(CaveDensityFunctions::ORE_GAP_AMPS),
        1.0,
        1.0);

    // MC 1.21.11: preliminarySurfaceLevel(offset, factor, amplified)
    // offset/factor 已转为 shared_ptr，因为它们同时被 slopedCheese 和 preliminarySurfaceLevel 引用。
    auto preliminarySurfaceLevelFunc = preliminarySurfaceLevel(offsetShared, factorShared, amplified);

    return NoiseRouter(std::move(barrierNoise),          // barrierNoise
        std::move(fluidLevelFloodednessNoise),           // fluidLevelFloodednessNoise
        std::move(fluidLevelSpreadNoise),                // fluidLevelSpreadNoise
        std::move(lavaNoise),                            // lavaNoise
        std::move(climate.temperature),                  // temperature
        std::move(climate.vegetation),                   // vegetation
        factory::sharedHolder(continentsForDepthShared), // continents
        factory::sharedHolder(erosionForDepthShared),    // erosion
        std::move(climateForRouter.depth),               // depth
        std::move(climateForRouter.ridges),              // ridges
        std::move(preliminarySurfaceLevelFunc),          // preliminarySurfaceLevel
        std::move(finalDensity),                         // finalDensity
        std::move(veinToggle),                           // veinToggle
        std::move(veinRidged),                           // veinRidged
        std::move(veinGap));                             // veinGap
}

NoiseRouter NoiseRouterData::nether(const RandomState& rs, u64 seed)
{
    // 共享版：仅 temperature/vegetation 的 shift 叶子从缓存获取，BlendedNoise 仍按 seed 重建
    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.375, 80.0, 60.0, 8.0);
    auto slid = slideNetherLike(std::move(base3dNoise), 0, 128);
    return noNewCaves(rs, seed, std::move(slid));
}

NoiseRouter NoiseRouterData::end(const RandomState& rs, u64 seed)
{
    // 末地路径仅使用 BlendedNoise 和 EndIslands，无 NormalNoise 叶子，rs 仅用于 API 统一。
    (void)rs;
    // MC 1.21: end finalDensity = postProcess(slideEndLike(add(endIslands(0L), blendedNoise_end)))
    // 注意: endIslands 使用固定种子 0，不受世界种子影响
    // erosion 槽位使用 cache2d(endIslands(0L))，用于 EndBiomeSource 选择生物群系
    auto endIslandsForErosion = factory::cache2DMarker(factory::endIslands(0));
    auto endIslandsForDensity = factory::endIslands(0);

    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.25, 80.0, 160.0, 4.0);
    auto slopedCheese = factory::add(std::move(endIslandsForDensity), std::move(base3dNoise));
    auto slid = slideEndLike(std::move(slopedCheese), 0, 128);
    auto finalDensity = postProcess(std::move(slid));

    return NoiseRouter(factory::constant(0.0), // barrierNoise
        factory::constant(0.0),                // fluidLevelFloodednessNoise
        factory::constant(0.0),                // fluidLevelSpreadNoise
        factory::constant(0.0),                // lavaNoise
        factory::constant(0.0),                // temperature
        factory::constant(0.0),                // vegetation
        factory::constant(0.0),                // continents
        std::move(endIslandsForErosion),       // erosion = cache2d(endIslands(0L))
        factory::constant(0.0),                // depth
        factory::constant(0.0),                // ridges
        factory::constant(0.0),                // preliminarySurfaceLevel
        std::move(finalDensity),               // finalDensity
        factory::constant(0.0),                // veinToggle
        factory::constant(0.0),                // veinRidged
        factory::constant(0.0));               // veinGap
}

} // namespace mc::world::gen::density
