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

#include "common/util/NibbleArray.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <climits>
#include <gtest/gtest.h>

// ============================================================================
// NibbleArray 测试
// ============================================================================

class NibbleArrayTest : public ::testing::Test {
protected:
    mc::NibbleArray array;
};

TEST_F(NibbleArrayTest, DefaultConstructor)
{
    EXPECT_TRUE(array.isEmpty());
}

TEST_F(NibbleArrayTest, FillTest)
{
    array.fill(7);
    EXPECT_EQ(array.get(0, 0, 0), 7);
    EXPECT_EQ(array.get(15, 15, 15), 7);
    EXPECT_EQ(array.get(8, 8, 8), 7);
}

TEST_F(NibbleArrayTest, SetAndGet)
{
    array.set(0, 0, 0, 5);
    EXPECT_EQ(array.get(0, 0, 0), 5);

    array.set(15, 15, 15, 12);
    EXPECT_EQ(array.get(15, 15, 15), 12);

    array.set(8, 8, 8, 15);
    EXPECT_EQ(array.get(8, 8, 8), 15);
}

TEST_F(NibbleArrayTest, BoundaryValues)
{
    array.set(5, 5, 5, 0);
    EXPECT_EQ(array.get(5, 5, 5), 0);

    array.set(5, 5, 5, 15);
    EXPECT_EQ(array.get(5, 5, 5), 15);
}

TEST_F(NibbleArrayTest, CopyTest)
{
    array.set(0, 0, 0, 10);
    array.set(5, 5, 5, 7);

    mc::NibbleArray copy = array.copy();
    EXPECT_EQ(copy.get(0, 0, 0), 10);
    EXPECT_EQ(copy.get(5, 5, 5), 7);

    array.set(0, 0, 0, 3);
    EXPECT_EQ(copy.get(0, 0, 0), 10);
}

TEST_F(NibbleArrayTest, IndexCalculation)
{
    for (mc::i32 y = 0; y < 16; ++y) {
        for (mc::i32 z = 0; z < 16; ++z) {
            for (mc::i32 x = 0; x < 16; ++x) {
                mc::i32 index = y * 256 + z * 16 + x;
                mc::u8 value = static_cast<mc::u8>(index % 16);
                array.set(x, y, z, value);
            }
        }
    }

    for (mc::i32 y = 0; y < 16; ++y) {
        for (mc::i32 z = 0; z < 16; ++z) {
            for (mc::i32 x = 0; x < 16; ++x) {
                mc::i32 index = y * 256 + z * 16 + x;
                mc::u8 expected = static_cast<mc::u8>(index % 16);
                EXPECT_EQ(array.get(x, y, z), expected);
            }
        }
    }
}

TEST_F(NibbleArrayTest, FilledStaticMethod)
{
    mc::NibbleArray filled = mc::NibbleArray::filled(12);
    EXPECT_FALSE(filled.isEmpty());

    for (int y = 0; y < 16; ++y) {
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                EXPECT_EQ(filled.get(x, y, z), 12);
            }
        }
    }
}

TEST_F(NibbleArrayTest, IndexWraparound)
{
    array.set(0, 0, 0, 5);
    EXPECT_EQ(array.get(16, 0, 0), 5);
    EXPECT_EQ(array.get(0, 16, 0), 5);
    EXPECT_EQ(array.get(0, 0, 16), 5);
}

TEST_F(NibbleArrayTest, LinearIndex)
{
    array.set(5, 10, 3, 7);
    mc::i32 index = mc::NibbleArray::getIndex(5, 10, 3);
    EXPECT_EQ(array.get(index), 7);
}

TEST_F(NibbleArrayTest, PackedByteTest)
{
    array.set(0, 0, 0, 5);
    array.set(1, 0, 0, 10);

    EXPECT_EQ(array.get(0, 0, 0), 5);
    EXPECT_EQ(array.get(1, 0, 0), 10);

    const auto& data = array.data();
    EXPECT_EQ(data.size(), mc::NibbleArray::BYTE_SIZE);
    mc::u8 packed = data[0];
    EXPECT_EQ(packed & 0xF, 5);
    EXPECT_EQ((packed >> 4) & 0xF, 10);
}

// ============================================================================
// SectionPos 测试
// ============================================================================

