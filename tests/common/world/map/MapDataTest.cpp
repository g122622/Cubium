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

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/map/MapBanner.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "common/world/map/MapFrame.hpp"
#include "common/world/map/MapIdTracker.hpp"
#include "common/world/map/MaterialColor.hpp"
#include "network/codec/PacketDeserializer.hpp"
#include "network/codec/PacketSerializer.hpp"
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
    // 对齐 MC 1.16.5 MapData#calculateMapCenter:
    //   i = 128 * (1 << scale)
    //   j = floor((x + 64) / i)
    //   center = j * i + i / 2 - 64
    // scale=0: i=128，center = j*128（对齐到 128 的整数倍）

    // scale=0, 原点
    i32 centerX = 0;
    i32 centerZ = 0;
    MapData::calculateMapCenter(0.0, 0.0, 0, centerX, centerZ);
    EXPECT_EQ(centerX, 0);
    EXPECT_EQ(centerZ, 0);

    // scale=0, x=64 落在网格边界 (64+64=128=1*128)，center 对齐到 128
    MapData::calculateMapCenter(64.0, 64.0, 0, centerX, centerZ);
    EXPECT_EQ(centerX, 128);
    EXPECT_EQ(centerZ, 128);

    // scale=0, 中心对齐到 128 的整数倍
    MapData::calculateMapCenter(100.0, 100.0, 0, centerX, centerZ);
    EXPECT_EQ(centerX % 128, 0);
    EXPECT_EQ(centerZ % 128, 0);
    EXPECT_EQ(centerX, 128);

    // scale=1: i=256, center = j*256 + 128 - 64 = j*256 + 64
    // 中心恒满足 center ≡ 64 (mod 256)
    MapData::calculateMapCenter(100.0, 100.0, 1, centerX, centerZ);
    EXPECT_EQ(centerX % 256, 64);
    EXPECT_EQ(centerZ % 256, 64);
    EXPECT_EQ(centerX, 64);
    EXPECT_EQ(centerZ, 64);
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
    // 对齐 MC 1.16.5 MapDecoration.Type：FRAME/TARGET_*/MANSION/MONUMENT/BANNER_*/RED_X
    // 在展示框上渲染（renderedOnFrame=true），PLAYER/RED_MARKER/PLAYER_OFF_MAP/
    // PLAYER_OFF_LIMITS 不在展示框上渲染。
    EXPECT_TRUE(isRenderedOnFrame(DecorationType::FRAME));
    EXPECT_FALSE(isRenderedOnFrame(DecorationType::PLAYER_OFF_MAP));
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

TEST_F(MapDataTest, DimensionNbtWritesStringFormat)
{
    // toNbt() 应写入字符串格式的维度标识符（Java 版 1.16+ 兼容）
    MapData data(1);
    data.initialize(100, 200, 2, true, false, MapDimensionId::Nether);

    nbt::tags::compound_tag tag;
    data.toNbt(tag);

    // 验证维度字段是字符串类型，值为 "minecraft:the_nether"
    auto it = tag.value.find("dimension");
    ASSERT_NE(it, tag.value.end());
    ASSERT_EQ(it->second->id(), nbt::TagId::String);
    auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
    EXPECT_EQ(strTag.value, "minecraft:the_nether");
}

TEST_F(MapDataTest, DimensionNbtWritesStringFormatOverworld)
{
    MapData data(2);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    nbt::tags::compound_tag tag;
    data.toNbt(tag);

    auto it = tag.value.find("dimension");
    ASSERT_NE(it, tag.value.end());
    ASSERT_EQ(it->second->id(), nbt::TagId::String);
    auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
    EXPECT_EQ(strTag.value, "minecraft:overworld");
}

TEST_F(MapDataTest, DimensionNbtWritesStringFormatEnd)
{
    MapData data(3);
    data.initialize(0, 0, 0, true, false, MapDimensionId::End);

    nbt::tags::compound_tag tag;
    data.toNbt(tag);

    auto it = tag.value.find("dimension");
    ASSERT_NE(it, tag.value.end());
    ASSERT_EQ(it->second->id(), nbt::TagId::String);
    auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
    EXPECT_EQ(strTag.value, "minecraft:the_end");
}

