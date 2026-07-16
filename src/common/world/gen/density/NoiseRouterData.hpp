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

#pragma once

#include "common/world/gen/density/NoiseRouter.hpp"
#include <memory>

namespace mc::world::gen {
class RandomState;
} // namespace mc::world::gen

namespace mc::world::gen::density {

/**
 * @brief 预定义噪声路由器配置
 *
 * 提供主世界（普通/大型生物群系）、下界和末地的 NoiseRouter 预设。
 * 所有噪声参数均来自 MC 1.21 源码的 NoiseRouterData。
 *
 * 使用方法：
 * @code
 * auto router = NoiseRouterData::overworld(rs, seed, false, false);
 * climate::Sampler sampler = router.createClimateSampler();
 * @endcode
 */
class NoiseRouterData {
public:
    /**
     * @brief 创建主世界噪声路由器
     *
     * 从 RandomState 的派生种子缓存获取 NormalNoise，跨区块复用，
     * 避免每区块 createRouterCopy 时重建 PerlinNoise 倍频置换表。
     * 叶子密度函数（NoiseDensity/ShiftedNoise/MappedNoise 等）通过 shared_ptr 共享底层噪声。
     *
     * @param rs 世界随机状态（提供噪声缓存）
     * @param seed 世界种子
     * @param largeBiomes 是否使用大型生物群系预设
     * @param amplified 是否使用放大化预设（影响 slideOverworld 和 preliminarySurfaceLevel）
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter overworld(const RandomState& rs, u64 seed, bool largeBiomes, bool amplified);

    /**
     * @brief 创建下界噪声路由器
     *
     * temperature/vegetation 的 shift 叶子从 RandomState 缓存获取，BlendedNoise 仍按 seed 重建。
     *
     * @param rs 世界随机状态（提供噪声缓存）
     * @param seed 世界种子
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter nether(const RandomState& rs, u64 seed);

    /**
     * @brief 创建末地噪声路由器
     *
     * 末地路径仅使用 BlendedNoise 和 EndIslands，无 NormalNoise 叶子，
     * rs 仅用于 API 统一。
     *
     * @param rs 世界随机状态（未使用，仅 API 统一）
     * @param seed 世界种子
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter end(const RandomState& rs, u64 seed);

    // ========== 噪声参数常量 ==========

    /// 温度噪声参数（主世界）
    static constexpr i32 TEMPERATURE_FIRST_OCTAVE = -10;
    static inline constexpr f64 TEMPERATURE_AMPLITUDES[] = {1.5, 0.0, 1.0, 0.0, 0.0, 0.0};

    /// 湿度噪声参数（主世界）
    static constexpr i32 VEGETATION_FIRST_OCTAVE = -8;
    static inline constexpr f64 VEGETATION_AMPLITUDES[] = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0};

    /// 大陆度噪声参数（主世界）
    static constexpr i32 CONTINENTALNESS_FIRST_OCTAVE = -9;
    static inline constexpr f64 CONTINENTALNESS_AMPLITUDES[] = {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0};

    /// 侵蚀噪声参数（主世界）
    static constexpr i32 EROSION_FIRST_OCTAVE = -9;
    static inline constexpr f64 EROSION_AMPLITUDES[] = {1.0, 1.0, 0.0, 1.0, 1.0};

    /// 奇异度噪声参数（主世界）
    static constexpr i32 RIDGE_FIRST_OCTAVE = -7;
    static inline constexpr f64 RIDGE_AMPLITUDES[] = {1.0, 2.0, 1.0, 0.0, 0.0, 0.0};

    /// 坐标偏移噪声参数
    static constexpr i32 SHIFT_FIRST_OCTAVE = -3;
    static inline constexpr f64 SHIFT_AMPLITUDES[] = {1.0, 1.0, 1.0, 0.0};

    /// 温度噪声参数（大型生物群系）
    static constexpr i32 TEMPERATURE_LARGE_FIRST_OCTAVE = -12;
    static inline constexpr f64 TEMPERATURE_LARGE_AMPLITUDES[] = {1.5, 0.0, 1.0, 0.0, 0.0, 0.0};

    /// 湿度噪声参数（大型生物群系）
    static constexpr i32 VEGETATION_LARGE_FIRST_OCTAVE = -10;
    static inline constexpr f64 VEGETATION_LARGE_AMPLITUDES[] = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0};

    /// 大陆度噪声参数（大型生物群系）
    static constexpr i32 CONTINENTALNESS_LARGE_FIRST_OCTAVE = -11;
    static inline constexpr f64 CONTINENTALNESS_LARGE_AMPLITUDES[] = {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0};

    /// 侵蚀噪声参数（大型生物群系）
    static constexpr i32 EROSION_LARGE_FIRST_OCTAVE = -11;
    static inline constexpr f64 EROSION_LARGE_AMPLITUDES[] = {1.0, 1.0, 0.0, 1.0, 1.0};

private:
    /**
     * @brief 主世界气候密度函数集合
     *
     * 包含 6 个气候密度函数：temperature, vegetation, continents, erosion, depth, ridges。
     */
    struct ClimateFunctions {
        std::unique_ptr<DensityFunction> temperature;
        std::unique_ptr<DensityFunction> vegetation;
        std::unique_ptr<DensityFunction> continents;
        std::unique_ptr<DensityFunction> erosion;
        std::unique_ptr<DensityFunction> depth;
        std::unique_ptr<DensityFunction> ridges;
    };

