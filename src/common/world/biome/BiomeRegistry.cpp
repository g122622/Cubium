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

#include "BiomeRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeFactory.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include <utility>

using namespace mc::trace;

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// BiomeRegistry 实现
// ============================================================================

BiomeRegistry& BiomeRegistry::instance()
{
    static BiomeRegistry instance;
    return instance;
}

BiomeRegistry::BiomeRegistry()
    : m_defaultBiome(Biomes::Plains, "plains")
{}

void BiomeRegistry::initialize()
{
    if (!m_registered.empty()) {
        return; // 已经初始化
    }
    _registerDefaultBiomes();
}

void BiomeRegistry::registerBiome(Biome biome)
{
    const BiomeId id = biome.id();
    if (id >= m_biomes.size()) {
        m_biomes.resize(id + 1);
        m_registered.resize(id + 1, false);
    }
    m_biomes[id] = std::move(biome);
    m_registered[id] = true;
}

const Biome& BiomeRegistry::get(BiomeId id) const
{
    if (id < m_biomes.size() && m_registered[id]) {
        return m_biomes[id];
    }
    return m_defaultBiome;
}

Biome& BiomeRegistry::getMutable(BiomeId id)
{
    if (id < m_biomes.size() && m_registered[id]) {
        return m_biomes[id];
    }
    return m_defaultBiome;
}

bool BiomeRegistry::hasBiome(BiomeId id) const
{
    return id < m_biomes.size() && m_registered[id];
}

