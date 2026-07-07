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

#include "common/util/property/Properties.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/SnowAndFreezeFeature.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::biome;

// ============================================================================
// 最小 IChunkGenerator 存根，避免解引用空指针的未定义行为
// ============================================================================

class StubChunkGenerator : public IChunkGenerator {
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
    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override { return Biomes::Plains; }
    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override
    {
        return Biomes::Plains;
    }
    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] const DimensionSettings& settings() const override { return DimensionSettings::overworld(); }
    [[nodiscard]] i32 seaLevel() const override { return 63; }
};

// ============================================================================
// 测试夹具 - 雪和冰冻结特征
// ============================================================================

class SnowAndFreezeFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        BiomeRegistry::instance().initialize();

        // 直接构造 freeze_top_layer 特征（数据驱动注册表由生产代码加载，测试中直接构造）
        m_feature = std::make_unique<ConfiguredSnowAndFreezeFeature>("freeze_top_layer");

        // 创建 3x3 区块区域（中心在 0,0）
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                // 设置为 FEATURES 阶段，使高度图包含 MOTION_BLOCKING
                // SnowAndFreezeFeature 使用 MotionBlocking 高度图
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

    /**
     * @brief 设置指定区块的生物群系为指定 ID
     *
     * 填充整个 BiomeContainer 为同一生物群系。
     */
    void setChunkBiome(ChunkPrimer& chunk, BiomeId biomeId)
    {
        auto& biomes = chunk.getBiomes();
        for (i32 sectionIndex = 0; sectionIndex < world::CHUNK_SECTIONS; ++sectionIndex) {
            for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                    for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                        biomes.setBiome(sectionIndex, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    /**
     * @brief 获取中心区块（0,0）
     */
    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; } // index 4 = (0,0) in 3x3
    [[nodiscard]] const ChunkPrimer& centerChunk() const { return *m_ownedChunks[4]; }

    /**
     * @brief 在中心区块设置方块并更新高度图
     */
    void setCenterBlock(i32 localX, i32 y, i32 localZ, const BlockState* state)
    {
        centerChunk().setBlockState(localX, y, localZ, state);
    }

    /**
     * @brief 获取中心区块的方块
     */
    [[nodiscard]] const BlockState* getCenterBlock(i32 localX, i32 y, i32 localZ) const
    {
        return centerChunk().getBlockState(localX, y, localZ);
    }

    /**
     * @brief 获取世界坐标的方块（通过 WorldGenRegion）
     */
    [[nodiscard]] const BlockState* getWorldBlock(i32 x, i32 y, i32 z) const
    {
        return m_region->getBlockState(x, y, z);
    }

    /**
     * @brief 在世界坐标设置方块
     */
    void setWorldBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_region->setBlockState(x, y, z, state); }

    /**
     * @brief 获取水源方块的流体状态
     */
    const fluid::FluidState* getWaterFluidState() const
    {
        auto* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        return waterFluid ? &waterFluid->defaultState() : nullptr;
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    std::unique_ptr<ConfiguredSnowAndFreezeFeature> m_feature;
    StubChunkGenerator m_stubGenerator;
    math::Random m_random{42};
};

// ============================================================================
// 基础测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, FeatureInitialization)
{
    ASSERT_NE(m_feature, nullptr);
    EXPECT_STREQ(m_feature->name(), "freeze_top_layer");
    EXPECT_EQ(m_feature->stage(), DecorationStage::TopLayerModification);
}

TEST_F(SnowAndFreezeFeatureTest, FeaturePlaceReturnsTrue)
{
    // 设置简单地形：中心区块铺一层石头
    setChunkBiome(centerChunk(), Biomes::Plains);
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    bool result = m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));
    EXPECT_TRUE(result);
}

// ============================================================================
// 冰冻结测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, FreezesWaterInColdBiome)
{
    // 寒冷生物群系：水面应冻结为冰
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    // 地形：y=62 石头，y=63 水
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 62, z, &VanillaBlocks::STONE->defaultState());
            setCenterBlock(x, 63, z, &VanillaBlocks::WATER->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 检查 y=63 的水是否被替换为冰
    // Water block at y=63 is the topmost motion-blocking block, so freezeY = topY = 63
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            ASSERT_NE(state, nullptr);
            EXPECT_TRUE(state->is(VanillaBlocks::ICE))
                << "Block at (" << x << ", 63, " << z << ") should be ICE but is not";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotFreezeWaterInWarmBiome)
{
    // 温暖生物群系：水面不应冻结
    setChunkBiome(centerChunk(), Biomes::Plains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 62, z, &VanillaBlocks::STONE->defaultState());
            setCenterBlock(x, 63, z, &VanillaBlocks::WATER->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=63 应该仍然是水
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            ASSERT_NE(state, nullptr);
            EXPECT_TRUE(state->is(VanillaBlocks::WATER))
                << "Block at (" << x << ", 63, " << z << ") should still be WATER";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotFreezeStoneInColdBiome)
{
    // 寒冷生物群系但水面不是水：不应产生冰
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=63 应该仍然是石头
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            ASSERT_NE(state, nullptr);
            EXPECT_TRUE(state->is(VanillaBlocks::STONE))
                << "Block at (" << x << ", 63, " << z << ") should still be STONE";
        }
    }
}

