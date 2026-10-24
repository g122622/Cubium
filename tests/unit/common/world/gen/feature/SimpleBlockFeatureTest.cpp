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
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/ConfiguredFeature.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/feature/SimpleBlockFeature.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <set>
#include <vector>

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::world::gen::feature;

namespace {

/// 最小 IChunkGenerator 存根
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

    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        return 0;
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override
    {
        return 0;
    }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override
    {
        return 64;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] const DimensionSettings& settings() const override
    {
        return DimensionSettings::overworld();
    }

    [[nodiscard]] i32 seaLevel() const override
    {
        return 63;
    }
};

} // namespace

class SimpleBlockFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        PlacementRegistry::instance().initialize();
        world::gen::feature::initializeBuiltinFeatureTypes();

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
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
    }

    ChunkPrimer& centerChunk()
    {
        return *m_ownedChunks[4];
    }

    /// 构造 simple_block 配置化特征：to_place 为 simple_state_provider 单一方块。
    std::unique_ptr<ConfiguredFeatureBase> makeSimpleBlock(const std::string& blockName)
    {
        nlohmann::json jsonObj;
        jsonObj["type"] = "minecraft:simple_block";
        jsonObj["config"]["to_place"]["type"] = "minecraft:simple_state_provider";
        jsonObj["config"]["to_place"]["state"]["Name"] = "minecraft:" + blockName;
        auto result = FeatureTypeRegistry::instance().create("simple_block", jsonObj["config"]);
        EXPECT_TRUE(result.success()) << "simple_block simple_state_provider should parse";
        return result.value();
    }

    /// 构造 simple_block 配置化特征：to_place 为 weighted_state_provider。
    /// entries 为 {blockName, weight} 列表。
    std::unique_ptr<ConfiguredFeatureBase> makeWeightedSimpleBlock(
        const std::vector<std::pair<std::string, i32>>& entries)
    {
        nlohmann::json entriesJson = nlohmann::json::array();
        for (const auto& entry : entries) {
            nlohmann::json e;
            e["data"]["Name"] = "minecraft:" + entry.first;
            e["weight"] = entry.second;
            entriesJson.push_back(std::move(e));
        }
        nlohmann::json jsonObj;
        jsonObj["type"] = "minecraft:simple_block";
        jsonObj["config"]["to_place"]["type"] = "minecraft:weighted_state_provider";
        jsonObj["config"]["to_place"]["entries"] = std::move(entriesJson);
        auto result = FeatureTypeRegistry::instance().create("simple_block", jsonObj["config"]);
        EXPECT_TRUE(result.success()) << "simple_block weighted_state_provider should parse";
        return result.value();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// 算法语义测试
// ============================================================================

TEST_F(SimpleBlockFeatureTest, SimpleStateProviderPlacesSingleBlock)
{
    // simple_state_provider：在空气处放置石头（nullptr=air，可替换）。
    auto feature = makeSimpleBlock("stone");
    ASSERT_NE(feature, nullptr);

    const BlockPos pos(8, 64, 8);
    math::Random random(1);
    const bool result = feature->place(*m_region, centerChunk(), m_stub, random, pos);
    EXPECT_TRUE(result);

    const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::STONE));
}

TEST_F(SimpleBlockFeatureTest, WeightedStateProviderParsesAndSamplesAllEntries)
{
    // weighted_state_provider：权重分布为 stone:1, dirt:1。
    // 重复放置多次（不同位置），两种方块都应出现（固定 seed 下概率上必然）。
    auto feature = makeWeightedSimpleBlock({{"stone", 1}, {"dirt", 1}});
    ASSERT_NE(feature, nullptr);

    math::Random random(42);
    std::set<const Block*> seen;
    // 在 chunk(0,0) 内的 5x8 网格放 40 个（全部落在 3x3 访问窗口内），weight 1:1 必然两种都采到
    for (i32 i = 0; i < 40; ++i) {
        const BlockPos pos((i % 5) * 3, 64, (i / 5) * 2);
        feature->place(*m_region, centerChunk(), m_stub, random, pos);
        const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
        if (s != nullptr && !s->isAir()) {
            seen.insert(&s->getBlock());
        }
    }
    EXPECT_EQ(seen.size(), 2u) << "weighted 1:1 应在 40 次放置内采到 stone 与 dirt 两种";
}

TEST_F(SimpleBlockFeatureTest, WeightedStateProviderRespectsWeights)
{
    // 全权重给 stone（weight 10），dirt 权重 0 → 永远只放 stone。
    auto feature = makeWeightedSimpleBlock({{"stone", 10}, {"dirt", 0}});
    ASSERT_NE(feature, nullptr);

    math::Random random(7);
    bool anyDirt = false;
    for (i32 i = 0; i < 20; ++i) {
        const BlockPos pos(i, 64, 8);
        feature->place(*m_region, centerChunk(), m_stub, random, pos);
        const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
        if (s != nullptr && s->is(VanillaBlocks::DIRT)) {
            anyDirt = true;
        }
    }
    EXPECT_FALSE(anyDirt) << "dirt 权重 0 时不应被采到";
}

