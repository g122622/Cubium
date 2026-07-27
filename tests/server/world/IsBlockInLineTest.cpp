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
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>
#include <filesystem>

using namespace mc;
using namespace mc::server;

// ============================================================================
// 测试夹具
// ============================================================================

class IsBlockInLineTest : public ::testing::Test {
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
        m_testDir = mc::test::makeUniqueTestDir("mc_isblockinline_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        VanillaBlocks::initialize();
        BlockTags::initialize();

        ServerWorldConfig config;
        config.viewDistance = 2;
        config.dimension = 0;
        config.seed = 54321;

        m_world = createTestWorld(config);
        m_world->setSharedStorage(&m_storage);
        auto result = m_world->initialize();
        ASSERT_TRUE(result.success()) << result.error().message();
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

    ServerWorld& world() { return *m_world; }

    /// 确保指定区块坐标处的区块已加载
    void ensureChunk(i32 chunkX, i32 chunkZ) { m_world->chunkManager()->getChunkSync(chunkX, chunkZ); }

    /// 在指定位置放置羊毛方块（属于 OCCLUDES_VIBRATION_SIGNALS 标签）
    void placeWool(i32 x, i32 y, i32 z)
    {
        const BlockState* woolState = &VanillaBlocks::WHITE_WOOL->defaultState();
        m_world->setBlockState(x, y, z, woolState);
    }

    /// 在指定位置放置石头方块（不属于 OCCLUDES_VIBRATION_SIGNALS 标签）
    void placeStone(i32 x, i32 y, i32 z)
    {
        const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
        m_world->setBlockState(x, y, z, stoneState);
    }

    std::unique_ptr<ServerWorld> m_world;
    std::filesystem::path m_testDir;
    world::storage::SingleLevelStorageManager m_storage;
};

// ============================================================================
// isBlockInLine 基础测试
//
// 注意：世界生成器会在 Y=64 附近（海平面）生成地形方块（石头、泥土等），
// 因此"无匹配方块"的测试使用 Y=300（远高于地形生成高度）来确保该位置是空气。
// "有匹配方块"的测试在 Y=300 高度显式放置方块，避免与生成地形冲突。
// ============================================================================

TEST_F(IsBlockInLineTest, SameStartAndEndPoint_NoMatchingBlock_ReturnsFalse)
{
    ensureChunk(0, 0);

    // Y=300 远高于地形生成高度，保证该位置是空气
    Vector3d pos(5.5, 300.5, 5.5);
    bool result = world().isBlockInLine(pos, pos, [](const BlockState&) { return true; });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, SameStartAndEndPoint_MatchingBlock_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 在起点位置放一个石头
    placeStone(5, 300, 5);

    Vector3d pos(5.5, 300.5, 5.5);
    bool result = world().isBlockInLine(
        pos, pos, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, AxisAlignedX_NoMatchingBlock_ReturnsFalse)
{
    ensureChunk(0, 0);

    // Y=300，沿 X 轴方向遍历，全部是空气
    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(15.5, 300.5, 0.5);
    bool result = world().isBlockInLine(from, to, [](const BlockState&) { return true; });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, AxisAlignedX_BlockInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 沿 X 轴方向遍历，在 (8, 300, 0) 放一个石头
    placeStone(8, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(15.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, AxisAlignedX_BlockNotOnPath_ReturnsFalse)
{
    ensureChunk(0, 0);

    // 沿 X 轴方向遍历（Y=300），石头在 Y=301，不在路径上
    placeStone(8, 301, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(15.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, AxisAlignedZ_BlockInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 沿 Z 轴方向遍历，在 (0, 300, 10) 放一个石头
    placeStone(0, 300, 10);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(0.5, 300.5, 15.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, AxisAlignedY_BlockInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 沿 Y 轴方向遍历，在 (0, 300, 0) 放一个石头
    placeStone(0, 300, 0);

    Vector3d from(0.5, 290.5, 0.5);
    Vector3d to(0.5, 310.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, DiagonalRay_BlockInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 对角线方向遍历，在 (5, 300, 5) 放一个石头
    placeStone(5, 300, 5);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 10.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, NegativeCoordinates_BlockInPath_ReturnsTrue)
{
    // 需要加载负坐标区块
    ensureChunk(-1, -1);

    // 在负坐标放置石头（Y=300，远离地形）
    placeStone(-5, 300, -5);

    Vector3d from(-10.5, 300.5, -10.5);
    Vector3d to(0.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, PredicateNeverMatches_ReturnsFalse)
{
    ensureChunk(0, 0);

    // 沿途放很多石头，但谓词只匹配羊毛
    for (int x = 0; x <= 10; ++x) {
        placeStone(x, 300, 0);
    }

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::WHITE_WOOL->blockId(); });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, PredicateMatchesWool_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 沿途放石头，中间放一个羊毛
    placeStone(3, 300, 0);
    placeWool(5, 300, 0);
    placeStone(7, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);

    // 搜索羊毛方块
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::WHITE_WOOL->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, BlockAfterEndPoint_NotFound)
{
    ensureChunk(0, 0);

    // 射线仅遍历起点到终点路径上的方块，终点之后（Y=300 无生成地形）的石头不在路径上
    placeStone(12, 300, 0); // 超出射线范围

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, NullPredicate_ReturnsFalse)
{
    ensureChunk(0, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);
    // 传入空谓词
    bool result = world().isBlockInLine(from, to, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, BlockAtStartPoint_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 起点方块匹配
    placeStone(0, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(5.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, BlockAtEndPoint_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 终点方块匹配
    placeStone(5, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(5.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, BlockTagPredicate_WoolInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 使用 BlockTags::OCCLUDES_VIBRATION_SIGNALS 作为谓词
    placeWool(5, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(state); });
    EXPECT_TRUE(result);
}

TEST_F(IsBlockInLineTest, BlockTagPredicate_StoneInPath_ReturnsFalse)
{
    ensureChunk(0, 0);

    // 石头不在 OCCLUDES_VIBRATION_SIGNALS 标签中
    placeStone(5, 300, 0);

    Vector3d from(0.5, 300.5, 0.5);
    Vector3d to(10.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(state); });
    EXPECT_FALSE(result);
}

TEST_F(IsBlockInLineTest, ReverseDirection_BlockInPath_ReturnsTrue)
{
    ensureChunk(0, 0);

    // 反方向遍历（从大到小）
    placeStone(5, 300, 0);

    Vector3d from(10.5, 300.5, 0.5);
    Vector3d to(0.5, 300.5, 0.5);
    bool result = world().isBlockInLine(
        from, to, [](const BlockState& state) { return state.blockId() == VanillaBlocks::STONE->blockId(); });
    EXPECT_TRUE(result);
}

// ============================================================================
// OCCLUDES_VIBRATION_SIGNALS 标签测试
// ============================================================================

class OccludesVibrationSignalsTagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(OccludesVibrationSignalsTagTest, ContainsWhiteWool)
{
    EXPECT_TRUE(BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(ResourceLocation("minecraft", "white_wool")));
}

TEST_F(OccludesVibrationSignalsTagTest, ContainsBlackWool)
{
    EXPECT_TRUE(BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(ResourceLocation("minecraft", "black_wool")));
}

TEST_F(OccludesVibrationSignalsTagTest, DoesNotContainCarpet)
{
    // 羊毛地毯不在此标签中（地毯仅属于 DAMPENS_VIBRATIONS，不属于 OCCLUDES_VIBRATION_SIGNALS）
    EXPECT_FALSE(BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(ResourceLocation("minecraft", "white_carpet")));
}

TEST_F(OccludesVibrationSignalsTagTest, DoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::OCCLUDES_VIBRATION_SIGNALS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(OccludesVibrationSignalsTagTest, TagIdIsCorrect)
{
    EXPECT_EQ(
        BlockTags::OCCLUDES_VIBRATION_SIGNALS().getId(), ResourceLocation("minecraft", "occludes_vibration_signals"));
}
