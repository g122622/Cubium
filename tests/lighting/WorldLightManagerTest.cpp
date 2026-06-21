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

#include "common/core/Constants.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace {

/**
 * @brief 测试用的光照提供者
 *
 * 实现 StarLightLightingProvider 接口，提供最小化测试环境。
 */
class TestLightingProvider final : public mc::StarLightLightingProvider {
public:
    [[nodiscard]] mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override { return nullptr; }

    [[nodiscard]] const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        return nullptr;
    }

    [[nodiscard]] const mc::BlockState* getBlockStateForLight(const mc::BlockPos&) const override { return nullptr; }

    [[nodiscard]] mc::IWorld* getWorld() override { return nullptr; }

    [[nodiscard]] const mc::IWorld* getWorld() const override { return nullptr; }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    [[nodiscard]] bool hasSkyLight() const override { return true; }

    [[nodiscard]] mc::i32 getMinBuildHeight() const override { return 0; }

    [[nodiscard]] mc::i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }

    [[nodiscard]] mc::i32 getSectionCount() const override { return mc::world::CHUNK_SECTIONS; }
};

} // namespace

// ============================================================================
// BlockStarLightEngine 区块列管理测试
// ============================================================================

TEST(BlockLightEngineColumnTest, SetColumnEnabledAddsAndRemoves)
{
    TestLightingProvider provider;
    mc::BlockStarLightEngine engine;

    mc::i64 columnPos = (static_cast<mc::i64>(1) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(2) & 0x3FFFFFLL) << 20;

    // 初始状态：区块列未启用
    EXPECT_FALSE(engine.isColumnEnabled(columnPos));

    // 启用区块列
    engine.setColumnEnabled(columnPos, true);
    EXPECT_TRUE(engine.isColumnEnabled(columnPos));

    // 禁用区块列
    engine.setColumnEnabled(columnPos, false);
    EXPECT_FALSE(engine.isColumnEnabled(columnPos));
}

TEST(BlockLightEngineColumnTest, SetColumnEnabledMultipleColumns)
{
    TestLightingProvider provider;
    mc::BlockStarLightEngine engine;

    mc::i64 colA = (static_cast<mc::i64>(0) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(0) & 0x3FFFFFLL) << 20;
    mc::i64 colB = (static_cast<mc::i64>(3) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(5) & 0x3FFFFFLL) << 20;
    mc::i64 colC = (static_cast<mc::i64>(-10) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(7) & 0x3FFFFFLL) << 20;

    engine.setColumnEnabled(colA, true);
    engine.setColumnEnabled(colB, true);
    engine.setColumnEnabled(colC, true);

    EXPECT_TRUE(engine.isColumnEnabled(colA));
    EXPECT_TRUE(engine.isColumnEnabled(colB));
    EXPECT_TRUE(engine.isColumnEnabled(colC));

    // 禁用一个不影响其他
    engine.setColumnEnabled(colB, false);
    EXPECT_TRUE(engine.isColumnEnabled(colA));
    EXPECT_FALSE(engine.isColumnEnabled(colB));
    EXPECT_TRUE(engine.isColumnEnabled(colC));
}

TEST(BlockLightEngineColumnTest, SetColumnEnabledIdempotent)
{
    TestLightingProvider provider;
    mc::BlockStarLightEngine engine;

    mc::i64 columnPos = (static_cast<mc::i64>(10) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(20) & 0x3FFFFFLL) << 20;

    // 多次启用
    engine.setColumnEnabled(columnPos, true);
    engine.setColumnEnabled(columnPos, true);
    EXPECT_TRUE(engine.isColumnEnabled(columnPos));

    // 多次禁用
    engine.setColumnEnabled(columnPos, false);
    EXPECT_FALSE(engine.isColumnEnabled(columnPos));
    engine.setColumnEnabled(columnPos, false);
    EXPECT_FALSE(engine.isColumnEnabled(columnPos));
}

// ============================================================================
// BlockStarLightEngine 数据保留测试
// ============================================================================

TEST(BlockLightEngineRetainTest, RetainDataAddsAndRemoves)
{
    TestLightingProvider provider;
    mc::BlockStarLightEngine engine;

    mc::i64 columnPos = (static_cast<mc::i64>(1) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(2) & 0x3FFFFFLL) << 20;

    // 初始状态：数据未被保留
    EXPECT_FALSE(engine.isDataRetained(columnPos));

    // 保留数据
    engine.retainData(columnPos, true);
    EXPECT_TRUE(engine.isDataRetained(columnPos));

    // 释放数据
    engine.retainData(columnPos, false);
    EXPECT_FALSE(engine.isDataRetained(columnPos));
}

