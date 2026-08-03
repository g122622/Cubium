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

#include "BiomeTags.hpp"
#include "BiomeIds.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeTag.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::world::biome {

// 使用 mc::Biomes 命名空间中的生物群系 ID 常量
using namespace mc::Biomes;

// ============================================================================
// BiomeTags 实现
// ============================================================================

std::unordered_map<ResourceLocation, std::unique_ptr<BiomeTag>>& BiomeTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<BiomeTag>> tags;
    return tags;
}

std::once_flag& BiomeTags::_getInitOnce()
{
    static std::once_flag once;
    return once;
}

// ========== 村庄标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_VILLAGE_DESERT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/village_desert"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/village_plains"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_VILLAGE_SAVANNA()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/village_savanna"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_VILLAGE_SNOWY()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/village_snowy"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_VILLAGE_TAIGA()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/village_taiga"));
    }
    return *tag;
}

// ========== 下界结构标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_BASTION_REMNANT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/bastion_remnant"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_FORTRESS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/fortress"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_NETHER_FOSSIL()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/nether_fossil"));
    }
    return *tag;
}

// ========== 主世界结构标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_BURIED_TREASURE()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/buried_treasure"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_DESERT_PYRAMID()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/desert_pyramid"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_END_CITY()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/end_city"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_IGLOO()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/igloo"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_JUNGLE_PYRAMID()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/jungle_pyramid"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_MANSION()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/mansion"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_MINESHAFT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/mineshaft"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_MINESHAFT_MESA()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/mineshaft_mesa"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_MONUMENT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/monument"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_OCEAN_RUIN_COLD()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ocean_ruin_cold"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_OCEAN_RUIN_WARM()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ocean_ruin_warm"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_PILLAGER_OUTPOST()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/pillager_outpost"));
    }
    return *tag;
}

// ========== 废弃传送门标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_DESERT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_desert"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_JUNGLE()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_jungle"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_MOUNTAIN()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_mountain"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_NETHER()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_nether"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_OCEAN()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_ocean"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_STANDARD()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_standard"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_SWAMP()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ruined_portal_swamp"));
    }
    return *tag;
}

// ========== 船只结构标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_SHIPWRECK()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/shipwreck"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_SHIPWRECK_BEACHED()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/shipwreck_beached"));
    }
    return *tag;
}

// ========== 其他结构标签 ==========

BiomeTag& BiomeTags::HAS_STRUCTURE_STRONGHOLD()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/stronghold"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_SWAMP_HUT()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/swamp_hut"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_TRAIL_RUINS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/trail_ruins"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_TRIAL_CHAMBERS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/trial_chambers"));
    }
    return *tag;
}

BiomeTag& BiomeTags::HAS_STRUCTURE_ANCIENT_CITY()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "has_structure/ancient_city"));
    }
    return *tag;
}

// ========== 游戏玩法标签 ==========

BiomeTag& BiomeTags::ALLOWS_SURFACE_SLIME_SPAWNS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "allows_surface_slime_spawns"));
    }
    return *tag;
}

// ========== 生物群系类型标签 ==========

BiomeTag& BiomeTags::IS_OCEAN()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_ocean"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_RIVER()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_river"));
    }
    return *tag;
}

// ========== 维度标签 ==========

BiomeTag& BiomeTags::IS_OVERWORLD()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_overworld"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_NETHER()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_nether"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_END()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_end"));
    }
    return *tag;
}

// ========== 地形类型标签 ==========

BiomeTag& BiomeTags::IS_DEEP_OCEAN()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_deep_ocean"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_BEACH()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_beach"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_MOUNTAIN()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_mountain"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_HILL()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_hill"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_TAIGA()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_taiga"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_JUNGLE()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_jungle"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_FOREST()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_forest"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_SAVANNA()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_savanna"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_BADLANDS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_badlands"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_MUSHROOM()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_mushroom"));
    }
    return *tag;
}

// ========== 更多游戏玩法标签 ==========

BiomeTag& BiomeTags::SPAWNS_COLD_VARIANT_FROGS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "spawns_cold_variant_frogs"));
    }
    return *tag;
}

BiomeTag& BiomeTags::SPAWNS_WARM_VARIANT_FROGS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "spawns_warm_variant_frogs"));
    }
    return *tag;
}

