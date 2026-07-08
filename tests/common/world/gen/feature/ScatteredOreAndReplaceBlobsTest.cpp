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

class ScatteredOreAndReplaceBlobsTest : public ::testing::Test {
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

    /// 在以 (cx,cy,cz) 为中心的实心立方体内填满 state，用于铺设矿石/替换球的宿主岩体。
    void fillBox(i32 cx, i32 cy, i32 cz, i32 half, const BlockState* state)
    {
        for (i32 dx = -half; dx <= half; ++dx) {
            for (i32 dy = -half; dy <= half; ++dy) {
                for (i32 dz = -half; dz <= half; ++dz) {
                    placeBlock(cx + dx, cy + dy, cz + dz, state);
                }
            }
        }
    }

    std::unique_ptr<ConfiguredFeatureBase> create(const std::string& type, const nlohmann::json& config)
    {
        auto result = FeatureTypeRegistry::instance().create(type, config);
        EXPECT_TRUE(result.success()) << "create(" << type << ") should parse";
        return result.value();
    }

    /// 统计以 (cx,cy,cz) 为中心、半径 half 的立方体内 targetBlock 的数量。
    i32 countBlock(i32 cx, i32 cy, i32 cz, i32 half, const Block* targetBlock)
    {
        i32 count = 0;
        for (i32 dx = -half; dx <= half; ++dx) {
            for (i32 dy = -half; dy <= half; ++dy) {
                for (i32 dz = -half; dz <= half; ++dz) {
                    const BlockState* s = m_region->getBlockState(cx + dx, cy + dy, cz + dz);
                    if (s != nullptr && s->is(targetBlock)) {
                        ++count;
                    }
                }
            }
        }
        return count;
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// scattered_ore
// ============================================================================

TEST_F(ScatteredOreAndReplaceBlobsTest, ScatteredOrePlacesOreInStoneHost)
{
    // MC ScatteredOreFeature：i=nextInt(size+1) 次散点放置；每次按 min(j,7) 在三轴各偏移
    // round((nextFloat-nextFloat)*j)，命中 canPlaceOre（target.test 通过且空气暴露校验）则放置。
    // 配置：size=8（i∈[0,8]），target=block_match(stone)，state=gold_ore，discard_chance=0（跳过空气检查）。
    // 在 origin 周围 7 格半径内填实心石头（覆盖最大偏移 7），保证散点落在石头内。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    fillBox(ox, oy, oz, 7, stone);

    nlohmann::json config;
    config["size"] = 8;
    config["discard_chance_on_air_exposure"] = 0.0;
    nlohmann::json target;
    target["state"]["Name"] = "minecraft:gold_ore";
    target["target"]["predicate_type"] = "minecraft:block_match";
    target["target"]["block"] = "minecraft:stone";
    config["targets"] = nlohmann::json::array({target});

    auto feature = create("scattered_ore", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(7);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    // MC ScatteredOreFeature 恒返回 true。
    EXPECT_TRUE(placed);

    // 散点范围内应至少有一处 stone 被替换为 gold_ore。
    const i32 goldCount = countBlock(ox, oy, oz, 7, VanillaBlocks::GOLD_ORE);
    EXPECT_GT(goldCount, 0) << "scattered_ore 应在石头宿主中放置至少一处金矿石";
}

TEST_F(ScatteredOreAndReplaceBlobsTest, ScatteredOreDiscardChanceOneRejectsAirAdjacent)
{
    // discard_chance_on_air_exposure=1.0：shouldSkipAirCheck 返回 false（不跳过），
    // isAdjacentToAir 为 true 则不放。origin 周围除自身列外多为空气 → 散点命中空气邻居时不放。
    // 关键：canPlaceOre 对每个候选位先做 target.test(stone)；只有落在石头里的候选才进入空气检查，
    // 而石头候选的 6 邻居若任一为空气则拒绝。构造仅 origin 单格石头，其 6 邻居全空气 → 全部拒绝。
    const i32 ox = 8, oy = 64, oz = 8;
    placeBlock(ox, oy, oz, &VanillaBlocks::STONE->defaultState()); // 仅 origin 一格石头

    nlohmann::json config;
    config["size"] = 8;
    config["discard_chance_on_air_exposure"] = 1.0;
    nlohmann::json target;
    target["state"]["Name"] = "minecraft:gold_ore";
    target["target"]["predicate_type"] = "minecraft:block_match";
    target["target"]["block"] = "minecraft:stone";
    config["targets"] = nlohmann::json::array({target});

    auto feature = create("scattered_ore", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(7);
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    // origin 处石头不应被替换（6 邻居全空气 → canPlaceOre 拒绝）。
    const BlockState* s = m_region->getBlockState(ox, oy, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::STONE)) << "discard_chance=1 且邻居为空气时不应放置矿石";
}

TEST_F(ScatteredOreAndReplaceBlobsTest, ScatteredOreRejectsWhenTargetBlockAbsent)
{
    // target=block_match(stone)，但宿主全是泥土 → target.test 恒 false → 不放置任何矿石。
    const i32 ox = 8, oy = 64, oz = 8;
    fillBox(ox, oy, oz, 7, &VanillaBlocks::DIRT->defaultState());

    nlohmann::json config;
    config["size"] = 8;
    config["discard_chance_on_air_exposure"] = 0.0;
    nlohmann::json target;
    target["state"]["Name"] = "minecraft:gold_ore";
    target["target"]["predicate_type"] = "minecraft:block_match";
    target["target"]["block"] = "minecraft:stone";
    config["targets"] = nlohmann::json::array({target});

    auto feature = create("scattered_ore", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(7);
    feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));

    EXPECT_EQ(countBlock(ox, oy, oz, 7, VanillaBlocks::GOLD_ORE), 0) << "无 stone 宿主不应放置金矿石";
}

// ============================================================================
// netherrack_replace_blobs
// ============================================================================

TEST_F(ScatteredOreAndReplaceBlobsTest, ReplaceBlobsReplacesNetherrackWithBlackstone)
{
    // MC ReplaceBlobsFeature：从 origin（Y 钳制 [minY+1,maxY]）向下找第一个 target 方块作球心；
    // i/j/k=radius.sample() 各一次，l=max(i,j,k)；withinManhattan(球心,i,j,k) 内 distManhattan<=l
    // 且 is(target) 的格子替换为 replaceState。
    // 配置：target=netherrack, state=blackstone, radius=uniform(3,3)（固定半径 3，确定性球）。
    // 在 origin 下方铺实心 netherrack，验证替换出 blackstone 球。
    const i32 ox = 8, oy = 64, oz = 8;
    const BlockState* netherrack = &VanillaBlocks::NETHERRACK->defaultState();
    // 在 origin 及下方铺 netherrack（findTarget 从 origin 向下找，origin 自身须为 netherrack）。
    fillBox(ox, oy, oz, 4, netherrack);

    nlohmann::json config;
    config["target"]["Name"] = "minecraft:netherrack";
    config["state"]["Name"] = "minecraft:blackstone";
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 3;
    config["radius"]["max_inclusive"] = 3;

    auto feature = create("netherrack_replace_blobs", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_TRUE(placed);

    // 半径 3 的曼哈顿球内应替换出 blackstone（球心 origin 至少自身被替换）。
    const i32 blackstoneCount = countBlock(ox, oy, oz, 4, VanillaBlocks::BLACKSTONE);
    EXPECT_GT(blackstoneCount, 0) << "应在 netherrack 中替换出 blackstone 球";
}

TEST_F(ScatteredOreAndReplaceBlobsTest, ReplaceBlobsReturnsFalseWhenTargetNotFound)
{
    // origin 下方无 target(netherrack) → findTarget 返回无效 → place 返回 false。
    const i32 ox = 8, oy = 64, oz = 8;
    fillBox(ox, oy, oz, 4, &VanillaBlocks::STONE->defaultState()); // 全是石头，无 netherrack

    nlohmann::json config;
    config["target"]["Name"] = "minecraft:netherrack";
    config["state"]["Name"] = "minecraft:blackstone";
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 3;
    config["radius"]["max_inclusive"] = 3;

    auto feature = create("netherrack_replace_blobs", config);
    ASSERT_NE(feature, nullptr);

    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, oy, oz));
    EXPECT_FALSE(placed);
}

TEST_F(ScatteredOreAndReplaceBlobsTest, ReplaceBlobsClampsOriginYToBuildHeight)
{
    // MC: origin Y 钳制到 [minY+1, maxY]。主世界 minY=-64。
    // 给 origin.y 远高于 maxY 的位置，findTarget 从钳制后的 Y 向下找 netherrack。
    // 在 oy-1 处放 netherrack，origin.y 设为极大值（会被钳到 maxY），仍能向下找到 netherrack。
    const i32 ox = 8, oz = 8;
    const i32 maxY = m_region->getMaxBuildHeight(); // 钳制上界
    const i32 targetY = maxY - 1;                   // 在钳制上界附近放 netherrack
    placeBlock(ox, targetY, oz, &VanillaBlocks::NETHERRACK->defaultState());

    nlohmann::json config;
    config["target"]["Name"] = "minecraft:netherrack";
    config["state"]["Name"] = "minecraft:blackstone";
    config["radius"]["type"] = "minecraft:uniform";
    config["radius"]["min_inclusive"] = 0;
    config["radius"]["max_inclusive"] = 0; // 半径 0：只替换球心一格

    auto feature = create("netherrack_replace_blobs", config);
    ASSERT_NE(feature, nullptr);

    // origin.y 设为远超 maxY 的值，应被钳制到 maxY，然后向下找到 targetY 处的 netherrack。
    math::Random random(1);
    const bool placed = feature->place(*m_region, centerChunk(), m_stub, random, BlockPos(ox, maxY + 50, oz));
    EXPECT_TRUE(placed);

    const BlockState* s = m_region->getBlockState(ox, targetY, oz);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::BLACKSTONE)) << "Y 钳制后应找到并替换 netherrack";
}