TEST_F(MapDataTest, DimensionNbtReadsLegacyIntegerFormat)
{
    // 测试从旧版整数格式的维度ID读取（向后兼容）
    nbt::tags::compound_tag tag;
    tag.put("dimension", static_cast<i32>(-1)); // 下界
    tag.put("xCenter", 0);
    tag.put("zCenter", 0);
    tag.put("scale", static_cast<i8>(0));

    auto restored = MapData::fromNbt(tag, 1);
    EXPECT_EQ(restored.dimension(), MapDimensionId::Nether);
}

TEST_F(MapDataTest, DimensionNbtReadsShortStringFormat)
{
    // 测试从短格式（无命名空间前缀）读取
    nbt::tags::compound_tag tag;
    tag.put("dimension", std::string("overworld"));
    tag.put("xCenter", 0);
    tag.put("zCenter", 0);
    tag.put("scale", static_cast<i8>(0));

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

// ============================================================================
// MapDecoration 网络序列化测试
// ============================================================================

TEST_F(MapDecorationTest, NetworkRoundTrip_WithoutCustomName)
{
    MapDecoration original(DecorationType::PLAYER, 10, -20, 5, nullptr);

    network::PacketSerializer ser;
    original.serialize(ser);

    network::PacketDeserializer deser(ser.buffer().data(), ser.buffer().size());
    auto restored = MapDecoration::deserialize(deser);

    EXPECT_EQ(restored.type(), DecorationType::PLAYER);
    EXPECT_EQ(restored.x(), 10);
    EXPECT_EQ(restored.y(), -20);
    EXPECT_EQ(restored.rotation(), 5u);
    EXPECT_EQ(restored.customName(), nullptr);
}

TEST_F(MapDecorationTest, NetworkRoundTrip_WithCustomName)
{
    auto name = std::make_unique<text::StringTextComponent>("Test Banner");
    MapDecoration original(DecorationType::BANNER_RED, 5, 10, 3, std::move(name));

    network::PacketSerializer ser;
    original.serialize(ser);

    network::PacketDeserializer deser(ser.buffer().data(), ser.buffer().size());
    auto restored = MapDecoration::deserialize(deser);

    EXPECT_EQ(restored.type(), DecorationType::BANNER_RED);
    EXPECT_EQ(restored.x(), 5);
    EXPECT_EQ(restored.y(), 10);
    EXPECT_EQ(restored.rotation(), 3u);
    ASSERT_NE(restored.customName(), nullptr);
    EXPECT_EQ(restored.customName()->getUnformattedText(), "Test Banner");
}

TEST_F(MapDecorationTest, NetworkRoundTrip_TranslationComponent)
{
    auto name = std::make_unique<text::TranslationTextComponent>("item.banner.red.name");
    MapDecoration original(DecorationType::BANNER_RED, 0, 0, 0, std::move(name));

    network::PacketSerializer ser;
    original.serialize(ser);

    network::PacketDeserializer deser(ser.buffer().data(), ser.buffer().size());
    auto restored = MapDecoration::deserialize(deser);

    ASSERT_NE(restored.customName(), nullptr);
    auto* translation = dynamic_cast<const text::TranslationTextComponent*>(restored.customName());
    ASSERT_NE(translation, nullptr);
}

TEST_F(MapDecorationTest, NetworkRoundTrip_RotationMasking)
{
    // 旋转值在网络序列化中应被掩码到0-15范围
    MapDecoration original(DecorationType::PLAYER, 0, 0, 20, nullptr); // 20 & 0x0F = 4

    network::PacketSerializer ser;
    original.serialize(ser);

    network::PacketDeserializer deser(ser.buffer().data(), ser.buffer().size());
    auto restored = MapDecoration::deserialize(deser);

    EXPECT_EQ(restored.rotation(), 4u);
}

TEST_F(MapDecorationTest, NetworkRoundTrip_InvalidIcon)
{
    // 无效图标值应回退为PLAYER
    network::PacketSerializer ser;
    ser.writeU8(200); // 无效的装饰类型
    ser.writeI8(0);
    ser.writeI8(0);
    ser.writeU8(0);
    ser.writeBool(false);

    network::PacketDeserializer deser(ser.buffer().data(), ser.buffer().size());
    auto restored = MapDecoration::deserialize(deser);
    EXPECT_EQ(restored.type(), DecorationType::PLAYER);
}

// ============================================================================
// MapDecoration / MapBanner ITextComponent JSON解析失败回退测试
// ============================================================================

TEST_F(MapDecorationTest, NbtRoundTrip_InvalidJsonFallback)
{
    // 写入一个包含无效JSON的name字段，验证回退为纯文本
    nbt::tags::compound_tag tag;
    tag.put("type", static_cast<i8>(DecorationType::BANNER_RED));
    tag.put("x", static_cast<i8>(10));
    tag.put("y", static_cast<i8>(-20));
    tag.put("rot", static_cast<i8>(3));
    tag.put("name", std::string("not valid json {"));

    auto restored = MapDecoration::fromNbt(tag);
    EXPECT_EQ(restored.type(), DecorationType::BANNER_RED);
    ASSERT_NE(restored.customName(), nullptr);
    // JSON解析失败时回退为纯文本组件
    EXPECT_EQ(restored.customName()->getUnformattedText(), "not valid json {");
}

TEST(MapBannerTest, NbtRoundTrip_InvalidJsonFallback)
{
    // 写入一个包含无效JSON的name字段，验证回退为纯文本
    nbt::tags::compound_tag tag;
    tag.put("X", 10);
    tag.put("Y", 20);
    tag.put("Z", 30);
    tag.put("Color", static_cast<i32>(DyeColor::Red));
    tag.put("name", std::string("broken json [[["));

    auto restored = MapBanner::fromNbt(tag);
    ASSERT_NE(restored.name(), nullptr);
    // JSON解析失败时回退为纯文本组件
    EXPECT_EQ(restored.name()->getUnformattedText(), "broken json [[[");
}

// ============================================================================
// MapData 下界旋转伪随机算法验证测试
// ============================================================================

class MapDataNetherRotationTest : public ::testing::Test {
protected:
    void SetUp() override { MaterialColor::initialize(); }
};

TEST_F(MapDataNetherRotationTest, NetherRotationUsesGameTime)
{
    // 需要一个能返回游戏时间的IWorld模拟
    // 使用BaseTestWorld并覆写getGameTime
    class NetherTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] u64 getGameTime() const override { return m_gameTime; }
        [[nodiscard]] DimensionId dimension() const override { return -1; } // Nether
        void setGameTime(u64 time) { m_gameTime = time; }

    private:
        u64 m_gameTime = 0;
    };

    NetherTestWorld world;
    world.setGameTime(100);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Nether);

    // 下界旋转应基于游戏时间而非实际朝向
    data.updateDecoration(DecorationType::PLAYER, &world, "player-1", 0.0, 0.0, 0.0, nullptr);
    const auto& decos = data.decorations();
    ASSERT_TRUE(decos.find("player-1") != decos.end());

    // gameTime=100: t=10, rotation = ((10*10*34187121 + 10*121) >> 15) & 15
    // = (3418712100 + 1210) >> 15 = 3418713310 >> 15 = 104213
    // 104213 & 15 = 104213 % 16 = 5
    u8 expectedRotation = static_cast<u8>(((10 * 10 * 34187121 + 10 * 121) >> 15) & 15);
    EXPECT_EQ(decos.at("player-1").rotation(), expectedRotation);
}

