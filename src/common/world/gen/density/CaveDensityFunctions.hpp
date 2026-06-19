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

#pragma once

#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/TerrainProvider.hpp"
#include <memory>

namespace mc::world::gen::density {

/**
 * @brief MC 1.21.11 洞穴密度函数构建器
 *
 * 对应 MC 1.21 NoiseRouterData 中的洞穴密度函数链：
 * - spaghetti_2d: 水平面条状洞穴
 * - spaghetti_roughness: 面条粗糙度
 * - entrances: 洞穴入口
 * - noodle: 面条洞穴
 * - pillars: 柱状结构
 * - underground: 组合洞穴密度
 *
 * 所有函数接收种子和必要的密度函数引用，返回完整的密度函数树。
 */
class CaveDensityFunctions {
public:
    /**
     * @brief 构建 spaghetti_2d 密度函数
     *
     * MC 1.21: 组合 SPAGHETTI_2D_MODULATOR, SPAGHETTI_2D 噪声和 Y 梯度
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> spaghetti2d(u64 seed);

    /**
     * @brief 构建 spaghetti_roughness 密度函数
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> spaghettiRoughness(u64 seed);

    /**
     * @brief 构建 entrances 密度函数
     *
     * MC 1.21: min(entranceTerm, add(roughness, spag3d))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> entrances(u64 seed);

    /**
     * @brief 构建 noodle 密度函数
     *
     * MC 1.21: rangeChoice(noodleToggle, -1e6, 0, constant(64), add(noodleThickness, noodleRidge))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> noodle(u64 seed, i32 minY, i32 maxY);

    /**
     * @brief 构建 pillars 密度函数
     *
     * MC 1.21: cacheOnce(mul(add(mul(pillarNoise, 2.0), pillarRareness), cube(pillarThickness)))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> pillars(u64 seed);

    /**
     * @brief 构建 underground 密度函数
     *
     * MC 1.21: max(min(min(caveDensity, entrances), add(spaghetti2d, roughness)),
     *              rangeChoice(pillars, -1e6, 0.03, constant(-1e6), pillars))
     */
    [[nodiscard]] static std::unique_ptr<DensityFunction> underground(u64 seed, i32 minY, i32 maxY);

    // ========== 噪声参数常量 ==========

    /// SPAGHETTI_2D_MODULATOR: firstOctave=-2, amplitudes={2.0, 1.0}
    static constexpr i32 SPAGHETTI_2D_MODULATOR_OCTAVE = -2;
    static inline const f64 SPAGHETTI_2D_MODULATOR_AMPS[] = {2.0, 1.0};

    /// SPAGHETTI_2D: firstOctave=-2, amplitudes={2.0, 1.0} (for WeirdScaledSampler TYPE2)
    static constexpr i32 SPAGHETTI_2D_OCTAVE = -2;
    static inline const f64 SPAGHETTI_2D_AMPS[] = {2.0, 1.0};

    /// SPAGHETTI_2D_ELEVATION: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_2D_ELEVATION_OCTAVE = -8;
    static inline const f64 SPAGHETTI_2D_ELEVATION_AMPS[] = {1.0};

    /// SPAGHETTI_2D_THICKNESS_MODULATOR: firstOctave=-11, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_2D_THICKNESS_OCTAVE = -11;
    static inline const f64 SPAGHETTI_2D_THICKNESS_AMPS[] = {1.0};

    /// SPAGHETTI_ROUGHNESS: firstOctave=-5, amplitudes={1.0, 1.0, 1.0, 1.0}
    static constexpr i32 SPAGHETTI_ROUGHNESS_OCTAVE = -5;
    static inline const f64 SPAGHETTI_ROUGHNESS_AMPS[] = {1.0, 1.0, 1.0, 1.0};

    /// SPAGHETTI_ROUGHNESS_MODULATOR: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_ROUGHNESS_MODULATOR_OCTAVE = -8;
    static inline const f64 SPAGHETTI_ROUGHNESS_MODULATOR_AMPS[] = {1.0};

    /// SPAGHETTI_3D_RARITY: firstOctave=-11, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_3D_RARITY_OCTAVE = -11;
    static inline const f64 SPAGHETTI_3D_RARITY_AMPS[] = {1.0};

    /// SPAGHETTI_3D_THICKNESS: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_3D_THICKNESS_OCTAVE = -8;
    static inline const f64 SPAGHETTI_3D_THICKNESS_AMPS[] = {1.0};