class SectionPosTest : public ::testing::Test {
protected:
};

TEST_F(SectionPosTest, Construction)
{
    mc::SectionPos pos(10, 5, -3);
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 5);
    EXPECT_EQ(pos.z, -3);
}

TEST_F(SectionPosTest, ToLongAndFromLong)
{
    mc::SectionPos original(100, -10, 200);
    mc::i64 encoded = original.toLong();
    mc::SectionPos decoded = mc::SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, original.x);
    EXPECT_EQ(decoded.y, original.y);
    EXPECT_EQ(decoded.z, original.z);
}

TEST_F(SectionPosTest, NegativeCoordinates)
{
    mc::SectionPos original(-50, -5, -100);
    mc::i64 encoded = original.toLong();
    mc::SectionPos decoded = mc::SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, original.x);
    EXPECT_EQ(decoded.y, original.y);
    EXPECT_EQ(decoded.z, original.z);
}

TEST_F(SectionPosTest, Offset)
{
    mc::SectionPos pos(10, 5, 20);

    mc::SectionPos up = pos.offset(mc::Direction::Up);
    EXPECT_EQ(up.x, 10);
    EXPECT_EQ(up.y, 6);
    EXPECT_EQ(up.z, 20);

    mc::SectionPos down = pos.offset(mc::Direction::Down);
    EXPECT_EQ(down.x, 10);
    EXPECT_EQ(down.y, 4);
    EXPECT_EQ(down.z, 20);

    mc::SectionPos north = pos.offset(mc::Direction::North);
    EXPECT_EQ(north.x, 10);
    EXPECT_EQ(north.y, 5);
    EXPECT_EQ(north.z, 19);

    mc::SectionPos south = pos.offset(mc::Direction::South);
    EXPECT_EQ(south.x, 10);
    EXPECT_EQ(south.y, 5);
    EXPECT_EQ(south.z, 21);

    mc::SectionPos west = pos.offset(mc::Direction::West);
    EXPECT_EQ(west.x, 9);
    EXPECT_EQ(west.y, 5);
    EXPECT_EQ(west.z, 20);

    mc::SectionPos east = pos.offset(mc::Direction::East);
    EXPECT_EQ(east.x, 11);
    EXPECT_EQ(east.y, 5);
    EXPECT_EQ(east.z, 20);
}

TEST_F(SectionPosTest, ToColumnLong)
{
    mc::SectionPos pos(10, 5, 20);
    mc::i64 columnPos = pos.toColumnLong();

    mc::SectionPos pos2(10, 15, 20);
    mc::i64 columnPos2 = pos2.toColumnLong();

    EXPECT_EQ(columnPos, columnPos2);
}

TEST_F(SectionPosTest, LargeCoordinates)
{
    mc::SectionPos pos(1000000, 100, -1000000);
    mc::i64 encoded = pos.toLong();
    mc::SectionPos decoded = mc::SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, pos.x);
    EXPECT_EQ(decoded.y, pos.y);
    EXPECT_EQ(decoded.z, pos.z);
}

TEST_F(SectionPosTest, FromBlockPosTest)
{
    mc::BlockPos blockPos(25, 40, -30);
    mc::SectionPos sectionPos(blockPos);

    EXPECT_EQ(sectionPos.x, 1);
    EXPECT_EQ(sectionPos.y, 2);
    EXPECT_EQ(sectionPos.z, -2);
}

TEST_F(SectionPosTest, WorldCoordsTest)
{
    mc::SectionPos pos(10, 5, 20);

    EXPECT_EQ(pos.worldX(), 160);
    EXPECT_EQ(pos.worldY(), 80);
    EXPECT_EQ(pos.worldZ(), 320);
}

// ============================================================================
// LightType 测试
// ============================================================================

class LightTypeTest : public ::testing::Test {
protected:
};

TEST_F(LightTypeTest, Values)
{
    EXPECT_EQ(static_cast<mc::i32>(mc::LightType::SKY), 0);
    EXPECT_EQ(static_cast<mc::i32>(mc::LightType::BLOCK), 1);
}

// ============================================================================
// InternalLight 测试
// ============================================================================

class InternalLightTest : public ::testing::Test {
protected:
};