    /**
     * @brief 创建主世界的气候密度函数
     *
     * 从 RandomState 缓存获取 NormalNoise，shift 叶子跨区块共享。
     *
     * @param rs 世界随机状态（提供噪声缓存）
     * @param seed 世界种子
     * @param largeBiomes 大型生物群系
     * @return 6 个气候密度函数
     */
    [[nodiscard]] static ClimateFunctions createOverworldClimate(const RandomState& rs, u64 seed, bool largeBiomes);

    /**
     * @brief 创建 peaksAndValleys 变换
     * weirdness → 山峰/山谷映射
     * 公式: -(|(|x| - 2/3| - 1/3|) * 3)
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> peaksAndValleys(std::unique_ptr<DensityFunction> ridges);

    /**
     * @brief slide 通用实现
     *
     * MC 1.21: 在顶部和底部应用 Y 方向渐变，使地形在维度边界平滑过渡。
     * 顶部 slide: Y 从 (startY+height-topSlideFromTop) 到 (startY+height-topSlideToTop) 从 1.0 渐变到 0.0
     * 底部 slide: Y 从 (startY+bottomSlideFromBottom) 到 (startY+bottomSlideToBottom) 从 0.0 渐变到 1.0
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slide(std::unique_ptr<DensityFunction> input,
        i32 startY,
        i32 height,
        i32 topSlideFromTop,
        i32 topSlideToTop,
        f64 topSlideTarget,
        i32 bottomSlideFromBottom,
        i32 bottomSlideToBottom,
        f64 bottomSlideTarget);

    /** @brief 主世界 slide 参数（amplified 影响顶部和底部 slide 范围） */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideOverworld(
        std::unique_ptr<DensityFunction> input, bool amplified);

    /** @brief 下界 slide 参数 */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideNetherLike(
        std::unique_ptr<DensityFunction> input, i32 startY, i32 height);

    /** @brief 末地 slide 参数 */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideEndLike(
        std::unique_ptr<DensityFunction> input, i32 startY, i32 height);

    /**
     * @brief 线性重映射密度函数
     *
     * MC 1.21.11: NoiseRouterData.remap(x, fromMin, fromMax, toMin, toMax)
     * 将 x 的 [fromMin, fromMax] 区间映射到 [toMin, toMax]：
     *   d0 = (toMax - toMin) / (fromMax - fromMin)
     *   d1 = toMin - fromMin * d0
     *   result = add(mul(x, constant(d0)), constant(d1))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> remap(
        std::unique_ptr<DensityFunction> input, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax);

    /**
     * @brief 将偏移量加到深度基线上
     *
     * MC 1.21.11: NoiseRouterData.offsetToDepth(x)
     *   = add(yClampedGradient(-64, 320, 1.5, -1.5), x)
     * 即 depth = 1.5 - (y+64)/384 * 3.0 + offset
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> offsetToDepth(std::unique_ptr<DensityFunction> offset);

    /**
     * @brief 创建 preliminarySurfaceLevel 密度函数
     *
     * MC 1.21.11: NoiseRouterData.preliminarySurfaceLevel(offset, factor, amplified)
     *   factorCached  = cache2d(factor)
     *   offsetCached  = cache2d(offset)
     *   upperBound    = remap(add(mul(0.2734375, factor.invert()), mul(-1.0, offset)),
     *                          1.5, -1.5, -64.0, 320.0).clamp(-40.0, 320.0)
     *   density       = add(slideOverworld(amplified,
     *                          add(noiseGradientDensity(factorCached, offsetToDepth(offsetCached)), -0.703125)
     *                              .clamp(-64.0, 64.0)),
     *                        -0.390625)
     *   return findTopSurface(density, upperBound, -64, cellHeight)
     *
     * cellHeight 固定为 8（主世界 NoiseSettings::getCellHeight() = sizeVertical * 4 = 2 * 4 = 8）。
     *
     * @param offset 地形偏移密度函数（slopedCheese 减去基线后）
     * @param factor 噪声因子密度函数
     * @param amplified 是否为 amplified 世界
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> preliminarySurfaceLevel(
        std::shared_ptr<DensityFunction> offset, std::shared_ptr<DensityFunction> factor, bool amplified);

    /**
     * @brief postProcess 变换
     *
     * MC 1.21: postProcess(x) = squeeze(interpolated(x) * 0.64)
     * 旧区块混合（blendDensity/Blender）已移除，本项目不兼容旧版存档。
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> postProcess(std::unique_ptr<DensityFunction> input);

    /**
     * @brief 创建 noNewCaves 路由器
     *
     * MC 1.21: 用于下界和末地的简单路由器，
     * 只设置 temperature/vegetation 为 shiftedNoise2d，
     * 其余通道为 constant(0.0)，finalDensity 经过 postProcess。
     */
    /**
     * @brief 创建 noNewCaves 路由器
     *
     * MC 1.21: 用于下界和末地的简单路由器，
     * 只设置 temperature/vegetation 为 shiftedNoise2d（从 RandomState 缓存获取 NormalNoise），
     * 其余通道为 constant(0.0)，finalDensity 经过 postProcess。
     *
     * @param rs 世界随机状态（提供噪声缓存）
     * @param seed 世界种子
     * @param finalDensity 经 postProcess 包装的最终密度函数
     */
    [[nodiscard]] static NoiseRouter noNewCaves(
        const RandomState& rs, u64 seed, std::unique_ptr<DensityFunction> finalDensity);
};

} // namespace mc::world::gen::density
