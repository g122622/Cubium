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

NoiseRouterData::ClimateFunctions NoiseRouterData::createOverworldClimate(u64 seed, bool largeBiomes)
{
    ClimateFunctions climate;

    // MC 1.21: SHIFT_X = flatCache(cache2D(shiftA(SHIFT)))
    // MC 1.21: SHIFT_Z = flatCache(cache2D(shiftB(SHIFT)))
    // 使用 Marker 包装器而非具体缓存实现，NoiseChunk 构造时会替换为区块特定实现
    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto shiftX = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    // 根据是否大型生物群系选择参数
    const i32 tempOctave = largeBiomes ? TEMPERATURE_LARGE_FIRST_OCTAVE : TEMPERATURE_FIRST_OCTAVE;
    const auto tempAmps = largeBiomes ? toVector(TEMPERATURE_LARGE_AMPLITUDES) : toVector(TEMPERATURE_AMPLITUDES);

    const i32 vegOctave = largeBiomes ? VEGETATION_LARGE_FIRST_OCTAVE : VEGETATION_FIRST_OCTAVE;
    const auto vegAmps = largeBiomes ? toVector(VEGETATION_LARGE_AMPLITUDES) : toVector(VEGETATION_AMPLITUDES);

    const i32 contOctave = largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE;
    const auto contAmps =
        largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES);

    const i32 eroOctave = largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE;
    const auto eroAmps = largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES);

    // MC 1.21: 气候噪声使用 shiftedNoise2d（带 SHIFT_X 和 SHIFT_Z 偏移）
    // shiftedNoise2d(shiftX, shiftZ, xzScale=0.25, seed, firstOctave, amplitudes)
    // 这确保相邻区块的气候参数平滑过渡

    climate.temperature = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX), std::move(shiftZ), 0.25, seed ^ 0x11111111ULL, tempOctave, tempAmps));

    // 需要重新创建 shift 噪声实例用于后续参数
    // MC 中每个 shiftedNoise2d 有自己的 shift 噪声副本，所以需要新建
    // 但由于 SHIFT_X 和 SHIFT_Z 已经被 move，这里需要重新创建
    // 实际上 MC 的做法是先创建 densityfunction (shiftX) 和 densityfunction1 (shiftZ)
    // 然后多次引用它们——我们的 shiftedNoise2d 内部持有 shift 噪声的副本
    // 所以需要重新创建用于后续的 climate 参数

    // 为后续气候参数重新创建偏移
    auto shiftX2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.vegetation = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX2), std::move(shiftZ2), 0.25, seed ^ 0x22222222ULL, vegOctave, vegAmps));

    auto shiftX3 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ3 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.continents = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX3), std::move(shiftZ3), 0.25, seed ^ 0x33333333ULL, contOctave, contAmps));

    auto shiftX4 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ4 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.erosion = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX4), std::move(shiftZ4), 0.25, seed ^ 0x44444444ULL, eroOctave, eroAmps));

    // depth 使用 YClampedGradient（Y 轴线性映射）
    // MC 1.21: fromY=-64, toY=320, fromValue=1.5, toValue=-1.5
    // 这意味着表面 depth≈1.5，高空 depth≈-1.5
    climate.depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 1.5, -1.5);

    // ridges（奇异度）使用 shiftedNoise2d
    auto shiftX5 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ5 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.ridges = factory::cache2DMarker(factory::shiftedNoise2d(std::move(shiftX5),
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
    // MC 1.21: overworld slide 参数
    // startY=-64, height=384
    // topSlide: fromTop=80, toTop=64, target=-0.078125
    // bottomSlide: fromBottom=0, toBottom=24, target=0.1171875
    // amplified 时 toTop=64→64 (不变), 但实际 MC amplified 只修改噪声参数
    (void)amplified; // amplified 通过噪声参数体现，slide 参数不变
    return slide(std::move(input),
        world::MIN_BUILD_HEIGHT,
        world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT,
        80,
        64,
        -0.078125,
        0,
        24,
        0.1171875);
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

std::unique_ptr<DensityFunction> NoiseRouterData::postProcess(std::unique_ptr<DensityFunction> input)
{
    // MC 1.21: postProcess(x) = squeeze(interpolated(blendDensity(x)) * 0.64)
    // blendDensity 暂时为恒等函数（旧区块混合未实现）
    auto blended = std::move(input); // blendDensity = identity for now
    auto interpolated = factory::interpolated(std::move(blended));
    auto scaled = factory::mul(factory::constant(0.64), std::move(interpolated));
    return factory::squeeze(std::move(scaled));
}

NoiseRouter NoiseRouterData::noNewCaves(u64 seed, std::unique_ptr<DensityFunction> finalDensity)
{
    // MC 1.21: noNewCaves 路由器
    // temperature/vegetation 使用 shiftedNoise2d
    // 其余通道为 constant(0.0)
    // finalDensity 经过 postProcess

    auto processedDensity = postProcess(std::move(finalDensity));

    // 偏移噪声（与主世界共享 SHIFT 参数）
    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto shiftX = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    auto temperature = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX), std::move(shiftZ), 0.25, seed ^ 0x11111111ULL, -4, {1.0, 1.0, 1.0, 1.0}));

    auto shiftX2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ2 = factory::flatCacheMarker(
        factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    auto vegetation = factory::cache2DMarker(factory::shiftedNoise2d(
        std::move(shiftX2), std::move(shiftZ2), 0.25, seed ^ 0x22222222ULL, -4, {1.0, 1.0, 1.0, 1.0}));

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

NoiseRouter NoiseRouterData::overworld(u64 seed, bool largeBiomes)
{
    auto climate = createOverworldClimate(seed, largeBiomes);

    // ========== 含水层噪声 ==========
    // MC 1.21: AQUIFER_BARRIER, AQUIFER_FLUID_LEVEL_FLOODEDNESS,
    //          AQUIFER_FLUID_LEVEL_SPREAD, AQUIFER_LAVA
    auto barrierNoise = factory::noise(seed ^ 0xA5100001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.5, 0.0);
    auto fluidLevelFloodednessNoise = factory::noise(seed ^ 0xA5100002ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.67, 0.0);
    auto fluidLevelSpreadNoise =
        factory::noise(seed ^ 0xA5100003ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.7142857142857143, 0.0);
    auto lavaNoise = factory::noise(seed ^ 0xA5100004ULL, -1, {1.0}, 1.0, 0.0);

    // 为 NoiseRouter 的气候字段创建独立副本
    auto climateForRouter = createOverworldClimate(seed, largeBiomes);

    // ========== MC 1.21 完整 finalDensity 管线 ==========
    //
    // MC 1.21 finalDensity 构建流程:
    // 1. depth = YClampedGradient(-64, 320, 1.5, -1.5)
    // 2. offset = splineWithBlending(add(constant(-0.50375), spline(overworldOffset)), blendOffset)
    // 3. factor = splineWithBlending(spline(overworldFactor), BLENDING_FACTOR=10.0)
    // 4. jaggedness = splineWithBlending(spline(overworldJaggedness), BLENDING_JAGGEDNESS=0.0)
    // 5. slopedCheese = noiseGradientDensity(factor, add(depth, jaggedness * noise(JAGGED).halfNegative()))
    //                   + BASE_3D_NOISE_OVERWORLD
    // 6. caveDensity = underground(...)
    // 7. cheeseAndCave = min(slopedCheese + BASE_3D_NOISE, caveDensity)
    //    其中 BASE_3D_NOISE 是一个小的 3D 噪声偏移
    // 8. noCavesOrNoodle = rangeChoice(slopedCheese, -1e6, 1.5625,
    //                       min(slopedCheese + 5.0 * entrances, caveDensity),
    //                       underground)
    // 9. finalDensity = min(postProcess(slideOverworld(noCavesOrNoodle)), noodle)

    // peaksAndValleys 变换: -(||ridges| - 2/3| - 1/3|) * 3
    auto ridgesPV = peaksAndValleys(std::move(climate.ridges));

    // --- offset: splineWithBlending(add(constant(GLOBAL_OFFSET), spline(overworldOffset)), blendOffset) ---
    // 创建共享的 climate 密度函数用于样条树
    // 注意: TerrainProvider 需要 shared_ptr 输入，因此从 climate 的 unique_ptr 转换
    // continents 和 erosion 用于样条树的多个子样条，需要 shared_ptr
    auto continentsShared = std::shared_ptr<DensityFunction>(std::move(climate.continents));
    auto erosionShared = std::shared_ptr<DensityFunction>(std::move(climate.erosion));
    auto ridgesPVShared = std::shared_ptr<DensityFunction>(std::move(ridgesPV));

    auto offsetSpline = TerrainProvider::overworldOffset(continentsShared, erosionShared, ridgesPVShared);
    auto offsetWithGlobal = factory::add(factory::constant(TerrainProvider::GLOBAL_OFFSET), std::move(offsetSpline));
    // blendOffset 暂时为 constant(0.0)
    auto offset = TerrainProvider::splineWithBlending(std::move(offsetWithGlobal), factory::constant(0.0));

    // --- factor: splineWithBlending(spline(overworldFactor), BLENDING_FACTOR=10.0) ---
    // overworldFactor 需要 weirdness，即 ridges（原始值，非 peaksAndValleys）
    // 需要重新创建 ridges 用于 factor/jaggedness
    // 注意: climate.ridges 已被 move，需要创建新的 ridges 密度函数
    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto ridgesForFactor = factory::cache2DMarker(
        factory::shiftedNoise2d(factory::flatCacheMarker(factory::cache2DMarker(
                                    factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            factory::flatCacheMarker(
                factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            0.25,
            seed ^ 0x55555555ULL,
            RIDGE_FIRST_OCTAVE,
            toVector(RIDGE_AMPLITUDES)));

    // weirdness 用于 factor 和 jaggedness 的深层嵌套
    // MC 1.21: weirdness = peaksAndValleys(ridges)，但这里用原始 ridges 值
    // 实际上 MC 的 factor 样条中的 weirdness 就是原始 ridges 参数
    auto weirdnessShared = std::shared_ptr<DensityFunction>(std::move(ridgesForFactor));

    // 创建 erosion 的共享引用用于 factor
    // 由于 erosionShared 已经被 overworldOffset 使用，需要创建新的
    auto erosionForFactor = factory::cache2DMarker(
        factory::shiftedNoise2d(factory::flatCacheMarker(factory::cache2DMarker(
                                    factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            factory::flatCacheMarker(
                factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            0.25,
            seed ^ 0x44444444ULL,
            largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE,
            largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES)));
    auto erosionForFactorShared = std::shared_ptr<DensityFunction>(std::move(erosionForFactor));

    // continents for factor (new shared copy)
    auto continentsForFactor = factory::cache2DMarker(
        factory::shiftedNoise2d(factory::flatCacheMarker(factory::cache2DMarker(
                                    factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            factory::flatCacheMarker(
                factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            0.25,
            seed ^ 0x33333333ULL,
            largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE,
            largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES)));
    auto continentsForFactorShared = std::shared_ptr<DensityFunction>(std::move(continentsForFactor));

    auto factorSpline = TerrainProvider::overworldFactor(
        continentsForFactorShared, erosionForFactorShared, weirdnessShared, weirdnessShared);
    auto factor = TerrainProvider::splineWithBlending(std::move(factorSpline), factory::constant(10.0));

    // --- jaggedness: splineWithBlending(spline(overworldJaggedness), BLENDING_JAGGEDNESS=0.0) ---
    auto continentsForJagged = factory::cache2DMarker(
        factory::shiftedNoise2d(factory::flatCacheMarker(factory::cache2DMarker(
                                    factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            factory::flatCacheMarker(
                factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            0.25,
            seed ^ 0x33333333ULL,
            largeBiomes ? CONTINENTALNESS_LARGE_FIRST_OCTAVE : CONTINENTALNESS_FIRST_OCTAVE,
            largeBiomes ? toVector(CONTINENTALNESS_LARGE_AMPLITUDES) : toVector(CONTINENTALNESS_AMPLITUDES)));
    auto erosionForJagged = factory::cache2DMarker(
        factory::shiftedNoise2d(factory::flatCacheMarker(factory::cache2DMarker(
                                    factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            factory::flatCacheMarker(
                factory::cache2DMarker(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES)))),
            0.25,
            seed ^ 0x44444444ULL,
            largeBiomes ? EROSION_LARGE_FIRST_OCTAVE : EROSION_FIRST_OCTAVE,
            largeBiomes ? toVector(EROSION_LARGE_AMPLITUDES) : toVector(EROSION_AMPLITUDES)));

    auto continentsForJaggedShared = std::shared_ptr<DensityFunction>(std::move(continentsForJagged));
    auto erosionForJaggedShared = std::shared_ptr<DensityFunction>(std::move(erosionForJagged));

    auto jaggednessSpline = TerrainProvider::overworldJaggedness(
        continentsForJaggedShared, erosionForJaggedShared, weirdnessShared, weirdnessShared);
    auto jaggedness = TerrainProvider::splineWithBlending(std::move(jaggednessSpline), factory::constant(0.0));

    // --- slopedCheese: noiseGradientDensity(factor, depth + offset + jaggedness * noise(JAGGED).halfNegative()) +
    // base3dNoise --- depth = YClampedGradient(-64, 320, 1.5, -1.5) offsetToDepth = yClampedGradient(-64, 320, 1.5,
    // -1.5) + offset
    auto depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, 1.5, -1.5);
    auto depthPlusOffset = factory::add(std::move(depth), std::move(offset));

    // jaggedNoise = noise(JAGGED, 1500.0, 0.0) — MC 1.21
    auto jaggedNoise = factory::noise(seed ^ 0x77777777ULL, -7, {1.0, 1.0, 1.0, 1.0}, 1500.0, 0.0);
    auto jaggedContribution = factory::mul(std::move(jaggedness), factory::halfNegative(std::move(jaggedNoise)));

    auto depthPlusOffsetPlusJagged = factory::add(std::move(depthPlusOffset), std::move(jaggedContribution));

    // noiseGradientDensity(factor, depthPlusOffsetPlusJagged) = 4 * (factor *
    // depthPlusOffsetPlusJagged).quarterNegative()
    auto slopedCheeseDensity =
        TerrainProvider::noiseGradientDensity(std::move(factor), std::move(depthPlusOffsetPlusJagged));

    // base3dNoise (overworld)
    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.125, 80.0, 160.0, 8.0);

    auto slopedCheese = factory::add(std::move(slopedCheeseDensity), std::move(base3dNoise));

    // MC 1.21: slopedCheese 需要被 caveCheeseFactor 和 rangeChoice 同时引用。
    // 使用 SharedHolder（类似 MC 的 Holder 机制）实现引用共享。
    // 先用 cacheAllInCellMarker(interpolated(...)) 包裹，确保在同一 cell 内只计算一次。
    auto slopedCheeseShared =
        std::shared_ptr<DensityFunction>(factory::cacheAllInCellMarker(factory::interpolated(std::move(slopedCheese))));

    // --- 洞穴密度 ---
    auto entrancesFunc = CaveDensityFunctions::entrances(seed);
    auto pillarsFunc = CaveDensityFunctions::pillars(seed);

    // caveLayer = 4.0 * square(noise(CAVE_LAYER, 8.0, 8.0))
    auto caveLayer = factory::mul(factory::constant(4.0),
        factory::square(factory::noise(seed ^ 0xB0000013ULL,
            CaveDensityFunctions::CAVE_LAYER_OCTAVE,
            toVector(CaveDensityFunctions::CAVE_LAYER_AMPS),
            8.0,
            8.0)));

    // caveCheese = noise(CAVE_CHEESE, 2/3, 2/3)
    auto caveCheese = factory::noise(seed ^ 0xB0000014ULL,
        CaveDensityFunctions::CAVE_CHEESE_OCTAVE,
        toVector(CaveDensityFunctions::CAVE_CHEESE_AMPS),
        0.6666666666666666,
        0.6666666666666666);

    // caveCheeseFactor = clamp(0.27 + caveCheese, -1, 1) + clamp(1.5 - 0.64*slopedCheese, 0, 0.5)
    // slopedCheese 通过 SharedHolder 引用，与 rangeChoice 共享
    auto caveCheeseFactor =
        factory::add(factory::clamp(factory::add(factory::constant(0.27), std::move(caveCheese)), -1.0, 1.0),
            factory::clamp(factory::add(factory::constant(1.5),
                               factory::mul(factory::constant(-0.64), factory::sharedHolder(slopedCheeseShared))),
                0.0,
                0.5));

    // caveDensity = caveLayer + caveCheeseFactor
    auto caveDensity = factory::add(std::move(caveLayer), std::move(caveCheeseFactor));

    // underground = max(min(min(caveDensity, entrances), add(spaghetti2d, roughness)), rangeChoice(pillars, ...))
    auto spag2d = CaveDensityFunctions::spaghetti2d(seed);
    auto spagRoughness = CaveDensityFunctions::spaghettiRoughness(seed);

    auto minCaveDensityEntrances = factory::min(std::move(caveDensity), std::move(entrancesFunc));
    auto minSpaghettiRoughness =
        factory::min(std::move(minCaveDensityEntrances), factory::add(std::move(spag2d), std::move(spagRoughness)));
    auto pillarCheck = factory::rangeChoice(
        std::move(pillarsFunc), -1000000.0, 0.03, factory::constant(-1000000.0), CaveDensityFunctions::pillars(seed));
    auto underground = factory::max(std::move(minSpaghettiRoughness), std::move(pillarCheck));

    // underground 也需要被 whenInRange 和 whenOutOfRange 同时引用
    // 使用 SharedHolder 实现共享
    auto undergroundShared = std::shared_ptr<DensityFunction>(std::move(underground));

    // --- noodle ---
    auto noodle = CaveDensityFunctions::noodle(seed, world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);

    // --- noCavesOrNoodle = rangeChoice(slopedCheese, -1e6, 1.5625, min(slopedCheese + 5*entrances, underground),
    // underground) ---
    // MC 1.21: 当 slopedCheese < 1.5625 时（地表附近），密度由 slopedCheese + 5*entrances 与 underground
    // 的较小值决定 当 slopedCheese >= 1.5625 时（深层），直接使用 underground
    // slopedCheese 和 underground 均通过 SharedHolder 引用
    auto entrancesForRange = CaveDensityFunctions::entrances(seed);
    auto slopedCheesePlus5Entrances = factory::add(
        factory::sharedHolder(slopedCheeseShared), factory::mul(factory::constant(5.0), std::move(entrancesForRange)));
    auto whenInRange = factory::min(std::move(slopedCheesePlus5Entrances), factory::sharedHolder(undergroundShared));

    auto noCavesOrNoodle = factory::rangeChoice(factory::sharedHolder(slopedCheeseShared),
        -1000000.0,
        1.5625,
        std::move(whenInRange),
        factory::sharedHolder(undergroundShared));

    // 应用主世界 slide 和 postProcess
    auto slid = slideOverworld(std::move(noCavesOrNoodle));
    auto processedDensity = postProcess(std::move(slid));

    // finalDensity = min(processedDensity, noodle)
    auto finalDensity = factory::min(std::move(processedDensity), std::move(noodle));

    // ========== 矿脉噪声 ==========
    // MC 1.21: yLimitedInterpolatable(Y, noise(ORE_VEININESS, 1.5, 1.5), -60, 50, 0)
    auto veinToggle = TerrainProvider::yLimitedInterpolatable(factory::yClampedGradient(world::MIN_BUILD_HEIGHT * 2,
                                                                  world::MAX_BUILD_HEIGHT * 2,
                                                                  static_cast<f64>(world::MIN_BUILD_HEIGHT * 2),
                                                                  static_cast<f64>(world::MAX_BUILD_HEIGHT * 2)),
        factory::noise(seed ^ 0xC0000001ULL,
            CaveDensityFunctions::ORE_VEININESS_OCTAVE,
            toVector(CaveDensityFunctions::ORE_VEININESS_AMPS),
            1.5,
            1.5),
        -60,
        50,
        0.0);

    // MC 1.21: veinRidged = add(constant(-0.08), max(abs(yLimitedInterpolatable(Y, noise(ORE_VEIN_A, 4.0, 4.0), -60,
    // 50, 0)),
    //                                            abs(yLimitedInterpolatable(Y, noise(ORE_VEIN_B, 4.0, 4.0), -60, 50,
    //                                            0))))
    auto veinAYLimited = TerrainProvider::yLimitedInterpolatable(factory::yClampedGradient(world::MIN_BUILD_HEIGHT * 2,
                                                                     world::MAX_BUILD_HEIGHT * 2,
                                                                     static_cast<f64>(world::MIN_BUILD_HEIGHT * 2),
                                                                     static_cast<f64>(world::MAX_BUILD_HEIGHT * 2)),
        factory::noise(seed ^ 0xC0000002ULL,
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
        factory::noise(seed ^ 0xC0000003ULL,
            CaveDensityFunctions::ORE_VEIN_B_OCTAVE,
            toVector(CaveDensityFunctions::ORE_VEIN_B_AMPS),
            4.0,
            4.0),
        -60,
        50,
        0.0);

    auto veinRidged = factory::add(factory::constant(-0.08),
        factory::max(factory::abs(std::move(veinAYLimited)), factory::abs(std::move(veinBYLimited))));

    // MC 1.21: veinGap = noise(ORE_GAP)
    auto veinGap = factory::noise(seed ^ 0xC0000004ULL,
        CaveDensityFunctions::ORE_GAP_OCTAVE,
        toVector(CaveDensityFunctions::ORE_GAP_AMPS),
        1.0,
        1.0);

    // ========== 预备表面高度 ==========
    // MC 1.21: 完整实现使用 offset 样条的 flatCache(cache2d(lerp(blendAlpha, 0, offset)))
    // 以及 findTopSurface 函数来计算 preliminarySurfaceLevel
    // TODO: Java 使用 findTopSurface 计算实际表面高度，当前简化为 constant(0.0)
    //       需要实现 findTopSurface 后替换为正确的密度函数
    auto preliminarySurfaceLevel = factory::constant(0.0);

    return NoiseRouter(std::move(barrierNoise), // barrierNoise
        std::move(fluidLevelFloodednessNoise),  // fluidLevelFloodednessNoise
        std::move(fluidLevelSpreadNoise),       // fluidLevelSpreadNoise
        std::move(lavaNoise),                   // lavaNoise
        std::move(climate.temperature),         // temperature
        std::move(climate.vegetation),          // vegetation
        std::move(climateForRouter.continents), // continents
        std::move(climateForRouter.erosion),    // erosion
        std::move(climateForRouter.depth),      // depth
        std::move(climateForRouter.ridges),     // ridges
        std::move(preliminarySurfaceLevel),     // preliminarySurfaceLevel
        std::move(finalDensity),                // finalDensity
        std::move(veinToggle),                  // veinToggle
        std::move(veinRidged),                  // veinRidged
        std::move(veinGap));                    // veinGap
}

NoiseRouter NoiseRouterData::nether(u64 seed)
{
    // MC 1.21: nether finalDensity = postProcess(slideNetherLike(add(blendedNoise, yClampedGradient(0, 128, 1.5,
    // -1.5))))
    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.375, 80.0, 60.0, 8.0);
    auto depth = factory::yClampedGradient(0, 128, 1.5, -1.5);
    auto density = factory::add(std::move(base3dNoise), std::move(depth));
    auto slid = slideNetherLike(std::move(density), 0, 128);
    return noNewCaves(seed, std::move(slid));
}

NoiseRouter NoiseRouterData::end(u64 seed)
{
    // MC 1.21: end finalDensity = postProcess(slideEndLike(add(cache2d(endIslands), blendedNoise_end)))
    // erosion 槽位使用 endIslands，用于 EndBiomeSource 选择生物群系
    auto endIslandsNoise = factory::endIslands(seed);
    auto endIslands2d = factory::cache2DMarker(factory::endIslands(seed));

    auto base3dNoise = factory::blendedNoise(seed, 0.25, 0.25, 80.0, 160.0, 4.0);
    auto slopedCheese = factory::add(std::move(endIslands2d), std::move(base3dNoise));
    auto slid = slideEndLike(std::move(slopedCheese), 0, 128);
    auto finalDensity = postProcess(std::move(slid));

    return NoiseRouter(factory::constant(0.0), // barrierNoise
        factory::constant(0.0),                // fluidLevelFloodednessNoise
        factory::constant(0.0),                // fluidLevelSpreadNoise
        factory::constant(0.0),                // lavaNoise
        factory::constant(0.0),                // temperature
        factory::constant(0.0),                // vegetation
        factory::constant(0.0),                // continents
        std::move(endIslandsNoise),            // erosion（用于 EndBiomeSource）
        factory::constant(0.0),                // depth
        factory::constant(0.0),                // ridges
        factory::constant(0.0),                // preliminarySurfaceLevel
        std::move(finalDensity),               // finalDensity
        factory::constant(0.0),                // veinToggle
        factory::constant(0.0),                // veinRidged
        factory::constant(0.0));               // veinGap
}

} // namespace mc::world::gen::density
