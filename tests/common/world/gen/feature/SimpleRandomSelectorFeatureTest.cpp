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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/world/gen/feature/FeatureTypeRegistry.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

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
    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override { return 0; }
    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override { return 0; }
    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] const DimensionSettings& settings() const override { return DimensionSettings::overworld(); }
    [[nodiscard]] i32 seaLevel() const override { return 63; }
};

} // namespace

class SimpleRandomSelectorFeatureTest : public ::testing::Test {
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

    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; }

    /// 构造一个内联 simple_block feature 对象（simple_state_provider 单一方块）。
    static nlohmann::json inlineSimpleBlock(const std::string& blockName)
    {
        nlohmann::json f;
        f["type"] = "minecraft:simple_block";
        f["config"]["to_place"]["type"] = "minecraft:simple_state_provider";
        f["config"]["to_place"]["state"]["Name"] = "minecraft:" + blockName;
        return f;
    }

    /// 构造 simple_random_selector 配置化特征：features 为内联 PlacedFeature 列表。
    /// each entry: {blockName, placementJson}。placementJson 为 nullptr 时用空数组。
    std::unique_ptr<ConfiguredFeatureBase> makeSelector(
        const std::vector<std::pair<std::string, nlohmann::json>>& entries)
    {
        nlohmann::json featuresArr = nlohmann::json::array();
        for (const auto& [blockName, placement] : entries) {
            nlohmann::json entry;
            entry["feature"] = inlineSimpleBlock(blockName);
            entry["placement"] = placement.is_null() ? nlohmann::json::array() : placement;
            featuresArr.push_back(std::move(entry));
        }
        nlohmann::json config;
        config["features"] = std::move(featuresArr);
        auto result = FeatureTypeRegistry::instance().create("simple_random_selector", config);
        EXPECT_TRUE(result.success()) << "simple_random_selector should parse";
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

TEST_F(SimpleRandomSelectorFeatureTest, InlineFeaturesParseAndPlaceOne)
{
    // 两个内联 simple_block（stone / dirt），空 placement。
    // 多次放置（不同位置）应两种都出现（均匀随机选择）。
    auto feature = makeSelector({{"stone", {}}, {"dirt", {}}});
    ASSERT_NE(feature, nullptr);

    math::Random random(42);
    std::set<const Block*> seen;
    for (i32 i = 0; i < 40; ++i) {
        const BlockPos pos((i % 5) * 3, 64, (i / 5) * 2);
        feature->place(*m_region, centerChunk(), m_stub, random, pos);
        const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
        if (s != nullptr && !s->isAir()) {
            seen.insert(&s->getBlock());
        }
    }
    EXPECT_EQ(seen.size(), 2u) << "均匀随机应在 40 次内选到两种方块";
}

TEST_F(SimpleRandomSelectorFeatureTest, SingleInlineFeaturePlaces)
{
    // 单个内联 feature（warm_ocean_vegetation 模式：3 选 1，但此处只放 1 个验证基本路径）。
    auto feature = makeSelector({{"cobblestone", {}}});
    ASSERT_NE(feature, nullptr);

    const BlockPos pos(8, 64, 8);
    math::Random random(1);
    const bool result = feature->place(*m_region, centerChunk(), m_stub, random, pos);
    EXPECT_TRUE(result);

    const BlockState* s = m_region->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::COBBLESTONE));
}

TEST_F(SimpleRandomSelectorFeatureTest, PlacementChainOnInlineFeatureIsExecuted)
{
    // 内联 feature 带 placement 链：height_range(constant absolute(70)) 把 origin 的 Y 改写到 70。
    // simple_block(stone) 在改写后的位置放置；origin(8,64,8) 应保持空气，stone 落在 (8,70,8)。
    // 以此验证 placement 链确实被执行（而非被忽略、特征在 origin 原样放置）。
    nlohmann::json placement = nlohmann::json::array();
    nlohmann::json heightRange;
    heightRange["type"] = "minecraft:height_range";
    heightRange["height"]["type"] = "minecraft:constant";
    heightRange["height"]["value"]["absolute"] = 70;
    placement.push_back(std::move(heightRange));

    auto feature = makeSelector({{"stone", placement}});
    ASSERT_NE(feature, nullptr);

    const BlockPos pos(8, 64, 8);
    math::Random random(7);
    feature->place(*m_region, centerChunk(), m_stub, random, pos);

    const BlockState* atOrigin = m_region->getBlockState(pos.x, pos.y, pos.z);
    EXPECT_TRUE(atOrigin == nullptr || atOrigin->isAir()) << "origin 不应被放置";

    const BlockState* atTarget = m_region->getBlockState(pos.x, 70, pos.z);
    ASSERT_NE(atTarget, nullptr);
    EXPECT_TRUE(atTarget->is(VanillaBlocks::STONE)) << "height_range 应把放置改写到 Y=70";
}

TEST_F(SimpleRandomSelectorFeatureTest, RejectsFeatureFieldObjectWithoutType)
{
    // 内联 feature 对象缺少 type → 严格报错。
    nlohmann::json config;
    nlohmann::json entry;
    entry["feature"] = nlohmann::json::object({{"config", nlohmann::json::object()}}); // 缺 type
    entry["placement"] = nlohmann::json::array();
    config["features"] = nlohmann::json::array({std::move(entry)});

    auto result = FeatureTypeRegistry::instance().create("simple_random_selector", config);
    EXPECT_FALSE(result.success());
}

TEST_F(SimpleRandomSelectorFeatureTest, RejectsMissingFeaturesArray)
{
    nlohmann::json config = nlohmann::json::object(); // 无 features
    auto result = FeatureTypeRegistry::instance().create("simple_random_selector", config);
    EXPECT_FALSE(result.success());
}

TEST_F(SimpleRandomSelectorFeatureTest, RejectsStringFeatureEntry)
{
    // 纯字符串 feature（旧设想形式）不被支持：每项必须是内联对象 {feature:{type,config}, placement:[...]}。
    nlohmann::json config;
    nlohmann::json entry;
    entry["feature"] = "minecraft:stone";
    entry["placement"] = nlohmann::json::array();
    config["features"] = nlohmann::json::array({std::move(entry)});

    auto result = FeatureTypeRegistry::instance().create("simple_random_selector", config);
    EXPECT_FALSE(result.success());
}