TEST_F(InternalLightTest, GetCelestialAngle)
{
    // 简单线性天体角度计算
    // celestialAngle = dayTime % 24000 / 24000.0
    // 0.0 = 日出, 0.25 = 正午, 0.5 = 日落, 0.75 = 午夜

    // 日出 (dayTime = 0): celestialAngle = 0.0
    EXPECT_NEAR(mc::InternalLightUtils::getCelestialAngle(0), 0.0f, 0.01f);

    // 正午 (dayTime = 6000): celestialAngle = 0.25
    EXPECT_NEAR(mc::InternalLightUtils::getCelestialAngle(6000), 0.25f, 0.01f);

    // 日落 (dayTime = 12000): celestialAngle = 0.5
    EXPECT_NEAR(mc::InternalLightUtils::getCelestialAngle(12000), 0.5f, 0.01f);

    // 午夜 (dayTime = 18000): celestialAngle = 0.75
    EXPECT_NEAR(mc::InternalLightUtils::getCelestialAngle(18000), 0.75f, 0.01f);
}

TEST_F(InternalLightTest, CalculateSkyDarkening)
{
    // 天空减暗计算：
    // sunHeight = sin(celestialAngle * 2π)
    // brightness = (sunHeight + 1) / 2
    // darkening = (1 - brightness) * 11
    //
    // 使用简单线性天体角度：
    // - 正午 (dayTime=6000, celestialAngle=0.25): sin(π/2)=1, brightness=1, darkening=0
    // - 午夜 (dayTime=18000, celestialAngle=0.75): sin(3π/2)=-1, brightness=0, darkening=11
    // - 日出/日落 (celestialAngle=0/0.5): sin=0, brightness=0.5, darkening≈5

    // 正午 (dayTime = 6000) - 最亮，减暗为0
    mc::i32 noonDarkening = mc::InternalLightUtils::calculateSkyDarkening(6000, false, false);
    EXPECT_EQ(noonDarkening, 0);

    // 午夜 (dayTime = 18000) - 最暗，减暗为11
    mc::i32 midnightDarkening = mc::InternalLightUtils::calculateSkyDarkening(18000, false, false);
    EXPECT_EQ(midnightDarkening, 11);

    // 日出 (dayTime = 0) - 中等亮度，约5
    mc::i32 sunriseDarkening = mc::InternalLightUtils::calculateSkyDarkening(0, false, false);
    EXPECT_GT(sunriseDarkening, noonDarkening);     // 比正午暗
    EXPECT_LT(sunriseDarkening, midnightDarkening); // 比午夜亮
    EXPECT_NEAR(sunriseDarkening, 5, 1);            // 约等于5

    // 日落 (dayTime = 12000) - 中等亮度，约5
    mc::i32 sunsetDarkening = mc::InternalLightUtils::calculateSkyDarkening(12000, false, false);
    EXPECT_GT(sunsetDarkening, noonDarkening);     // 比正午暗
    EXPECT_LT(sunsetDarkening, midnightDarkening); // 比午夜亮
    EXPECT_NEAR(sunsetDarkening, 5, 1);            // 约等于5

    // 下雨 - 天空变暗（+3，最多到11）
    mc::i32 rainDarkening = mc::InternalLightUtils::calculateSkyDarkening(6000, true, false);
    EXPECT_EQ(rainDarkening, 3); // 正午 + 3 = 3

    // 雷暴 - 天空更暗（+10，最多到11）
    mc::i32 thunderDarkening = mc::InternalLightUtils::calculateSkyDarkening(6000, false, true);
    EXPECT_EQ(thunderDarkening, 10); // 正午 + 10 = 10
}

TEST_F(InternalLightTest, CalculateRawBrightness)
{
    EXPECT_EQ(mc::InternalLightUtils::calculateRawBrightness(15, 0, 0), 15);
    EXPECT_EQ(mc::InternalLightUtils::calculateRawBrightness(0, 15, 0), 15);
    EXPECT_EQ(mc::InternalLightUtils::calculateRawBrightness(0, 15, 10), 5);
    EXPECT_EQ(mc::InternalLightUtils::calculateRawBrightness(10, 15, 10), 10);
    EXPECT_EQ(mc::InternalLightUtils::calculateRawBrightness(5, 15, 20), 5);
}

