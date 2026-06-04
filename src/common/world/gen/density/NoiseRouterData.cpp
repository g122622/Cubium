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
    // 两者都使用相同的 SHIFT 噪声参数: firstOctave=-3, amplitudes=[1, 1, 1, 0]
    const i32 shiftSeed = static_cast<i32>(seed ^ 0x66666666ULL);
    auto shiftX = factory::flatCache(
        factory::cache2D(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ = factory::flatCache(
        factory::cache2D(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

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

    climate.temperature = factory::cache2D(factory::shiftedNoise2d(
        std::move(shiftX), std::move(shiftZ), 0.25, seed ^ 0x11111111ULL, tempOctave, tempAmps));

    // 需要重新创建 shift 噪声实例用于后续参数
    // MC 中每个 shiftedNoise2d 有自己的 shift 噪声副本，所以需要新建
    // 但由于 SHIFT_X 和 SHIFT_Z 已经被 move，这里需要重新创建
    // 实际上 MC 的做法是先创建 densityfunction (shiftX) 和 densityfunction1 (shiftZ)
    // 然后多次引用它们——我们的 shiftedNoise2d 内部持有 shift 噪声的副本
    // 所以需要重新创建用于后续的 climate 参数

    // 为后续气候参数重新创建偏移
    auto shiftX2 = factory::flatCache(
        factory::cache2D(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ2 = factory::flatCache(
        factory::cache2D(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.vegetation = factory::cache2D(factory::shiftedNoise2d(
        std::move(shiftX2), std::move(shiftZ2), 0.25, seed ^ 0x22222222ULL, vegOctave, vegAmps));

    auto shiftX3 = factory::flatCache(
        factory::cache2D(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ3 = factory::flatCache(
        factory::cache2D(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.continents = factory::cache2D(factory::shiftedNoise2d(
        std::move(shiftX3), std::move(shiftZ3), 0.25, seed ^ 0x33333333ULL, contOctave, contAmps));

    auto shiftX4 = factory::flatCache(
        factory::cache2D(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ4 = factory::flatCache(
        factory::cache2D(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.erosion = factory::cache2D(factory::shiftedNoise2d(
        std::move(shiftX4), std::move(shiftZ4), 0.25, seed ^ 0x44444444ULL, eroOctave, eroAmps));

    // depth 使用 YClampedGradient（Y 轴线性映射）
    // MC 1.21: fromY=-64, toY=320, fromValue=1.5, toValue=-1.5
    // 这意味着表面 depth≈1.5，高空 depth≈-1.5
    climate.depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT - 1, 1.5, -1.5);

    // ridges（奇异度）使用 shiftedNoise2d
    auto shiftX5 = factory::flatCache(
        factory::cache2D(factory::shiftA(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));
    auto shiftZ5 = factory::flatCache(
        factory::cache2D(factory::shiftB(shiftSeed, SHIFT_FIRST_OCTAVE, toVector(SHIFT_AMPLITUDES))));

    climate.ridges = factory::cache2D(factory::shiftedNoise2d(std::move(shiftX5),
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

NoiseRouter NoiseRouterData::overworld(u64 seed, bool largeBiomes)
{
    auto climate = createOverworldClimate(seed, largeBiomes);

    // 洞穴噪声（简化版本，使用常量零）
    auto zero = factory::constant(0.0);

    // 最终密度（简化版本，后续需要完善 spline 计算）
    auto finalDensity = factory::constant(0.0);

    // 矿脉噪声（简化版本）
    auto veinToggle = factory::constant(0.0);
    auto veinRidged = factory::constant(0.0);
    auto veinGap = factory::constant(0.0);

    // 预备表面高度（简化版本）
    auto preliminarySurfaceLevel = factory::constant(0.0);

    return NoiseRouter(std::move(zero),     // barrierNoise
        factory::constant(0.0),             // fluidLevelFloodednessNoise
        factory::constant(0.0),             // fluidLevelSpreadNoise
        factory::constant(0.0),             // lavaNoise
        std::move(climate.temperature),     // temperature
        std::move(climate.vegetation),      // vegetation
        std::move(climate.continents),      // continents
        std::move(climate.erosion),         // erosion
        std::move(climate.depth),           // depth
        std::move(climate.ridges),          // ridges
        std::move(preliminarySurfaceLevel), // preliminarySurfaceLevel
        std::move(finalDensity),            // finalDensity
        std::move(veinToggle),              // veinToggle
        std::move(veinRidged),              // veinRidged
        std::move(veinGap));                // veinGap
}

NoiseRouter NoiseRouterData::nether(u64 seed)
{
    // 下界使用简单的温度和湿度噪声
    auto temperature = factory::cache2D(factory::noise(seed ^ 0x11111111ULL, -4, {1.0, 1.0, 1.0, 1.0}, 0.25, 0.0));
    auto vegetation = factory::cache2D(factory::noise(seed ^ 0x22222222ULL, -4, {1.0, 1.0, 1.0, 1.0}, 0.25, 0.0));

    // 下界不使用大陆度、侵蚀、深度
    auto zero = factory::constant(0.0);

    return NoiseRouter(factory::constant(0.0),        // barrierNoise
        factory::constant(0.0),                       // fluidLevelFloodednessNoise
        factory::constant(0.0),                       // fluidLevelSpreadNoise
        factory::constant(0.0),                       // lavaNoise
        std::move(temperature),                       // temperature
        std::move(vegetation),                        // vegetation
        factory::constant(0.0),                       // continents
        factory::constant(0.0),                       // erosion
        factory::yClampedGradient(0, 128, 1.5, -1.5), // depth (下界 Y 范围 0-128)
        factory::constant(0.0),                       // ridges
        factory::constant(0.0),                       // preliminarySurfaceLevel
        factory::constant(0.0),                       // finalDensity
        factory::constant(0.0),                       // veinToggle
        factory::constant(0.0),                       // veinRidged
        factory::constant(0.0));                      // veinGap
}

NoiseRouter NoiseRouterData::end(u64 seed)
{
    // 末地使用岛屿噪声
    auto endIslands = factory::endIslands(seed);

    // 末地气候参数全部为零
    auto zero = factory::constant(0.0);

    return NoiseRouter(factory::constant(0.0),        // barrierNoise
        factory::constant(0.0),                       // fluidLevelFloodednessNoise
        factory::constant(0.0),                       // fluidLevelSpreadNoise
        factory::constant(0.0),                       // lavaNoise
        factory::constant(0.0),                       // temperature
        factory::constant(0.0),                       // vegetation
        factory::constant(0.0),                       // continents
        factory::constant(0.0),                       // erosion
        factory::yClampedGradient(0, 128, 1.5, -1.5), // depth (末地 Y 范围 0-128)
        factory::constant(0.0),                       // ridges
        factory::constant(0.0),                       // preliminarySurfaceLevel
        std::move(endIslands),                        // finalDensity
        factory::constant(0.0),                       // veinToggle
        factory::constant(0.0),                       // veinRidged
        factory::constant(0.0));                      // veinGap
}

} // namespace mc::world::gen::density