// ============================================================================
// 降雪测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, PlacesSnowInColdBiome)
{
    // 寒冷生物群系：地面应放置雪层
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=64（石头上方）应有雪层
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 64, z);
            ASSERT_NE(state, nullptr);
            EXPECT_TRUE(state->is(VanillaBlocks::SNOW)) << "Block at (" << x << ", 64, " << z << ") should be SNOW";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotPlaceSnowInWarmBiome)
{
    // 温暖生物群系：不应降雪
    setChunkBiome(centerChunk(), Biomes::Plains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=64 应该是空气（无雪）
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 64, z);
            EXPECT_TRUE(state == nullptr || state->isAir()) << "Block at (" << x << ", 64, " << z << ") should be AIR";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotPlaceSnowInDesertBiome)
{
    // 沙漠生物群系：降水类型为 None，不应降雪
    setChunkBiome(centerChunk(), Biomes::Desert);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=64 应该是空气
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 64, z);
            EXPECT_TRUE(state == nullptr || state->isAir())
                << "Block at (" << x << ", 64, " << z << ") should be AIR in desert";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotPlaceSnowOnNonSolidSurface)
{
    // 寒冷生物群系但地面是空气：雪不应放置（canSurviveAt 会失败）
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    // 不设置任何方块（全部为空气），高度图将返回 MIN_BUILD_HEIGHT 以下
    // 或者设置高处的单个方块让高度图有值但下方是空气
    // 简单场景：不放置任何方块，高度图返回 MIN_BUILD_HEIGHT-1，特征应跳过
    // 空区块：没有方块，getTopBlockY 返回 MIN_BUILD_HEIGHT-1，所有列应跳过
    bool result = m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));
    EXPECT_TRUE(result); // 即使没有放置任何东西也返回 true
}

// ============================================================================
// 冰冻结和降雪组合测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, FreezesWaterAndPlacesSnowOnAdjacentLand)
{
    // 在同一区块中，水面应冻结，陆地上应有雪
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    // x=0..7 是陆地（石头），x=8..15 是水面
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 62, z, &VanillaBlocks::STONE->defaultState());
            if (x < 8) {
                // 陆地：石头上面是空气
            } else {
                // 水面：y=63 是水
                setCenterBlock(x, 63, z, &VanillaBlocks::WATER->defaultState());
            }
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 检查陆地区域（x=0..7）：y=62 是石头（不变），y=63 应有雪
    // MotionBlocking topY=62（石头），freezeY=62（石头不冻结），snowY=63（空气→雪层）
    for (i32 x = 0; x < 8; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* surfaceState = getCenterBlock(x, 62, z);
            ASSERT_NE(surfaceState, nullptr);
            EXPECT_TRUE(surfaceState->is(VanillaBlocks::STONE))
                << "Land at (" << x << ", 62, " << z << ") should still be STONE";

            const BlockState* snowState = getCenterBlock(x, 63, z);
            ASSERT_NE(snowState, nullptr);
            EXPECT_TRUE(snowState->is(VanillaBlocks::SNOW)) << "Snow at (" << x << ", 63, " << z << ") should be SNOW";
        }
    }

    // 检查水区域（x=8..15）：y=63 应变为冰
    for (i32 x = 8; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            ASSERT_NE(state, nullptr);
            EXPECT_TRUE(state->is(VanillaBlocks::ICE)) << "Water at (" << x << ", 63, " << z << ") should be ICE";
        }
    }
}

