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

} // namespace

class RandomPatchFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // random_patch 工厂递归解析内联 simple_block（FeatureTypeRegistry）+
        // block_predicate_filter placement 链（PlacementRegistry 严格校验），故二者均需初始化。
        PlacementRegistry::instance().initialize();
        world::gen::feature::initializeBuiltinFeatureTypes();

        // 3x3 区块区域（中心 0,0）
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

    /// 用指定方块填充长方体（初始化 section + 构造环境）
    void fill(i32 x0, i32 y0, i32 z0, i32 x1, i32 y1, i32 z1, const BlockState* state)
    {
        for (i32 x = x0; x <= x1; ++x) {
            for (i32 y = y0; y <= y1; ++y) {
                for (i32 z = z0; z <= z1; ++z) {
                    m_region->setBlockState(x, y, z, state);
                }
            }
        }
    }

    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; }

    /// 构造一个 random_patch config：内联 simple_block(石头) + block_predicate_filter(air)
    /// tries/xz_spread/y_spread 由参数指定。结果存入 m_ownedFeature。
    void makePatchConfig(i32 tries, i32 xzSpread, i32 ySpread)
    {
        const std::string jsonStr = R"({
            "type": "minecraft:random_patch",
            "config": {
                "feature": {
                    "feature": {
                        "type": "minecraft:simple_block",
                        "config": {
                            "to_place": {
                                "type": "minecraft:simple_state_provider",
                                "state": { "Name": "minecraft:stone" }
                            }
                        }
                    },
                    "placement": [
                        {
                            "type": "minecraft:block_predicate_filter",
                            "predicate": { "type": "minecraft:matching_blocks", "blocks": "minecraft:air" }
                        }
                    ]
                },
                "tries": )" +
            std::to_string(tries) + R"(,
                "xz_spread": )" +
            std::to_string(xzSpread) + R"(,
                "y_spread": )" +
            std::to_string(ySpread) + R"(
            }
        })";
        auto jsonObj = nlohmann::json::parse(jsonStr);
        auto result = FeatureTypeRegistry::instance().create("random_patch", jsonObj["config"]);
        ASSERT_TRUE(result.success()) << "create random_patch factory should succeed";
        m_ownedFeature = result.value();
        ASSERT_NE(m_ownedFeature, nullptr);
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
    std::unique_ptr<ConfiguredFeatureBase> m_ownedFeature;
};

// ============================================================================
// 算法语义测试
// ============================================================================

TEST_F(RandomPatchFeatureTest, PlacesStoneOnAirWithinSpread)
{
    // 全空气环境：origin 周围 ±spread 内全部为空气，simple_block 在 air 处放置石头。
    // block_predicate_filter(matching_blocks=air) 通过 → simple_block 放置石头。
    makePatchConfig(/*tries=*/32, /*xz_spread=*/4, /*y_spread=*/1);
    ASSERT_NE(m_ownedFeature, nullptr);

    // origin 在空气区，spread=4 内尽量覆盖同区块
    const BlockPos origin(8, 64, 8);
    // ChunkPrimer 未初始化 section 的 getBlockState 返回 nullptr（项目以 nullptr 表示空气，
    // 空气不持久化到 section）。全空气环境即所有候选位置均为 nullptr；matching_blocks(air)
    // 与 SimpleBlockFeature 均需把 nullptr 当作可替换的空气，random_patch 才能放上石头。
    math::Random random(123);
    const bool result = m_ownedFeature->place(*m_region, centerChunk(), m_stub, random, origin);
    EXPECT_TRUE(result) << "random_patch 应在空气区成功放置至少一次";

    // 统计 origin ±5 范围内的石头数量（应 > 0）
    i32 stoneCount = 0;
    for (i32 x = origin.x - 5; x <= origin.x + 5; ++x) {
        for (i32 y = origin.y - 2; y <= origin.y + 2; ++y) {
            for (i32 z = origin.z - 5; z <= origin.z + 5; ++z) {
                const BlockState* s = m_region->getBlockState(x, y, z);
                if (s != nullptr && s->is(VanillaBlocks::STONE)) {
                    ++stoneCount;
                }
            }
        }
    }
    EXPECT_GT(stoneCount, 0) << "应在空气区生成若干石头";
}

