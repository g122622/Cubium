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

#include <gtest/gtest.h>

#include "client/world/ClientWorld.hpp"
#include "common/world/chunk/data/Heightmap.hpp"

using namespace mc;
using namespace mc::client;

/**
 * @brief ClientWorld 天空可见性和高度图查询测试
 */
class ClientWorldSkyAndHeightmapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto result = m_world.initialize(12345);
        ASSERT_TRUE(result.success()) << "Failed to initialize ClientWorld";
    }

    void TearDown() override { m_world.destroy(); }

    ClientWorld m_world;
};

// ========== hasSkyLight() 测试 ==========

TEST_F(ClientWorldSkyAndHeightmapTest, HasSkyLightInOverworld)
{
    // 主世界（维度 0）有天空光照
    m_world.setDimensionId(0);
    EXPECT_TRUE(m_world.hasSkyLight());
}

TEST_F(ClientWorldSkyAndHeightmapTest, NoSkyLightInNether)
{
    // 下界（维度 -1）没有天空光照
    m_world.setDimensionId(-1);
    EXPECT_FALSE(m_world.hasSkyLight());
}

TEST_F(ClientWorldSkyAndHeightmapTest, NoSkyLightInTheEnd)
{
    // 末地（维度 1）没有天空光照
    m_world.setDimensionId(1);
    EXPECT_FALSE(m_world.hasSkyLight());
}

// ========== canSeeSky() 维度测试 ==========

TEST_F(ClientWorldSkyAndHeightmapTest, CanSeeSkyInOverworldWithMaxSkyLight)
{
    // 主世界中，没有区块数据时 getSkyLight 默认返回 15，所以 canSeeSky 返回 true
    m_world.setDimensionId(0);
    EXPECT_TRUE(m_world.hasSkyLight());
    // 空世界没有区块数据，getSkyLight 返回 15（默认值），所以 canSeeSky 返回 true
    EXPECT_TRUE(m_world.canSeeSky(BlockPos(0, 100, 0)));
}

TEST_F(ClientWorldSkyAndHeightmapTest, CannotSeeSkyInNether)
{
    // 下界中，即使没有区块数据也不能看到天空（维度检查优先）
    m_world.setDimensionId(-1);
    EXPECT_FALSE(m_world.canSeeSky(BlockPos(0, 100, 0)));
}

TEST_F(ClientWorldSkyAndHeightmapTest, CannotSeeSkyInTheEnd)
{
    // 末地中，即使没有区块数据也不能看到天空（维度检查优先）
    m_world.setDimensionId(1);
    EXPECT_FALSE(m_world.canSeeSky(BlockPos(0, 100, 0)));
}

TEST_F(ClientWorldSkyAndHeightmapTest, CanSeeSkyDimensionCheckBeforeSkyLight)
{
    // 验证维度检查在天空光照检查之前执行
    // 下界中即使设置维度为 -1，canSeeSky 也应该返回 false
    m_world.setDimensionId(-1);
    EXPECT_FALSE(m_world.hasSkyLight());
    EXPECT_FALSE(m_world.canSeeSky(BlockPos(0, 64, 0)));
}

// ========== getTopBlockY() 测试 ==========

TEST_F(ClientWorldSkyAndHeightmapTest, GetTopBlockYReturnsMinBuildHeightWithoutChunk)
{
    // 没有区块数据时，getTopBlockY 应该返回 MIN_BUILD_HEIGHT
    constexpr i32 MIN_BUILD_HEIGHT = -64; // mc::world::MIN_BUILD_HEIGHT
    EXPECT_EQ(m_world.getTopBlockY(mc::world::chunk::HeightmapType::MotionBlocking, 0, 0), MIN_BUILD_HEIGHT);
}

TEST_F(ClientWorldSkyAndHeightmapTest, GetTopBlockYWithWorldSurfaceType)
{
    // 使用 WorldSurface 类型
    constexpr i32 MIN_BUILD_HEIGHT = -64;
    EXPECT_EQ(m_world.getTopBlockY(mc::world::chunk::HeightmapType::WorldSurface, 0, 0), MIN_BUILD_HEIGHT);
}

TEST_F(ClientWorldSkyAndHeightmapTest, GetTopBlockYWithOceanFloorType)
{
    // 使用 OceanFloor 类型
    constexpr i32 MIN_BUILD_HEIGHT = -64;
    EXPECT_EQ(m_world.getTopBlockY(mc::world::chunk::HeightmapType::OceanFloor, 0, 0), MIN_BUILD_HEIGHT);
}

// ========== getHeight() 与 getTopBlockY() 一致性测试 ==========

TEST_F(ClientWorldSkyAndHeightmapTest, GetHeightReturnsMinBuildHeightWithoutChunk)
{
    // 没有区块数据时，getHeight 也应该返回 MIN_BUILD_HEIGHT
    constexpr i32 MIN_BUILD_HEIGHT = -64;
    EXPECT_EQ(m_world.getHeight(0, 0), MIN_BUILD_HEIGHT);
}
