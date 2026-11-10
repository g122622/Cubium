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

/**
 * @file MapBiomePreviewTest.cpp
 * @brief 探险地图的生物群系预览底图
 *
 * 覆盖 `world::map::renderBiomePreviewMap` 的三条关键语义：
 * 1. 全是水体生物群系时，内部像素被涂成水体色（ADOBE 系）；
 * 2. 全是陆地生物群系时，不写任何像素（预览保持空白，交给后续的真实地形绘制）；
 * 3. 水陆各占一半时，交界两侧都上色，且陆地一侧用棕色系。
 *
 * 这里只需要一个 `getBiome` 可控的世界桩和一个初始化过中心/缩放的 `MapData`，
 * 不需要 MapDataManager 或物品注册表——这正是把逻辑从 FilledMapItem 抽出来的收益。
 */

#include "common/world/map/MapBiomePreview.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MaterialColor.hpp"

#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 只回答"某个 X 坐标以西是水、以东是陆"的世界，y/z 不参与判定。
class BiomePreviewTestWorld final : public mc::test::BaseTestWorld {
public:
    BiomePreviewTestWorld(BiomeId waterBiome, BiomeId landBiome, i32 waterUntilX)
        : m_waterBiome(waterBiome)
        , m_landBiome(landBiome)
        , m_waterUntilX(waterUntilX)
    {}

    [[nodiscard]] BiomeId getBiome(i32 x, i32 /*y*/, i32 /*z*/) const override
    {
        return x < m_waterUntilX ? m_waterBiome : m_landBiome;
    }

private:
    BiomeId m_waterBiome;
    BiomeId m_landBiome;
    i32 m_waterUntilX;
};

/// 建一张中心在原点、scale 为 0 的地图（1 像素 = 1 格）
world::map::MapData _makeMap()
{
    world::map::MapData data;
    data.initialize(0, 0, 0, true, true, MapDimensionId::Overworld);
    return data;
}

constexpr i32 MAP_SIZE = world::map::MapData::MAP_SIZE;

/// 统计地图上非空像素数量
i32 _paintedPixels(const world::map::MapData& data)
{
    i32 count = 0;
    for (i32 row = 0; row < MAP_SIZE; ++row) {
        for (i32 column = 0; column < MAP_SIZE; ++column) {
            if (data.getColor(column, row) != 0) {
                ++count;
            }
        }
    }
    return count;
}

} // namespace

// 全水：内部应当被涂上水体色，且中心像素一定是水体色系（4 个 shade 之一）
TEST(MapBiomePreviewTest, AllWaterPaintsWaterColor)
{
    world::biome::BiomeTags::initialize();
    BiomePreviewTestWorld world(world::biome::Biomes::Ocean, world::biome::Biomes::Plains, MAP_SIZE * 2);
    auto data = _makeMap();

    world::map::renderBiomePreviewMap(world, data);

    EXPECT_GT(_paintedPixels(data), 0) << "an all-water preview must paint something";

    const u8 centerColor = data.getColor(MAP_SIZE / 2, MAP_SIZE / 2);
    EXPECT_EQ(centerColor / 4, static_cast<u8>(world::map::MaterialColorId::ADOBE))
        << "water must be painted with the ADOBE (orange) color family";
}

// 全陆：没有任何水，因此不写像素，预览保持空白
TEST(MapBiomePreviewTest, AllLandLeavesPreviewBlank)
{
    world::biome::BiomeTags::initialize();
    BiomePreviewTestWorld world(world::biome::Biomes::Ocean, world::biome::Biomes::Plains, -MAP_SIZE * 2);
    auto data = _makeMap();

    world::map::renderBiomePreviewMap(world, data);

    EXPECT_EQ(_paintedPixels(data), 0) << "a land-only preview must stay blank";
}

// 水陆各半：交界处的陆地像素用棕色系（BROWN），体现"岸边"过渡
TEST(MapBiomePreviewTest, ShorelinePaintsBrownOnLandSide)
{
    world::biome::BiomeTags::initialize();
    // 西半是海、东半是平原
    BiomePreviewTestWorld world(world::biome::Biomes::Ocean, world::biome::Biomes::Plains, 0);
    auto data = _makeMap();

    world::map::renderBiomePreviewMap(world, data);

    // 中心像素在 x=0 上，属于陆地侧（x < 0 才是水），它的西邻居是水，应当被涂成棕色系
    const u8 centerColor = data.getColor(MAP_SIZE / 2, MAP_SIZE / 2);
    ASSERT_NE(centerColor, 0) << "the land pixel next to water must be painted";
    EXPECT_EQ(centerColor / 4, static_cast<u8>(world::map::MaterialColorId::BROWN));

    // 而水体内部应当是水体色
    const u8 waterColor = data.getColor(MAP_SIZE / 2 - 10, MAP_SIZE / 2);
    ASSERT_NE(waterColor, 0);
    EXPECT_EQ(waterColor / 4, static_cast<u8>(world::map::MaterialColorId::ADOBE));
}

// scale 变化时采样步长随之变化：scale=1 时每 2 格采一次，仍然能区分水陆
TEST(MapBiomePreviewTest, HonorsMapScaleWhenSampling)
{
    world::biome::BiomeTags::initialize();
    BiomePreviewTestWorld world(world::biome::Biomes::Ocean, world::biome::Biomes::Plains, 0);
    world::map::MapData data;
    data.initialize(0, 0, 1, true, true, MapDimensionId::Overworld);

    world::map::renderBiomePreviewMap(world, data);

    // 水体一侧必须被涂色。
    const u8 westColor = data.getColor(MAP_SIZE / 2 - 10, MAP_SIZE / 2);
    ASSERT_NE(westColor, 0) << "the water side must be painted at scale 1";

    // 远离水的陆地不参与上色——只有"岸边"陆地才涂棕色，这既省像素也更贴近原版：
    // 整片内陆涂成棕色没有意义，真实地形会在后续 update() 里覆盖它。
    const u8 eastColor = data.getColor(MAP_SIZE / 2 + 10, MAP_SIZE / 2);
    EXPECT_EQ(eastColor, 0) << "inland far from water must stay blank";
}
