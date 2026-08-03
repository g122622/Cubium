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

#include "Noises.hpp"
#include "common/core/Types.hpp"
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::gen::noise {

namespace {
std::unordered_map<std::string, NoiseParameters> s_registry;

// 线程安全的初始化守卫
std::once_flag s_initFlag;

// 数据驱动加载状态：true 时 get()/has() 跳过 initialize() 兜底
bool s_loadedFromDatapack = false;
} // namespace

const NoiseParameters& Noises::get(const std::string& name)
{
    // 仅在未数据驱动加载时走 initialize() 兜底；数据驱动加载后注册表已由 NoiseLoader 注入。
    if (!s_loadedFromDatapack) {
        std::call_once(s_initFlag, initialize);
    }
    auto it = s_registry.find(name);
    if (it == s_registry.end()) {
        throw std::out_of_range("Unknown noise: " + name);
    }
    return it->second;
}

bool Noises::has(const std::string& name)
{
    if (!s_loadedFromDatapack) {
        std::call_once(s_initFlag, initialize);
    }
    return s_registry.contains(name);
}

void Noises::registerNoise(const std::string& name, i32 firstOctave, std::vector<f64> amplitudes)
{
    s_registry.emplace(name, NoiseParameters{firstOctave, std::move(amplitudes)});
}

void Noises::clear() noexcept
{
    s_registry.clear();
}

void Noises::markLoadedFromDatapack(bool loaded) noexcept
{
    s_loadedFromDatapack = loaded;
}

bool Noises::isLoadedFromDatapack() noexcept
{
    return s_loadedFromDatapack;
}

void Noises::initialize()
{
    // 由 std::call_once 保证只调用一次

    // ========== 气候噪声 ==========
    // MC 1.21 NoiseData.java — registerBiomeNoises(offset=0)
    registerNoise(TEMPERATURE, -10, {1.5, 0.0, 1.0, 0.0, 0.0, 0.0});
    registerNoise(VEGETATION, -8, {1.0, 1.0, 0.0, 0.0, 0.0, 0.0});
    registerNoise(CONTINENTALNESS, -9, {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0});
    registerNoise(EROSION, -9, {1.0, 1.0, 0.0, 1.0, 1.0});
    // MC 1.21 NoiseData.java — registerBiomeNoises(offset=-2)
    registerNoise(TEMPERATURE_LARGE, -12, {1.5, 0.0, 1.0, 0.0, 0.0, 0.0});
    registerNoise(VEGETATION_LARGE, -10, {1.0, 1.0, 0.0, 0.0, 0.0, 0.0});
    registerNoise(CONTINENTALNESS_LARGE, -11, {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0});
    registerNoise(EROSION_LARGE, -11, {1.0, 1.0, 0.0, 1.0, 1.0});
    registerNoise(RIDGE, -7, {1.0, 2.0, 1.0, 0.0, 0.0, 0.0});
    registerNoise(SHIFT, -3, {1.0, 1.0, 1.0, 0.0});

    // ========== 洞穴噪声 ==========
    registerNoise(AQUIFER_BARRIER, -3, {1.0});
    registerNoise(AQUIFER_FLUID_LEVEL_FLOODEDNESS, -7, {1.0});
    registerNoise(AQUIFER_LAVA, -1, {1.0});
    registerNoise(AQUIFER_FLUID_LEVEL_SPREAD, -5, {1.0});
    registerNoise(PILLAR, -7, {1.0, 1.0});
    registerNoise(PILLAR_RARENESS, -8, {1.0});
    registerNoise(PILLAR_THICKNESS, -8, {1.0});
    registerNoise(SPAGHETTI_2D, -7, {1.0});
    registerNoise(SPAGHETTI_2D_ELEVATION, -8, {1.0});
    registerNoise(SPAGHETTI_2D_MODULATOR, -11, {1.0});
    registerNoise(SPAGHETTI_2D_THICKNESS, -11, {1.0});
    registerNoise(SPAGHETTI_3D_1, -7, {1.0});
    registerNoise(SPAGHETTI_3D_2, -7, {1.0});
    registerNoise(SPAGHETTI_3D_RARITY, -11, {1.0});
    registerNoise(SPAGHETTI_3D_THICKNESS, -8, {1.0});
    registerNoise(SPAGHETTI_ROUGHNESS, -5, {1.0});
    registerNoise(SPAGHETTI_ROUGHNESS_MODULATOR, -8, {1.0});
    registerNoise(CAVE_ENTRANCE, -7, {0.4, 0.5, 1.0});
    registerNoise(CAVE_LAYER, -8, {1.0});
    registerNoise(CAVE_CHEESE, -8, {0.5, 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 2.0, 0.0});

    // ========== 矿脉噪声 ==========
    registerNoise(ORE_VEININESS, -8, {1.0});
    registerNoise(ORE_VEIN_A, -7, {1.0});
    registerNoise(ORE_VEIN_B, -7, {1.0});
    registerNoise(ORE_GAP, -5, {1.0});

    // ========== 面条洞穴噪声 ==========
    registerNoise(NOODLE, -8, {1.0});
    registerNoise(NOODLE_THICKNESS, -8, {1.0});
    registerNoise(NOODLE_RIDGE_A, -7, {1.0});
    registerNoise(NOODLE_RIDGE_B, -7, {1.0});

    // ========== 地表噪声 ==========
    // MC 1.21 NoiseData.java — 权威参数
    registerNoise(SURFACE, -6, {1.0, 1.0, 1.0});
    registerNoise(SURFACE_SECONDARY, -6, {1.0, 1.0, 0.0, 1.0});
    registerNoise(CLAY_BANDS_OFFSET, -8, {1.0});
    registerNoise(BADLANDS_PILLAR, -2, {1.0, 1.0, 1.0, 1.0});
    registerNoise(BADLANDS_PILLAR_ROOF, -8, {1.0});
    registerNoise(BADLANDS_SURFACE, -6, {1.0, 1.0, 1.0});
    registerNoise(ICEBERG_PILLAR, -6, {1.0, 1.0, 1.0, 1.0});
    registerNoise(ICEBERG_PILLAR_ROOF, -3, {1.0});
    registerNoise(ICEBERG_SURFACE, -6, {1.0, 1.0, 1.0});
    registerNoise(SWAMP, -2, {1.0});
    registerNoise(CALCITE, -9, {1.0, 1.0, 1.0, 1.0});
    registerNoise(GRAVEL, -8, {1.0, 1.0, 1.0, 1.0});
    registerNoise(POWDER_SNOW, -6, {1.0, 1.0, 1.0, 1.0});
    registerNoise(PACKED_ICE, -7, {1.0, 1.0, 1.0, 1.0});
    registerNoise(ICE, -4, {1.0, 1.0, 1.0, 1.0});

    // ========== 下界噪声 ==========
    registerNoise(SOUL_SAND_LAYER, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334});
    registerNoise(GRAVEL_LAYER, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334});
    registerNoise(PATCH, -5, {1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334});
    registerNoise(NETHERRACK, -3, {1.0, 0.0, 0.0, 0.35});
    registerNoise(NETHER_WART, -3, {1.0, 0.0, 0.0, 0.9});
    registerNoise(NETHER_STATE_SELECTOR, -4, {1.0});

    // ========== 其他噪声 ==========
    registerNoise(JAGGED, -16, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
}

} // namespace mc::world::gen::noise
