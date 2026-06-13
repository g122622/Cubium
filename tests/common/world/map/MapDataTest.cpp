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
#include "common/util/color/DyeColor.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/map/MapBanner.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "common/world/map/MapFrame.hpp"
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

// ============================================================================
// MapDecoration ITextComponent 序列化测试
// ============================================================================

TEST_F(MapDecorationTest, NbtRoundTrip_WithCustomName)
{
    auto name = std::make_unique<text::StringTextComponent>("Test Banner");
    MapDecoration original(DecorationType::BANNER_RED, 10, -20, 5, std::move(name));

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto restored = MapDecoration::fromNbt(tag);
    EXPECT_EQ(restored.type(), DecorationType::BANNER_RED);
    EXPECT_EQ(restored.x(), 10);
    EXPECT_EQ(restored.y(), -20);
    EXPECT_EQ(restored.rotation(), 5);
    ASSERT_NE(restored.customName(), nullptr);
    EXPECT_EQ(restored.customName()->getUnformattedText(), "Test Banner");
}

TEST_F(MapDecorationTest, NbtRoundTrip_WithoutCustomName)
{
    MapDecoration original(DecorationType::PLAYER, 5, 10, 3, nullptr);

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto restored = MapDecoration::fromNbt(tag);
    EXPECT_EQ(restored.type(), DecorationType::PLAYER);
    EXPECT_EQ(restored.x(), 5);
    EXPECT_EQ(restored.y(), 10);
    EXPECT_EQ(restored.rotation(), 3);
    EXPECT_EQ(restored.customName(), nullptr);
}

TEST_F(MapDecorationTest, NbtRoundTrip_TranslationComponent)
{
    auto name = std::make_unique<text::TranslationTextComponent>("item.banner.red.name");
    MapDecoration original(DecorationType::BANNER_RED, 0, 0, 0, std::move(name));

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto restored = MapDecoration::fromNbt(tag);
    ASSERT_NE(restored.customName(), nullptr);
    // 翻译组件应保留翻译键
    auto* translation = dynamic_cast<const text::TranslationTextComponent*>(restored.customName());
    ASSERT_NE(translation, nullptr);
}

TEST_F(MapDecorationTest, DeepCopy_WithCustomName)
{
    auto name = std::make_unique<text::StringTextComponent>("Original");
    MapDecoration original(DecorationType::MANSION, 1, 2, 3, std::move(name));

    MapDecoration copy = original.deepCopy();
    EXPECT_EQ(copy.type(), DecorationType::MANSION);
    ASSERT_NE(copy.customName(), nullptr);
    EXPECT_EQ(copy.customName()->getUnformattedText(), "Original");

    // 深拷贝应独立于原对象
    EXPECT_NE(original.customName(), copy.customName());
}

TEST_F(MapDecorationTest, RotationMasking)
{
    // 旋转值应被掩码到0-15范围
    MapDecoration deco(DecorationType::PLAYER, 0, 0, 20, nullptr); // 20 & 0x0F = 4
    EXPECT_EQ(deco.rotation(), 4u);

    MapDecoration deco2(DecorationType::PLAYER, 0, 0, 255, nullptr); // 255 & 0x0F = 15
    EXPECT_EQ(deco2.rotation(), 15u);
}

// ============================================================================
// MapBanner 测试
// ============================================================================

TEST(MapBannerTest, NbtRoundTrip_WithColor)
{
    BlockPos pos(100, 64, -200);
    MapBanner original(pos, DyeColor::Red, nullptr);

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto restored = MapBanner::fromNbt(tag);
    EXPECT_EQ(restored.pos().x, 100);
    EXPECT_EQ(restored.pos().y, 64);
    EXPECT_EQ(restored.pos().z, -200);
    EXPECT_EQ(restored.color(), DyeColor::Red);
    EXPECT_EQ(restored.name(), nullptr);
}

TEST(MapBannerTest, NbtRoundTrip_WithCustomName)
{
    BlockPos pos(50, 70, 80);
    auto name = std::make_unique<text::StringTextComponent>("My Banner");
    MapBanner original(pos, DyeColor::Blue, std::move(name));

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto restored = MapBanner::fromNbt(tag);
    EXPECT_EQ(restored.pos().x, 50);
    EXPECT_EQ(restored.color(), DyeColor::Blue);
    ASSERT_NE(restored.name(), nullptr);
    EXPECT_EQ(restored.name()->getUnformattedText(), "My Banner");
}

