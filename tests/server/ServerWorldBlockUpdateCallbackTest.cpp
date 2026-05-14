#include <gtest/gtest.h>

#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"

#include <utility>
#include <vector>

using namespace mc;
using namespace mc::server;

namespace {

class ServerWorldBlockUpdateCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 12345;
        // 注意：isDebugWorld 字段已移除，改用 isDebugWorld() 方法通过检测区块生成器类型判断

        m_world = std::make_unique<ServerWorld>(config);
        auto result = m_world->initialize();
        ASSERT_TRUE(result.success());
    }

    void TearDown() override
    {
        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
    }

    std::unique_ptr<ServerWorld> m_world;
};

} // namespace

TEST_F(ServerWorldBlockUpdateCallbackTest, SetBlockInvokesBlockChangedCallback)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockState(0, 64, 0, stoneState);
    m_world->setBlockState(0, 64, 0, nullptr);

    ASSERT_EQ(blockUpdates.size(), 2u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, stoneState->stateId());
    EXPECT_EQ(blockUpdates[1].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[1].second, 0u);
}

TEST_F(ServerWorldBlockUpdateCallbackTest, BreakingIceAboveSolidTurnsIntoWater)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::ICE->defaultState());

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    ASSERT_TRUE(m_world->setBlockState(0, 64, 0, nullptr));

    const BlockState* finalState = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());

    ASSERT_EQ(blockUpdates.size(), 1u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(ServerWorldBlockUpdateCallbackTest, BreakingIceWithoutSupportTurnsIntoAir)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setBlockState(0, 63, 0, nullptr);
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::ICE->defaultState());

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    ASSERT_TRUE(m_world->setBlockState(0, 64, 0, nullptr));

    const BlockState* finalState = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());

    ASSERT_EQ(blockUpdates.size(), 1u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, 0u);
}