TEST_F(InternalLightTest, IsDarkEnoughForSpawning)
{
    EXPECT_TRUE(mc::InternalLightUtils::isDarkEnoughForSpawning(0));
    EXPECT_TRUE(mc::InternalLightUtils::isDarkEnoughForSpawning(7));
    EXPECT_FALSE(mc::InternalLightUtils::isDarkEnoughForSpawning(8));
    EXPECT_FALSE(mc::InternalLightUtils::isDarkEnoughForSpawning(15));
}

TEST_F(InternalLightTest, DaytimeNighttime)
{
    EXPECT_TRUE(mc::InternalLightUtils::isDaytime(0));
    EXPECT_TRUE(mc::InternalLightUtils::isDaytime(6000));
    EXPECT_TRUE(mc::InternalLightUtils::isDaytime(11999));
    EXPECT_FALSE(mc::InternalLightUtils::isDaytime(12000));

    EXPECT_TRUE(mc::InternalLightUtils::isNighttime(12000));
    EXPECT_TRUE(mc::InternalLightUtils::isNighttime(18000));
    EXPECT_TRUE(mc::InternalLightUtils::isNighttime(23999));
    EXPECT_FALSE(mc::InternalLightUtils::isNighttime(0));
    EXPECT_FALSE(mc::InternalLightUtils::isNighttime(6000));
}

TEST_F(InternalLightTest, MoonPhase)
{
    EXPECT_EQ(mc::InternalLightUtils::getMoonPhase(0), 0);
    EXPECT_EQ(mc::InternalLightUtils::getMoonPhase(24000), 1);
    EXPECT_EQ(mc::InternalLightUtils::getMoonPhase(48000), 2);
    EXPECT_EQ(mc::InternalLightUtils::getMoonPhase(168000), 7);
    EXPECT_EQ(mc::InternalLightUtils::getMoonPhase(192000), 0);
}

// ============================================================================
// LightEngineUtils 测试
// ============================================================================

class LightEngineUtilsTest : public ::testing::Test {
protected:
};

TEST_F(LightEngineUtilsTest, PackUnpackPos)
{
    // 测试位置编码和解码
    mc::i32 x = 100, y = 64, z = 50;
    mc::i64 packed = mc::LightEngineUtils::packPos(x, y, z);

    mc::i32 unpackedX, unpackedY, unpackedZ;
    mc::LightEngineUtils::unpackPos(packed, unpackedX, unpackedY, unpackedZ);

    EXPECT_EQ(unpackedX, x);
    EXPECT_EQ(unpackedY, y);
    EXPECT_EQ(unpackedZ, z);
}

TEST_F(LightEngineUtilsTest, PackUnpackNegativePos)
{
    // 测试负坐标（注意：Y坐标限制在12位有符号范围内，-2048到2047）
    mc::i32 x = -100, y = -10, z = -200;
    mc::i64 packed = mc::LightEngineUtils::packPos(x, y, z);

    mc::i32 unpackedX, unpackedY, unpackedZ;
    mc::LightEngineUtils::unpackPos(packed, unpackedX, unpackedY, unpackedZ);

    EXPECT_EQ(unpackedX, x);
    EXPECT_EQ(unpackedY, y);
    EXPECT_EQ(unpackedZ, z);
}

TEST_F(LightEngineUtilsTest, OffsetPos)
{
    // 测试位置偏移
    mc::i64 pos = mc::LightEngineUtils::packPos(10, 20, 30);

    // 向上偏移
    mc::i64 upPos = mc::LightEngineUtils::offsetPos(pos, mc::Direction::Up);
    mc::i32 x, y, z;
    mc::LightEngineUtils::unpackPos(upPos, x, y, z);
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 21);
    EXPECT_EQ(z, 30);

    // 向下偏移
    mc::i64 downPos = mc::LightEngineUtils::offsetPos(pos, mc::Direction::Down);
    mc::LightEngineUtils::unpackPos(downPos, x, y, z);
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 19);
    EXPECT_EQ(z, 30);

    // 向北偏移
    mc::i64 northPos = mc::LightEngineUtils::offsetPos(pos, mc::Direction::North);
    mc::LightEngineUtils::unpackPos(northPos, x, y, z);
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 20);
    EXPECT_EQ(z, 29);
}