void BiomeRegistry::_registerDefaultBiomes()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "BiomeRegistry::_registerDefaultBiomes");

    // 注册所有默认生物群系

    // === 基础生物群系 (0-13) ===
    registerBiome(BiomeFactory::createOcean());
    registerBiome(BiomeFactory::createPlains());
    registerBiome(BiomeFactory::createDesert());
    registerBiome(BiomeFactory::createMountains());
    registerBiome(BiomeFactory::createForest());
    registerBiome(BiomeFactory::createTaiga());
    registerBiome(BiomeFactory::createSwamp());
    registerBiome(BiomeFactory::createRiver());
    registerBiome(BiomeFactory::createFrozenOcean());
    registerBiome(BiomeFactory::createFrozenRiver());
    registerBiome(BiomeFactory::createSnowyPlains());

    // === 蘑菇岛 (14-15) ===
    registerBiome(BiomeFactory::createMushroomFields());
    // mushroom_field_shore 是 1.16.5 已删除变体，vanilla 1.21.11 无此群系，不再注册。

    // === 海滩 (16) ===
    registerBiome(BiomeFactory::createBeach());

    // === 山地变体和丘陵 (17-20) ===
    // desert_hills/wooded_hills/taiga_hills/mountain_edge 均为 1.16.5 已删除变体，不再注册。

    // === 丛林 (21-23) ===
    registerBiome(BiomeFactory::createJungle());
    // jungle_hills 是 1.16.5 已删除变体，不再注册。
    registerBiome(BiomeFactory::createJungleEdge());

    // === 深海和石岸 (24-25) ===
    registerBiome(BiomeFactory::createDeepOcean());
    registerBiome(BiomeFactory::createStoneShore());

    // === 雪地海滩 (26) ===
    registerBiome(BiomeFactory::createSnowyBeach());

    // === 桦木森林 (27-28) ===
    registerBiome(BiomeFactory::createBirchForest());
    // birch_forest_hills 是 1.16.5 已删除变体，不再注册。

    // === 黑森林 (29) ===
    registerBiome(BiomeFactory::createDarkForest());

    // === 雪地针叶林 (30-31) ===
    registerBiome(BiomeFactory::createSnowyTaiga());
    // snowy_taiga_hills 是 1.16.5 已删除变体，不再注册。

    // === 大型针叶林 (32-33) ===
    registerBiome(BiomeFactory::createGiantTreeTaiga());
    // giant_tree_taiga_hills 是 1.16.5 已删除变体，不再注册。

    // === 热带草原 (34-36) ===
    registerBiome(BiomeFactory::createWoodedMountains());
    registerBiome(BiomeFactory::createSavanna());
    registerBiome(BiomeFactory::createSavannaPlateau());

    // === 恶地 (37-39) ===
    registerBiome(BiomeFactory::createBadlands());
    registerBiome(BiomeFactory::createWoodedBadlandsPlateau());
    // badlands_plateau 是 1.16.5 已删除变体，不再注册。

    // === 海洋温度变体 (44-50) ===
    registerBiome(BiomeFactory::createWarmOcean());
    registerBiome(BiomeFactory::createLukewarmOcean());
    registerBiome(BiomeFactory::createColdOcean());
    // deep_warm_ocean 是 1.16.5 已删除变体，不再注册。
    registerBiome(BiomeFactory::createDeepLukewarmOcean());
    registerBiome(BiomeFactory::createDeepColdOcean());
    registerBiome(BiomeFactory::createDeepFrozenOcean());

    // === 特殊地形变体 ===
    registerBiome(BiomeFactory::createIceSpikes());

    // === 丛林变体 (168-169) ===
    registerBiome(BiomeFactory::createBambooJungle());
    // bamboo_jungle_hills 是 1.16.5 已删除变体，不再注册。

    // === 森林变体 ===
    registerBiome(BiomeFactory::createFlowerForest());
    registerBiome(BiomeFactory::createTallBirchForest());
    // tall_birch_hills 是 1.16.5 已删除变体，不再注册。
    // dark_forest_hills 是 1.16.5 已删除变体，不再注册。

    // === 巨型针叶林变体 (160-161) ===
    registerBiome(BiomeFactory::createGiantSpruceTaiga());
    // giant_spruce_taiga_hills 是 1.16.5 已删除变体，不再注册。

    // === 稀有变体生物群系 (129-167) ===
    registerBiome(BiomeFactory::createSunflowerPlains());
    // 以下为 1.16.5 已删除的 "modified*" / "*_hills" / shattered_savanna_plateau 等变体，
    // vanilla 1.21.11 已移除，不再注册：desert_lakes、taiga_mountains、swamp_hills、
    // modified_jungle、modified_jungle_edge、snowy_taiga_mountains、modified_gravelly_mountains、
    // shattered_savanna_plateau、modified_wooded_badlands_plateau、modified_badlands_plateau。
    registerBiome(BiomeFactory::createGravellyMountains());
    registerBiome(BiomeFactory::createShatteredSavanna());
    registerBiome(BiomeFactory::createErodedBadlands());

    // === 下界生物群系 (8, 170-173) ===
    registerBiome(BiomeFactory::createNetherWastes());   // ID: 8
    registerBiome(BiomeFactory::createSoulSandValley()); // ID: 170
    registerBiome(BiomeFactory::createCrimsonForest());  // ID: 171
    registerBiome(BiomeFactory::createWarpedForest());   // ID: 172
    registerBiome(BiomeFactory::createBasaltDeltas());   // ID: 173

    // === 末地生物群系 (9, 40-43) ===
    registerBiome(BiomeFactory::createTheEnd());          // ID: 9
    registerBiome(BiomeFactory::createSmallEndIslands()); // ID: 40
    registerBiome(BiomeFactory::createEndMidlands());     // ID: 41
    registerBiome(BiomeFactory::createEndHighlands());    // ID: 42
    registerBiome(BiomeFactory::createEndBarrens());      // ID: 43

    // === 新生物群系 (174-185) ===
    registerBiome(BiomeFactory::createMeadow());         // ID: 174
    registerBiome(BiomeFactory::createGrove());          // ID: 175
    registerBiome(BiomeFactory::createSnowySlopes());    // ID: 176
    registerBiome(BiomeFactory::createJaggedPeaks());    // ID: 177
    registerBiome(BiomeFactory::createFrozenPeaks());    // ID: 178
    registerBiome(BiomeFactory::createStonyPeaks());     // ID: 179
    registerBiome(BiomeFactory::createDripstoneCaves()); // ID: 180
    registerBiome(BiomeFactory::createLushCaves());      // ID: 181
    registerBiome(BiomeFactory::createDeepDark());       // ID: 182
    registerBiome(BiomeFactory::createMangroveSwamp());  // ID: 183
    registerBiome(BiomeFactory::createCherryGrove());    // ID: 184
    registerBiome(BiomeFactory::createPaleGarden());     // ID: 185
    registerBiome(BiomeFactory::createTheVoid());        // ID: 55
}

} // namespace biome
} // namespace world
} // namespace mc