TEST_F(RandomPatchFeatureTest, BlockPredicateFilterBlocksNonAirPlacement)
{
    // 全石头环境：block_predicate_filter(matching_blocks=air) 在石头处不通过 → simple_block 不放置。
    // 故 tries 次全部失败，place 返回 false，且不改变任何方块（仍是石头）。
    fill(-8, 60, -8, 23, 70, 23, &VanillaBlocks::STONE->defaultState());
    makePatchConfig(/*tries=*/32, /*xz_spread=*/4, /*y_spread=*/1);
    ASSERT_NE(m_ownedFeature, nullptr);

    const BlockPos origin(8, 64, 8);
    math::Random random(456);
    const bool result = m_ownedFeature->place(*m_region, centerChunk(), m_stub, random, origin);
    EXPECT_FALSE(result) << "全固体环境下 block_predicate_filter 应阻止任何放置";

    // 确认 origin 附近没有"新放置的石头改变"——此处无法区分新旧石头，
    // 但可断言 origin 处仍为石头（未被改写为其他）。
    const BlockState* s = m_region->getBlockState(origin.x, origin.y, origin.z);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->is(VanillaBlocks::STONE));
}

TEST_F(RandomPatchFeatureTest, TriangleSpreadStaysWithinBounds)
{
    // xz_spread=2：偏移 dx=nextInt(3)-nextInt(3) ∈ [-2,2]，故所有放置在 origin±2 内。
    makePatchConfig(/*tries=*/128, /*xz_spread=*/2, /*y_spread=*/0);
    ASSERT_NE(m_ownedFeature, nullptr);

    const BlockPos origin(8, 64, 8);
    math::Random random(789);
    m_ownedFeature->place(*m_region, centerChunk(), m_stub, random, origin);

    // 扫描 origin±3 范围外是否出现石头（不应有）
    i32 outOfBounds = 0;
    for (i32 x = origin.x - 6; x <= origin.x + 6; ++x) {
        for (i32 y = origin.y - 2; y <= origin.y + 2; ++y) {
            for (i32 z = origin.z - 6; z <= origin.z + 6; ++z) {
                const BlockState* s = m_region->getBlockState(x, y, z);
                if (s != nullptr && s->is(VanillaBlocks::STONE)) {
                    if (std::abs(x - origin.x) > 2 || std::abs(z - origin.z) > 2) {
                        ++outOfBounds;
                    }
                }
            }
        }
    }
    EXPECT_EQ(outOfBounds, 0) << "xz_spread=2 时放置不应超出 origin±2 的 XZ 范围";
}

TEST_F(RandomPatchFeatureTest, ZeroTriesPlacesNothing)
{
    // tries=0：循环不执行，place 返回 false，不放置任何方块。
    makePatchConfig(/*tries=*/0, /*xz_spread=*/4, /*y_spread=*/1);
    ASSERT_NE(m_ownedFeature, nullptr);

    const BlockPos origin(8, 64, 8);
    math::Random random(0);
    const bool result = m_ownedFeature->place(*m_region, centerChunk(), m_stub, random, origin);
    EXPECT_FALSE(result);

    i32 stoneCount = 0;
    for (i32 x = origin.x - 5; x <= origin.x + 5; ++x) {
        for (i32 z = origin.z - 5; z <= origin.z + 5; ++z) {
            const BlockState* s = m_region->getBlockState(x, origin.y, z);
            if (s != nullptr && s->is(VanillaBlocks::STONE)) {
                ++stoneCount;
            }
        }
    }
    EXPECT_EQ(stoneCount, 0);
}