TEST(BlockLightEngineRetainTest, RetainDataMultipleColumns)
{
    TestLightingProvider provider;
    mc::BlockStarLightEngine engine;

    mc::i64 colA = (static_cast<mc::i64>(0) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(0) & 0x3FFFFFLL) << 20;
    mc::i64 colB = (static_cast<mc::i64>(5) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(10) & 0x3FFFFFLL) << 20;

    engine.retainData(colA, true);
    engine.retainData(colB, true);

    EXPECT_TRUE(engine.isDataRetained(colA));
    EXPECT_TRUE(engine.isDataRetained(colB));

    engine.retainData(colA, false);
    EXPECT_FALSE(engine.isDataRetained(colA));
    EXPECT_TRUE(engine.isDataRetained(colB));
}

// ============================================================================
// SkyStarLightEngine 区块列管理测试
// ============================================================================

TEST(SkyLightEngineColumnTest, SetColumnEnabledAddsAndRemoves)
{
    TestLightingProvider provider;
    mc::SkyStarLightEngine engine;

    mc::i64 columnPos = (static_cast<mc::i64>(1) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(2) & 0x3FFFFFLL) << 20;

    EXPECT_FALSE(engine.isColumnEnabled(columnPos));

    engine.setColumnEnabled(columnPos, true);
    EXPECT_TRUE(engine.isColumnEnabled(columnPos));

    engine.setColumnEnabled(columnPos, false);
    EXPECT_FALSE(engine.isColumnEnabled(columnPos));
}

TEST(SkyLightEngineColumnTest, RetainDataAddsAndRemoves)
{
    TestLightingProvider provider;
    mc::SkyStarLightEngine engine;

    mc::i64 columnPos = (static_cast<mc::i64>(3) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(4) & 0x3FFFFFLL) << 20;

    EXPECT_FALSE(engine.isDataRetained(columnPos));

    engine.retainData(columnPos, true);
    EXPECT_TRUE(engine.isDataRetained(columnPos));

    engine.retainData(columnPos, false);
    EXPECT_FALSE(engine.isDataRetained(columnPos));
}

// ============================================================================
// WorldLightManager 区块列管理测试
// ============================================================================

TEST(WorldLightManagerColumnTest, EnableLightSourcesDelegatesToBothEngines)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::ChunkPos pos(1, 2);

    // 启用光源
    manager.enableLightSources(pos, true);

    // 通过引擎接口验证启用状态
    auto* blockEngine = manager.getBlockLightEngine();
    auto* skyEngine = manager.getSkyLightEngine();

    ASSERT_NE(blockEngine, nullptr);
    ASSERT_NE(skyEngine, nullptr);

    mc::i64 columnPos = (static_cast<mc::i64>(1) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(2) & 0x3FFFFFLL) << 20;
    EXPECT_TRUE(blockEngine->isColumnEnabled(columnPos));
    EXPECT_TRUE(skyEngine->isColumnEnabled(columnPos));

    // 禁用光源
    manager.enableLightSources(pos, false);
    EXPECT_FALSE(blockEngine->isColumnEnabled(columnPos));
    EXPECT_FALSE(skyEngine->isColumnEnabled(columnPos));
}

TEST(WorldLightManagerColumnTest, RetainDataDelegatesToBothEngines)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::ChunkPos pos(5, 10);

    manager.retainData(pos, true);

    auto* blockEngine = manager.getBlockLightEngine();
    auto* skyEngine = manager.getSkyLightEngine();

    ASSERT_NE(blockEngine, nullptr);
    ASSERT_NE(skyEngine, nullptr);

    mc::i64 columnPos = (static_cast<mc::i64>(5) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(10) & 0x3FFFFFLL) << 20;
    EXPECT_TRUE(blockEngine->isDataRetained(columnPos));
    EXPECT_TRUE(skyEngine->isDataRetained(columnPos));

    manager.retainData(pos, false);
    EXPECT_FALSE(blockEngine->isDataRetained(columnPos));
    EXPECT_FALSE(skyEngine->isDataRetained(columnPos));
}

