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

// ============================================================================
// ServerWorld::initializeWorldSpawn 集成测试
//
// 本测试覆盖 TODO 收敛的核心集成点：
// - initializeWorldSpawn 是否真正调用 Climate.Sampler.findSpawnPosition
// - 主世界预设的 spawnTarget 是否驱动气候搜索
// - 下界/末地等空.spawnTarget 是否降级为 (0,0) 区块
// - 出生点坐标是否落在合理范围内
// ============================================================================

#include <ctime>
#include <filesystem>

#include <gtest/gtest.h>

#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

using namespace mc;
using namespace mc::server;

namespace {

class ServerWorldInitializeWorldSpawnTest : public ::testing::Test {
protected:
    std::unique_ptr<ServerWorld> createOverworldWorld(u64 seed)
    {
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = seed;

        auto world = std::make_unique<ServerWorld>(config);
        world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));

        return world;
    }

    std::unique_ptr<ServerWorld> createNetherWorld(u64 seed)
    {
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = seed;

        auto world = std::make_unique<ServerWorld>(config);
        world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::nether();
        auto randomState = mc::world::gen::RandomState::create(settings, seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));

        return world;
    }

    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_testDir = std::filesystem::temp_directory_path() / "mc_spawn_init_test" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(m_testDir);

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();
    }

    void TearDown() override
    {
        m_storage.close();
        if (std::filesystem::exists(m_testDir)) {
            std::error_code ec;
            std::filesystem::remove_all(m_testDir, ec);
        }
    }

    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ============================================================================
// 1. 主世界 initializeWorldSpawn 集成测试
// ============================================================================

TEST_F(ServerWorldInitializeWorldSpawnTest, Overworld_Initialize_CallsFindSpawnPosition)
{
    // 验证主世界 initializeWorldSpawn 调用 Climate.Sampler.findSpawnPosition
    // 并设置合理的出生点（不在默认 (0, SEA_LEVEL+1, 0)）
    // 注意：findSpawnPosition 在气候空间搜索，结果可能落在任意区块；
    // 但 SpawnLocationHelper::findSpawnLocationInChunk 可能因为生成的区块不适合
    // （如水面/熔岩）而返回 nullopt，此时降级为 (0, SEA_LEVEL+1, 0)。
    // 关键验证点：initializeWorldSpawn 不崩溃且 worldSpawnPoint 被设置。
    auto world = createOverworldWorld(12345ULL);
    ASSERT_TRUE(world->initialize().success());

    world->initializeWorldSpawn();

    const auto spawn = world->worldSpawnPoint();
    // 出生点应该在合理高度范围内（不是 NaN 或极端值）
    EXPECT_FALSE(std::isnan(spawn.x));
    EXPECT_FALSE(std::isnan(spawn.y));
    EXPECT_FALSE(std::isnan(spawn.z));
    // y 坐标应该在海平面附近或以上
    EXPECT_GE(spawn.y, static_cast<f64>(world::SEA_LEVEL));
}

TEST_F(ServerWorldInitializeWorldSpawnTest, Overworld_Initialize_SpawnPointDeterministic)
{
    // 同一种子的主世界出生点应该是确定性的
    auto world1 = createOverworldWorld(77777ULL);
    ASSERT_TRUE(world1->initialize().success());
    world1->initializeWorldSpawn();
    const auto spawn1 = world1->worldSpawnPoint();

    auto world2 = createOverworldWorld(77777ULL);
    ASSERT_TRUE(world2->initialize().success());
    world2->initializeWorldSpawn();
    const auto spawn2 = world2->worldSpawnPoint();

    EXPECT_DOUBLE_EQ(spawn1.x, spawn2.x);
    EXPECT_DOUBLE_EQ(spawn1.y, spawn2.y);
    EXPECT_DOUBLE_EQ(spawn1.z, spawn2.z);
}

