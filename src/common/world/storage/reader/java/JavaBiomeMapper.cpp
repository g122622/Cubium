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

#include "JavaBiomeMapper.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include <string>

namespace mc::world::storage::reader::java {

JavaBiomeMapper::JavaBiomeMapper()
{
    _initializeMappings();
}

BiomeId JavaBiomeMapper::mapBiome(const std::string& biomeName)
{
    auto it = m_nameToId.find(biomeName);
    if (it != m_nameToId.end()) {
        return it->second;
    }

    // 去掉 "minecraft:" 前缀再试
    if (biomeName.starts_with("minecraft:")) {
        std::string name = biomeName.substr(10);
        it = m_nameToId.find(name);
        if (it != m_nameToId.end()) {
            return it->second;
        }
    }

    // 未知生物群系默认返回海洋
    return Biomes::Ocean;
}

BiomeId JavaBiomeMapper::mapBiome(i32 numericId)
{
    // Java 1.16.5 生物群系数值 ID 与项目内部 ID 一致
    if (numericId >= 0 && numericId <= 255) {
        return static_cast<BiomeId>(numericId);
    }
    return Biomes::Ocean;
}

void JavaBiomeMapper::_initializeMappings()
{
    // Java 1.16.5 生物群系名称→ID 映射
    // 同时注册旧名称和新名称（1.18+ 重命名）以便兼容

    // 基础生物群系 (0-13)
    m_nameToId["minecraft:ocean"] = Biomes::Ocean;
    m_nameToId["minecraft:plains"] = Biomes::Plains;
    m_nameToId["minecraft:desert"] = Biomes::Desert;
    m_nameToId["minecraft:mountains"] = Biomes::Mountains;
    m_nameToId["minecraft:extreme_hills"] = Biomes::Mountains;
    m_nameToId["minecraft:windswept_hills"] = Biomes::Mountains;
    m_nameToId["minecraft:forest"] = Biomes::Forest;
    m_nameToId["minecraft:taiga"] = Biomes::Taiga;
    m_nameToId["minecraft:swamp"] = Biomes::Swamp;
    m_nameToId["minecraft:river"] = Biomes::River;
    m_nameToId["minecraft:nether_wastes"] = Biomes::NetherWastes;
    m_nameToId["minecraft:the_end"] = Biomes::TheEnd;
    m_nameToId["minecraft:frozen_ocean"] = Biomes::FrozenOcean;
    m_nameToId["minecraft:frozen_river"] = Biomes::FrozenRiver;
    m_nameToId["minecraft:snowy_plains"] = Biomes::SnowyPlains;
    m_nameToId["minecraft:snowy_tundra"] = Biomes::SnowyPlains;
    m_nameToId["minecraft:snowy_mountains"] = Biomes::SnowyMountains;

    // 蘑菇岛 (14-15)
    m_nameToId["minecraft:mushroom_fields"] = Biomes::MushroomFields;
    m_nameToId["minecraft:mushroom_field_shore"] = Biomes::MushroomFieldShore;

    // 海滩 (16)
    m_nameToId["minecraft:beach"] = Biomes::Beach;

    // 丘陵 (17-20)
    m_nameToId["minecraft:desert_hills"] = Biomes::DesertHills;
    m_nameToId["minecraft:wooded_hills"] = Biomes::WoodedHills;
    m_nameToId["minecraft:taiga_hills"] = Biomes::TaigaHills;
    m_nameToId["minecraft:mountain_edge"] = Biomes::MountainEdge;

    // 丛林 (21-23)
    m_nameToId["minecraft:jungle"] = Biomes::Jungle;
    m_nameToId["minecraft:jungle_hills"] = Biomes::JungleHills;
    m_nameToId["minecraft:jungle_edge"] = Biomes::JungleEdge;
    m_nameToId["minecraft:sparse_jungle"] = Biomes::JungleEdge;

    // 深海和石岸 (24-25)
    m_nameToId["minecraft:deep_ocean"] = Biomes::DeepOcean;
    m_nameToId["minecraft:stony_shore"] = Biomes::StoneShore;
    m_nameToId["minecraft:stone_shore"] = Biomes::StoneShore;

    // 雪地海滩 (26)
    m_nameToId["minecraft:snowy_beach"] = Biomes::SnowyBeach;

    // 桦木森林 (27-28)
    m_nameToId["minecraft:birch_forest"] = Biomes::BirchForest;
    m_nameToId["minecraft:birch_forest_hills"] = Biomes::BirchForestHills;

    // 黑森林 (29)
    m_nameToId["minecraft:dark_forest"] = Biomes::DarkForest;

    // 雪地针叶林 (30-31)
    m_nameToId["minecraft:snowy_taiga"] = Biomes::SnowyTaiga;
    m_nameToId["minecraft:snowy_taiga_hills"] = Biomes::SnowyTaigaHills;

    // 大型针叶林 (32-33)
    m_nameToId["minecraft:giant_tree_taiga"] = Biomes::GiantTreeTaiga;
    m_nameToId["minecraft:old_growth_pine_taiga"] = Biomes::GiantTreeTaiga;
    m_nameToId["minecraft:giant_tree_taiga_hills"] = Biomes::GiantTreeTaigaHills;
    m_nameToId["minecraft:old_growth_pine_taiga_hills"] = Biomes::GiantTreeTaigaHills;

    // 山地变体和热带草原 (34-36)
    m_nameToId["minecraft:wooded_mountains"] = Biomes::WoodedMountains;
    m_nameToId["minecraft:extreme_hills_with_trees"] = Biomes::WoodedMountains;
    m_nameToId["minecraft:windswept_forest"] = Biomes::WoodedMountains;
    m_nameToId["minecraft:savanna"] = Biomes::Savanna;
    m_nameToId["minecraft:savanna_plateau"] = Biomes::SavannaPlateau;

    // 恶地 (37-39)
    m_nameToId["minecraft:badlands"] = Biomes::Badlands;
    m_nameToId["minecraft:wooded_badlands_plateau"] = Biomes::WoodedBadlandsPlateau;
    m_nameToId["minecraft:wooded_badlands"] = Biomes::WoodedBadlandsPlateau;
    m_nameToId["minecraft:badlands_plateau"] = Biomes::BadlandsPlateau;

    // 末地生物群系 (40-43)
    m_nameToId["minecraft:small_end_islands"] = Biomes::SmallEndIslands;
    m_nameToId["minecraft:end_midlands"] = Biomes::EndMidlands;
    m_nameToId["minecraft:end_highlands"] = Biomes::EndHighlands;
    m_nameToId["minecraft:end_barrens"] = Biomes::EndBarrens;

    // 海洋温度变体 (44-50)
    m_nameToId["minecraft:warm_ocean"] = Biomes::WarmOcean;
    m_nameToId["minecraft:lukewarm_ocean"] = Biomes::LukewarmOcean;
    m_nameToId["minecraft:cold_ocean"] = Biomes::ColdOcean;
    m_nameToId["minecraft:deep_warm_ocean"] = Biomes::DeepWarmOcean;
    m_nameToId["minecraft:deep_lukewarm_ocean"] = Biomes::DeepLukewarmOcean;
    m_nameToId["minecraft:deep_cold_ocean"] = Biomes::DeepColdOcean;
    m_nameToId["minecraft:deep_frozen_ocean"] = Biomes::DeepFrozenOcean;

    // 变体/稀有生物群系 (129+)
    m_nameToId["minecraft:sunflower_plains"] = Biomes::SunflowerPlains;
    m_nameToId["minecraft:desert_lakes"] = Biomes::DesertLakes;
    m_nameToId["minecraft:gravelly_mountains"] = Biomes::GravellyMountains;
    m_nameToId["minecraft:flower_forest"] = Biomes::FlowerForest;
    m_nameToId["minecraft:taiga_mountains"] = Biomes::TaigaMountains;
    m_nameToId["minecraft:swamp_hills"] = Biomes::SwampHills;
    m_nameToId["minecraft:ice_spikes"] = Biomes::IceSpikes;
    m_nameToId["minecraft:modified_jungle"] = Biomes::ModifiedJungle;
    m_nameToId["minecraft:modified_jungle_edge"] = Biomes::ModifiedJungleEdge;
    m_nameToId["minecraft:tall_birch_forest"] = Biomes::TallBirchForest;
    m_nameToId["minecraft:tall_birch_hills"] = Biomes::TallBirchHills;
    m_nameToId["minecraft:dark_forest_hills"] = Biomes::DarkForestHills;
    m_nameToId["minecraft:snowy_taiga_mountains"] = Biomes::SnowyTaigaMountains;
    m_nameToId["minecraft:giant_spruce_taiga"] = Biomes::GiantSpruceTaiga;
    m_nameToId["minecraft:old_growth_spruce_taiga"] = Biomes::GiantSpruceTaiga;
    m_nameToId["minecraft:giant_spruce_taiga_hills"] = Biomes::GiantSpruceTaigaHills;
    m_nameToId["minecraft:modified_gravelly_mountains"] = Biomes::ModifiedGravellyMountains;
    m_nameToId["minecraft:shattered_savanna"] = Biomes::ShatteredSavanna;
    m_nameToId["minecraft:windswept_savanna"] = Biomes::ShatteredSavanna;
    m_nameToId["minecraft:shattered_savanna_plateau"] = Biomes::ShatteredSavannaPlateau;
    m_nameToId["minecraft:eroded_badlands"] = Biomes::ErodedBadlands;
    m_nameToId["minecraft:modified_wooded_badlands_plateau"] = Biomes::ModifiedWoodedBadlandsPlateau;
    m_nameToId["minecraft:modified_badlands_plateau"] = Biomes::ModifiedBadlandsPlateau;
    m_nameToId["minecraft:bamboo_jungle"] = Biomes::BambooJungle;
    m_nameToId["minecraft:bamboo_jungle_hills"] = Biomes::BambooJungleHills;

    // 下界生物群系 (170-173)
    m_nameToId["minecraft:soul_sand_valley"] = Biomes::SoulSandValley;
    m_nameToId["minecraft:crimson_forest"] = Biomes::CrimsonForest;
    m_nameToId["minecraft:warped_forest"] = Biomes::WarpedForest;
    m_nameToId["minecraft:basalt_deltas"] = Biomes::BasaltDeltas;

    // 1.18+ 新生物群系映射到最接近的已有 ID
    m_nameToId["minecraft:meadow"] = Biomes::Plains;
    m_nameToId["minecraft:grove"] = Biomes::Taiga;
    m_nameToId["minecraft:snowy_slopes"] = Biomes::SnowyPlains;
    m_nameToId["minecraft:jagged_peaks"] = Biomes::Mountains;
    m_nameToId["minecraft:frozen_peaks"] = Biomes::SnowyMountains;
    m_nameToId["minecraft:stony_peaks"] = Biomes::Mountains;
    m_nameToId["minecraft:dripstone_caves"] = Biomes::TheEnd; // 无精确映射
    m_nameToId["minecraft:lush_caves"] = Biomes::TheEnd;      // 无精确映射
}

} // namespace mc::world::storage::reader::java