TEST_F(MapDataNetherRotationTest, NetherRotationIndependentOfFacing)
{
    class NetherTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] u64 getGameTime() const override { return m_gameTime; }
        [[nodiscard]] DimensionId dimension() const override { return -1; }
        void setGameTime(u64 time) { m_gameTime = time; }

    private:
        u64 m_gameTime = 1000;
    };

    NetherTestWorld world;

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Nether);

    // 不同朝向应得到相同的旋转（下界中朝向被忽略）
    data.updateDecoration(DecorationType::PLAYER, &world, "player-north", 0.0, 0.0, 0.0, nullptr);
    u8 rotNorth = data.decorations().at("player-north").rotation();

    data.updateDecoration(DecorationType::PLAYER, &world, "player-east", 0.0, 0.0, 90.0, nullptr);
    u8 rotEast = data.decorations().at("player-east").rotation();

    data.updateDecoration(DecorationType::PLAYER, &world, "player-south", 0.0, 0.0, 180.0, nullptr);
    u8 rotSouth = data.decorations().at("player-south").rotation();

    EXPECT_EQ(rotNorth, rotEast);
    EXPECT_EQ(rotNorth, rotSouth);
}

TEST_F(MapDataNetherRotationTest, OverworldRotationDependsOnFacing)
{
    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 主世界中不同朝向应产生不同的旋转值
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-0", 0.0, 0.0, 0.0, nullptr);
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-90", 0.0, 0.0, 90.0, nullptr);
    data.updateDecoration(DecorationType::PLAYER, nullptr, "player-180", 0.0, 0.0, 180.0, nullptr);

    u8 rot0 = data.decorations().at("player-0").rotation();
    u8 rot90 = data.decorations().at("player-90").rotation();
    u8 rot180 = data.decorations().at("player-180").rotation();

    // 至少有两个旋转值不同
    EXPECT_FALSE(rot0 == rot90 && rot90 == rot180);
}