TEST(MapBannerTest, GetDecorationType)
{
    BlockPos pos(0, 0, 0);
    MapBanner white(pos, DyeColor::White, nullptr);
    MapBanner red(pos, DyeColor::Red, nullptr);
    MapBanner black(pos, DyeColor::Black, nullptr);

    EXPECT_EQ(white.getDecorationType(), DecorationType::BANNER_WHITE);
    EXPECT_EQ(red.getDecorationType(), DecorationType::BANNER_RED);
    EXPECT_EQ(black.getDecorationType(), DecorationType::BANNER_BLACK);
}

TEST(MapBannerTest, CopySemantics)
{
    BlockPos pos1(10, 20, 30);
    auto name = std::make_unique<text::StringTextComponent>("My Banner");
    MapBanner original(pos1, DyeColor::Red, std::move(name));

    // 拷贝构造应深拷贝名称
    MapBanner copy(original);
    EXPECT_EQ(copy.pos().x, 10);
    EXPECT_EQ(copy.color(), DyeColor::Red);
    ASSERT_NE(copy.name(), nullptr);
    EXPECT_EQ(copy.name()->getUnformattedText(), "My Banner");

    // 深拷贝应独立于原对象
    EXPECT_NE(original.name(), copy.name());
}

TEST(MapBannerTest, Equals)
{
    BlockPos pos1(10, 20, 30);
    BlockPos pos2(10, 20, 30);
    BlockPos pos3(10, 20, 40); // 不同的z

    MapBanner a(pos1, DyeColor::Red, nullptr);
    MapBanner b(pos2, DyeColor::Red, std::make_unique<text::StringTextComponent>("Name"));
    MapBanner c(pos3, DyeColor::Red, nullptr);
    MapBanner d(pos1, DyeColor::Blue, nullptr);

    // equals 只比较位置和颜色，不比较名称
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

TEST(MapBannerTest, GetMapDecorationId)
{
    BlockPos pos(5, 10, 15);
    MapBanner banner(pos, DyeColor::White, nullptr);
    EXPECT_EQ(banner.getMapDecorationId(), "banner-5-10-15");
}

// ============================================================================
// MapData 维度序列化测试
// ============================================================================

TEST_F(MapDataTest, DimensionNbtRoundTrip)
{
    MapData data(1);
    data.initialize(100, 200, 2, true, false, MapDimensionId::Nether);

    nbt::tags::compound_tag tag;
    data.toNbt(tag);

    auto restored = MapData::fromNbt(tag, 1);
    EXPECT_EQ(restored.dimension(), MapDimensionId::Nether);
}

TEST_F(MapDataTest, DimensionNbtStringFormat)
{
    // 测试从字符串格式的维度ID读取
    nbt::tags::compound_tag tag;
    tag.put("dimension", std::string("minecraft:the_end"));
    tag.put("xCenter", 0);
    tag.put("zCenter", 0);
    tag.put("scale", static_cast<i8>(0));

    auto restored = MapData::fromNbt(tag, 1);
    EXPECT_EQ(restored.dimension(), MapDimensionId::End);
}

TEST_F(MapDataTest, DimensionNbtStringFormatNether)
{
    nbt::tags::compound_tag tag;
    tag.put("dimension", std::string("minecraft:the_nether"));
    tag.put("xCenter", 0);
    tag.put("zCenter", 0);
    tag.put("scale", static_cast<i8>(0));

    auto restored = MapData::fromNbt(tag, 1);
    EXPECT_EQ(restored.dimension(), MapDimensionId::Nether);
}

TEST_F(MapDataTest, DimensionNbtDefaultOverworld)
{
    // 无维度字段时默认为主世界
    nbt::tags::compound_tag tag;
    tag.put("xCenter", 0);
    tag.put("zCenter", 0);

    auto restored = MapData::fromNbt(tag, 1);
    EXPECT_EQ(restored.dimension(), MapDimensionId::Overworld);
}

// ============================================================================
// MapData 下界旋转测试
// ============================================================================

TEST_F(MapDataTest, CalculateRotation_Overworld)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 主世界中旋转基于实际朝向角度
    // 0度 -> (0+8)*16/360 = 0
    // 90度 -> (90+8)*16/360 ≈ 4
    // 180度 -> (180-8)*16/360 ≈ 7
    // 270度 -> (270+8)*16/360 ≈ 12
    // 注意：calculateRotation 是私有方法，通过 updateDecoration 间接测试
    data.updateDecoration(DecorationType::PLAYER, nullptr, "test", 0.0, 0.0, 0.0, nullptr);
    const auto& decos = data.decorations();
    ASSERT_TRUE(decos.find("test") != decos.end());
    EXPECT_EQ(decos.at("test").rotation(), 0u); // 0度
}