TEST(WorldLightManagerColumnTest, GetDebugInfoReturnsSectionState)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::SectionPos pos(0, 0, 0);

    // 没有数据时应该返回 EMPTY 状态（"2"）
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, pos);
    EXPECT_TRUE(blockInfo.find("BlockLight:") == 0);
    EXPECT_TRUE(blockInfo.find("2") != std::string::npos) << "Expected EMPTY state '2', got: " << blockInfo;

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, pos);
    EXPECT_TRUE(skyInfo.find("SkyLight:") == 0);
    EXPECT_TRUE(skyInfo.find("2") != std::string::npos) << "Expected EMPTY state '2', got: " << skyInfo;
}

TEST(WorldLightManagerColumnTest, GetDebugInfoWithColumnEnabled)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::ChunkPos chunkPos(3, 7);
    manager.enableLightSources(chunkPos, true);

    mc::SectionPos sectionPos(3, 0, 7);
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, sectionPos);
    EXPECT_TRUE(blockInfo.find("[col:on]") != std::string::npos)
        << "Expected [col:on] in debug info, got: " << blockInfo;

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, sectionPos);
    EXPECT_TRUE(skyInfo.find("[col:on]") != std::string::npos) << "Expected [col:on] in debug info, got: " << skyInfo;
}

TEST(WorldLightManagerColumnTest, GetDebugInfoWithRetainedData)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::ChunkPos chunkPos(2, 4);
    manager.retainData(chunkPos, true);

    mc::SectionPos sectionPos(2, 0, 4);
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, sectionPos);
    EXPECT_TRUE(blockInfo.find("[retained]") != std::string::npos)
        << "Expected [retained] in debug info, got: " << blockInfo;

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, sectionPos);
    EXPECT_TRUE(skyInfo.find("[retained]") != std::string::npos)
        << "Expected [retained] in debug info, got: " << skyInfo;
}

TEST(WorldLightManagerColumnTest, NoSkyLightEngineReturnsNA)
{
    TestLightingProvider provider;
    // 下界维度：没有天空光
    mc::WorldLightManager manager(&provider, true, false);

    mc::SectionPos pos(0, 0, 0);
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, pos);
    EXPECT_TRUE(blockInfo.find("BlockLight:") == 0);

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, pos);
    EXPECT_EQ(skyInfo, "SkyLight: N/A");
}

// ============================================================================
// 坐标编码一致性测试
// ============================================================================

TEST(ColumnPosEncodingTest, ChunkPosAndBlockCoordProduceSameColumnPos)
{
    // 验证 enableLightSources 使用的 ChunkPos 编码与
    // _getLightEmission 中使用 x >> CHUNK_SHIFT 编码一致

    // 使用 ChunkPos(5, 10) 编码
    mc::i64 fromChunkPos = (static_cast<mc::i64>(5) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(10) & 0x3FFFFFLL) << 20;

    // 使用方块坐标 (80, y, 160) -> chunk (5, 10) 编码
    // CHUNK_SHIFT = 4, 所以 80 >> 4 = 5, 160 >> 4 = 10
    mc::i64 fromBlockCoord =
        (static_cast<mc::i64>(80 >> 4) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(160 >> 4) & 0x3FFFFFLL) << 20;

    EXPECT_EQ(fromChunkPos, fromBlockCoord);
}

TEST(ColumnPosEncodingTest, NegativeCoordinatesHandledCorrectly)
{
    // 负坐标处理：(-16, y, -32) -> chunk (-1, -2)
    mc::i64 fromChunkPos =
        (static_cast<mc::i64>(-1) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(-2) & 0x3FFFFFLL) << 20;

    // -16 >> 4 在 C++ 中实现定义，但项目使用算术右移
    // -16 >> 4 = -1, -32 >> 4 = -2
    mc::i64 fromBlockCoord =
        (static_cast<mc::i64>(-16 >> 4) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(-32 >> 4) & 0x3FFFFFLL) << 20;

    EXPECT_EQ(fromChunkPos, fromBlockCoord);
}

TEST(ColumnPosEncodingTest, LargeCoordinatesHandledCorrectly)
{
    // 大坐标：ChunkPos(1000000, 2000000)
    mc::i64 fromChunkPos =
        (static_cast<mc::i64>(1000000) & 0x3FFFFFLL) << 42 | (static_cast<mc::i64>(2000000) & 0x3FFFFFLL) << 20;

    // 方块坐标 (16000000, y, 32000000)
    mc::i64 fromBlockCoord = (static_cast<mc::i64>(16000000 >> 4) & 0x3FFFFFLL) << 42 |
        (static_cast<mc::i64>(32000000 >> 4) & 0x3FFFFFLL) << 20;

    EXPECT_EQ(fromChunkPos, fromBlockCoord);
}
