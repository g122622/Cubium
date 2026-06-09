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

#include "common/core/Types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::noise {

/**
 * @brief 噪声参数（MC 1.21 NoiseParameters / NormalNoise.NoiseParameters）
 *
 * 定义一个命名噪声的 firstOctave 和 amplitudes。
 * 对应 Java 的 NormalNoise.NoiseParameters(firstOctave, amplitudes...)。
 */
struct NoiseParameters {
    i32 firstOctave;
    std::vector<f64> amplitudes;
};

/**
 * @brief 噪声注册表（MC 1.21 Noises + NoiseData）
 *
 * 提供所有 MC 1.21 命名噪声的参数定义。
 * 与 Java 的 Noises.java（ResourceKey 定义）和 NoiseData.java（参数值）对应。
 *
 * 使用方式：
 *   auto& params = Noises::get("minecraft:surface");
 *   auto noise = NormalNoise(rng, params.firstOctave, params.amplitudes);
 */
class Noises {
public:
    /**
     * @brief 获取命名噪声的参数
     * @param name 噪声名称（如 "minecraft:surface"）
     * @return 噪声参数引用
     * @throws std::out_of_range 如果名称不存在
     */
    [[nodiscard]] static const NoiseParameters& get(const std::string& name);

    /**
     * @brief 检查命名噪声是否存在
     */
    [[nodiscard]] static bool has(const std::string& name);

    /**
     * @brief 初始化注册表（首次调用时自动调用）
     */
    static void initialize();

    // ========== 气候噪声 ==========
    static constexpr const char* TEMPERATURE = "minecraft:temperature";
    static constexpr const char* VEGETATION = "minecraft:vegetation";
    static constexpr const char* CONTINENTALNESS = "minecraft:continentalness";
    static constexpr const char* EROSION = "minecraft:erosion";
    static constexpr const char* TEMPERATURE_LARGE = "minecraft:temperature_large";
    static constexpr const char* VEGETATION_LARGE = "minecraft:vegetation_large";
    static constexpr const char* CONTINENTALNESS_LARGE = "minecraft:continentalness_large";
    static constexpr const char* EROSION_LARGE = "minecraft:erosion_large";
    static constexpr const char* RIDGE = "minecraft:ridge";
    static constexpr const char* SHIFT = "minecraft:shift";

    // ========== 洞穴噪声 ==========
    static constexpr const char* AQUIFER_BARRIER = "minecraft:aquifer_barrier";
    static constexpr const char* AQUIFER_FLUID_LEVEL_FLOODEDNESS = "minecraft:aquifer_fluid_level_floodedness";
    static constexpr const char* AQUIFER_LAVA = "minecraft:aquifer_lava";
    static constexpr const char* AQUIFER_FLUID_LEVEL_SPREAD = "minecraft:aquifer_fluid_level_spread";
    static constexpr const char* PILLAR = "minecraft:pillar";
    static constexpr const char* PILLAR_RARENESS = "minecraft:pillar_rareness";
    static constexpr const char* PILLAR_THICKNESS = "minecraft:pillar_thickness";
    static constexpr const char* SPAGHETTI_2D = "minecraft:spaghetti_2d";
    static constexpr const char* SPAGHETTI_2D_ELEVATION = "minecraft:spaghetti_2d_elevation";
    static constexpr const char* SPAGHETTI_2D_MODULATOR = "minecraft:spaghetti_2d_modulator";
    static constexpr const char* SPAGHETTI_2D_THICKNESS = "minecraft:spaghetti_2d_thickness";
    static constexpr const char* SPAGHETTI_3D_1 = "minecraft:spaghetti_3d_1";
    static constexpr const char* SPAGHETTI_3D_2 = "minecraft:spaghetti_3d_2";
    static constexpr const char* SPAGHETTI_3D_RARITY = "minecraft:spaghetti_3d_rarity";
    static constexpr const char* SPAGHETTI_3D_THICKNESS = "minecraft:spaghetti_3d_thickness";
    static constexpr const char* SPAGHETTI_ROUGHNESS = "minecraft:spaghetti_roughness";
    static constexpr const char* SPAGHETTI_ROUGHNESS_MODULATOR = "minecraft:spaghetti_roughness_modulator";
    static constexpr const char* CAVE_ENTRANCE = "minecraft:cave_entrance";
    static constexpr const char* CAVE_LAYER = "minecraft:cave_layer";
    static constexpr const char* CAVE_CHEESE = "minecraft:cave_cheese";

    // ========== 矿脉噪声 ==========
    static constexpr const char* ORE_VEININESS = "minecraft:ore_veininess";
    static constexpr const char* ORE_VEIN_A = "minecraft:ore_vein_a";
    static constexpr const char* ORE_VEIN_B = "minecraft:ore_vein_b";
    static constexpr const char* ORE_GAP = "minecraft:ore_gap";

    // ========== 面条洞穴噪声 ==========
    static constexpr const char* NOODLE = "minecraft:noodle";
    static constexpr const char* NOODLE_THICKNESS = "minecraft:noodle_thickness";
    static constexpr const char* NOODLE_RIDGE_A = "minecraft:noodle_ridge_a";
    static constexpr const char* NOODLE_RIDGE_B = "minecraft:noodle_ridge_b";

    // ========== 地表噪声 ==========
    static constexpr const char* SURFACE = "minecraft:surface";
    static constexpr const char* SURFACE_SECONDARY = "minecraft:surface_secondary";
    static constexpr const char* CLAY_BANDS_OFFSET = "minecraft:clay_bands_offset";
    static constexpr const char* BADLANDS_PILLAR = "minecraft:badlands_pillar";
    static constexpr const char* BADLANDS_PILLAR_ROOF = "minecraft:badlands_pillar_roof";
    static constexpr const char* BADLANDS_SURFACE = "minecraft:badlands_surface";
    static constexpr const char* ICEBERG_PILLAR = "minecraft:iceberg_pillar";
    static constexpr const char* ICEBERG_PILLAR_ROOF = "minecraft:iceberg_pillar_roof";
    static constexpr const char* ICEBERG_SURFACE = "minecraft:iceberg_surface";
    static constexpr const char* SWAMP = "minecraft:swamp";
    static constexpr const char* CALCITE = "minecraft:calcite";
    static constexpr const char* GRAVEL = "minecraft:gravel";
    static constexpr const char* POWDER_SNOW = "minecraft:powder_snow";
    static constexpr const char* PACKED_ICE = "minecraft:packed_ice";
    static constexpr const char* ICE = "minecraft:ice";

    // ========== 下界噪声 ==========
    static constexpr const char* SOUL_SAND_LAYER = "minecraft:soul_sand_layer";
    static constexpr const char* GRAVEL_LAYER = "minecraft:gravel_layer";
    static constexpr const char* PATCH = "minecraft:patch";
    static constexpr const char* NETHERRACK = "minecraft:netherrack";
    static constexpr const char* NETHER_WART = "minecraft:nether_wart";
    static constexpr const char* NETHER_STATE_SELECTOR = "minecraft:nether_state_selector";

    // ========== 其他噪声 ==========
    static constexpr const char* JAGGED = "minecraft:jagged";

private:
    static std::unordered_map<std::string, NoiseParameters> s_registry;
    static bool s_initialized;

    static void registerNoise(const std::string& name, i32 firstOctave, std::vector<f64> amplitudes);
};

} // namespace mc::world::gen::noise
