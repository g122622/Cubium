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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/feature/RandomSelectorFeature.hpp"
#include "common/world/gen/feature/SimpleBlockFeature.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::gen::feature;

namespace {

/// 最小 IChunkGenerator 存根（参考 MonsterRoomFeatureTest）
class StubGenerator : public IChunkGenerator {
public:
    void generateStructureStarts(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateStructureReferences(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateBiomes(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateNoise(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void placeFeatures(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    i32 spawnInitialMobs(
        WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/, std::vector<SpawnedEntityData>& /*outEntities*/) override
    {
        return 0;
    }
    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override { return 0; }
    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override { return 0; }
    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] const DimensionSettings& settings() const override { return DimensionSettings::overworld(); }
    [[nodiscard]] i32 seaLevel() const override { return 63; }
};

/// 注册一个 simple_block 子特征（放置指定方块），返回其 ResourceLocation
ResourceLocation registerSimpleBlock(const char* id, const BlockState* state)
{
    auto config = std::make_unique<cave::SimpleBlockConfig>(state);
    auto feature = std::make_unique<cave::ConfiguredSimpleBlockFeature>(std::move(config), id);
    ResourceLocation rl("minecraft", id);
    ConfiguredFeatureRegistry::instance().registerFeature(std::move(feature), rl);
    return rl;
}

} // namespace

class RandomSelectorFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 3x3 区块区域（中心 0,0），origin 在中心
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                chunk->setPersistedStatus(ChunkStatuses::FEATURES);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }
        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));

        // 用石头预填充放置点周围，初始化区块 section（未初始化的 section 读写返回 null）
        fillSolid(-2, 62, -2, 2, 66, 2);

        m_stoneId = registerSimpleBlock("rsel_test_stone", &VanillaBlocks::STONE->defaultState());
        m_dirtId = registerSimpleBlock("rsel_test_dirt", &VanillaBlocks::DIRT->defaultState());
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
        ConfiguredFeatureRegistry::instance().clear();
    }

    /// 用石头填充长方体（初始化 section）
    void fillSolid(i32 x0, i32 y0, i32 z0, i32 x1, i32 y1, i32 z1)
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        for (i32 x = x0; x <= x1; ++x) {
            for (i32 y = y0; y <= y1; ++y) {
                for (i32 z = z0; z <= z1; ++z) {
                    m_region->setBlockState(x, y, z, stone);
                }
            }
        }
    }

    /// 把 origin 设为空气后放置，返回放置后方块是否为 stone
    bool placedStoneAfter(i32 seed)
    {
        const BlockPos pos(0, 64, 0);
        m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::AIR->defaultState());
        math::Random random(seed);
        RandomSelectorFeature::place(*m_region, *m_ownedChunks[4], m_stub, random, pos, m_config);
        const BlockState* state = m_region->getBlockState(pos.x, pos.y, pos.z);
        return state != nullptr && state->is(VanillaBlocks::STONE);
    }

    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
    ResourceLocation m_stoneId;
    ResourceLocation m_dirtId;
    RandomSelectorFeatureConfig m_config;
};

// ============================================================================
// 算法语义测试
// ============================================================================

TEST_F(RandomSelectorFeatureTest, ChanceOneAlwaysTriggersFeature)
{
    // chance=1.0：nextFloat() ∈ [0,1) 恒 < 1.0，必命中 features[0]
    m_config.features.push_back({m_stoneId, 1.0f});
    m_config.defaultFeatureId = m_dirtId;

    for (i32 seed = 0; seed < 50; ++seed) {
        EXPECT_TRUE(placedStoneAfter(seed)) << "chance=1.0 should always place stone (seed=" << seed << ")";
    }
}

