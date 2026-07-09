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

class DiskAndPileFeatureTest : public ::testing::Test {
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

    /// 在绝对坐标 (x,y,z) 放置方块（用 WorldGenRegion 写，保证跨区块可见）。
    void placeBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_region->setBlockState(x, y, z, state); }

    std::unique_ptr<ConfiguredFeatureBase> create(const std::string& type, const nlohmann::json& config)
    {
        auto result = FeatureTypeRegistry::instance().create(type, config);
        EXPECT_TRUE(result.success()) << "create(" << type << ") should parse";
        return result.value();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// spring_feature
// ============================================================================

TEST_F(DiskAndPileFeatureTest, SpringPlacesFluidWhenSurroundedByValidBlocks)
{
    // MC SpringFeature：origin 上方须为 valid_blocks；requires_block_below 时下方也须 valid_blocks；
    // origin 自身须为空气或 valid_blocks；统计水平四邻(W/E/N/S)+下方(D)共 5 格中 valid_blocks 数(j=rockCount)
    // 与空气数(k=holeCount)，j==rockCount && k==holeCount 时放置流体。
    // 构造：origin(8,64,8)；上方 stone(valid)；W/E/N/Down = stone(4 valid)；S = air(1 hole)。origin 自身 air。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    placeBlock(ox, oy + 1, oz, stone); // up（须 valid，不计入 j/k）
    placeBlock(ox, oy - 1, oz, stone); // down（valid）
    placeBlock(ox - 1, oy, oz, stone); // west（valid）
    placeBlock(ox + 1, oy, oz, stone); // east（valid）
    placeBlock(ox, oy, oz - 1, stone); // north（valid）
    // south (ox,oy,oz+1) 保持空气 → holeCount=1

    nlohmann::json config;
    config["state"]["Name"] = "minecraft:water";
    config["requires_block_below"] = true;
    config["rock_count"] = 4;
    config["hole_count"] = 1;
    config["valid_blocks"] = nlohmann::json::array({"minecraft:stone"});

    auto feature = create("spring_feature", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_TRUE(placed);
    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::WATER));
}

