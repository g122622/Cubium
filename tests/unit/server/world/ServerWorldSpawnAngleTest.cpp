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
 * @file ServerWorldSpawnAngleTest.cpp
 * @brief ServerWorld 世界出生数据（RespawnData）单元测试
 *
 * 覆盖出生数据的默认值、朝向归一化（yaw [-180,180]、pitch [-90,90]）、
 * 脚部坐标换算（spawnPosition）以及位置/朝向的独立访问。
 */

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/spawn/RespawnData.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include <gtest/gtest.h>

#include <limits>

using namespace mc;
using namespace mc::server;

class ServerWorldSpawnAngleTest : public ::testing::Test {
protected:
    static std::unique_ptr<ServerWorld> createTestWorld(const ServerWorldConfig& config)
    {
        auto world = std::make_unique<ServerWorld>(config);
        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));
        return world;
    }

    void SetUp() override
    {
        VanillaBlocks::initialize();
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 12345;
        world = createTestWorld(config);
    }

    void TearDown() override
    {
        world.reset();
    }

    std::unique_ptr<ServerWorld> world;
};

TEST_F(ServerWorldSpawnAngleTest, DefaultRespawnDataIsOverworldOriginWithZeroRotation)
{
    const RespawnData& data = world->respawnData();
    EXPECT_EQ(data.dimension(), DimensionManager::OVERWORLD);
    EXPECT_EQ(data.pos(), BlockPos(0, 0, 0));
    EXPECT_FLOAT_EQ(data.yaw, 0.0f);
    EXPECT_FLOAT_EQ(data.pitch, 0.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SpawnPositionIsBottomCenterOfSpawnBlock)
{
    // 出生数据存的是脚下方块；spawnPosition 返回方块中心 + 方块上表面。
    world->setRespawnData(RespawnData::of(0, BlockPos(100, 63, -200), 135.0f, 0.0f));

    const Vector3d pos = world->spawnPosition();
    EXPECT_DOUBLE_EQ(pos.x, 100.5);
    EXPECT_DOUBLE_EQ(pos.y, 64.0);
    EXPECT_DOUBLE_EQ(pos.z, -199.5);
    EXPECT_FLOAT_EQ(world->respawnData().yaw, 135.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetRespawnDataKeepsPositionWhenRotationChanges)
{
    world->setRespawnData(RespawnData::of(0, BlockPos(100, 64, 200), 0.0f, 0.0f));
    world->setRespawnData(RespawnData::of(0, BlockPos(100, 64, 200), 90.0f, 30.0f));

    EXPECT_EQ(world->respawnData().pos(), BlockPos(100, 64, 200));
    EXPECT_FLOAT_EQ(world->respawnData().yaw, 90.0f);
    EXPECT_FLOAT_EQ(world->respawnData().pitch, 30.0f);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnDataOfNormalizesRotation)
{
    // yaw 折叠到 [-180, 180]，pitch 夹紧到 [-90, 90]。
    const RespawnData data = RespawnData::of(0, BlockPos(1, 2, 3), 270.0f, 135.0f);
    EXPECT_FLOAT_EQ(data.yaw, -90.0f);
    EXPECT_FLOAT_EQ(data.pitch, 90.0f);

    const RespawnData negative = RespawnData::of(0, BlockPos(1, 2, 3), -540.0f, -135.0f);
    EXPECT_FLOAT_EQ(negative.yaw, -180.0f);
    EXPECT_FLOAT_EQ(negative.pitch, -90.0f);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnDataNbtRoundTrip)
{
    const RespawnData original = RespawnData::of(DimensionManager::NETHER, BlockPos(-12, 70, 345), 45.0f, -20.0f);
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    const auto decoded = RespawnData::fromNbt(tag);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, original);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnConfigNbtRoundTripWithoutForced)
{
    // forced=false 时 NBT 中不写 forced 键，解码后仍为 false。
    const RespawnConfig original(RespawnData::of(0, BlockPos(5, 64, 5), 0.0f, 0.0f), false);
    nbt::tags::compound_tag tag;
    original.toNbt(tag);
    EXPECT_EQ(tag.value.count("forced"), 0u);

    const auto decoded = RespawnConfig::fromNbt(tag);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_FALSE(decoded->forced);
    EXPECT_EQ(decoded->respawnData.globalPos, original.respawnData.globalPos);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnConfigNbtRoundTripWithForced)
{
    const RespawnConfig original(RespawnData::of(0, BlockPos(5, 64, 5), 90.0f, 15.0f), true);
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    const auto decoded = RespawnConfig::fromNbt(tag);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded->forced);
    EXPECT_TRUE(decoded->isSamePosition(original));
    EXPECT_FLOAT_EQ(decoded->respawnData.yaw, 90.0f);
    EXPECT_FLOAT_EQ(decoded->respawnData.pitch, 15.0f);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnConfigIsSamePositionIgnoresRotationAndForced)
{
    const RespawnConfig bed(RespawnData::of(0, BlockPos(5, 64, 5), 0.0f, 0.0f), false);
    const RespawnConfig sameBlockDifferentRotation(RespawnData::of(0, BlockPos(5, 64, 5), 90.0f, 10.0f), true);

    EXPECT_TRUE(bed.isSamePosition(sameBlockDifferentRotation));
    EXPECT_FALSE(bed.isSamePosition(std::optional<RespawnConfig>{}));
    EXPECT_FALSE(bed.isSamePosition(RespawnConfig(RespawnData::of(0, BlockPos(6, 64, 5), 0.0f, 0.0f), false)));
}

TEST_F(ServerWorldSpawnAngleTest, RespawnConfigGetDimensionOrDefaultFallsBackToOverworld)
{
    EXPECT_EQ(RespawnConfig::getDimensionOrDefault(nullptr), DimensionManager::OVERWORLD);

    const RespawnConfig netherConfig(RespawnData::of(DimensionManager::NETHER, BlockPos(0, 64, 0), 0.0f, 0.0f), false);
    EXPECT_EQ(RespawnConfig::getDimensionOrDefault(&netherConfig), DimensionManager::NETHER);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnDataFromNbtRejectsNonFiniteRotation)
{
    // 非有限 yaw/pitch 会让归一化产生 NaN：按缺失处理回退到 0。
    const RespawnData original = RespawnData::of(0, BlockPos(1, 2, 3), 0.0f, 0.0f);
    nbt::tags::compound_tag tag;
    original.toNbt(tag);
    tag.put("yaw", std::numeric_limits<f32>::quiet_NaN());
    tag.put("pitch", std::numeric_limits<f32>::infinity());

    const auto decoded = RespawnData::fromNbt(tag);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_FLOAT_EQ(decoded->yaw, 0.0f);
    EXPECT_FLOAT_EQ(decoded->pitch, 0.0f);
}

TEST_F(ServerWorldSpawnAngleTest, RespawnDataFromNbtRejectsMissingPosition)
{
    nbt::tags::compound_tag tag;
    tag.put("dimension", std::string("minecraft:overworld"));
    EXPECT_FALSE(RespawnData::fromNbt(tag).has_value());
}