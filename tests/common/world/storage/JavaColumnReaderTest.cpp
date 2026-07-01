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

#include "common/world/WorldConstants.hpp"
#include "common/world/dimension/DimensionType.hpp"
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

} // namespace
} // namespace mc