TEST_F(MapDataNetherRotationTest, NetherRotationDeterministic)
{
    class NetherTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] u64 getGameTime() const override { return 5000; }
        [[nodiscard]] DimensionId dimension() const override { return -1; }
    };

    NetherTestWorld world;

    MapData data1(1);
    data1.initialize(0, 0, 0, true, false, MapDimensionId::Nether);
    data1.updateDecoration(DecorationType::PLAYER, &world, "p1", 0.0, 0.0, 45.0, nullptr);

    MapData data2(2);
    data2.initialize(0, 0, 0, true, false, MapDimensionId::Nether);
    data2.updateDecoration(DecorationType::PLAYER, &world, "p2", 0.0, 0.0, 270.0, nullptr);

    // 相同游戏时间应产生相同旋转，无论朝向如何
    EXPECT_EQ(data1.decorations().at("p1").rotation(), data2.decorations().at("p2").rotation());
}

// ============================================================================
// MapData 旗帜交互集成测试（使用IWorld模拟）
// ============================================================================

class MapDataBannerTest : public ::testing::Test {
protected:
    void SetUp() override { MaterialColor::initialize(); }
};

TEST_F(MapDataBannerTest, RemoveStaleBanners_RemovesInvalidBanner)
{
    // removeStaleBanners在指定的区块坐标上检查旗帜是否仍然存在
    // 如果该位置没有旗帜方块实体，应移除该旗帜标记
    // 由于BaseTestWorld::getBlockEntity返回nullptr，所有旗帜都会被视为失效

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        // getBlockEntity默认返回nullptr，意味着所有旗帜都已失效
        using BaseTestWorld::getBlockEntity;
    };

    BannerTestWorld world;

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 使用addBanner直接添加旗帜标记（不经过tryAddBanner的世界交互检查）
    BlockPos pos(10, 64, 20);
    MapBanner banner(pos, DyeColor::Red, nullptr);
    data.addBanner(banner);

    std::string bannerId = banner.getMapDecorationId();
    EXPECT_EQ(data.banners().size(), 1u);
    EXPECT_TRUE(data.decorations().find(bannerId) != data.decorations().end());

    // 调用removeStaleBanners - 由于getBlockEntity返回nullptr，旗帜应被移除
    data.removeStaleBanners(world, 10, 20);

    EXPECT_EQ(data.banners().size(), 0u);
    EXPECT_TRUE(data.decorations().find(bannerId) == data.decorations().end());
    EXPECT_TRUE(data.isDirty());
}

