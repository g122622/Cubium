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

namespace mc::world::gen::density {

/**
 * @brief 预定义噪声路由器配置
 *
 * 提供主世界（普通/大型生物群系）、下界和末地的 NoiseRouter 预设。
 * 所有噪声参数均来自 MC 1.21 源码的 NoiseRouterData。
 *
 * 使用方法：
 * @code
 * auto router = NoiseRouterData::overworld(seed, false);
 * climate::Sampler sampler = router.createClimateSampler();
 * @endcode
 */
class NoiseRouterData {
public:
    /**
     * @brief 创建主世界噪声路由器
     * @param seed 世界种子
     * @param largeBiomes 是否使用大型生物群系预设
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter overworld(u64 seed, bool largeBiomes);

    /**
     * @brief 创建下界噪声路由器
     * @param seed 世界种子
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter nether(u64 seed);

    /**
     * @brief 创建末地噪声路由器
     * @param seed 世界种子
     * @return 配置好的 NoiseRouter
     */
    [[nodiscard]] static NoiseRouter end(u64 seed);

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
     * @brief 创建主世界的气候密度函数
     * @param seed 种子
     * @param largeBiomes 大型生物群系
     * @return 6 个气候密度函数（temperature, vegetation, continents, erosion, depth, ridges）
     */
    struct ClimateFunctions {
        std::unique_ptr<DensityFunction> temperature;
        std::unique_ptr<DensityFunction> vegetation;
        std::unique_ptr<DensityFunction> continents;
        std::unique_ptr<DensityFunction> erosion;
        std::unique_ptr<DensityFunction> depth;
        std::unique_ptr<DensityFunction> ridges;
    };

    [[nodiscard]] static ClimateFunctions createOverworldClimate(u64 seed, bool largeBiomes);

    /**
     * @brief 创建 peaksAndValleys 变换
     * weirdness → 山峰/山谷映射
     * 公式: -(|(|x| - 2/3| - 1/3|) * 3)
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> peaksAndValleys(std::unique_ptr<DensityFunction> ridges);
};

} // namespace mc::world::gen::density