TEST_F(MapDataTest, CalculateRotation_90Degrees)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    data.updateDecoration(DecorationType::PLAYER, nullptr, "test", 0.0, 0.0, 90.0, nullptr);
    const auto& decos = data.decorations();
    ASSERT_TRUE(decos.find("test") != decos.end());
    EXPECT_EQ(decos.at("test").rotation(), 4u); // (90+8)*16/360 ≈ 4.3 -> 4
}

// ============================================================================
// MapData 旗帜和装饰物管理测试
// ============================================================================

TEST_F(MapDataTest, DecorationCountLimit)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 添加大量非FRAME装饰物不应崩溃
    for (i32 i = 0; i < 300; ++i) {
        data.updateDecoration(DecorationType::PLAYER, nullptr, "player-" + std::to_string(i), 0.0, 0.0, 0.0, nullptr);
    }
    // 所有PLAYER装饰物都应存在
    EXPECT_EQ(data.decorations().size(), 300u);
}

TEST_F(MapDataTest, RemoveDecoration)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 0.0, 0.0, 0.0, nullptr);
    EXPECT_EQ(data.decorations().size(), 1u);

    data.removeDecoration("player-1");
    EXPECT_EQ(data.decorations().size(), 0u);
}

TEST_F(MapDataTest, BannerNbtRoundTrip)
{
    MapData data(1);
    data.initialize(100, 200, 0, true, false, MapDimensionId::Overworld);

    // 通过 addFrame 和 banner 手动构建带名称的 MapData
    BlockPos pos(110, 64, 210);
    MapBanner banner(pos, DyeColor::Red, std::make_unique<text::StringTextComponent>("Red Banner"));

    // 写入和读回 banner 数据
    nbt::tags::compound_tag bannerTag;
    banner.toNbt(bannerTag);

    auto restoredBanner = MapBanner::fromNbt(bannerTag);
    EXPECT_EQ(restoredBanner.color(), DyeColor::Red);
    ASSERT_NE(restoredBanner.name(), nullptr);
    EXPECT_EQ(restoredBanner.name()->getUnformattedText(), "Red Banner");
    EXPECT_EQ(restoredBanner.pos().x, 110);
    EXPECT_EQ(restoredBanner.pos().y, 64);
    EXPECT_EQ(restoredBanner.pos().z, 210);
}

TEST_F(MapDataTest, UpdateDecoration_OffMap)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 超出地图范围但在追踪范围内的玩家
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 200.0, 0.0, 0.0, nullptr);
    const auto& decos = data.decorations();
    ASSERT_TRUE(decos.find("player-1") != decos.end());
    EXPECT_EQ(decos.at("player-1").type(), DecorationType::PLAYER_OFF_MAP);
}

TEST_F(MapDataTest, UpdateDecoration_OffLimits)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, true, MapDimensionId::Overworld); // unlimitedTracking=true

    // 远离地图范围的玩家，启用无限追踪
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 5000.0, 0.0, 0.0, nullptr);
    const auto& decos = data.decorations();
    ASSERT_TRUE(decos.find("player-1") != decos.end());
    EXPECT_EQ(decos.at("player-1").type(), DecorationType::PLAYER_OFF_LIMITS);
}

TEST_F(MapDataTest, UpdateDecoration_OffLimitsNoTracking)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld); // unlimitedTracking=false

    // 远离地图范围的玩家，未启用无限追踪 -> 装饰物应被移除
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-1", 5000.0, 0.0, 0.0, nullptr);
    EXPECT_TRUE(data.decorations().find("player-1") == data.decorations().end());
}

TEST_F(MapDataTest, UpdateDecoration_NonPlayerOffMap)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 非玩家类型的装饰物超出范围应直接移除
    data.updateDecoration(DecorationType::BANNER_RED, nullptr, "banner-1", 200.0, 0.0, 0.0, nullptr);
    EXPECT_TRUE(data.decorations().find("banner-1") == data.decorations().end());
}