TEST_F(RandomSelectorFeatureTest, ChanceZeroNeverTriggersFallsToDefault)
{
    // chance=0.0：nextFloat() ∈ [0,1) 恒 >= 0，必不命中，走 default
    m_config.features.push_back({m_stoneId, 0.0f});
    m_config.defaultFeatureId = m_dirtId;

    for (i32 seed = 0; seed < 50; ++seed) {
        EXPECT_FALSE(placedStoneAfter(seed)) << "chance=0.0 should always fall to default dirt (seed=" << seed << ")";
    }
}

TEST_F(RandomSelectorFeatureTest, EmptyFeaturesAlwaysFallsToDefault)
{
    // features 为空：直接走 default
    m_config.defaultFeatureId = m_dirtId;

    for (i32 seed = 0; seed < 20; ++seed) {
        EXPECT_FALSE(placedStoneAfter(seed));
    }
}

TEST_F(RandomSelectorFeatureTest, SequentialProbabilityDistribution)
{
    // features=[{stone, 0.5}]，default=dirt。
    // MC 语义：50% 命中 stone，50% 走 default dirt。大样本统计应近似 50/50。
    m_config.features.push_back({m_stoneId, 0.5f});
    m_config.defaultFeatureId = m_dirtId;

    i32 stoneCount = 0;
    const i32 trials = 4000;
    for (i32 seed = 0; seed < trials; ++seed) {
        if (placedStoneAfter(seed)) {
            ++stoneCount;
        }
    }
    // 允许 ±5% 误差（4000 样本下 3σ ≈ 2.4%）
    const f32 ratio = static_cast<f32>(stoneCount) / static_cast<f32>(trials);
    EXPECT_NEAR(ratio, 0.5f, 0.05f) << "stone ratio should be ~0.5, got " << ratio;
}

TEST_F(RandomSelectorFeatureTest, SequentialOrderMatters)
{
    // features=[{stone,0.5}, {dirt,1.0}]，default=stone。
    // MC 语义：先检查 stone(0.5)，命中即返回 stone；未命中(50%)再检查 dirt(1.0)，必命中。
    // 故期望：stone ≈ 50%，dirt ≈ 50%，default(stone) ≈ 0%。
    m_config.features.push_back({m_stoneId, 0.5f});
    m_config.features.push_back({m_dirtId, 1.0f});
    m_config.defaultFeatureId = m_stoneId;

    i32 stoneCount = 0;
    i32 dirtCount = 0;
    const i32 trials = 4000;
    for (i32 seed = 0; seed < trials; ++seed) {
        const BlockPos pos(0, 64, 0);
        m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::AIR->defaultState());
        math::Random random(seed);
        RandomSelectorFeature::place(*m_region, *m_ownedChunks[4], m_stub, random, pos, m_config);
        const BlockState* state = m_region->getBlockState(pos.x, pos.y, pos.z);
        if (state != nullptr && state->is(VanillaBlocks::STONE)) {
            ++stoneCount;
        } else {
            ++dirtCount;
        }
    }
    const f32 stoneRatio = static_cast<f32>(stoneCount) / static_cast<f32>(trials);
    const f32 dirtRatio = static_cast<f32>(dirtCount) / static_cast<f32>(trials);
    // stone ≈ 50%（首项命中），dirt ≈ 50%（第二项 1.0 兜底），default 永不触发
    EXPECT_NEAR(stoneRatio, 0.5f, 0.05f);
    EXPECT_NEAR(dirtRatio, 0.5f, 0.05f);
}

TEST_F(RandomSelectorFeatureTest, UnregisteredFeatureIdReturnsFalse)
{
    // 子特征 id 未注册：命中时应返回 false（容错，不崩溃）
    m_config.features.push_back({ResourceLocation("minecraft", "rsel_test_nonexistent"), 1.0f});
    m_config.defaultFeatureId = ResourceLocation("minecraft", "rsel_test_nonexistent_default");

    const BlockPos pos(0, 64, 0);
    m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::AIR->defaultState());
    math::Random random(42);
    const bool result = RandomSelectorFeature::place(*m_region, *m_ownedChunks[4], m_stub, random, pos, m_config);
    EXPECT_FALSE(result);
}