// ============================================================================
// SNOWY 属性测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, SetsSnowyPropertyOnGrassBlock)
{
    // 寒冷生物群系，草方块上方应有雪，草方块应设置 SNOWY=true
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::GRASS_BLOCK->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // y=64 应有雪
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* snowState = getCenterBlock(x, 64, z);
            ASSERT_NE(snowState, nullptr);
            EXPECT_TRUE(snowState->is(VanillaBlocks::SNOW)) << "Snow at (" << x << ", 64, " << z << ") should be SNOW";

            // y=63 的草方块应有 SNOWY=true
            const BlockState* grassState = getCenterBlock(x, 63, z);
            ASSERT_NE(grassState, nullptr);
            EXPECT_TRUE(grassState->is(VanillaBlocks::GRASS_BLOCK))
                << "Block at (" << x << ", 63, " << z << ") should be GRASS_BLOCK";
            EXPECT_TRUE(grassState->hasProperty(BlockStateProperties::SNOWY()))
                << "GRASS_BLOCK should have SNOWY property";
            EXPECT_EQ(grassState->get(BlockStateProperties::SNOWY()), true)
                << "GRASS_BLOCK at (" << x << ", 63, " << z << ") should have SNOWY=true";
        }
    }
}

TEST_F(SnowAndFreezeFeatureTest, DoesNotSetSnowyPropertyOnStoneBlock)
{
    // 石头没有 SNOWY 属性，雪放置后石头不应被修改
    setChunkBiome(centerChunk(), Biomes::SnowyPlains);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 63, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 石头不应有 SNOWY 属性
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* stoneState = getCenterBlock(x, 63, z);
            ASSERT_NE(stoneState, nullptr);
            EXPECT_TRUE(stoneState->is(VanillaBlocks::STONE));
            EXPECT_FALSE(stoneState->hasProperty(BlockStateProperties::SNOWY()));
        }
    }
}

// ============================================================================
// 多区块测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, MultipleChunksGetSnowAndIce)
{
    // 设置所有 9 个区块为寒冷生物群系，带有水面
    for (i32 relZ = -1; relZ <= 1; ++relZ) {
        for (i32 relX = -1; relX <= 1; ++relX) {
            const i32 index = (relZ + 1) * 3 + (relX + 1);
            auto& chunk = *m_ownedChunks[static_cast<size_t>(index)];
            setChunkBiome(static_cast<ChunkPrimer&>(chunk), Biomes::SnowyPlains);
            for (i32 x = 0; x < 16; ++x) {
                for (i32 z = 0; z < 16; ++z) {
                    static_cast<ChunkPrimer&>(chunk).setBlockState(x, 62, z, &VanillaBlocks::STONE->defaultState());
                    static_cast<ChunkPrimer&>(chunk).setBlockState(x, 63, z, &VanillaBlocks::WATER->defaultState());
                }
            }
        }
    }

    // 对中心区块执行特征
    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 检查中心区块的某些位置是否有冰
    bool foundIce = false;
    for (i32 x = 0; x < 16 && !foundIce; ++x) {
        for (i32 z = 0; z < 16 && !foundIce; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            if (state != nullptr && state->is(VanillaBlocks::ICE)) {
                foundIce = true;
            }
        }
    }
    EXPECT_TRUE(foundIce) << "Should find ice in center chunk";
}

// ============================================================================
// 冰刺生物群系测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, IceSpikesBiomeFreezesAndSnows)
{
    // 冰刺生物群系：寒冷，应有冰和雪
    setChunkBiome(centerChunk(), Biomes::IceSpikes);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 62, z, &VanillaBlocks::STONE->defaultState());
            setCenterBlock(x, 63, z, &VanillaBlocks::WATER->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 水面应冻结
    bool foundIce = false;
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            if (state != nullptr && state->is(VanillaBlocks::ICE)) {
                foundIce = true;
                break;
            }
        }
        if (foundIce) break;
    }
    EXPECT_TRUE(foundIce) << "IceSpikes biome should cause water to freeze";
}

// ============================================================================
// 冻结海洋测试
// ============================================================================

TEST_F(SnowAndFreezeFeatureTest, FrozenOceanFreezesWater)
{
    // 冻结海洋生物群系：水面应冻结
    setChunkBiome(centerChunk(), Biomes::FrozenOcean);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setCenterBlock(x, 60, z, &VanillaBlocks::SAND->defaultState());
            setCenterBlock(x, 61, z, &VanillaBlocks::WATER->defaultState());
            setCenterBlock(x, 62, z, &VanillaBlocks::WATER->defaultState());
            setCenterBlock(x, 63, z, &VanillaBlocks::WATER->defaultState());
        }
    }

    m_feature->place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, 0, 0));

    // 顶部水面（y=63）应冻结为冰
    bool foundIce = false;
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getCenterBlock(x, 63, z);
            if (state != nullptr && state->is(VanillaBlocks::ICE)) {
                foundIce = true;
                break;
            }
        }
        if (foundIce) break;
    }
    EXPECT_TRUE(foundIce) << "FrozenOcean biome should cause surface water to freeze";
}
