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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "common/world/map/MapIdTracker.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <memory>

using namespace mc;
using namespace mc::world::map;

/**
 * @brief MapData 测试
 */
class MapDataTest : public ::testing::Test {
protected:
    void SetUp() override { MaterialColor::initialize(); }
};

TEST_F(MapDataTest, DefaultConstruction)
{
    MapData data;
    EXPECT_EQ(data.mapId(), 0);
    EXPECT_EQ(data.xCenter(), 0);
    EXPECT_EQ(data.zCenter(), 0);
    EXPECT_EQ(data.scale(), 0);
    EXPECT_FALSE(data.locked());
    EXPECT_TRUE(data.trackingPosition());
    EXPECT_FALSE(data.unlimitedTracking());
}

TEST_F(MapDataTest, InitializeWithCenter)
{
    MapData data(42);
    data.initialize(100, 200, 2, true, false, MapDimensionId::Overworld);

    EXPECT_EQ(data.mapId(), 42);
    EXPECT_EQ(data.xCenter(), 100);
    EXPECT_EQ(data.zCenter(), 200);
    EXPECT_EQ(data.scale(), 2);
    EXPECT_TRUE(data.trackingPosition());
    EXPECT_FALSE(data.unlimitedTracking());
}

TEST_F(MapDataTest, CalculateMapCenter)
{
    // scale=0: 128像素，每像素1方块，中心对齐到128
    i32 centerX = 0;
    i32 centerZ = 0;
    MapData::calculateMapCenter(0.0, 0.0, 0, centerX, centerZ);
    EXPECT_EQ(centerX, 0);
    EXPECT_EQ(centerZ, 0);

    // scale=0, 非零坐标
    MapData::calculateMapCenter(64.0, 64.0, 0, centerX, centerZ);
    EXPECT_EQ(centerX, 64);
    EXPECT_EQ(centerZ, 64);

    // scale=1: 每像素2方块，对齐到256
    MapData::calculateMapCenter(100.0, 100.0, 1, centerX, centerZ);
    // 128 * 2 = 256, 对齐到256的整数倍
    EXPECT_EQ(centerX % 256, 0);
    EXPECT_EQ(centerZ % 256, 0);
}

TEST_F(MapDataTest, SetAndGetColor)
{
    MapData data;
    data.setColor(5, 10, 42);
    EXPECT_EQ(data.getColor(5, 10), 42);

    // 边界值
    data.setColor(0, 0, 255);
    EXPECT_EQ(data.getColor(0, 0), 255);

    data.setColor(127, 127, 128);
    EXPECT_EQ(data.getColor(127, 127), 128);
}

TEST_F(MapDataTest, SetColorAndDirty)
{
    MapData data;
    EXPECT_FALSE(data.isDirty());

    data.setColor(10, 20, 100);
    EXPECT_TRUE(data.isDirty());

    data.clearDirty();
    EXPECT_FALSE(data.isDirty());
}

TEST_F(MapDataTest, UpdateDecoration)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 添加玩家装饰
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 10.0, 20.0, 90.0, nullptr);

    const auto& decorations = data.decorations();
    EXPECT_EQ(decorations.size(), 1u);
    EXPECT_TRUE(decorations.find("player-1") != decorations.end());

    const auto& deco = decorations.at("player-1");
    EXPECT_EQ(deco.type(), DecorationType::PLAYER);
    EXPECT_EQ(deco.rotation(), 4u); // 90度 / 22.5 = 4
}

TEST_F(MapDataTest, LockFrom)
{
    MapData source(1);
    source.initialize(100, 200, 2, true, false, MapDimensionId::Overworld);
    source.setColor(50, 50, 42);
    source.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 10.0, 20.0, 0.0, nullptr);

    MapData locked(2);
    locked.lockFrom(source);

    EXPECT_TRUE(locked.locked());
    EXPECT_EQ(locked.xCenter(), source.xCenter());
    EXPECT_EQ(locked.zCenter(), source.zCenter());
    EXPECT_EQ(locked.scale(), source.scale());
    EXPECT_EQ(locked.getColor(50, 50), 42);
}

TEST_F(MapDataTest, CopyFrom)
{
    MapData source(1);
    source.initialize(100, 200, 2, true, false, MapDimensionId::Overworld);
    source.updateDecoration(DecorationType::RED_MARKER, nullptr, "marker-1", 30.0, 40.0, 0.0, nullptr);

    MapData target(2);
    target.copyFrom(source);

    // 装饰物应该被复制
    const auto& decorations = target.decorations();
    EXPECT_TRUE(decorations.find("marker-1") != decorations.end());
}

TEST_F(MapDataTest, GetMapName)
{
    EXPECT_EQ(MapData::getMapName(0), "map_0");
    EXPECT_EQ(MapData::getMapName(42), "map_42");
    EXPECT_EQ(MapData::getMapName(9999), "map_9999");
}

/**
 * @brief MapDecoration 测试
 */
class MapDecorationTest : public ::testing::Test {
protected:
    void SetUp() override { MaterialColor::initialize(); }
};

TEST_F(MapDecorationTest, DecorationTypeValues)
{
    EXPECT_EQ(static_cast<u8>(DecorationType::PLAYER), 0);
    EXPECT_EQ(static_cast<u8>(DecorationType::FRAME), 1);
    EXPECT_EQ(static_cast<u8>(DecorationType::RED_MARKER), 2);
    EXPECT_EQ(static_cast<u8>(DecorationType::MANSION), 8);
    EXPECT_EQ(static_cast<u8>(DecorationType::MONUMENT), 9);
    EXPECT_EQ(static_cast<u8>(DecorationType::RED_X), 26);
}

TEST_F(MapDecorationTest, RenderedOnFrame)
{
    // FRAME和PLAYER_OFF_MAP只在展示框中渲染
    EXPECT_TRUE(isRenderedOnFrame(DecorationType::FRAME));
    EXPECT_TRUE(isRenderedOnFrame(DecorationType::PLAYER_OFF_MAP));
    EXPECT_FALSE(isRenderedOnFrame(DecorationType::PLAYER));
    EXPECT_FALSE(isRenderedOnFrame(DecorationType::RED_MARKER));
}

TEST_F(MapDecorationTest, HasMapColor)
{
    // MANSION和MONUMENT有地图颜色
    EXPECT_TRUE(hasMapColor(DecorationType::MANSION));
    EXPECT_TRUE(hasMapColor(DecorationType::MONUMENT));
    // PLAYER没有地图颜色
    EXPECT_FALSE(hasMapColor(DecorationType::PLAYER));
}

/**
 * @brief MapIdTracker 测试
 */
TEST(MapIdTrackerTest, AllocateIds)
{
    MapIdTracker tracker;
    EXPECT_EQ(tracker.getNextId(), 0);
    EXPECT_EQ(tracker.getNextId(), 1);
    EXPECT_EQ(tracker.getNextId(), 2);
}

TEST(MapIdTrackerTest, NbtRoundTrip)
{
    MapIdTracker tracker;
    tracker.getNextId(); // 0
    tracker.getNextId(); // 1
    tracker.getNextId(); // 2

    nbt::tags::compound_tag tag;
    tracker.writeToNbt(tag);

    MapIdTracker restored;
    restored.readFromNbt(tag);
    EXPECT_EQ(restored.getNextId(), 3);
}