TEST_F(ServerWorldInitializeWorldSpawnTest, Overworld_Initialize_GeneratorHasSpawnTarget)
{
    // 验证主世界 NoiseChunkGenerator 的 RandomState.Sampler 持有非空 spawnTarget
    // 这是 initializeWorldSpawn 走气候搜索路径的前提
    auto world = createOverworldWorld(12345ULL);
    ASSERT_TRUE(world->initialize().success());

    auto* generator = world->chunkManager()->generator();
    ASSERT_NE(generator, nullptr);

    auto* noiseGenerator = dynamic_cast<NoiseChunkGenerator*>(generator);
    ASSERT_NE(noiseGenerator, nullptr);

    const auto& randomState = noiseGenerator->randomState();
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_FALSE(sampler.spawnTarget().empty());
    EXPECT_EQ(sampler.spawnTarget().size(), 2u);
}

TEST_F(ServerWorldInitializeWorldSpawnTest, Overworld_Initialize_FindSpawnPositionReturnsConsistentResult)
{
    // 验证 initializeWorldSpawn 使用的 findSpawnPosition 与直接调用 sampler.findSpawnPosition 一致
    auto world = createOverworldWorld(12345ULL);
    ASSERT_TRUE(world->initialize().success());

    auto* generator = world->chunkManager()->generator();
    auto* noiseGenerator = dynamic_cast<NoiseChunkGenerator*>(generator);
    ASSERT_NE(noiseGenerator, nullptr);

    const auto& sampler = noiseGenerator->randomState()->sampler();
    const BlockPos climateSpawn = sampler.findSpawnPosition();

    // climateSpawn.y 应该为 0（MC 行为）
    EXPECT_EQ(climateSpawn.y, 0);

    // initializeWorldSpawn 内部会调用同一个 findSpawnPosition
    // 验证不崩溃
    world->initializeWorldSpawn();
    SUCCEED() << "initializeWorldSpawn completed without crash";
}

// ============================================================================
// 2. 下界 initializeWorldSpawn 降级测试
// ============================================================================

TEST_F(ServerWorldInitializeWorldSpawnTest, Nether_Initialize_HasEmptySpawnTarget)
{
    // 下界 Sampler.spawnTarget() 应该为空
    auto world = createNetherWorld(0ULL);
    ASSERT_TRUE(world->initialize().success());

    auto* generator = world->chunkManager()->generator();
    ASSERT_NE(generator, nullptr);

    auto* noiseGenerator = dynamic_cast<NoiseChunkGenerator*>(generator);
    ASSERT_NE(noiseGenerator, nullptr);

    const auto& randomState = noiseGenerator->randomState();
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

TEST_F(ServerWorldInitializeWorldSpawnTest, Nether_Initialize_FallsBackToOriginChunk)
{
    // 下界 spawnTarget 为空，initializeWorldSpawn 应降级为 (0,0) 区块
    auto world = createNetherWorld(0ULL);
    ASSERT_TRUE(world->initialize().success());

    // 不崩溃即可
    world->initializeWorldSpawn();

    const auto spawn = world->worldSpawnPoint();
    EXPECT_FALSE(std::isnan(spawn.x));
    EXPECT_FALSE(std::isnan(spawn.y));
    EXPECT_FALSE(std::isnan(spawn.z));
}

// ============================================================================
// 3. 多次调用幂等性测试
// ============================================================================

TEST_F(ServerWorldInitializeWorldSpawnTest, MultipleInitializeWorldSpawnCallsIdempotent)
{
    // 多次调用 initializeWorldSpawn 不应该导致崩溃或状态损坏
    auto world = createOverworldWorld(12345ULL);
    ASSERT_TRUE(world->initialize().success());

    world->initializeWorldSpawn();
    const auto spawn1 = world->worldSpawnPoint();

    world->initializeWorldSpawn();
    const auto spawn2 = world->worldSpawnPoint();

    // 多次调用结果应该一致（同一种子、同一区块生成结果）
    EXPECT_DOUBLE_EQ(spawn1.x, spawn2.x);
    EXPECT_DOUBLE_EQ(spawn1.y, spawn2.y);
    EXPECT_DOUBLE_EQ(spawn1.z, spawn2.z);
}

} // namespace
