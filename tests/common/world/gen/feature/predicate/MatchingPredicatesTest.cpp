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
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/parser/BlockPredicateParser.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"

#include <memory>
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

class MatchingPredicatesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        PlacementRegistry::instance().initialize();

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

    /// 通过 BlockPredicateParser 解析一个 block predicate JSON 对象。
    std::unique_ptr<predicate::BlockPredicate> parse(const nlohmann::json& obj)
    {
        auto r = parser::BlockPredicateParser::parse(obj);
        EXPECT_TRUE(r.success()) << "predicate parse should succeed";
        return r.value();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// matching_fluids 谓词
// ============================================================================

TEST_F(MatchingPredicatesTest, MatchingFluidsEmptyMatchesAir)
{
    // 空气（ChunkData 未初始化 section → nullptr BlockState）的流体状态为空，
    // matching_fluids(minecraft:empty) 应返回 true。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_fluids";
    obj["fluids"] = "minecraft:empty";
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    // 中心区块该位置未填充任何方块 → nullptr → 空 → 匹配 empty。
    EXPECT_TRUE(pred->test(*m_region, pos));
}

TEST_F(MatchingPredicatesTest, MatchingFluidsEmptyRejectsWater)
{
    // 在目标位置放置水方块，fluids=empty 应不匹配。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_fluids";
    obj["fluids"] = "minecraft:empty";
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::WATER->defaultState());
    EXPECT_FALSE(pred->test(*m_region, pos));
}

TEST_F(MatchingPredicatesTest, MatchingFluidsWaterMatchesWater)
{
    // fluids=["minecraft:water","minecraft:flowing_water"] 应匹配水方块。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_fluids";
    obj["fluids"] = nlohmann::json::array({"minecraft:water", "minecraft:flowing_water"});
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    m_region->setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::WATER->defaultState());
    EXPECT_TRUE(pred->test(*m_region, pos));

    // 空气（empty）不匹配 water。
    const BlockPos airPos(4, 64, 4);
    EXPECT_FALSE(pred->test(*m_region, airPos));
}

TEST_F(MatchingPredicatesTest, MatchingFluidsRejectsUnknownFluid)
{
    // 未知流体 id 应解析失败（严格报错）。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_fluids";
    obj["fluids"] = "minecraft:does_not_exist";
    auto r = parser::BlockPredicateParser::parse(obj);
    EXPECT_FALSE(r.success());
}

TEST_F(MatchingPredicatesTest, MatchingFluidsOffsetShiftsTestPosition)
{
    // offset=[0,-1,0]：在 pos 正下方放水，pos 本身为空气。
    // fluids=water 应匹配（因为测试位置是 pos+offset=下方的水）。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_fluids";
    obj["fluids"] = "minecraft:water";
    obj["offset"] = nlohmann::json::array({0, -1, 0});
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    const BlockPos below(8, 63, 8);
    m_region->setBlockState(below.x, below.y, below.z, &VanillaBlocks::WATER->defaultState());
    // pos 自身为空气，但 offset 后测试 below(水) → 匹配 water。
    EXPECT_TRUE(pred->test(*m_region, pos));
}

// ============================================================================
// matching_blocks offset 支持
// ============================================================================

TEST_F(MatchingPredicatesTest, MatchingBlocksOffsetShiftsTestPosition)
{
    // 对齐 patch_melon 的用法：matching_blocks(grass_block, offset=[0,-1,0])。
    // pos 自身为空气（nullptr），下方为 grass_block → 应匹配。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_blocks";
    obj["blocks"] = "minecraft:grass_block";
    obj["offset"] = nlohmann::json::array({0, -1, 0});
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    const BlockPos below(8, 63, 8);
    m_region->setBlockState(below.x, below.y, below.z, &VanillaBlocks::GRASS_BLOCK->defaultState());
    // pos 自身不是 grass_block，但 offset 后测试 below → 匹配。
    EXPECT_TRUE(pred->test(*m_region, pos));

    // 没有 offset 时 pos 自身（空气）不匹配 grass_block。
    nlohmann::json noOffset;
    noOffset["type"] = "minecraft:matching_blocks";
    noOffset["blocks"] = "minecraft:grass_block";
    auto predNoOffset = parse(noOffset);
    ASSERT_NE(predNoOffset, nullptr);
    EXPECT_FALSE(predNoOffset->test(*m_region, pos));
}

TEST_F(MatchingPredicatesTest, MatchingBlocksOffsetArrayAppliesToMultiBlockAnyOf)
{
    // 多方块 matching_blocks + offset：下方为 dirt，blocks=[grass_block, dirt]。
    nlohmann::json obj;
    obj["type"] = "minecraft:matching_blocks";
    obj["blocks"] = nlohmann::json::array({"minecraft:grass_block", "minecraft:dirt"});
    obj["offset"] = nlohmann::json::array({0, -1, 0});
    auto pred = parse(obj);
    ASSERT_NE(pred, nullptr);

    const BlockPos pos(8, 64, 8);
    const BlockPos below(8, 63, 8);
    m_region->setBlockState(below.x, below.y, below.z, &VanillaBlocks::DIRT->defaultState());
    EXPECT_TRUE(pred->test(*m_region, pos));
}