    /// SPAGHETTI_3D_1: firstOctave=-7, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_3D_1_OCTAVE = -7;
    static inline const f64 SPAGHETTI_3D_1_AMPS[] = {1.0};

    /// SPAGHETTI_3D_2: firstOctave=-7, amplitudes={1.0}
    static constexpr i32 SPAGHETTI_3D_2_OCTAVE = -7;
    static inline const f64 SPAGHETTI_3D_2_AMPS[] = {1.0};

    /// CAVE_ENTRANCE: firstOctave=-7, amplitudes={0.4, 0.5, 1.0}
    static constexpr i32 CAVE_ENTRANCE_OCTAVE = -7;
    static inline const f64 CAVE_ENTRANCE_AMPS[] = {0.4, 0.5, 1.0};

    /// CAVE_LAYER: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 CAVE_LAYER_OCTAVE = -8;
    static inline const f64 CAVE_LAYER_AMPS[] = {1.0};

    /// CAVE_CHEESE: firstOctave=-8, amplitudes={0.5, 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 2.0, 0.0}
    static constexpr i32 CAVE_CHEESE_OCTAVE = -8;
    static inline const f64 CAVE_CHEESE_AMPS[] = {0.5, 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 2.0, 0.0};

    /// PILLAR: firstOctave=-7, amplitudes={1.0, 1.0} (xzScale=25, yScale=0.3)
    static constexpr i32 PILLAR_OCTAVE = -7;
    static inline const f64 PILLAR_AMPS[] = {1.0, 1.0};

    /// PILLAR_RARENESS: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 PILLAR_RARENESS_OCTAVE = -8;
    static inline const f64 PILLAR_RARENESS_AMPS[] = {1.0};

    /// PILLAR_THICKNESS: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 PILLAR_THICKNESS_OCTAVE = -8;
    static inline const f64 PILLAR_THICKNESS_AMPS[] = {1.0};

    /// NOODLE: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 NOODLE_OCTAVE = -8;
    static inline const f64 NOODLE_AMPS[] = {1.0};

    /// NOODLE_THICKNESS: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 NOODLE_THICKNESS_OCTAVE = -8;
    static inline const f64 NOODLE_THICKNESS_AMPS[] = {1.0};

    /// NOODLE_RIDGE_A: firstOctave=-7, amplitudes={1.0} (xzScale=2.6667, yScale=2.6667)
    static constexpr i32 NOODLE_RIDGE_A_OCTAVE = -7;
    static inline const f64 NOODLE_RIDGE_A_AMPS[] = {1.0};

    /// NOODLE_RIDGE_B: same as NOODLE_RIDGE_A but different seed
    static constexpr i32 NOODLE_RIDGE_B_OCTAVE = -7;
    static inline const f64 NOODLE_RIDGE_B_AMPS[] = {1.0};

    /// JAGGED: firstOctave=-16, amplitudes={1.0×17} (xzScale=1500, yScale=0)
    static constexpr i32 JAGGED_OCTAVE = -16;
    static inline const f64 JAGGED_AMPS[] = {
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    /// BASE_3D_NOISE_OVERWORLD: firstOctave=-7, amplitudes={1.0, 1.0, 1.0, 1.0}
    static constexpr i32 BASE_3D_NOISE_OCTAVE = -7;
    static inline const f64 BASE_3D_NOISE_AMPS[] = {1.0, 1.0, 1.0, 1.0};

    /// ORE_VEININESS: firstOctave=-8, amplitudes={1.0}
    static constexpr i32 ORE_VEININESS_OCTAVE = -8;
    static inline const f64 ORE_VEININESS_AMPS[] = {1.0};

    /// ORE_VEIN_A: firstOctave=-7, amplitudes={1.0}
    static constexpr i32 ORE_VEIN_A_OCTAVE = -7;
    static inline const f64 ORE_VEIN_A_AMPS[] = {1.0};

    /// ORE_VEIN_B: firstOctave=-7, amplitudes={1.0}
    static constexpr i32 ORE_VEIN_B_OCTAVE = -7;
    static inline const f64 ORE_VEIN_B_AMPS[] = {1.0};

    /// ORE_GAP: firstOctave=-5, amplitudes={1.0}
    static constexpr i32 ORE_GAP_OCTAVE = -5;
    static inline const f64 ORE_GAP_AMPS[] = {1.0};

    /// SURFACE_DENSITY_THRESHOLD: 当 slopedCheese < 1.5625 时使用洞穴密度
    static constexpr f64 SURFACE_DENSITY_THRESHOLD = 1.5625;
};

} // namespace mc::world::gen::density
