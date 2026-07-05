/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/world/storage/reader/java/JavaColumnReader.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "common/world/storage/reader/java/JavaChunkReader.hpp"
#include <sstream>
#include <gtest/gtest.h>

namespace mc {
namespace {

// ============================================================================
// DimensionType 属性测试
// ============================================================================

TEST(DimensionTypeOverworldTest, Properties)
{
    const auto type = DimensionType::overworld();
    EXPECT_EQ(type.id(), 0);
    EXPECT_EQ(type.minHeight(), world::MIN_BUILD_HEIGHT); // -64
    EXPECT_EQ(type.maxHeight(), world::MAX_BUILD_HEIGHT); // 320
    EXPECT_TRUE(type.hasSkyLight());
    EXPECT_FALSE(type.hasCeiling());
    EXPECT_FALSE(type.ultraWarm());
    EXPECT_TRUE(type.natural());
    EXPECT_FLOAT_EQ(type.coordinateScale(), 1.0f);
}

TEST(DimensionTypeNetherTest, Properties)
{
    const auto type = DimensionType::nether();
    EXPECT_EQ(type.id(), -1);
    EXPECT_EQ(type.minHeight(), 0);   // 下界从 Y=0 开始
    EXPECT_EQ(type.maxHeight(), 128); // 下界最高到 Y=128
    EXPECT_FALSE(type.hasSkyLight()); // 下界没有天空光照
    EXPECT_TRUE(type.hasCeiling());   // 下界有基岩顶板
    EXPECT_TRUE(type.ultraWarm());    // 下界超热（水会蒸发）
    EXPECT_FALSE(type.natural());
    EXPECT_FLOAT_EQ(type.coordinateScale(), 8.0f); // 下界坐标缩放 8x
    EXPECT_TRUE(type.hasFixedTime());
    EXPECT_EQ(type.fixedTimeValue(), 18000);
}

TEST(DimensionTypeTheEndTest, Properties)
{
    const auto type = DimensionType::theEnd();
    EXPECT_EQ(type.id(), 1);
    EXPECT_EQ(type.minHeight(), world::MIN_BUILD_HEIGHT); // -64
    EXPECT_EQ(type.maxHeight(), world::MAX_BUILD_HEIGHT); // 320
    EXPECT_FALSE(type.hasSkyLight());                     // 末地没有天空光照
    EXPECT_FALSE(type.hasCeiling());
    EXPECT_FALSE(type.ultraWarm());
    EXPECT_FALSE(type.natural()); // 末地不是自然维度
    EXPECT_FLOAT_EQ(type.coordinateScale(), 1.0f);
    EXPECT_TRUE(type.hasFixedTime());
    EXPECT_EQ(type.fixedTimeValue(), 6000);
}

TEST(DimensionTypeFromIdTest, ReturnsCorrectType)
{
    EXPECT_EQ(DimensionType::fromId(0).id(), 0);
    EXPECT_EQ(DimensionType::fromId(-1).id(), -1);
    EXPECT_EQ(DimensionType::fromId(1).id(), 1);
    // 未知维度回退到主世界
    EXPECT_EQ(DimensionType::fromId(999).id(), 0);
}

// ============================================================================
// 维度特定区块读取逻辑测试
// ============================================================================

TEST(DimensionSpecificSectionRangeTest, OverworldSectionRange)
{
    // 主世界: minHeight=-64, maxHeight=320
    const auto type = DimensionType::overworld();
    const i32 minSectionY = type.minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (type.maxHeight() - 1) / world::CHUNK_SECTION_HEIGHT;

    // 主世界 section Y 范围: -4 到 19（24个section）
    EXPECT_EQ(minSectionY, -4);
    EXPECT_EQ(maxSectionY, 19);
    EXPECT_EQ(maxSectionY - minSectionY + 1, world::CHUNK_SECTIONS);
}

TEST(DimensionSpecificSectionRangeTest, NetherSectionRange)
{
    // 下界: minHeight=0, maxHeight=128
    const auto type = DimensionType::nether();
    const i32 minSectionY = type.minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (type.maxHeight() - 1) / world::CHUNK_SECTION_HEIGHT;

    // 下界 section Y 范围: 0 到 7（8个section）
    EXPECT_EQ(minSectionY, 0);
    EXPECT_EQ(maxSectionY, 7);
    EXPECT_EQ(maxSectionY - minSectionY + 1, 8); // 128 / 16 = 8 sections
}

TEST(DimensionSpecificSectionRangeTest, TheEndSectionRange)
{
    // 末地: minHeight=-64, maxHeight=320
    const auto type = DimensionType::theEnd();
    const i32 minSectionY = type.minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (type.maxHeight() - 1) / world::CHUNK_SECTION_HEIGHT;

    // 末地 section Y 范围与主世界相同: -4 到 19
    EXPECT_EQ(minSectionY, -4);
    EXPECT_EQ(maxSectionY, 19);
}

TEST(DimensionSpecificSectionRangeTest, OverworldRejectsOutOfBoundsSections)
{
    // 主世界: section Y 范围 [-4, 19]
    const auto type = DimensionType::overworld();
    const i32 minSectionY = type.minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (type.maxHeight() - 1) / world::CHUNK_SECTION_HEIGHT;

    // section Y=-5 应该被跳过（低于主世界最低section）
    EXPECT_LT(-5, minSectionY);
    // section Y=20 应该被跳过（高于主世界最高section）
    EXPECT_GT(20, maxSectionY);
    // section Y=-4 到 19 应该被接受
    EXPECT_GE(-4, minSectionY);
    EXPECT_LE(19, maxSectionY);
}

TEST(DimensionSpecificSectionRangeTest, NetherRejectsOutOfBoundsSections)
{
    // 下界: section Y 范围 [0, 7]
    const auto type = DimensionType::nether();
    const i32 minSectionY = type.minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (type.maxHeight() - 1) / world::CHUNK_SECTION_HEIGHT;

    // 下界不应接受 section Y=-4（主世界底部section，在下界范围外）
    EXPECT_LT(-4, minSectionY);
    // 下界不应接受 section Y=8
    EXPECT_GT(8, maxSectionY);
    // 下界应接受 section Y=0 到 7
    EXPECT_GE(0, minSectionY);
    EXPECT_LE(7, maxSectionY);
}

TEST(DimensionSpecificSkyLightTest, OnlyOverworldHasSkyLight)
{
    // 对应 MC Java SerializableChunkData.read() 中的 hasSkyLight 门控
    EXPECT_TRUE(DimensionType::overworld().hasSkyLight());
    EXPECT_FALSE(DimensionType::nether().hasSkyLight());
    EXPECT_FALSE(DimensionType::theEnd().hasSkyLight());
}

TEST(DimensionSpecificHeightmapOffsetTest, HeightOffsetMatchesMinHeight)
{
    // 高度图解包偏移应与维度的 minHeight 一致
    // 对应 MC Java SerializableChunkData.parse() 中的 LevelHeightAccessor.getMinSectionY()
    const i32 overworldOffset = DimensionType::overworld().minHeight();
    const i32 netherOffset = DimensionType::nether().minHeight();
    const i32 endOffset = DimensionType::theEnd().minHeight();

    EXPECT_EQ(overworldOffset, -64); // 主世界高度偏移
    EXPECT_EQ(netherOffset, 0);      // 下界高度偏移为0
    EXPECT_EQ(endOffset, -64);       // 末地高度偏移与主世界相同
}

TEST(DimensionSpecificBiomeBaseSectionYTest, BiomeBaseSectionY)
{
    // 旧版3D生物群系的baseSectionY应使用维度感知的minHeight
    const i32 overworldBase = DimensionType::overworld().minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 netherBase = DimensionType::nether().minHeight() / world::CHUNK_SECTION_HEIGHT;
    const i32 endBase = DimensionType::theEnd().minHeight() / world::CHUNK_SECTION_HEIGHT;

    // 主世界: -64 / 16 = -4
    EXPECT_EQ(overworldBase, -4);
    // 下界: 0 / 16 = 0（不同于全局 MIN_BUILD_HEIGHT / 16 = -4）
    EXPECT_EQ(netherBase, 0);
    // 末地: -64 / 16 = -4
    EXPECT_EQ(endBase, -4);
}

// ============================================================================
// JavaColumnReader 高度图加载测试
// ============================================================================
//
// 验证 JavaColumnReader::_readHeightmaps 通过 readColumn 公开 API 正确加载
// Java 版 Anvil 存档中的高度图数据到 ChunkData。
// 关键回归点：旧实现中 applyHeightmapArray 调用 chunk.updateHeightmap(type, x, y, z, nullptr)
// 试图逐列写入，但 updateHeightmap 内部依赖 _isOpaque(state)，state 为 nullptr 时返回 false，
// 导致所有列的写入被跳过，持久化的高度值从未真正进入 m_heightmaps。
// 现实现使用 setHeightmapFromStorage 绕过 _isOpaque 判定，整列写入。

namespace {
using namespace mc::nbt::tags;
using mc::world::storage::reader::java::JavaBiomeMapper;
using mc::world::storage::reader::java::JavaBlockStateMapper;
using mc::world::storage::reader::java::JavaChunkReader;
using mc::world::storage::reader::java::JavaColumnReader;

// 构造一个最小的合法 Java 列 NBT（含 xPos/zPos/Status/Heightmaps），
// 序列化为 Java Edition NBT 字节流供 JavaColumnReader::readColumn 解析。
// 使用扁平格式（无 Level 包装），unwrapColumnRoot 在没有 Level 时直接返回 root。
std::vector<u8> buildColumnNbtWithHeightmaps(
    i32 xPos, i32 zPos, const std::string& status, const compound_tag& heightmaps)
{
    compound_tag root(true);
    root.value["xPos"] = std::make_unique<int_tag>(xPos);
    root.value["zPos"] = std::make_unique<int_tag>(zPos);
    root.value["Status"] = std::make_unique<string_tag>(status);
    // 深拷贝 heightmaps 到 root
    compound_tag heightmapsCopy;
    for (const auto& [key, value] : heightmaps.value) {
        heightmapsCopy.value[key] = value->copy();
    }
    root.value["Heightmaps"] = std::make_unique<compound_tag>(std::move(heightmapsCopy));

    std::ostringstream stream(std::ios::binary);
    stream << mc::nbt::contexts::java;
    root.write(stream);
    auto str = stream.str();
    return std::vector<u8>(str.begin(), str.end());
}

// 打包 9 位高度值到 long[]（padded 格式，Java 1.13+）
std::vector<i64> packPadded9Bit(const std::vector<u32>& values)
{
    constexpr i32 BITS_PER_ENTRY = 9;
    constexpr i32 VALUES_PER_LONG = 64 / BITS_PER_ENTRY; // 7
    const size_t packedSize = (values.size() + VALUES_PER_LONG - 1) / VALUES_PER_LONG;
    std::vector<i64> packed(packedSize, 0);
    for (size_t i = 0; i < values.size(); ++i) {
        const size_t longIndex = i / VALUES_PER_LONG;
        const i32 bitOffset = static_cast<i32>(i % VALUES_PER_LONG) * BITS_PER_ENTRY;
        packed[longIndex] |= static_cast<i64>(values[i]) << bitOffset;
    }
    return packed;
}
} // namespace

TEST(JavaColumnReaderHeightmapTest, LoadsWorldSurfaceHeightmapFromLongArray)
{
    // 构造 WORLD_SURFACE 高度图：每列 Y+1 = x+z+1（即 Y = x+z）
    std::vector<u32> heights(256);
    for (i32 z = 0; z < 16; ++z) {
        for (i32 x = 0; x < 16; ++x) {
            heights[static_cast<size_t>(z * 16 + x)] = static_cast<u32>(x + z + 1);
        }
    }
    auto packed = packPadded9Bit(heights);

    compound_tag heightmaps;
    auto longArrayTag = std::make_unique<longarray_tag>();
    longArrayTag->value = packed;
    heightmaps.value["WORLD_SURFACE"] = std::move(longArrayTag);

    const auto nbtBytes = buildColumnNbtWithHeightmaps(0, 0, "full", heightmaps);

    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader chunkReader(blockMapper, biomeMapper);
    JavaColumnReader columnReader(chunkReader);

    auto result = columnReader.readColumn(nbtBytes, 0, 0, 0);
    ASSERT_TRUE(result.success());
    ASSERT_TRUE(result.value().has_value());
    auto chunk = std::move(result.value().value());

    // 主世界 minHeight=-64，Java 存储值已是 Y+1-minY，加 heightOffset(-64) 后 = Y+1（绝对）
    // x=5, z=5: Java 存储值 = 11, 加偏移 -64 = -53（内部 Y+1 语义），最高方块 Y = -54
    // 但 x+z+1 = 11 是 Java 存储值（相对 minY），Y = (11-1) + minY = 10 + (-64) = -54
    // ChunkData::getTopBlockY 返回 Y（= internal - 1）
    const BlockCoord expected = 5 + 5; // x+z = 10，Y = 10 + minHeight = -54
    EXPECT_EQ(
        chunk.getTopBlockY(mc::HeightmapType::WorldSurface, 5, 5), expected + DimensionType::overworld().minHeight());
}

TEST(JavaColumnReaderHeightmapTest, LoadsAllHeightmapTypes)
{
    // 为每种类型构造独立的高度图
    std::vector<u32> wsHeights(256);
    std::vector<u32> ofHeights(256);
    std::vector<u32> mbHeights(256);
    std::vector<u32> mbnlHeights(256);
    std::vector<u32> wswgHeights(256);
    std::vector<u32> ofwgHeights(256);
    std::vector<u32> lbHeights(256);

    for (i32 z = 0; z < 16; ++z) {
        for (i32 x = 0; x < 16; ++x) {
            const size_t idx = static_cast<size_t>(z * 16 + x);
            // 使用互不相同的值，便于验证每种类型独立加载
            wsHeights[idx] = static_cast<u32>(x + z + 10);
            ofHeights[idx] = static_cast<u32>(x + z + 20);
            mbHeights[idx] = static_cast<u32>(x + z + 30);
            mbnlHeights[idx] = static_cast<u32>(x + z + 40);
            wswgHeights[idx] = static_cast<u32>(x + z + 50);
            ofwgHeights[idx] = static_cast<u32>(x + z + 60);
            lbHeights[idx] = static_cast<u32>(x + z + 70);
        }
    }

    compound_tag heightmaps;
    auto addHeightmap = [&heightmaps](const std::string& name, const std::vector<u32>& values) {
        auto tag = std::make_unique<longarray_tag>();
        tag->value = packPadded9Bit(values);
        heightmaps.value[name] = std::move(tag);
    };
    addHeightmap("WORLD_SURFACE", wsHeights);
    addHeightmap("OCEAN_FLOOR", ofHeights);
    addHeightmap("MOTION_BLOCKING", mbHeights);
    addHeightmap("MOTION_BLOCKING_NO_LEAVES", mbnlHeights);
    addHeightmap("WORLD_SURFACE_WG", wswgHeights);
    addHeightmap("OCEAN_FLOOR_WG", ofwgHeights);
    addHeightmap("LIGHT_BLOCKING", lbHeights);

    const auto nbtBytes = buildColumnNbtWithHeightmaps(0, 0, "full", heightmaps);

    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader chunkReader(blockMapper, biomeMapper);
    JavaColumnReader columnReader(chunkReader);

    auto result = columnReader.readColumn(nbtBytes, 0, 0, 0);
    ASSERT_TRUE(result.success());
    ASSERT_TRUE(result.value().has_value());
    auto chunk = std::move(result.value().value());

    // 主世界 minHeight = -64
    const i32 heightOffset = DimensionType::overworld().minHeight();
    // 验证 (5, 5) 列每种类型的高度
    // Java 存储值 = Y+1-minY，加 heightOffset = Y+1（绝对内部存储），getTopBlockY 返回 Y = internal-1
    const auto verifyType = [&](mc::HeightmapType type, u32 javaStoredValue) {
        const BlockCoord expectedY = static_cast<BlockCoord>(javaStoredValue) + heightOffset - 1;
        EXPECT_EQ(chunk.getTopBlockY(type, 5, 5), expectedY)
            << "Heightmap type " << static_cast<int>(type) << " mismatch";
    };
    verifyType(mc::HeightmapType::WorldSurface, wsHeights[85]);             // 5+5+10=20
    verifyType(mc::HeightmapType::OceanFloor, ofHeights[85]);               // 30
    verifyType(mc::HeightmapType::MotionBlocking, mbHeights[85]);           // 40
    verifyType(mc::HeightmapType::MotionBlockingNoLeaves, mbnlHeights[85]); // 50
    verifyType(mc::HeightmapType::WorldSurfaceWG, wswgHeights[85]);         // 60
    verifyType(mc::HeightmapType::OceanFloorWG, ofwgHeights[85]);           // 70
    verifyType(mc::HeightmapType::LightBlocking, lbHeights[85]);            // 80
}

TEST(JavaColumnReaderHeightmapTest, LoadsLegacyIntArrayHeightMap)
{
    // 旧版 HeightMap int[256] 数组：语义为 Y+1（与 Heightmap 内部存储一致，但相对世界原点而非 minY）
    // 对应旧版 Java 1.8-1.13 的 HeightMap 字段
    compound_tag root(true);
    root.value["xPos"] = std::make_unique<int_tag>(0);
    root.value["zPos"] = std::make_unique<int_tag>(0);
    root.value["Status"] = std::make_unique<string_tag>("full");

    auto intArray = std::make_unique<intarray_tag>();
    intArray->value.resize(256);
    for (i32 z = 0; z < 16; ++z) {
        for (i32 x = 0; x < 16; ++x) {
            const i32 index = z * 16 + x;
            // Y+1 = x+z+50（绝对世界坐标，旧版格式无 minY 偏移）
            intArray->value[static_cast<size_t>(index)] = x + z + 50;
        }
    }
    root.value["HeightMap"] = std::move(intArray);

    std::ostringstream stream(std::ios::binary);
    stream << mc::nbt::contexts::java;
    root.write(stream);
    auto str = stream.str();
    std::vector<u8> nbtBytes(str.begin(), str.end());

    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader chunkReader(blockMapper, biomeMapper);
    JavaColumnReader columnReader(chunkReader);

    auto result = columnReader.readColumn(nbtBytes, 0, 0, 0);
    ASSERT_TRUE(result.success());
    ASSERT_TRUE(result.value().has_value());
    auto chunk = std::move(result.value().value());

    // 旧版格式：HeightMap int 数组直接存储 Y+1（绝对），无 minY 偏移
    // x=5, z=5: Y+1 = 60, Y = 59
    EXPECT_EQ(chunk.getTopBlockY(mc::HeightmapType::WorldSurface, 5, 5), 59);
    // WorldSurfaceWG 也应被填充（_readHeightmaps 旧版路径同时填充两者）
    EXPECT_EQ(chunk.getTopBlockY(mc::HeightmapType::WorldSurfaceWG, 5, 5), 59);
}

} // namespace
} // namespace mc
