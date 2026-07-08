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
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/FeatureTypeRegistry.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/predicate/MatchingBlockPredicate.hpp"
#include "common/world/gen/placement/EnvironmentScanPlacement.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

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

class EnvironmentScanAndRuleBasedProviderTest : public ::testing::Test {
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

    void placeBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_region->setBlockState(x, y, z, state); }

    std::unique_ptr<ConfiguredFeatureBase> create(const std::string& type, const nlohmann::json& config)
    {
        auto result = FeatureTypeRegistry::instance().create(type, config);
        EXPECT_TRUE(result.success()) << "create(" << type << ") should parse: " << result.error().message();
        return result.value();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// parseRuleBased：disk 的 state_provider 无 type 字段（MC 1.21.11 RuleBasedBlockStateProvider）
// ============================================================================

TEST_F(EnvironmentScanAndRuleBasedProviderTest, DiskStateProviderParsesWithoutTypeField)
{
    // disk_clay 等 vanilla JSON：state_provider 仅 {fallback, rules}，无 "type" 字段。
    // MC 1.21.11 DiskConfiguration.stateProvider 为 RuleBasedBlockStateProvider（独立 record，
    // CODEC 无 type）。项目 parseRuleBased 接受此结构。
    nlohmann::json config;
    config["half_height"] = 1;
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 2;
    config["radius"]["max_inclusive"] = 3;
    // 无 config["state_provider"]["type"] —— 与 vanilla disk JSON 一致
    config["state_provider"]["fallback"]["type"] = "minecraft:simple_state_provider";
    config["state_provider"]["fallback"]["state"]["Name"] = "minecraft:clay";
    config["state_provider"]["rules"] = nlohmann::json::array();
    config["target"]["type"] = "minecraft:matching_blocks";
    config["target"]["blocks"] = nlohmann::json::array({"minecraft:dirt"});

    auto feature = create("disk", config);
    ASSERT_NE(feature, nullptr);
    // 解析成功即证明 parseRuleBased 接受无 type 的 {fallback,rules}。
    EXPECT_STREQ(feature->name(), "disk");
}

TEST_F(EnvironmentScanAndRuleBasedProviderTest, DiskReplacesDirtWithClayUsingTypelessProvider)
{
    // 端到端：vanilla 风格 disk（无 type 的 rule_based provider）在 dirt 上替换为 clay。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy, oz, &VanillaBlocks::DIRT->defaultState());

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
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::CLAY)) << "无 type 的 rule_based provider 应正确替换 dirt->clay";
}

// ============================================================================
// environment_scan placement
// ============================================================================

TEST_F(EnvironmentScanAndRuleBasedProviderTest, EnvironmentScanFindsTargetAboveStart)
{
    // MC EnvironmentScanPlacement：从 basePos 沿 direction 逐格扫描，
    // 遇满足 targetCondition 的位置返回该位置；扫描路径须满足 allowedSearchCondition。
    // 构造：basePos(8,64,8) 为 air，向上 3 格 (65,66,67) 为 air，(8,68,8) 为 stone。
    // direction=up，target=matching_blocks(stone)，allowed=matching_blocks(air)，max_steps=12。
    // 扫描：64(air,target? no)->65->66->67->68(stone,target? yes) 返回 (8,68,8)。
    const i32 bx = 8, by = 64, bz = 8;
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    placeBlock(bx, by, bz, air);
    placeBlock(bx, by + 1, bz, air);
    placeBlock(bx, by + 2, bz, air);
    placeBlock(bx, by + 3, bz, air);
    placeBlock(bx, by + 4, bz, stone);

    auto targetPred =
        std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::STONE);
    auto allowedPred = std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::AIR);
    EnvironmentScanConfig config(Direction::Up, std::move(targetPred), std::move(allowedPred), 12);

    EnvironmentScanPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, bx);
    EXPECT_EQ(positions[0].y, by + 4);
    EXPECT_EQ(positions[0].z, bz);
}

TEST_F(EnvironmentScanAndRuleBasedProviderTest, EnvironmentScanReturnsStartWhenTargetAlreadyMet)
{
    // basePos 自身满足 targetCondition → 循环首次检查即返回 basePos。
    const i32 bx = 8, by = 64, bz = 8;
    placeBlock(bx, by, bz, &VanillaBlocks::STONE->defaultState()); // 起点即 stone

    auto targetPred =
        std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::STONE);
    // allowedSearchCondition 缺省（nullptr = 无约束，等价 always_true）
    EnvironmentScanConfig config(Direction::Up, std::move(targetPred), nullptr, 12);

    EnvironmentScanPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].y, by) << "起点已满足 target 应直接返回起点";
}

TEST_F(EnvironmentScanAndRuleBasedProviderTest, EnvironmentScanReturnsEmptyWhenMaxStepsExceeded)
{
    // 全 air，target=stone，max_steps=3：扫描 3 步未遇 stone → 返回空。
    const i32 bx = 8, by = 64, bz = 8;
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    for (i32 dy = 0; dy < 10; ++dy) {
        placeBlock(bx, by + dy, bz, air);
    }

    auto targetPred =
        std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::STONE);
    auto allowedPred = std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::AIR);
    EnvironmentScanConfig config(Direction::Up, std::move(targetPred), std::move(allowedPred), 3);

    EnvironmentScanPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    EXPECT_TRUE(positions.empty()) << "max_steps 内未找到 target 应返回空";
}

TEST_F(EnvironmentScanAndRuleBasedProviderTest, EnvironmentScanStopsWhenAllowedConditionFails)
{
    // allowed=matching_blocks(air)，但扫描路径上第 2 格为 stone（非 air）→ 跳出循环。
    // target=matching_blocks(stone) 在更远处，但路径被阻断，循环结束后 currentPos(非 target) → 空。
    const i32 bx = 8, by = 64, bz = 8;
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();
    placeBlock(bx, by, bz, air);                                       // 0
    placeBlock(bx, by + 1, bz, air);                                   // 1
    placeBlock(bx, by + 2, bz, dirt);                                  // 2：非 air → 阻断 allowed
    placeBlock(bx, by + 3, bz, &VanillaBlocks::STONE->defaultState()); // 3：target，但路径已断

    auto targetPred =
        std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::STONE);
    auto allowedPred = std::make_unique<mc::world::gen::feature::predicate::MatchingBlockPredicate>(VanillaBlocks::AIR);
    EnvironmentScanConfig config(Direction::Up, std::move(targetPred), std::move(allowedPred), 12);

    EnvironmentScanPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    EXPECT_TRUE(positions.empty()) << "扫描路径被 allowed 阻断且 currentPos 非 target 应返回空";
}