TEST_F(LightEngineUtilsTest, WorldToSectionPos)
{
    // 测试世界坐标转区块段坐标
    mc::i64 worldPos = mc::LightEngineUtils::packPos(20, 35, 40);
    mc::i64 sectionPos = mc::LightEngineUtils::worldToSectionPos(worldPos);

    mc::SectionPos section = mc::SectionPos::fromLong(sectionPos);
    EXPECT_EQ(section.x, 1); // 20 / 16 = 1
    EXPECT_EQ(section.y, 2); // 35 / 16 = 2
    EXPECT_EQ(section.z, 2); // 40 / 16 = 2
}

TEST_F(LightEngineUtilsTest, ExtractNibbleIndices)
{
    // 测试从世界位置提取区块段内坐标
    mc::i64 pos = mc::LightEngineUtils::packPos(5, 18, 10);
    mc::i32 x, localY, z;
    mc::LightEngineUtils::extractNibbleIndices(pos, x, localY, z);

    EXPECT_EQ(x, 5);      // x & 0xF = 5
    EXPECT_EQ(localY, 2); // 18 & 0xF = 2
    EXPECT_EQ(z, 10);     // z & 0xF = 10
}

TEST_F(LightEngineUtilsTest, RootPos)
{
    EXPECT_EQ(mc::LightEngineUtils::ROOT_POS, LONG_MAX);
}

// ============================================================================
// Direction 光照相关测试
// ============================================================================

class DirectionLightTest : public ::testing::Test {
protected:
};

// ============================================================================
// SWMRNibbleArray 测试
// ============================================================================

class SWMRNibbleArrayTest : public ::testing::Test {
protected:
};

TEST_F(SWMRNibbleArrayTest, UpdatingWritesStayInvisibleUntilUpdateVisible)
{
    mc::SWMRNibbleArray nibble;

    EXPECT_EQ(nibble.getVisible(0), 0);
    EXPECT_EQ(nibble.getUpdating(0), 0);

    nibble.set(0, 9);

    EXPECT_EQ(nibble.getUpdating(0), 9);
    EXPECT_EQ(nibble.getVisible(0), 0);

    EXPECT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(0), 9);
}

TEST_F(SWMRNibbleArrayTest, SetFullStaysInvisibleUntilUpdateVisible)
{
    mc::SWMRNibbleArray nibble;

    nibble.setFull();
    EXPECT_EQ(nibble.getUpdating(0), 15);
    EXPECT_EQ(nibble.getVisible(0), 0);

    EXPECT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(0), 15);
}

TEST_F(DirectionLightTest, FromDelta)
{
    EXPECT_EQ(mc::Directions::fromDelta(0, -1, 0), mc::Direction::Down);
    EXPECT_EQ(mc::Directions::fromDelta(0, 1, 0), mc::Direction::Up);
    EXPECT_EQ(mc::Directions::fromDelta(0, 0, -1), mc::Direction::North);
    EXPECT_EQ(mc::Directions::fromDelta(0, 0, 1), mc::Direction::South);
    EXPECT_EQ(mc::Directions::fromDelta(-1, 0, 0), mc::Direction::West);
    EXPECT_EQ(mc::Directions::fromDelta(1, 0, 0), mc::Direction::East);
    EXPECT_EQ(mc::Directions::fromDelta(0, 0, 0), mc::Direction::None);
    EXPECT_EQ(mc::Directions::fromDelta(1, 1, 0), mc::Direction::None);
}

TEST_F(DirectionLightTest, Offsets)
{
    EXPECT_EQ(mc::Directions::xOffset(mc::Direction::Down), 0);
    EXPECT_EQ(mc::Directions::yOffset(mc::Direction::Down), -1);
    EXPECT_EQ(mc::Directions::zOffset(mc::Direction::Down), 0);

    EXPECT_EQ(mc::Directions::xOffset(mc::Direction::Up), 0);
    EXPECT_EQ(mc::Directions::yOffset(mc::Direction::Up), 1);
    EXPECT_EQ(mc::Directions::zOffset(mc::Direction::Up), 0);

    EXPECT_EQ(mc::Directions::xOffset(mc::Direction::East), 1);
    EXPECT_EQ(mc::Directions::yOffset(mc::Direction::East), 0);
    EXPECT_EQ(mc::Directions::zOffset(mc::Direction::East), 0);
}