BiomeTag& BiomeTags::WITHOUT_ZOMBIE_SIEGES()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "without_zombie_sieges"));
    }
    return *tag;
}

BiomeTag& BiomeTags::WITHOUT_WANDERING_TRADER_SPAWNS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "without_wandering_trader_spawns"));
    }
    return *tag;
}

BiomeTag& BiomeTags::WITHOUT_PATROL_SPAWNS()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "without_patrol_spawns"));
    }
    return *tag;
}

BiomeTag& BiomeTags::STRONGHOLD_BIASED_TO()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "stronghold_biased_to"));
    }
    return *tag;
}

BiomeTag& BiomeTags::REQUIRED_OCEAN_MONUMENT_SURROUNDING()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "required_ocean_monument_surrounding"));
    }
    return *tag;
}

BiomeTag& BiomeTags::MINESHAFT_BLOCKING()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mineshaft_blocking"));
    }
    return *tag;
}

BiomeTag& BiomeTags::WATER_ON_MAP_OUTLINES()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "water_on_map_outlines"));
    }
    return *tag;
}

BiomeTag& BiomeTags::PRODUCES_CORALS_FROM_BONEMEAL()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "produces_corals_from_bonemeal"));
    }
    return *tag;
}

BiomeTag& BiomeTags::IS_VOID()
{
    static BiomeTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "is_void"));
    }
    return *tag;
}

// ============================================================================
// 初始化
// ============================================================================

