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
 * @brief ServerWorld 出生点朝向（spawnAngle）单元测试
 *
 * 测试 ServerWorld 的 spawnAngle 存储、getter/setter、
 * setWorldSpawnPoint 带 angle 参数的重载。
 *
 * 注意：applyLevelRuntimeData 测试需要完整的存储基础设施，
 * 因此不在本文件中测试。该方法的 spawnAngle 逻辑仅一行
 * m_spawnAngle = runtimeData.spawnAngle，已通过代码审查确认。
 */

#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;

// ============================================================================
// 测试固件
// ============================================================================

class ServerWorldSpawnAngleTest : public ::testing::Test {
protected:
    static std::unique_ptr<ServerWorld> createTestWorld(const ServerWorldConfig& config)
    {
        auto world = std::make_unique<ServerWorld>(config);
        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
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

    void TearDown() override { world.reset(); }

    std::unique_ptr<ServerWorld> world;
};

// ============================================================================
// 默认值测试
// ============================================================================

TEST_F(ServerWorldSpawnAngleTest, DefaultSpawnAngleIsZero)
{
    // 新创建的 ServerWorld 出生点朝向默认为 0
    EXPECT_FLOAT_EQ(world->spawnAngle(), 0.0f);
}

// ============================================================================
// setSpawnAngle 测试
// ============================================================================

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAnglePositive)
{
    world->setSpawnAngle(90.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 90.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAngleNegative)
{
    world->setSpawnAngle(-45.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), -45.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAngleBoundary180)
{
    world->setSpawnAngle(180.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 180.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAngleBoundaryNegative180)
{
    world->setSpawnAngle(-180.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), -180.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAngleZero)
{
    world->setSpawnAngle(45.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 45.0f);

    world->setSpawnAngle(0.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 0.0f);
}

// ============================================================================
// setWorldSpawnPoint 带 angle 参数测试
// ============================================================================

TEST_F(ServerWorldSpawnAngleTest, SetWorldSpawnPointWithAngle)
{
    Vector3d pos(100.5, 64.0, -200.5);
    world->setWorldSpawnPoint(pos, 135.0f);

    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().x, 100.5);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().y, 64.0);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().z, -200.5);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 135.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetWorldSpawnPointDefaultAngle)
{
    // 不指定 angle 时默认为 0.0f
    Vector3d pos(50.0, 70.0, 100.0);
    world->setSpawnAngle(90.0f); // 先设置一个非零值
    EXPECT_FLOAT_EQ(world->spawnAngle(), 90.0f);

    world->setWorldSpawnPoint(pos);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().x, 50.0);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 0.0f); // 默认重置为 0
}

TEST_F(ServerWorldSpawnAngleTest, SetWorldSpawnPointOverridesAngle)
{
    // 先设置朝向为 90 度
    world->setSpawnAngle(90.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 90.0f);

    // 通过 setWorldSpawnPoint 更新位置和朝向
    Vector3d pos(200.0, 80.0, 300.0);
    world->setWorldSpawnPoint(pos, -45.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), -45.0f);
}

// ============================================================================
// setWorldSpawnPoint 和 setSpawnAngle 独立操作测试
// ============================================================================

TEST_F(ServerWorldSpawnAngleTest, SetSpawnAngleDoesNotAffectPosition)
{
    Vector3d pos(100.0, 64.0, 200.0);
    world->setWorldSpawnPoint(pos, 0.0f);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().x, 100.0);

    world->setSpawnAngle(90.0f);
    // 位置不变
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().x, 100.0);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().y, 64.0);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().z, 200.0);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 90.0f);
}

TEST_F(ServerWorldSpawnAngleTest, SetWorldSpawnPointWithoutAngleResetsAngle)
{
    // 先设置朝向
    world->setSpawnAngle(45.0f);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 45.0f);

    // setWorldSpawnPoint 不带 angle 默认 0.0f，会重置朝向
    Vector3d pos(50.0, 70.0, 100.0);
    world->setWorldSpawnPoint(pos);
    EXPECT_FLOAT_EQ(world->spawnAngle(), 0.0f);
}

// ============================================================================
// 世界出生点位置保持不变测试
// ============================================================================

TEST_F(ServerWorldSpawnAngleTest, DefaultSpawnPoint)
{
    // 默认出生点在 (0, SEA_LEVEL+1, 0)
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().x, 0.0);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().y, static_cast<f64>(world::SEA_LEVEL) + 1.0);
    EXPECT_DOUBLE_EQ(world->worldSpawnPoint().z, 0.0);
}