TEST_F(SimpleBlockFeatureTest, WeightedProviderBlocksPlacementWhenNonReplaceable)
{
    // 先把目标位置填成石头（不可替换），weighted simple_block 放置应失败。
    auto feature = makeWeightedSimpleBlock({{"dirt", 1}});
    ASSERT_NE(feature, nullptr);

    const BlockPos pos(8, 64, 8);
    m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    math::Random random(1);
    const bool result = feature->place(*m_region, centerChunk(), m_stub, random, pos);
    EXPECT_FALSE(result) << "石头不可替换，放置应失败";

    const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::STONE)) << "原石头不应被覆盖";
}

TEST_F(SimpleBlockFeatureTest, ScheduleTickRecordsPendingBlockTickOnPlacedBlock)
{
    // 对齐 vanilla SimpleBlockFeature：scheduleTick 为 true 时以延迟 1 调度放置后方块的刻。
    // 配置化特征默认不调度刻，此处直接构造配置并调用底层 place。
    const cave::SimpleBlockConfig config(&VanillaBlocks::SAND->defaultState(), /*tick=*/true);
    const BlockPos pos(5, 64, 5);
    math::Random random(3);
    ASSERT_TRUE(cave::SimpleBlockFeature::place(*m_region, random, pos, config));

    const auto& ticks = centerChunk().pendingBlockTicks();
    ASSERT_EQ(ticks.size(), 1u);
    EXPECT_EQ(ticks[0].pos, pos);
    EXPECT_EQ(ticks[0].block, VanillaBlocks::SAND);
    EXPECT_EQ(ticks[0].delay, 1);
    EXPECT_EQ(ticks[0].priority, world::tick::TickPriority::Normal);

    // 写入区块数据后，待执行刻转移到 ChunkData（区块发布时由主线程转交 TickManager）。
    auto data = centerChunk().toChunkData();
    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->pendingBlockTicks().size(), 1u);
    EXPECT_EQ(data->pendingBlockTicks()[0].block, VanillaBlocks::SAND);
    EXPECT_TRUE(centerChunk().pendingBlockTicks().empty());
    data->clearPendingBlockTicks();
    EXPECT_TRUE(data->pendingBlockTicks().empty());
}

TEST_F(SimpleBlockFeatureTest, WithoutScheduleTickNoPendingBlockTick)
{
    // 默认 scheduleTick=false：放置后不产生任何待执行刻。
    const cave::SimpleBlockConfig config(&VanillaBlocks::SAND->defaultState());
    const BlockPos pos(6, 64, 6);
    math::Random random(4);
    ASSERT_TRUE(cave::SimpleBlockFeature::place(*m_region, random, pos, config));
    EXPECT_TRUE(centerChunk().pendingBlockTicks().empty());
}

TEST_F(SimpleBlockFeatureTest, PlacesBothHalvesOfTallFlower)
{
    using namespace mc;
    world::gen::feature::cave::SimpleBlockConfig config(&VanillaBlocks::ROSE_BUSH->defaultState(), false);
    math::JavaLegacyRandom random(42);
    const BlockPos pos(8, 1, 8);
    m_region->setBlockState(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    ASSERT_TRUE(world::gen::feature::cave::SimpleBlockFeature::place(*m_region, random, pos, config));
    const BlockState* lower = m_region->getBlockState(pos);
    const BlockState* upper = m_region->getBlockState(pos.up());
    ASSERT_NE(lower, nullptr);
    ASSERT_NE(upper, nullptr);
    EXPECT_TRUE(lower->is(VanillaBlocks::ROSE_BUSH));
    EXPECT_TRUE(upper->is(VanillaBlocks::ROSE_BUSH));
    EXPECT_EQ(lower->get(blocks::DoublePlantBlock::halfProperty()), BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_EQ(upper->get(blocks::DoublePlantBlock::halfProperty()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(SimpleBlockFeatureTest, RejectsTallFlowerWithoutChangingLowerBlockWhenUpperSpaceIsOccupied)
{
    using namespace mc;
    world::gen::feature::cave::SimpleBlockConfig config(&VanillaBlocks::ROSE_BUSH->defaultState(), false);
    const BlockPos pos(8, 1, 8);
    m_region->setBlockState(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    // 即使上方植物可替换，也不满足双层植物要求的空格。
    for (const BlockState* obstacle :
        {&VanillaBlocks::STONE->defaultState(), &VanillaBlocks::SHORT_GRASS->defaultState()}) {
        m_region->setBlockState(pos.up(), obstacle);
        math::JavaLegacyRandom random(42);
        EXPECT_FALSE(world::gen::feature::cave::SimpleBlockFeature::place(*m_region, random, pos, config));
        const BlockState* lower = m_region->getBlockState(pos);
        EXPECT_TRUE(lower == nullptr || lower->isAir());
        EXPECT_EQ(m_region->getBlockState(pos.up()), obstacle);
    }
}

TEST_F(SimpleBlockFeatureTest, RejectsTallFlowerAtTopBoundaryWithoutWritingLowerHalf)
{
    cave::SimpleBlockConfig config(&VanillaBlocks::ROSE_BUSH->defaultState(), false);
    math::JavaLegacyRandom random(42);
    const BlockPos pos(8, world::MAX_BUILD_HEIGHT - 1, 8);
    m_region->setBlockState(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_FALSE(cave::SimpleBlockFeature::place(*m_region, random, pos, config));
    const BlockState* state = m_region->getBlockState(pos);
    EXPECT_TRUE(state == nullptr || state->isAir());
}