TEST_F(MapDataBannerTest, RemoveStaleBanners_NoMatchKeepsBanners)
{
    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        using BaseTestWorld::getBlockEntity;
    };

    BannerTestWorld world;

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 添加一个旗帜在位置(10, 64, 20)
    BlockPos pos(10, 64, 20);
    MapBanner banner(pos, DyeColor::Red, nullptr);
    data.addBanner(banner);

    EXPECT_EQ(data.banners().size(), 1u);

    // 在不同的区块坐标调用 - 不匹配任何旗帜位置
    data.removeStaleBanners(world, 999, 999);

    // 旗帜应保留（因为坐标不匹配，不会被检查）
    EXPECT_EQ(data.banners().size(), 1u);
}

TEST_F(MapDataBannerTest, RemoveStaleBanners_KeepsValidBanner)
{
    // 测试当旗帜方块实体存在且颜色匹配时，旗帜应保留
    // 需要一个能返回BannerEntity的世界模拟

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] mc::BlockEntity* getBlockEntity(const BlockPos& pos) override
        {
            // 只在特定位置返回旗帜实体
            if (pos.x == m_bannerPos.x && pos.y == m_bannerPos.y && pos.z == m_bannerPos.z) {
                return &m_bannerEntity;
            }
            return nullptr;
        }

        void setBannerPos(const BlockPos& pos) { m_bannerPos = pos; }

        blockentity::BannerEntity& bannerEntity() { return m_bannerEntity; }

    private:
        BlockPos m_bannerPos;
        blockentity::BannerEntity m_bannerEntity{BlockPos(0, 0, 0)};
    };

    BannerTestWorld world;
    BlockPos bannerPos(10, 64, 20);
    world.setBannerPos(bannerPos);
    world.bannerEntity().setBaseColor(DyeColor::Red);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 使用addBanner添加红色旗帜
    MapBanner banner(bannerPos, DyeColor::Red, nullptr);
    data.addBanner(banner);

    EXPECT_EQ(data.banners().size(), 1u);

    // removeStaleBanners应找到BannerEntity且颜色匹配，保留旗帜
    data.removeStaleBanners(world, 10, 20);

    EXPECT_EQ(data.banners().size(), 1u);
}

TEST_F(MapDataBannerTest, RemoveStaleBanners_RemovesColorChangedBanner)
{
    // 当旗帜颜色改变时，应移除旧的旗帜标记

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] mc::BlockEntity* getBlockEntity(const BlockPos& pos) override
        {
            if (pos.x == m_bannerPos.x && pos.y == m_bannerPos.y && pos.z == m_bannerPos.z) {
                return &m_bannerEntity;
            }
            return nullptr;
        }

        void setBannerPos(const BlockPos& pos) { m_bannerPos = pos; }

        blockentity::BannerEntity& bannerEntity() { return m_bannerEntity; }

    private:
        BlockPos m_bannerPos;
        blockentity::BannerEntity m_bannerEntity{BlockPos(0, 0, 0)};
    };

    BannerTestWorld world;
    BlockPos bannerPos(10, 64, 20);
    world.setBannerPos(bannerPos);
    // 世界中的旗帜现在是蓝色的
    world.bannerEntity().setBaseColor(DyeColor::Blue);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 地图数据中记录的是红色旗帜
    MapBanner banner(bannerPos, DyeColor::Red, nullptr);
    data.addBanner(banner);

    EXPECT_EQ(data.banners().size(), 1u);

    // removeStaleBanners应检测到颜色不匹配，移除旗帜
    data.removeStaleBanners(world, 10, 20);

    EXPECT_EQ(data.banners().size(), 0u);
}

