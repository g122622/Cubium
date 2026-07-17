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

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"

#include <unordered_map>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace settings {

/**
 * @brief world_preset 中单个维度的生成器配置（MC 1.21.11 worldgen/world_preset）
 *
 * 对应 JSON: `dimensions.<key>.generator = { type, biome_source, settings }`。
 * 解析期把 biome_source 与 settings 收敛为装配期直接可用的字段：
 * - Noise 生成器：noiseSettings 是 noise_settings 资源位置（RandomState::create 查 NoiseSettingsRegistry）
 * - Flat 生成器：flatSettings 是内联 settings 对象解析结果（FlatLevelGeneratorSettings）
 * - Debug 生成器：无额外配置
 */
struct WorldPresetGenerator {
    /// 生成器类型（generator.type）
    enum class Type : u8 {
        Noise, ///< minecraft:noise → NoiseChunkGenerator
        Flat,  ///< minecraft:flat  → FlatChunkGenerator
        Debug  ///< minecraft:debug → DebugChunkGenerator
    };

    /// biome_source 类型（generator.biome_source.type）
    enum class BiomeSourceType : u8 {
        MultiNoise, ///< minecraft:multi_noise（preset 名映射 createOverworld/createNether）
        TheEnd,     ///< minecraft:the_end → EndBiomeSource
        Fixed       ///< minecraft:fixed（biome RL → FixedBiomeSource）
    };

    Type type = Type::Noise;
    BiomeSourceType biomeSourceType = BiomeSourceType::MultiNoise;

    /// MultiNoise 预设名（minecraft:overworld / minecraft:nether），仅 MultiNoise 有效
    ResourceLocation multiNoisePreset;
    /// Fixed 生物群系资源位置，仅 Fixed 有效（经 BiomeLoader::biomeIdByName 解析）
    ResourceLocation fixedBiome;
    /// noise_settings 资源位置，仅 Noise 有效
    ResourceLocation noiseSettings;
    /// flat 内联 settings 解析结果，仅 Flat 有效
    FlatLevelGeneratorSettings flatSettings;
};

/**
 * @brief world_preset 中单个维度（MC 1.21.11 worldgen/world_preset）
 *
 * 对应 JSON: `dimensions.<key> = { type, generator }`。
 * dimensionType 是 dimension_type 资源位置（minecraft:overworld/the_nether/the_end），
 * 映射到 DimensionType 静态工厂（DimensionType 当前非数据驱动，仅 3 工厂）。
 */
struct WorldPresetDimension {
    ResourceLocation dimensionType;
    WorldPresetGenerator generator;
};

/**
 * @brief 世界预设（MC 1.21.11 worldgen/world_preset）
 *
 * 顶层 JSON: `{ dimensions: { minecraft:overworld/the_nether/the_end: { type, generator } } }`。
 * 原版 6 个 world_preset（normal/flat/large_biomes/amplified/debug_all_block_states/
 * single_biome_surface），每个含固定三维度键。ServerDimensionManager 按 DimensionId 映射
 * 维度键查表装配（OVERWORLD→minecraft:overworld, NETHER→minecraft:the_nether, THE_END→minecraft:the_end）。
 *
 * WorldPreset 可拷贝（持 unordered_map + 可拷贝的 WorldPresetDimension）。
 */
struct WorldPreset {
    /// 维度键 → 维度配置。键为 minecraft:overworld / minecraft:the_nether / minecraft:the_end
    std::unordered_map<ResourceLocation, WorldPresetDimension> dimensions;

    /**
     * @brief 从 world_preset JSON 解析世界预设
     *
     * @param root 顶层 JSON 对象（{ dimensions: {...} }）
     * @param id 预设资源位置（用于错误日志）
     * @return 世界预设，或错误
     */
    [[nodiscard]] static Result<WorldPreset> fromJson(const nlohmann::json& root, const ResourceLocation& id);
};

} // namespace settings
} // namespace gen
} // namespace world
} // namespace mc
