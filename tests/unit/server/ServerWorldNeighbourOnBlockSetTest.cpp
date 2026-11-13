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

#include "common/TempDirHelper.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/chunk/ServerChunkManager.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"

#include <filesystem>
#include <memory>
#include <utility>

using namespace mc;
using namespace mc::server;

namespace {

/**
 * @brief `/fill` 与 `/clone` 的 strict 模式所需的"抑制副作用 + 延后补发邻居更新"通路测试
 *
 * strict 的语义（原版 `FillCommand` 传 `2 | 816`）：写入期间不触发邻居副作用，
 * 全部写完后再逐位置调用 `updateNeighboursOnBlockSet` 统一补发。
 */
class ServerWorldNeighbourOnBlockSetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_testDir = mc::test::makeUniqueTestDir("mc_neighbour_on_block_set_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 12345;

        m_world = std::make_unique<ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_world->setChunkManager(std::move(chunkManager));

        auto result = m_world->initialize();
        ASSERT_TRUE(result.success());
    }

    void TearDown() override
    {
        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
        m_storage.close();
        mc::test::removeTestDir(m_testDir);
    }

    std::unique_ptr<ServerWorld> m_world;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

} // namespace

TEST_F(ServerWorldNeighbourOnBlockSetTest, SkipAllSideEffectsSuppressesImmediateNeighbourUpdate)
{
    // 在 (0,64,0) 放一块会被邻居更新影响的方块（冰：上方方块消失会融化成水），
    // 再用 strict 标志在 (0,64,1) 写入石头。写入期间不通知邻居 → 冰保持原样。
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::ICE->defaultState());
    m_world->setBlockState(0, 65, 0, &VanillaBlocks::STONE->defaultState());

    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const i32 strictFlags =
        world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_SKIP_ALL_SIDEEFFECTS;

    // 先在冰正上方放个空气：strict 写入不触发邻居更新。
    ASSERT_TRUE(m_world->setBlockState(0, 65, 0, nullptr, strictFlags));
    // 因为抑制了形状/邻居更新，冰不会立刻按"失去上方支撑"处理。
    EXPECT_TRUE(m_world->getBlockState(0, 65, 0)->isAir());
    EXPECT_FALSE(m_world->getBlockState(0, 64, 0)->isAir())
        << "ice should still be present while side effects are suppressed";

    // 同一位置用普通标志重写会恢复副作用传播，作为对照。
    ASSERT_TRUE(m_world->setBlockState(0, 65, 0, stone));
    EXPECT_FALSE(m_world->getBlockState(0, 65, 0)->isAir());
}

TEST_F(ServerWorldNeighbourOnBlockSetTest, DeferredNeighbourUpdatePropagatesAfterBatchWrite)
{
    // 同一批写入完成后调用 updateNeighboursOnBlockSet，邻居才看到变化。
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    m_world->setBlockState(1, 64, 0, &VanillaBlocks::STONE->defaultState());

    const BlockState* oldState = m_world->getBlockState(1, 64, 0);
    ASSERT_NE(oldState, nullptr);

    ASSERT_TRUE(m_world->setBlockState(1, 64, 0, nullptr));

    // 补发不应崩溃，且写入结果保持（补发只通知邻居，不改自身状态）。
    m_world->updateNeighboursOnBlockSet(BlockPos(1, 64, 0), *oldState);

    EXPECT_TRUE(m_world->getBlockState(1, 64, 0)->isAir());
    EXPECT_FALSE(m_world->getBlockState(0, 64, 0)->isAir());
}

TEST_F(ServerWorldNeighbourOnBlockSetTest, DeferredUpdateWithSameBlockKeepsState)
{
    // 同一方块只换状态（不涉及移除）时，补发邻居更新不应改变自身状态。
    const BlockState* slabDefault = &VanillaBlocks::OAK_SLAB->defaultState();
    ASSERT_TRUE(m_world->setBlockState(0, 64, 0, slabDefault));

    const BlockState* before = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(before, nullptr);
    m_world->updateNeighboursOnBlockSet(BlockPos(0, 64, 0), *before);

    EXPECT_FALSE(m_world->getBlockState(0, 64, 0)->isAir());
}