TEST_F(MapDataBannerTest, TryAddBanner_AddsBanner)
{
    // 测试tryAddBanner在BannerEntity存在时成功添加旗帜

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] mc::BlockEntity* getBlockEntity(const BlockPos& pos) override
        {
            if (pos.x == m_bannerPos.x && pos.y == m_bannerPos.y && pos.z == m_bannerPos.z) {
                return &m_bannerEntity;
            }
            return nullptr;
        }

        void setBannerPos(const BlockPos& pos) { m_bannerPos = pos; }

        blockentity::BannerEntity& bannerEntity() { return m_bannerEntity; }

    private:
        BlockPos m_bannerPos;
        blockentity::BannerEntity m_bannerEntity{BlockPos(0, 0, 0)};
    };

    BannerTestWorld world;
    BlockPos bannerPos(10, 64, 20);
    world.setBannerPos(bannerPos);
    world.bannerEntity().setBaseColor(DyeColor::Red);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // tryAddBanner应成功添加旗帜
    bool result = data.tryAddBanner(world, bannerPos);
    EXPECT_TRUE(result);
    EXPECT_EQ(data.banners().size(), 1u);

    // 旗帜应使用正确的颜色
    const auto& banners = data.banners();
    auto it = banners.find("banner-10-64-20");
    ASSERT_TRUE(it != banners.end());
    EXPECT_EQ(it->second.color(), DyeColor::Red);
}

TEST_F(MapDataBannerTest, TryAddBanner_ToggleRemovesExisting)
{
    // 测试tryAddBanner的切换行为：再次添加相同旗帜应移除它

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] mc::BlockEntity* getBlockEntity(const BlockPos& pos) override
        {
            if (pos.x == m_bannerPos.x && pos.y == m_bannerPos.y && pos.z == m_bannerPos.z) {
                return &m_bannerEntity;
            }
            return nullptr;
        }

        void setBannerPos(const BlockPos& pos) { m_bannerPos = pos; }

        blockentity::BannerEntity& bannerEntity() { return m_bannerEntity; }

    private:
        BlockPos m_bannerPos;
        blockentity::BannerEntity m_bannerEntity{BlockPos(0, 0, 0)};
    };

    BannerTestWorld world;
    BlockPos bannerPos(10, 64, 20);
    world.setBannerPos(bannerPos);
    world.bannerEntity().setBaseColor(DyeColor::Red);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 第一次添加应成功
    bool result1 = data.tryAddBanner(world, bannerPos);
    EXPECT_TRUE(result1);
    EXPECT_EQ(data.banners().size(), 1u);

    // 第二次添加相同旗帜应切换（移除）
    bool result2 = data.tryAddBanner(world, bannerPos);
    EXPECT_TRUE(result2);
    EXPECT_EQ(data.banners().size(), 0u);
}

TEST_F(MapDataBannerTest, TryAddBanner_FailsWithoutBannerEntity)
{
    // 测试tryAddBanner在BannerEntity不存在时返回false

    class EmptyWorld : public mc::test::BaseTestWorld {
    public:
        using BaseTestWorld::getBlockEntity;
    };

    EmptyWorld world;

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    BlockPos pos(10, 64, 20);
    bool result = data.tryAddBanner(world, pos);
    EXPECT_FALSE(result);
    EXPECT_EQ(data.banners().size(), 0u);
}

TEST_F(MapDataBannerTest, TryAddBanner_FailsOutOfRange)
{
    // 测试tryAddBanner在旗帜超出地图范围时返回false

    class BannerTestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] mc::BlockEntity* getBlockEntity(const BlockPos&) override { return &m_bannerEntity; }

        blockentity::BannerEntity& bannerEntity() { return m_bannerEntity; }

    private:
        blockentity::BannerEntity m_bannerEntity{BlockPos(0, 0, 0)};
    };

    BannerTestWorld world;
    world.bannerEntity().setBaseColor(DyeColor::Red);

    MapData data(1);
    data.initialize(0, 0, 0, true, false, MapDimensionId::Overworld);

    // 旗帜在(5000, 64, 5000)，超出地图范围（中心0,0，scale=0时半径64）
    BlockPos farPos(5000, 64, 5000);
    bool result = data.tryAddBanner(world, farPos);
    EXPECT_FALSE(result);
    EXPECT_EQ(data.banners().size(), 0u);
}

