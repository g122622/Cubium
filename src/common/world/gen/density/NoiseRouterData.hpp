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
 * auto router = NoiseRouterData::overworld(rs, seed, false);
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
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter overworld(const RandomState& rs, u64 seed, bool largeBiomes);

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

    /** @brief 主世界 slide 参数 */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideOverworld(
        std::unique_ptr<DensityFunction> input, bool amplified = false);

    /** @brief 下界 slide 参数 */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideNetherLike(
        std::unique_ptr<DensityFunction> input, i32 startY, i32 height);

    /** @brief 末地 slide 参数 */
    [[nodiscard]] static std::unique_ptr<DensityFunction> slideEndLike(
        std::unique_ptr<DensityFunction> input, i32 startY, i32 height);

    /**
     * @brief postProcess 变换
     *
     * MC 1.21: postProcess(x) = squeeze(interpolated(blendDensity(x)) * 0.64)
     * blendDensity 在 Blender 系统未实现时为恒等函数。
     * TODO: 实现 BlendDensity 密度函数类和 Blender 系统后，替换为完整的 blendDensity
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