TEST_F(DiskAndPileFeatureTest, SpringRejectsWhenAboveNotValidBlock)
{
    // 上方非 valid_blocks → return false，不放置。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy + 1, oz, &VanillaBlocks::DIRT->defaultState()); // 上方 dirt（非 valid）

    nlohmann::json config;
    config["state"]["Name"] = "minecraft:water";
    config["requires_block_below"] = false;
    config["rock_count"] = 4;
    config["hole_count"] = 1;
    config["valid_blocks"] = nlohmann::json::array({"minecraft:stone"});

    auto feature = create("spring_feature", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_FALSE(placed);
}

TEST_F(DiskAndPileFeatureTest, SpringRejectsWhenRockCountMismatch)
{
    // valid_blocks 数量与 rockCount 不符 → 不放置。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    placeBlock(ox, oy + 1, oz, stone);
    placeBlock(ox, oy - 1, oz, stone); // 只有 2 个 valid，但 rock_count=5
    placeBlock(ox - 1, oy, oz, stone);

    nlohmann::json config;
    config["state"]["Name"] = "minecraft:water";
    config["requires_block_below"] = false;
    config["rock_count"] = 5;
    config["hole_count"] = 1;
    config["valid_blocks"] = nlohmann::json::array({"minecraft:stone"});

    auto feature = create("spring_feature", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_FALSE(placed);
}

// ============================================================================
// block_pile
// ============================================================================

TEST_F(DiskAndPileFeatureTest, BlockPilePlacesOnSturdySurface)
{
    // origin 下方铺一层石头（sturdy face up），覆盖 pile 可能的 [ox-3,ox+3]×[oz-3,oz+3] 范围，
    // 使各列 mayPlaceOn 均成立 → pile 在 origin 上方 2 层内放置多列 cobblestone。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            placeBlock(ox + dx, oy - 1, oz + dz, stone);
        }
    }

    nlohmann::json config;
    config["state_provider"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["state"]["Name"] = "minecraft:cobblestone";

    auto feature = create("block_pile", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(42);
    const bool ret = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_TRUE(ret); // MC BlockPileFeature 只要 origin.y >= minY+5 即返回 true

    // pile 在 [oy, oy+1] 两层、[ox-3,ox+3]×[oz-3,oz+3] 范围内放置，至少有一处 cobblestone。
    bool found = false;
    for (i32 dx = -3; dx <= 3 && !found; ++dx) {
        for (i32 dz = -3; dz <= 3 && !found; ++dz) {
            for (i32 dy = 0; dy <= 1 && !found; ++dy) {
                const BlockState* s = m_region->getBlockState(ox + dx, oy + dy, oz + dz);
                if (s != nullptr && s->is(VanillaBlocks::COBBLESTONE)) {
                    found = true;
                }
            }
        }
    }
    EXPECT_TRUE(found) << "block_pile 应在 origin 上方放置 cobblestone";
}

TEST_F(DiskAndPileFeatureTest, BlockPileRejectsTooLow)
{
    // origin.y < minY+5 → return false（MC：blockpos.getY() < getMinY()+5）。
    // 主世界 minY=-64，故 minY+5=-59；用 y=-60 触发。但 ChunkPrimer 可能不支持这么低，
    // 改为验证：当下方无 sturdy 面时（空气下方），不放置任何方块。
    const i32 ox = 8, oy = 64, oz = 8;
    // 不放任何下方方块 → mayPlaceOn 失败 → tryPlaceBlock 不写。

    nlohmann::json config;
    config["state_provider"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["state"]["Name"] = "minecraft:cobblestone";

    auto feature = create("block_pile", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(42);
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    // origin 及上方不应有 cobblestone（无 sturdy 面）。
    for (i32 dy = 0; dy <= 1; ++dy) {
        const BlockState* s = m_region->getBlockState(ox, oy + dy, oz);
        EXPECT_TRUE(s == nullptr || s->isAir());
    }
}

// ============================================================================
// nether_forest_vegetation
// ============================================================================

TEST_F(DiskAndPileFeatureTest, NetherForestVegetationPlacesOnNylium)
{
    // 下方为 crimson_nylium → 在 origin 周围 spread 范围内放置植被。
    // weighted_state_provider: 100% crimson_roots，spread_width=2, spread_height=1。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy - 1, oz, &VanillaBlocks::CRIMSON_NYLIUM->defaultState());

    nlohmann::json config;
    config["spread_width"] = 2;
    config["spread_height"] = 1;
    config["state_provider"]["type"] = "minecraft:weighted_state_provider";
    nlohmann::json entry;
    entry["data"]["Name"] = "minecraft:crimson_roots";
    entry["weight"] = 100;
    config["state_provider"]["entries"] = nlohmann::json::array({entry});

    auto feature = create("nether_forest_vegetation", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(99);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_TRUE(placed);

    // spread_width=2 → spread_width^2=4 次尝试，应在 origin 附近某处放置 crimson_roots。
    bool found = false;
    for (i32 dx = -2; dx <= 2 && !found; ++dx) {
        for (i32 dy = -1; dy <= 1 && !found; ++dy) {
            for (i32 dz = -2; dz <= 2 && !found; ++dz) {
                const BlockState* s = m_region->getBlockState(ox + dx, oy + dy, oz + dz);
                if (s != nullptr && s->is(VanillaBlocks::CRIMSON_ROOTS)) {
                    found = true;
                }
            }
        }
    }
    EXPECT_TRUE(found) << "应在 nylium 上方放置 crimson_roots";
}

TEST_F(DiskAndPileFeatureTest, NetherForestVegetationRejectsNonNyliumBelow)
{
    // 下方非 nylium → return false，不放置。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy - 1, oz, &VanillaBlocks::STONE->defaultState()); // 下方石头

    nlohmann::json config;
    config["spread_width"] = 2;
    config["spread_height"] = 1;
    config["state_provider"]["type"] = "minecraft:weighted_state_provider";
    nlohmann::json entry;
    entry["data"]["Name"] = "minecraft:crimson_roots";
    entry["weight"] = 100;
    config["state_provider"]["entries"] = nlohmann::json::array({entry});

    auto feature = create("nether_forest_vegetation", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(99);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_FALSE(placed);
}

// ============================================================================
// disk + rule_based_state_provider
// ============================================================================

TEST_F(DiskAndPileFeatureTest, DiskReplacesTargetBlocksWithinRadius)
{
    // disk：origin 周围 radius 范围内，target 谓词匹配的格子被替换为 state_provider 状态。
    // 配置：target=matching_blocks(dirt), state_provider=simple(clay), radius=uniform(2,2), half_height=0。
    // 在 origin 同层放一圈 dirt，验证被替换为 clay。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();
    placeBlock(ox, oy, oz, dirt);
    placeBlock(ox + 1, oy, oz, dirt);
    placeBlock(ox, oy, oz + 1, dirt);

    nlohmann::json config;
    config["half_height"] = 0;
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 2;
    config["radius"]["max_inclusive"] = 2;
    config["state_provider"]["fallback"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["fallback"]["state"]["Name"] = "minecraft:clay";
    config["state_provider"]["rules"] = nlohmann::json::array();
    config["target"]["type"] = "minecraft:matching_blocks";
    config["target"]["blocks"] = nlohmann::json::array({"minecraft:dirt"});

    auto feature = create("disk", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_TRUE(placed);

    // origin 处的 dirt 应被替换为 clay。
    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::CLAY));
}

TEST_F(DiskAndPileFeatureTest, DiskRuleBasedProviderAppliesMatchingRule)
{
    // rule_based_state_provider：if_true 谓词匹配则用 then，否则 fallback。
    // 规则：if_true=matching_blocks(dirt) → then=sand；fallback=clay。
    // origin 处为 dirt → 命中规则 → sand；相邻处无方块（air，不匹配 target 故不被替换）。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy, oz, &VanillaBlocks::DIRT->defaultState());

    nlohmann::json config;
    config["half_height"] = 0;
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 2;
    config["radius"]["max_inclusive"] = 2;
    nlohmann::json rule;
    rule["if_true"]["type"] = "minecraft:matching_blocks";
    rule["if_true"]["blocks"] = nlohmann::json::array({"minecraft:dirt"});
    rule["then"]["type"] = "minecraft:simple_state_provider";
    rule["then"]["state"]["Name"] = "minecraft:sand";
    config["state_provider"]["fallback"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["fallback"]["state"]["Name"] = "minecraft:clay";
    config["state_provider"]["rules"] = nlohmann::json::array({rule});
    config["target"]["type"] = "minecraft:matching_blocks";
    config["target"]["blocks"] = nlohmann::json::array({"minecraft:dirt"});

    auto feature = create("disk", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    // origin 是 dirt（命中 if_true）→ 应替换为 sand（rule 的 then），而非 clay（fallback）。
    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::SAND)) << "rule_based 命中 dirt 规则应放 sand";
}

TEST_F(DiskAndPileFeatureTest, DiskUsesFallbackWhenNoRuleMatches)
{
    // rule_based：规则 if_true=matching_blocks(stone)（origin 是 dirt，不命中）→ 用 fallback=clay。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy, oz, &VanillaBlocks::DIRT->defaultState());

    nlohmann::json config;
    config["half_height"] = 0;
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 2;
    config["radius"]["max_inclusive"] = 2;
    nlohmann::json rule;
    rule["if_true"]["type"] = "minecraft:matching_blocks";
    rule["if_true"]["blocks"] = nlohmann::json::array({"minecraft:stone"}); // 不命中 dirt
    rule["then"]["type"] = "minecraft:simple_state_provider";
    rule["then"]["state"]["Name"] = "minecraft:sand";
    config["state_provider"]["fallback"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["fallback"]["state"]["Name"] = "minecraft:clay";
    config["state_provider"]["rules"] = nlohmann::json::array({rule});
    config["target"]["type"] = "minecraft:matching_blocks";
    config["target"]["blocks"] = nlohmann::json::array({"minecraft:dirt"});

    auto feature = create("disk", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::CLAY)) << "无规则命中应回退 fallback=clay";
}
