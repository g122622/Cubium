/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, to to permit persons to whom the Software is
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

    // 气候噪声（温度、湿度、大陆度、侵蚀）
    // 完整版本使用 ShiftedNoise（带坐标偏移），当前简化版本使用普通 Noise
    // xzScale = 0.25, yScale = 0（2D 噪声）

    climate.temperature = factory::cache2D(factory::noise(seed ^ 0x11111111ULL, tempOctave, tempAmps, 0.25, 0.0));

    climate.vegetation = factory::cache2D(factory::noise(seed ^ 0x22222222ULL, vegOctave, vegAmps, 0.25, 0.0));

    climate.continents = factory::cache2D(factory::noise(seed ^ 0x33333333ULL, contOctave, contAmps, 0.25, 0.0));

    climate.erosion = factory::cache2D(factory::noise(seed ^ 0x44444444ULL, eroOctave, eroAmps, 0.25, 0.0));

    // depth 使用 YClampedGradient（Y 轴线性映射）
    // MC 1.21: fromY=-64, toY=320, fromValue=1.5, toValue=-1.5
    // 这意味着表面 depth≈1.5，高空 depth≈-1.5
    climate.depth = factory::yClampedGradient(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT - 1, 1.5, -1.5);

    // ridges（奇异度）使用基础噪声
    climate.ridges = factory::cache2D(
        factory::noise(seed ^ 0x55555555ULL, RIDGE_FIRST_OCTAVE, toVector(RIDGE_AMPLITUDES), 0.25, 0.0));

    return climate;
}

std::unique_ptr<DensityFunction> NoiseRouterData::peaksAndValleys(std::unique_ptr<DensityFunction> ridges)
{
    // MC 1.21: mul(add(add(abs(ridges), constant(-2/3)).abs(), constant(-1/3)), constant(-3))
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