// ============================================================================
// MapDimensionId 工具函数测试
// ============================================================================

TEST(MapDimensionIdTest, DimensionIdToString_Overworld)
{
    EXPECT_EQ(dimensionIdToString(MapDimensionId::Overworld), "minecraft:overworld");
    EXPECT_EQ(dimensionIdToString(static_cast<DimensionId>(0)), "minecraft:overworld");
}

TEST(MapDimensionIdTest, DimensionIdToString_Nether)
{
    EXPECT_EQ(dimensionIdToString(MapDimensionId::Nether), "minecraft:the_nether");
    EXPECT_EQ(dimensionIdToString(static_cast<DimensionId>(-1)), "minecraft:the_nether");
}

TEST(MapDimensionIdTest, DimensionIdToString_End)
{
    EXPECT_EQ(dimensionIdToString(MapDimensionId::End), "minecraft:the_end");
    EXPECT_EQ(dimensionIdToString(static_cast<DimensionId>(1)), "minecraft:the_end");
}

TEST(MapDimensionIdTest, DimensionIdFromString_NamespacedFormat)
{
    EXPECT_EQ(dimensionIdFromString("minecraft:overworld"), MapDimensionId::Overworld);
    EXPECT_EQ(dimensionIdFromString("minecraft:the_nether"), MapDimensionId::Nether);
    EXPECT_EQ(dimensionIdFromString("minecraft:the_end"), MapDimensionId::End);
}

TEST(MapDimensionIdTest, DimensionIdFromString_ShortFormat)
{
    EXPECT_EQ(dimensionIdFromString("overworld"), MapDimensionId::Overworld);
    EXPECT_EQ(dimensionIdFromString("the_nether"), MapDimensionId::Nether);
    EXPECT_EQ(dimensionIdFromString("the_end"), MapDimensionId::End);
}

TEST(MapDimensionIdTest, DimensionIdFromString_NumericFormat)
{
    EXPECT_EQ(dimensionIdFromString("0"), MapDimensionId::Overworld);
    EXPECT_EQ(dimensionIdFromString("-1"), MapDimensionId::Nether);
    EXPECT_EQ(dimensionIdFromString("1"), MapDimensionId::End);
}

TEST(MapDimensionIdTest, DimensionIdFromString_UnknownDefaultsToOverworld)
{
    EXPECT_EQ(dimensionIdFromString("minecraft:custom_dim"), MapDimensionId::Overworld);
    EXPECT_EQ(dimensionIdFromString("custom_dimension"), MapDimensionId::Overworld);
    EXPECT_EQ(dimensionIdFromString(""), MapDimensionId::Overworld);
}

TEST(MapDimensionIdTest, DimensionNameToId)
{
    EXPECT_EQ(dimensionNameToId("minecraft:overworld"), static_cast<DimensionId>(0));
    EXPECT_EQ(dimensionNameToId("minecraft:the_nether"), static_cast<DimensionId>(-1));
    EXPECT_EQ(dimensionNameToId("minecraft:the_end"), static_cast<DimensionId>(1));
    EXPECT_EQ(dimensionNameToId("overworld"), static_cast<DimensionId>(0));
    EXPECT_EQ(dimensionNameToId("the_nether"), static_cast<DimensionId>(-1));
    EXPECT_EQ(dimensionNameToId("the_end"), static_cast<DimensionId>(1));
}

TEST(MapDimensionIdTest, RoundTrip_AllDimensions)
{
    // 验证 toString -> fromString 往返一致性
    for (auto dim : {MapDimensionId::Overworld, MapDimensionId::Nether, MapDimensionId::End}) {
        std::string_view name = dimensionIdToString(dim);
        MapDimensionId restored = dimensionIdFromString(name);
        EXPECT_EQ(restored, dim) << "Round trip failed for dimension";
    }
}
