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
 * @file PlayerSpawnFinderTest.cpp
 * @brief 玩家出生位置搜索器单元测试
 *
 * 覆盖候选数量与步长计算、高度修正语义（上探间隙后落到其底部）以及
 * "无碰撞无液体"判定。半径搜索路径依赖真实世界的区块加载与生成，由
 * ServerWorld 集成测试与 GameTest 覆盖。
 */

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/player/PlayerSpawnFinder.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"

#include <filesystem>

using namespace mc;
using namespace mc::server;

namespace {

/// 候选数量上限
constexpr i32 ABSOLUTE_MAX_ATTEMPTS = 1024;

class PlayerSpawnFinderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_testDir = mc::test::makeUniqueTestDir("mc_player_spawn_finder_test");
        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        ServerWorldConfig config;
        config.viewDistance = 2;
        config.dimension = 0;
        config.seed = 424242ULL;
        m_world = std::make_unique<ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        m_world->setChunkManager(std::make_unique<ServerChunkManager>(*m_world, std::move(generator)));
        ASSERT_TRUE(m_world->initialize().success());
    }

    void TearDown() override
    {
        m_world.reset();
        m_storage.close();
        mc::test::removeTestDir(m_testDir);
    }

    std::unique_ptr<ServerWorld> m_world;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

TEST_F(PlayerSpawnFinderTest, CandidateCountCapsAtAbsoluteMax)
{
    EXPECT_EQ(PlayerSpawnFinder::candidateCount(0), 1);
    EXPECT_EQ(PlayerSpawnFinder::candidateCount(1), 9);
    EXPECT_EQ(PlayerSpawnFinder::candidateCount(10), 441);
    EXPECT_EQ(PlayerSpawnFinder::candidateCount(100), ABSOLUTE_MAX_ATTEMPTS);
}

TEST_F(PlayerSpawnFinderTest, CoprimeStepFollowsVanillaRule)
{
    // n <= 16 取 n-1；否则取 17。
    EXPECT_EQ(PlayerSpawnFinder::coprime(1), 0);
    EXPECT_EQ(PlayerSpawnFinder::coprime(9), 8);
    EXPECT_EQ(PlayerSpawnFinder::coprime(16), 15);
    EXPECT_EQ(PlayerSpawnFinder::coprime(17), 17);
    EXPECT_EQ(PlayerSpawnFinder::coprime(441), 17);
}

TEST_F(PlayerSpawnFinderTest, FixupSpawnHeightLandsOnFloorTop)
{
    // 高度修正要求目标列已加载：先同步加载 (0,0) 区块。
    ASSERT_NE(m_world->chunkManager()->requestFullChunkSync(0, 0), nullptr);
    const auto position = PlayerSpawnFinder::fixupSpawnHeight(*m_world, BlockPos(0, 70, 0));
    ASSERT_TRUE(position.has_value());
    const BlockPos landed(static_cast<i32>(std::floor(position->x)),
        static_cast<i32>(std::floor(position->y)),
        static_cast<i32>(std::floor(position->z)));
    // 目标列已加载：落在无碰撞无液体处，且脚下方块实心（间隙底部）。
    EXPECT_TRUE(PlayerSpawnFinder::noCollisionNoLiquid(*m_world, landed));
    const BlockState* below = m_world->getBlockState(landed.x, landed.y - 1, landed.z);
    EXPECT_TRUE(below != nullptr && below->isSolid());
}

TEST_F(PlayerSpawnFinderTest, FixupSpawnHeightHandlesUnloadedColumn)
{
    // fixupSpawnHeight 不加载区块（原版 ScoreboardFix 语义）：未加载列无法做碰撞扫描，
    // 但函数必须给出确定坐标而不是让登录流程失去落点。
    const auto position = PlayerSpawnFinder::fixupSpawnHeight(*m_world, BlockPos(100000, 70, 100000));
    ASSERT_TRUE(position.has_value());
    EXPECT_GE(position->y, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT));
    EXPECT_LE(position->y, static_cast<f32>(mc::world::MAX_BUILD_HEIGHT));
}

TEST_F(PlayerSpawnFinderTest, NoCollisionNoLiquidRejectsSolidBlock)
{
    ASSERT_NE(m_world->chunkManager()->requestFullChunkSync(0, 0), nullptr);
    const i32 topY = m_world->getHeight(0, 0);
    EXPECT_TRUE(PlayerSpawnFinder::noCollisionNoLiquid(*m_world, BlockPos(0, topY, 0)));
    EXPECT_FALSE(PlayerSpawnFinder::noCollisionNoLiquid(*m_world, BlockPos(0, topY - 1, 0)));
}

TEST_F(PlayerSpawnFinderTest, FindSpawnReturnsValidPositionNearSuggestion)
{
    const BlockPos suggestion(0, 70, 0);
    const auto position = PlayerSpawnFinder::findSpawn(*m_world, suggestion);
    ASSERT_TRUE(position.has_value());

    const BlockPos landed(static_cast<i32>(std::floor(position->x)),
        static_cast<i32>(std::floor(position->y)),
        static_cast<i32>(std::floor(position->z)));
    EXPECT_TRUE(PlayerSpawnFinder::noCollisionNoLiquid(*m_world, landed));

    // spawnRadius 默认 10，候选应落在以建议位置为中心的 21x21 方形范围内。
    EXPECT_LE(std::abs(landed.x - suggestion.x), 10);
    EXPECT_LE(std::abs(landed.z - suggestion.z), 10);
}

TEST_F(PlayerSpawnFinderTest, FindSpawnIsDeterministicForSameSuggestion)
{
    const auto first = PlayerSpawnFinder::findSpawn(*m_world, BlockPos(0, 70, 0));
    const auto second = PlayerSpawnFinder::findSpawn(*m_world, BlockPos(0, 70, 0));
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_FLOAT_EQ(first->x, second->x);
    EXPECT_FLOAT_EQ(first->y, second->y);
    EXPECT_FLOAT_EQ(first->z, second->z);
}

} // namespace