void BiomeTags::initialize()
{
    // 线程安全：std::call_once 保证全进程只填充一次 _getTags()。多 worker 并发首次调用时，
    // 只有一个线程执行 lambda 填充，其余线程在 call_once 屏障等待，避免并发修改无锁的静态
    // unordered_map 导致损坏/悬空指针（曾引发 BiomeTag::contains 读 0x38 的 use-after-free）。
    std::call_once(_getInitOnce(), []() {
        auto& tags = _getTags();

        // ========== 村庄标签 ==========
        // village_desert: minecraft:desert
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/village_desert"));
            tag->addAll({Biomes::Desert});
            tags[tag->getId()] = std::move(tag);
        }

        // village_plains: minecraft:plains, minecraft:meadow
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/village_plains"));
            tag->addAll({Biomes::Plains, Biomes::Meadow});
            tags[tag->getId()] = std::move(tag);
        }

        // village_savanna: minecraft:savanna
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/village_savanna"));
            tag->addAll({Biomes::Savanna});
            tags[tag->getId()] = std::move(tag);
        }

        // village_snowy: minecraft:snowy_plains
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/village_snowy"));
            tag->addAll({Biomes::SnowyPlains});
            tags[tag->getId()] = std::move(tag);
        }

        // village_taiga: minecraft:taiga
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/village_taiga"));
            tag->addAll({Biomes::Taiga});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 下界结构标签 ==========

        // bastion_remnant: minecraft:crimson_forest, minecraft:nether_wastes,
        //                  minecraft:soul_sand_valley, minecraft:warped_forest
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/bastion_remnant"));
            tag->addAll({Biomes::CrimsonForest, Biomes::NetherWastes, Biomes::SoulSandValley, Biomes::WarpedForest});
            tags[tag->getId()] = std::move(tag);
        }

        // fortress: #minecraft:is_nether — 下界所有生物群系
        // 下界生物群系：nether_wastes(8), soul_sand_valley(170), crimson_forest(171),
        //              warped_forest(172), basalt_deltas(173)
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/fortress"));
            tag->addAll({Biomes::NetherWastes,
                Biomes::SoulSandValley,
                Biomes::CrimsonForest,
                Biomes::WarpedForest,
                Biomes::BasaltDeltas});
            tags[tag->getId()] = std::move(tag);
        }

        // nether_fossil: minecraft:soul_sand_valley
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/nether_fossil"));
            tag->addAll({Biomes::SoulSandValley});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 主世界结构标签 ==========

        // buried_treasure: #minecraft:is_beach — 所有海滩生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/buried_treasure"));
            tag->addAll({Biomes::Beach, Biomes::SnowyBeach, Biomes::StoneShore});
            tags[tag->getId()] = std::move(tag);
        }

        // desert_pyramid: minecraft:desert
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/desert_pyramid"));
            tag->addAll({Biomes::Desert});
            tags[tag->getId()] = std::move(tag);
        }

        // end_city: minecraft:end_highlands, minecraft:end_midlands
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/end_city"));
            tag->addAll({Biomes::EndHighlands, Biomes::EndMidlands});
            tags[tag->getId()] = std::move(tag);
        }

        // igloo: minecraft:snowy_taiga, minecraft:snowy_plains, minecraft:snowy_slopes
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/igloo"));
            tag->addAll({Biomes::SnowyTaiga, Biomes::SnowyPlains, Biomes::SnowySlopes});
            tags[tag->getId()] = std::move(tag);
        }

        // jungle_pyramid: minecraft:bamboo_jungle, minecraft:jungle
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/jungle_pyramid"));
            tag->addAll({Biomes::BambooJungle, Biomes::Jungle});
            tags[tag->getId()] = std::move(tag);
        }

        // mansion: minecraft:dark_forest, minecraft:pale_garden
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/mansion"));
            tag->addAll({Biomes::DarkForest, Biomes::PaleGarden});
            tags[tag->getId()] = std::move(tag);
        }

        // mineshaft: 多种主世界生物群系
        // 包含: #is_ocean, #is_river, #is_beach, #is_mountain, #is_hill, #is_taiga,
        //        #is_jungle, #is_forest 以及 stony_shore, mushroom_fields, ice_spikes,
        //        windswept_savanna, desert, savanna, snowy_plains, plains,
        //        sunflower_plains, swamp, mangrove_swamp, savanna_plateau,
        //        dripstone_caves, lush_caves
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/mineshaft"));
            tag->addAll({
                // 海洋生物群系
                Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean,
                // 河流
                Biomes::River,
                Biomes::FrozenRiver,
                // 海滩
                Biomes::Beach,
                Biomes::SnowyBeach,
                Biomes::StoneShore,
                // 山地
                Biomes::Mountains,
                Biomes::WoodedMountains,
                Biomes::GravellyMountains,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                // 丘陵
                Biomes::WoodedHills,
                Biomes::TaigaHills,
                Biomes::DesertHills,
                Biomes::JungleHills,
                Biomes::BirchForestHills,
                Biomes::DarkForestHills,
                Biomes::SnowyTaigaHills,
                Biomes::GiantTreeTaigaHills,
                Biomes::SnowyMountains,
                Biomes::TallBirchHills,
                Biomes::GiantSpruceTaigaHills,
                // 针叶林
                Biomes::Taiga,
                Biomes::SnowyTaiga,
                Biomes::GiantTreeTaiga,
                Biomes::GiantSpruceTaiga,
                // 丛林
                Biomes::Jungle,
                Biomes::JungleEdge,
                Biomes::BambooJungle,
                // 森林
                Biomes::Forest,
                Biomes::BirchForest,
                Biomes::DarkForest,
                Biomes::FlowerForest,
                Biomes::TallBirchForest,
                Biomes::CherryGrove,
                // 其他特定生物群系
                Biomes::MushroomFields,
                Biomes::IceSpikes,
                Biomes::ShatteredSavanna,
                Biomes::Desert,
                Biomes::Savanna,
                Biomes::SnowyPlains,
                Biomes::Plains,
                Biomes::SunflowerPlains,
                Biomes::Swamp,
                Biomes::MangroveSwamp,
                Biomes::SavannaPlateau,
                Biomes::DripstoneCaves,
                Biomes::LushCaves,
                Biomes::Meadow,
                Biomes::Grove,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // mineshaft_mesa: #minecraft:is_badlands — 恶地生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/mineshaft_mesa"));
            tag->addAll({Biomes::Badlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::ErodedBadlands,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau});
            tags[tag->getId()] = std::move(tag);
        }

        // monument: #minecraft:is_deep_ocean — 深海生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/monument"));
            tag->addAll({Biomes::DeepOcean,
                Biomes::DeepFrozenOcean,
                Biomes::DeepWarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::DeepColdOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // ocean_ruin_cold: 冷水海洋生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ocean_ruin_cold"));
            tag->addAll({Biomes::FrozenOcean,
                Biomes::ColdOcean,
                Biomes::Ocean,
                Biomes::DeepFrozenOcean,
                Biomes::DeepColdOcean,
                Biomes::DeepOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // ocean_ruin_warm: 温水海洋生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ocean_ruin_warm"));
            tag->addAll({Biomes::LukewarmOcean, Biomes::WarmOcean, Biomes::DeepLukewarmOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // pillager_outpost: desert, plains, savanna, snowy_plains, taiga,
        //                   #is_mountain, grove
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/pillager_outpost"));
            tag->addAll({Biomes::Desert,
                Biomes::Plains,
                Biomes::Savanna,
                Biomes::SnowyPlains,
                Biomes::Taiga,
                Biomes::Mountains,
                Biomes::WoodedMountains,
                Biomes::GravellyMountains,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                Biomes::Grove,
                Biomes::ModifiedGravellyMountains});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 废弃传送门标签 ==========

        // ruined_portal_desert: minecraft:desert
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_desert"));
            tag->addAll({Biomes::Desert});
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_jungle: #minecraft:is_jungle
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_jungle"));
            tag->addAll({Biomes::Jungle,
                Biomes::JungleHills,
                Biomes::JungleEdge,
                Biomes::BambooJungle,
                Biomes::BambooJungleHills,
                Biomes::ModifiedJungle,
                Biomes::ModifiedJungleEdge});
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_mountain: #is_badlands, #is_hill, savanna_plateau,
        //                         windswept_savanna, stony_shore, #is_mountain
        {
            auto tag =
                std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_mountain"));
            tag->addAll({
                // 恶地
                Biomes::Badlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::ErodedBadlands,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau,
                // 丘陵
                Biomes::WoodedHills,
                Biomes::TaigaHills,
                Biomes::DesertHills,
                Biomes::JungleHills,
                Biomes::BirchForestHills,
                Biomes::DarkForestHills,
                Biomes::SnowyTaigaHills,
                Biomes::GiantTreeTaigaHills,
                Biomes::SnowyMountains,
                Biomes::TallBirchHills,
                Biomes::GiantSpruceTaigaHills,
                // 山地
                Biomes::Mountains,
                Biomes::WoodedMountains,
                Biomes::GravellyMountains,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                Biomes::ModifiedGravellyMountains,
                // 其他
                Biomes::SavannaPlateau,
                Biomes::ShatteredSavanna,
                Biomes::StoneShore,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_nether: #minecraft:is_nether
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_nether"));
            tag->addAll({Biomes::NetherWastes,
                Biomes::SoulSandValley,
                Biomes::CrimsonForest,
                Biomes::WarpedForest,
                Biomes::BasaltDeltas});
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_ocean: #minecraft:is_ocean
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_ocean"));
            tag->addAll({Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_standard: #is_beach, #is_river, #is_taiga, #is_forest,
        //                         mushroom_fields, ice_spikes, dripstone_caves, lush_caves,
        //                         savanna, snowy_plains, plains, sunflower_plains
        {
            auto tag =
                std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_standard"));
            tag->addAll({
                // 海滩
                Biomes::Beach,
                Biomes::SnowyBeach,
                Biomes::StoneShore,
                // 河流
                Biomes::River,
                Biomes::FrozenRiver,
                // 针叶林
                Biomes::Taiga,
                Biomes::SnowyTaiga,
                Biomes::GiantTreeTaiga,
                Biomes::GiantSpruceTaiga,
                // 森林
                Biomes::Forest,
                Biomes::BirchForest,
                Biomes::DarkForest,
                Biomes::FlowerForest,
                Biomes::TallBirchForest,
                Biomes::CherryGrove,
                // 其他
                Biomes::MushroomFields,
                Biomes::IceSpikes,
                Biomes::DripstoneCaves,
                Biomes::LushCaves,
                Biomes::Savanna,
                Biomes::SnowyPlains,
                Biomes::Plains,
                Biomes::SunflowerPlains,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // ruined_portal_swamp: minecraft:swamp, minecraft:mangrove_swamp
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ruined_portal_swamp"));
            tag->addAll({Biomes::Swamp, Biomes::MangroveSwamp});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 船只结构标签 ==========

        // shipwreck: #minecraft:is_ocean
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/shipwreck"));
            tag->addAll({Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // shipwreck_beached: #minecraft:is_beach
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/shipwreck_beached"));
            tag->addAll({Biomes::Beach, Biomes::SnowyBeach, Biomes::StoneShore});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 其他结构标签 ==========

        // stronghold: #minecraft:is_overworld — 主世界所有生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/stronghold"));
            // 主世界生物群系（排除下界和末地）
            tag->addAll({
                Biomes::Ocean,
                Biomes::Plains,
                Biomes::Desert,
                Biomes::Mountains,
                Biomes::Forest,
                Biomes::Taiga,
                Biomes::Swamp,
                Biomes::River,
                Biomes::FrozenOcean,
                Biomes::FrozenRiver,
                Biomes::SnowyPlains,
                Biomes::SnowyMountains,
                Biomes::MushroomFields,
                Biomes::MushroomFieldShore,
                Biomes::Beach,
                Biomes::DesertHills,
                Biomes::WoodedHills,
                Biomes::TaigaHills,
                Biomes::MountainEdge,
                Biomes::Jungle,
                Biomes::JungleHills,
                Biomes::JungleEdge,
                Biomes::DeepOcean,
                Biomes::StoneShore,
                Biomes::SnowyBeach,
                Biomes::BirchForest,
                Biomes::BirchForestHills,
                Biomes::DarkForest,
                Biomes::SnowyTaiga,
                Biomes::SnowyTaigaHills,
                Biomes::GiantTreeTaiga,
                Biomes::GiantTreeTaigaHills,
                Biomes::WoodedMountains,
                Biomes::Savanna,
                Biomes::SavannaPlateau,
                Biomes::Badlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::WarmOcean,
                Biomes::LukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepWarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::DeepColdOcean,
                Biomes::DeepFrozenOcean,
                Biomes::SunflowerPlains,
                Biomes::DesertLakes,
                Biomes::GravellyMountains,
                Biomes::FlowerForest,
                Biomes::TaigaMountains,
                Biomes::SwampHills,
                Biomes::IceSpikes,
                Biomes::ModifiedJungle,
                Biomes::ModifiedJungleEdge,
                Biomes::TallBirchForest,
                Biomes::TallBirchHills,
                Biomes::DarkForestHills,
                Biomes::SnowyTaigaMountains,
                Biomes::GiantSpruceTaiga,
                Biomes::GiantSpruceTaigaHills,
                Biomes::ModifiedGravellyMountains,
                Biomes::ShatteredSavanna,
                Biomes::ShatteredSavannaPlateau,
                Biomes::ErodedBadlands,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau,
                Biomes::BambooJungle,
                Biomes::BambooJungleHills,
                Biomes::Meadow,
                Biomes::Grove,
                Biomes::SnowySlopes,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                Biomes::DripstoneCaves,
                Biomes::LushCaves,
                Biomes::DeepDark,
                Biomes::MangroveSwamp,
                Biomes::CherryGrove,
                Biomes::PaleGarden,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // swamp_hut: minecraft:swamp
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/swamp_hut"));
            tag->addAll({Biomes::Swamp});
            tags[tag->getId()] = std::move(tag);
        }

        // trail_ruins: taiga, snowy_taiga, old_growth_pine_taiga, old_growth_spruce_taiga,
        //              old_growth_birch_forest, jungle
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/trail_ruins"));
            tag->addAll({Biomes::Taiga,
                Biomes::SnowyTaiga,
                Biomes::OldGrowthPineTaiga,
                Biomes::OldGrowthSpruceTaiga,
                Biomes::OldGrowthBirchForest,
                Biomes::Jungle});
            tags[tag->getId()] = std::move(tag);
        }

        // trial_chambers: 大量主世界生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/trial_chambers"));
            tag->addAll({
                Biomes::MushroomFields,
                Biomes::DeepFrozenOcean,
                Biomes::FrozenOcean,
                Biomes::DeepColdOcean,
                Biomes::ColdOcean,
                Biomes::DeepOcean,
                Biomes::Ocean,
                Biomes::DeepLukewarmOcean,
                Biomes::LukewarmOcean,
                Biomes::WarmOcean,
                Biomes::StoneShore,
                Biomes::Swamp,
                Biomes::MangroveSwamp,
                Biomes::SnowySlopes,
                Biomes::SnowyPlains,
                Biomes::SnowyBeach,
                Biomes::GravellyMountains,
                Biomes::Grove,
                Biomes::Mountains,
                Biomes::SnowyTaiga,
                Biomes::WoodedMountains,
                Biomes::Taiga,
                Biomes::Plains,
                Biomes::Meadow,
                Biomes::Beach,
                Biomes::Forest,
                Biomes::OldGrowthSpruceTaiga,
                Biomes::FlowerForest,
                Biomes::BirchForest,
                Biomes::DarkForest,
                Biomes::PaleGarden,
                Biomes::SavannaPlateau,
                Biomes::Savanna,
                Biomes::Jungle,
                Biomes::Badlands,
                Biomes::Desert,
                Biomes::WoodedBadlands,
                Biomes::JaggedPeaks,
                Biomes::StonyPeaks,
                Biomes::FrozenRiver,
                Biomes::River,
                Biomes::IceSpikes,
                Biomes::OldGrowthPineTaiga,
                Biomes::SunflowerPlains,
                Biomes::OldGrowthBirchForest,
                Biomes::SparseJungle,
                Biomes::BambooJungle,
                Biomes::ErodedBadlands,
                Biomes::ShatteredSavanna,
                Biomes::CherryGrove,
                Biomes::FrozenPeaks,
                Biomes::DripstoneCaves,
                Biomes::LushCaves,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // ancient_city: minecraft:deep_dark
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "has_structure/ancient_city"));
            tag->addAll({Biomes::DeepDark});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 游戏玩法标签 ==========

        // allows_surface_slime_spawns: 沼泽和红树林沼泽允许地表史莱姆生成
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "allows_surface_slime_spawns"));
            tag->addAll({Biomes::Swamp, Biomes::MangroveSwamp});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 生物群系类型标签 ==========

        // is_ocean: 所有海洋生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_ocean"));
            tag->addAll({Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // is_river: 所有河流生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_river"));
            tag->addAll({Biomes::River, Biomes::FrozenRiver});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 维度标签 ==========

        // is_overworld: 主世界所有生物群系（与 stronghold 标签的生物群系列表相同）
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_overworld"));
            tag->addAll({
                Biomes::Ocean,
                Biomes::Plains,
                Biomes::Desert,
                Biomes::Mountains,
                Biomes::Forest,
                Biomes::Taiga,
                Biomes::Swamp,
                Biomes::River,
                Biomes::FrozenOcean,
                Biomes::FrozenRiver,
                Biomes::SnowyPlains,
                Biomes::SnowyMountains,
                Biomes::MushroomFields,
                Biomes::MushroomFieldShore,
                Biomes::Beach,
                Biomes::DesertHills,
                Biomes::WoodedHills,
                Biomes::TaigaHills,
                Biomes::MountainEdge,
                Biomes::Jungle,
                Biomes::JungleHills,
                Biomes::JungleEdge,
                Biomes::DeepOcean,
                Biomes::StoneShore,
                Biomes::SnowyBeach,
                Biomes::BirchForest,
                Biomes::BirchForestHills,
                Biomes::DarkForest,
                Biomes::SnowyTaiga,
                Biomes::SnowyTaigaHills,
                Biomes::GiantTreeTaiga,
                Biomes::GiantTreeTaigaHills,
                Biomes::WoodedMountains,
                Biomes::Savanna,
                Biomes::SavannaPlateau,
                Biomes::Badlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::WarmOcean,
                Biomes::LukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepWarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::DeepColdOcean,
                Biomes::DeepFrozenOcean,
                Biomes::SunflowerPlains,
                Biomes::DesertLakes,
                Biomes::GravellyMountains,
                Biomes::FlowerForest,
                Biomes::TaigaMountains,
                Biomes::SwampHills,
                Biomes::IceSpikes,
                Biomes::ModifiedJungle,
                Biomes::ModifiedJungleEdge,
                Biomes::TallBirchForest,
                Biomes::TallBirchHills,
                Biomes::DarkForestHills,
                Biomes::SnowyTaigaMountains,
                Biomes::GiantSpruceTaiga,
                Biomes::GiantSpruceTaigaHills,
                Biomes::ModifiedGravellyMountains,
                Biomes::ShatteredSavanna,
                Biomes::ShatteredSavannaPlateau,
                Biomes::ErodedBadlands,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau,
                Biomes::BambooJungle,
                Biomes::BambooJungleHills,
                Biomes::Meadow,
                Biomes::Grove,
                Biomes::SnowySlopes,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                Biomes::DripstoneCaves,
                Biomes::LushCaves,
                Biomes::DeepDark,
                Biomes::MangroveSwamp,
                Biomes::CherryGrove,
                Biomes::PaleGarden,
            });
            tags[tag->getId()] = std::move(tag);
        }

        // is_nether: 下界所有生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_nether"));
            tag->addAll({Biomes::NetherWastes,
                Biomes::SoulSandValley,
                Biomes::CrimsonForest,
                Biomes::WarpedForest,
                Biomes::BasaltDeltas});
            tags[tag->getId()] = std::move(tag);
        }

        // is_end: 末地所有生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_end"));
            tag->addAll({Biomes::TheEnd,
                Biomes::SmallEndIslands,
                Biomes::EndMidlands,
                Biomes::EndHighlands,
                Biomes::EndBarrens});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 地形类型标签 ==========

        // is_deep_ocean: 深海生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_deep_ocean"));
            tag->addAll({Biomes::DeepOcean,
                Biomes::DeepFrozenOcean,
                Biomes::DeepWarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::DeepColdOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // is_beach: 海滩生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_beach"));
            tag->addAll({Biomes::Beach, Biomes::SnowyBeach, Biomes::StoneShore});
            tags[tag->getId()] = std::move(tag);
        }

        // is_mountain: 山地生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_mountain"));
            tag->addAll({Biomes::Mountains,
                Biomes::WoodedMountains,
                Biomes::GravellyMountains,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::StonyPeaks,
                Biomes::ModifiedGravellyMountains});
            tags[tag->getId()] = std::move(tag);
        }

        // is_hill: 丘陵生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_hill"));
            tag->addAll({Biomes::WoodedHills,
                Biomes::TaigaHills,
                Biomes::DesertHills,
                Biomes::JungleHills,
                Biomes::BirchForestHills,
                Biomes::DarkForestHills,
                Biomes::SnowyTaigaHills,
                Biomes::GiantTreeTaigaHills,
                Biomes::SnowyMountains,
                Biomes::TallBirchHills,
                Biomes::GiantSpruceTaigaHills});
            tags[tag->getId()] = std::move(tag);
        }

        // is_taiga: 针叶林生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_taiga"));
            tag->addAll({Biomes::Taiga, Biomes::SnowyTaiga, Biomes::GiantTreeTaiga, Biomes::GiantSpruceTaiga});
            tags[tag->getId()] = std::move(tag);
        }

        // is_jungle: 丛林生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_jungle"));
            tag->addAll({Biomes::Jungle,
                Biomes::JungleHills,
                Biomes::JungleEdge,
                Biomes::BambooJungle,
                Biomes::BambooJungleHills,
                Biomes::ModifiedJungle,
                Biomes::ModifiedJungleEdge});
            tags[tag->getId()] = std::move(tag);
        }

        // is_forest: 森林生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_forest"));
            tag->addAll({Biomes::Forest,
                Biomes::BirchForest,
                Biomes::DarkForest,
                Biomes::FlowerForest,
                Biomes::TallBirchForest,
                Biomes::CherryGrove,
                Biomes::PaleGarden});
            tags[tag->getId()] = std::move(tag);
        }

        // is_savanna: 热带草原生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_savanna"));
            tag->addAll(
                {Biomes::Savanna, Biomes::SavannaPlateau, Biomes::ShatteredSavanna, Biomes::ShatteredSavannaPlateau});
            tags[tag->getId()] = std::move(tag);
        }

        // is_badlands: 恶地生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_badlands"));
            tag->addAll({Biomes::Badlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::ErodedBadlands,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau});
            tags[tag->getId()] = std::move(tag);
        }

        // is_mushroom: 蘑菇岛生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_mushroom"));
            tag->addAll({Biomes::MushroomFields, Biomes::MushroomFieldShore});
            tags[tag->getId()] = std::move(tag);
        }

        // ========== 更多游戏玩法标签 ==========

        // spawns_cold_variant_frogs: 寒冷变体青蛙生成
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "spawns_cold_variant_frogs"));
            tag->addAll({Biomes::SnowyPlains,
                Biomes::IceSpikes,
                Biomes::SnowyTaiga,
                Biomes::SnowyBeach,
                Biomes::FrozenOcean,
                Biomes::FrozenRiver,
                Biomes::SnowyMountains,
                Biomes::Grove,
                Biomes::SnowySlopes,
                Biomes::JaggedPeaks,
                Biomes::FrozenPeaks,
                Biomes::DeepFrozenOcean,
                Biomes::SnowyTaigaHills,
                Biomes::SnowyTaigaMountains});
            tags[tag->getId()] = std::move(tag);
        }

        // spawns_warm_variant_frogs: 温暖变体青蛙生成
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "spawns_warm_variant_frogs"));
            tag->addAll({Biomes::Desert,
                Biomes::Badlands,
                Biomes::ErodedBadlands,
                Biomes::WoodedBadlandsPlateau,
                Biomes::BadlandsPlateau,
                Biomes::ModifiedWoodedBadlandsPlateau,
                Biomes::ModifiedBadlandsPlateau,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::Savanna,
                Biomes::SavannaPlateau,
                Biomes::ShatteredSavanna,
                Biomes::ShatteredSavannaPlateau,
                Biomes::StonyPeaks,
                Biomes::MangroveSwamp,
                Biomes::BasaltDeltas,
                Biomes::CrimsonForest,
                Biomes::NetherWastes,
                Biomes::SoulSandValley,
                Biomes::WarpedForest});
            tags[tag->getId()] = std::move(tag);
        }

        // without_zombie_sieges: 无僵尸围城
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "without_zombie_sieges"));
            tags[tag->getId()] = std::move(tag);
        }

        // without_wandering_trader_spawns: 无流浪商人
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "without_wandering_trader_spawns"));
            tags[tag->getId()] = std::move(tag);
        }

        // without_patrol_spawns: 无巡逻队
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "without_patrol_spawns"));
            tags[tag->getId()] = std::move(tag);
        }

        // stronghold_biased_to: 偏向要塞生成
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "stronghold_biased_to"));
            tags[tag->getId()] = std::move(tag);
        }

        // required_ocean_monument_surrounding: 需要海底神殿周围
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "required_ocean_monument_surrounding"));
            tag->addAll({Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean,
                Biomes::River,
                Biomes::FrozenRiver,
                Biomes::Beach,
                Biomes::SnowyBeach,
                Biomes::StoneShore});
            tags[tag->getId()] = std::move(tag);
        }

        // mineshaft_blocking: 阻止矿井生成
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "mineshaft_blocking"));
            tags[tag->getId()] = std::move(tag);
        }

        // water_on_map_outlines: 地图上显示水
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "water_on_map_outlines"));
            tag->addAll({Biomes::Ocean,
                Biomes::DeepOcean,
                Biomes::FrozenOcean,
                Biomes::DeepFrozenOcean,
                Biomes::WarmOcean,
                Biomes::DeepWarmOcean,
                Biomes::LukewarmOcean,
                Biomes::DeepLukewarmOcean,
                Biomes::ColdOcean,
                Biomes::DeepColdOcean,
                Biomes::River,
                Biomes::FrozenRiver,
                Biomes::Swamp,
                Biomes::MangroveSwamp});
            tags[tag->getId()] = std::move(tag);
        }

        // produces_corals_from_bonemeal: 骨粉可生成珊瑚
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "produces_corals_from_bonemeal"));
            tag->addAll({Biomes::WarmOcean, Biomes::DeepWarmOcean});
            tags[tag->getId()] = std::move(tag);
        }

        // is_void: 虚空生物群系
        {
            auto tag = std::make_unique<BiomeTag>(ResourceLocation("minecraft", "is_void"));
            tag->addAll({Biomes::TheVoid});
            tags[tag->getId()] = std::move(tag);
        }

        spdlog::info("BiomeTags: initialized {} has_structure tags", tags.size());
    });
}

// ============================================================================
// 查询方法
// ============================================================================

BiomeTag* BiomeTags::getTag(const ResourceLocation& id)
{
    // 确保已初始化：call_once 保证只填充一次。多 worker 并发调用时，首个线程填充，
    // 其余在 call_once 屏障等待，随后 _getTags() 只读访问安全。
    initialize();
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool BiomeTags::hasStructure(BiomeId biomeId, const ResourceLocation& tagId)
{
    auto* tag = getTag(tagId);
    if (tag == nullptr) {
        return false;
    }
    return tag->contains(biomeId);
}

void BiomeTags::forEachTag(std::function<void(const BiomeTag&)> callback)
{
    initialize();
    auto& tags = _getTags();
    for (const auto& [id, tag] : tags) {
        callback(*tag);
    }
}

} // namespace mc::world::biome
