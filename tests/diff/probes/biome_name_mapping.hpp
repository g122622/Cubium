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
 *
 */

#pragma once

/**
 * @file biome_name_mapping.hpp
 * @brief 数据包 biome 名 → `BiomeId` 的映射（供探针共用）
 *
 * 服务端侧在 `BiomeLoader` 内维护同一张表；探针没有服务端依赖，这里保留一份**仅供探针**
 * 的副本。两边不一致时 `biome_spawns` 会直接暴露出来（原版有的群系探针列不出来）。
 */

#include "common/world/biome/BiomeIds.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::diff::biome {

/// 数据包文件名（`plains`、`stony_shore`）→ BiomeId
[[nodiscard]] inline std::optional<mc::BiomeId> biomeIdByDataPackName(std::string_view name)
{
    static const std::unordered_map<std::string, mc::BiomeId> MAP = {
    {"ocean", world::biome::Biomes::Ocean},
    {"plains", world::biome::Biomes::Plains},
    {"desert", world::biome::Biomes::Desert},
    {"forest", world::biome::Biomes::Forest},
    {"taiga", world::biome::Biomes::Taiga},
    {"swamp", world::biome::Biomes::Swamp},
    {"river", world::biome::Biomes::River},
    {"nether_wastes", world::biome::Biomes::NetherWastes},
    {"the_end", world::biome::Biomes::TheEnd},
    {"frozen_ocean", world::biome::Biomes::FrozenOcean},
    {"frozen_river", world::biome::Biomes::FrozenRiver},
    {"snowy_plains", world::biome::Biomes::SnowyPlains},
    {"mushroom_fields", world::biome::Biomes::MushroomFields},
    {"beach", world::biome::Biomes::Beach},
    {"snowy_beach", world::biome::Biomes::SnowyBeach},
    {"birch_forest", world::biome::Biomes::BirchForest},
    {"dark_forest", world::biome::Biomes::DarkForest},
    {"snowy_taiga", world::biome::Biomes::SnowyTaiga},
    {"savanna", world::biome::Biomes::Savanna},
    {"savanna_plateau", world::biome::Biomes::SavannaPlateau},
    {"badlands", world::biome::Biomes::Badlands},
    {"eroded_badlands", world::biome::Biomes::ErodedBadlands},
    {"small_end_islands", world::biome::Biomes::SmallEndIslands},
    {"end_midlands", world::biome::Biomes::EndMidlands},
    {"end_highlands", world::biome::Biomes::EndHighlands},
    {"end_barrens", world::biome::Biomes::EndBarrens},
    {"warm_ocean", world::biome::Biomes::WarmOcean},
    {"lukewarm_ocean", world::biome::Biomes::LukewarmOcean},
    {"cold_ocean", world::biome::Biomes::ColdOcean},
    {"deep_warm_ocean", world::biome::Biomes::DeepWarmOcean},
    {"deep_lukewarm_ocean", world::biome::Biomes::DeepLukewarmOcean},
    {"deep_cold_ocean", world::biome::Biomes::DeepColdOcean},
    {"deep_frozen_ocean", world::biome::Biomes::DeepFrozenOcean},
    {"deep_ocean", world::biome::Biomes::DeepOcean},
    {"the_void", world::biome::Biomes::TheVoid},
    {"sunflower_plains", world::biome::Biomes::SunflowerPlains},
    {"flower_forest", world::biome::Biomes::FlowerForest},
    {"ice_spikes", world::biome::Biomes::IceSpikes},
    {"tall_birch_forest", world::biome::Biomes::TallBirchForest},
    {"soul_sand_valley", world::biome::Biomes::SoulSandValley},
    {"crimson_forest", world::biome::Biomes::CrimsonForest},
    {"warped_forest", world::biome::Biomes::WarpedForest},
    {"basalt_deltas", world::biome::Biomes::BasaltDeltas},
    {"meadow", world::biome::Biomes::Meadow},
    {"grove", world::biome::Biomes::Grove},
    {"snowy_slopes", world::biome::Biomes::SnowySlopes},
    {"jagged_peaks", world::biome::Biomes::JaggedPeaks},
    {"frozen_peaks", world::biome::Biomes::FrozenPeaks},
    {"stony_peaks", world::biome::Biomes::StonyPeaks},
    {"dripstone_caves", world::biome::Biomes::DripstoneCaves},
    {"lush_caves", world::biome::Biomes::LushCaves},
    {"mangrove_swamp", world::biome::Biomes::MangroveSwamp},
    {"cherry_grove", world::biome::Biomes::CherryGrove},
    {"pale_garden", world::biome::Biomes::PaleGarden},
    {"jungle", world::biome::Biomes::Jungle},
    {"bamboo_jungle", world::biome::Biomes::BambooJungle},
    {"stony_shore", world::biome::Biomes::StonyShore},
    {"old_growth_pine_taiga", world::biome::Biomes::OldGrowthPineTaiga},
    {"old_growth_spruce_taiga", world::biome::Biomes::OldGrowthSpruceTaiga},
    {"old_growth_birch_forest", world::biome::Biomes::OldGrowthBirchForest},
    {"sparse_jungle", world::biome::Biomes::SparseJungle},
    {"windswept_hills", world::biome::Biomes::WindsweptHills},
    {"windswept_forest", world::biome::Biomes::WindsweptForest},
    {"windswept_gravelly_hills", world::biome::Biomes::WindsweptGravellyHills},
    {"windswept_savanna", world::biome::Biomes::WindsweptSavanna},
    {"wooded_badlands", world::biome::Biomes::WoodedBadlands},
    {"deep_dark", world::biome::Biomes::DeepDark},
    };
    const auto found = MAP.find(std::string(name));
    return found != MAP.end() ? std::optional<mc::BiomeId>(found->second) : std::nullopt;
}

} // namespace mc::diff::biome
