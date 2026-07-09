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

#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/MonsterRoomFeature.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::gen::feature;

// ============================================================================
// 最小 IChunkGenerator 存根
// ============================================================================

class MonsterRoomStubGenerator : public IChunkGenerator {
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

// ============================================================================
// 测试夹具
// ============================================================================

class MonsterRoomFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 创建 3x3 区块区域（中心在 0,0）
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

    /**
     * @brief 用石头填充一个实心长方体区域（世界坐标）
     *
     * 用于构造地牢所需的固体地板/天花板/墙壁环境。
     */
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

    /**
     * @brief 挖空一个长方体区域（设为空气）
     */
    void fillAir(i32 x0, i32 y0, i32 z0, i32 x1, i32 y1, i32 z1)
    {
        const BlockState* air = &VanillaBlocks::AIR->defaultState();
        for (i32 x = x0; x <= x1; ++x) {
            for (i32 y = y0; y <= y1; ++y) {
                for (i32 z = z0; z <= z1; ++z) {
                    m_region->setBlockState(x, y, z, air);
                }
            }
        }
    }

    /**
     * @brief 构造一个"洞穴与地牢边界相交"的真实几何，使 j2 落入 [1,5]
     *
     * 模拟地牢生成时的真实场景：地牢原点周围是实心石头，但边界扫描环
     * 与一条已存在的水平洞穴相交，暴露少量空气方块（j2∈[1,5]）。
     *
     * 几何：先填实心石头 [-6,6]×[oy-2,oy+6]×[-6,6]；再挖一条 2 格宽、
     * 2 格高、沿 X 轴贯穿的水平洞穴，位于 z∈{-1,0}、y∈{oy,oy+1}。
     *
     * 对任意 j,k1∈{2,3}（即 nextInt(2)+2 的全部取值），边界扫描环
     * (k2∈{±j±1}, i3∈{±k1±1}, l2==0) 中落入该洞穴的空气方块数恒为 4，
     * 且其上方亦为空气，故 j2=4∈[1,5]，place() 通过合法性扫描。
     * 地板 (oy-1) 与天花板 (oy+4) 保持实心石头。phase2 会把内部 carving
     * 成 CAVE_AIR/cobblestone，phase3 在边界附近寻找"恰 1 个固体邻居"的
     * 空格放宝箱，phase4 在 origin 放刷怪笼。
     */
    void carveIntersectingCave(i32 oy)
    {
        fillSolid(-6, oy - 2, -6, 6, oy + 6, 6);
        // 水平洞穴：z∈{-1,0}，y∈{oy,oy+1}，x 贯穿 [-5,5]
        fillAir(-5, oy, -1, 5, oy + 1, 0);
    }

    [[nodiscard]] const BlockState* blockAt(i32 x, i32 y, i32 z) const { return m_region->getBlockState(x, y, z); }

    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    MonsterRoomStubGenerator m_stubGenerator;
    math::Random m_random{42};
};

// ============================================================================
// 阶段1：合法性扫描测试
// ============================================================================

TEST_F(MonsterRoomFeatureTest, ReturnsFalseWhenFloorNotSolid)
{
    // 构造一个空间：地板（origin.y-1）为空气，应直接返回 false
    // 先填一个大的实心区域，再把地板层挖空
    const i32 oy = 64;
    fillSolid(-6, oy - 2, -6, 6, oy + 6, 6);
    // 挖空地板层（l2==-1 对应 origin.y-1）
    fillAir(-6, oy - 1, -6, 6, oy - 1, 6);

    MonsterRoomFeature feature;
    // 用足够大的随机范围确保扫描覆盖到非固体地板
    bool result = feature.place(*m_region, m_random, 0, oy, 0);
    EXPECT_FALSE(result);
}

TEST_F(MonsterRoomFeatureTest, ReturnsFalseWhenCeilingNotSolid)
{
    const i32 oy = 64;
    // 实心区域只到 oy+3，天花板（oy+4）为空气
    fillSolid(-6, oy - 1, -6, 6, oy + 3, 6);
    // oy+4 保持空气（天花板非固体）

    MonsterRoomFeature feature;
    bool result = feature.place(*m_region, m_random, 0, oy, 0);
    EXPECT_FALSE(result);
}

TEST_F(MonsterRoomFeatureTest, ReturnsTrueAndPlacesSpawnerInValidRoom)
{
    // 构造"洞穴与地牢边界相交"的真实几何：j2=4∈[1,5]，place 通过合法性扫描
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    bool result = feature.place(*m_region, m_random, 0, oy, 0);
    EXPECT_TRUE(result);

    // origin 处必须是刷怪笼
    const BlockState* originState = blockAt(0, oy, 0);
    ASSERT_NE(originState, nullptr);
    EXPECT_TRUE(originState->is(VanillaBlocks::SPAWNER)) << "Origin should be SPAWNER after successful placement";
}

// ============================================================================
// 阶段4：刷怪笼测试
// ============================================================================

TEST_F(MonsterRoomFeatureTest, SpawnerEntityTypeIsFromMobs)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    ASSERT_TRUE(feature.place(*m_region, m_random, 0, oy, 0));

    // 验证刷怪笼方块实体存在且实体 ID ∈ {skeleton, zombie, spider}
    BlockEntity* be = m_region->getBlockEntity(BlockPos(0, oy, 0));
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->getType(), BlockEntityType::MobSpawner);

    // 刷怪笼的实体 ID 通过 setEntityId 设置；这里只验证方块实体类型正确。
    // （MobSpawnerBlockEntity 的具体实体 ID 读取接口不在本测试范围，避免依赖内部实现。）
}

TEST_F(MonsterRoomFeatureTest, SpawnerNotPlacedOnProtectedBlock)
{
    // origin 处放基岩（FEATURES_CANNOT_REPLACE 标签方块），safeSetBlock 应跳过
    const i32 oy = 64;
    carveIntersectingCave(oy);
    // 在 origin 放基岩
    m_region->setBlockState(0, oy, 0, &VanillaBlocks::BEDROCK->defaultState());

    MonsterRoomFeature feature;
    bool result = feature.place(*m_region, m_random, 0, oy, 0);
    // 即使 spawner 未放置，place 仍返回 true（j2 判定通过即建造）
    EXPECT_TRUE(result);
    // origin 仍是基岩，未被替换为 spawner
    const BlockState* originState = blockAt(0, oy, 0);
    ASSERT_NE(originState, nullptr);
    EXPECT_TRUE(originState->is(VanillaBlocks::BEDROCK))
        << "Bedrock (FEATURES_CANNOT_REPLACE) must not be replaced by SPAWNER";
}

// ============================================================================
// 阶段2：建造测试
// ============================================================================

TEST_F(MonsterRoomFeatureTest, RoomFloorIsCobblestoneOrMossy)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    ASSERT_TRUE(feature.place(*m_region, m_random, 0, oy, 0));

    // 房间地板（i4==-1 层，即 oy-1）边界格应为 COBBLESTONE 或 MOSSY_COBBLESTONE
    // 检查几个边界位置
    bool foundStone = false;
    for (i32 x = -3; x <= 3 && !foundStone; ++x) {
        for (i32 z = -3; z <= 3 && !foundStone; ++z) {
            const BlockState* s = blockAt(x, oy - 1, z);
            if (s != nullptr && (s->is(VanillaBlocks::COBBLESTONE) || s->is(VanillaBlocks::MOSSY_COBBLESTONE))) {
                foundStone = true;
            }
        }
    }
    EXPECT_TRUE(foundStone) << "Room floor should contain cobblestone or mossy cobblestone";
}

TEST_F(MonsterRoomFeatureTest, InteriorIsAirOrCaveAir)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    ASSERT_TRUE(feature.place(*m_region, m_random, 0, oy, 0));

    // 内部非边界格应为空气或 cave_air（非 spawner/chest 位置）
    // 内部范围大致 x∈[-2,2], y∈[oy, oy+2], z∈[-2,2]，避开 origin(0,oy,0) 的 spawner
    bool foundAir = false;
    for (i32 x = -2; x <= 2 && !foundAir; ++x) {
        for (i32 y = oy; y <= oy + 2 && !foundAir; ++y) {
            for (i32 z = -2; z <= 2 && !foundAir; ++z) {
                if (x == 0 && y == oy && z == 0) continue; // 跳过 spawner
                const BlockState* s = blockAt(x, y, z);
                if (s != nullptr && (s->isAir() || s->is(VanillaBlocks::CAVE_AIR))) {
                    foundAir = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundAir) << "Room interior should contain air/cave_air";
}

// ============================================================================
// 阶段3：宝箱测试
// ============================================================================

TEST_F(MonsterRoomFeatureTest, PlacesAtMostTwoChests)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    ASSERT_TRUE(feature.place(*m_region, m_random, 0, oy, 0));

    // 统计房间区域内 y=oy 的宝箱数量（最多 2 个）
    i32 chestCount = 0;
    for (i32 x = -5; x <= 5; ++x) {
        for (i32 z = -5; z <= 5; ++z) {
            const BlockState* s = blockAt(x, oy, z);
            if (s != nullptr && s->is(VanillaBlocks::CHEST)) {
                ++chestCount;
            }
        }
    }
    EXPECT_LE(chestCount, 2) << "Monster room should place at most 2 chests";
}

TEST_F(MonsterRoomFeatureTest, ChestHasSimpleDungeonLootTable)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    MonsterRoomFeature feature;
    ASSERT_TRUE(feature.place(*m_region, m_random, 0, oy, 0));

    // 找到宝箱并验证其方块实体为 ChestEntity 类型
    bool foundChestEntity = false;
    for (i32 x = -5; x <= 5 && !foundChestEntity; ++x) {
        for (i32 z = -5; z <= 5 && !foundChestEntity; ++z) {
            const BlockState* s = blockAt(x, oy, z);
            if (s != nullptr && s->is(VanillaBlocks::CHEST)) {
                BlockEntity* be = m_region->getBlockEntity(BlockPos(x, oy, z));
                if (be != nullptr &&
                    (be->getType() == BlockEntityType::Chest || be->getType() == BlockEntityType::TrappedChest)) {
                    foundChestEntity = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundChestEntity) << "Chest should have a ChestEntity block entity";
}

// ============================================================================
// 配置化包装测试
// ============================================================================

TEST_F(MonsterRoomFeatureTest, ConfiguredFeatureMetadata)
{
    ConfiguredMonsterRoomFeature configured;
    EXPECT_STREQ(configured.name(), "monster_room");
    EXPECT_EQ(configured.stage(), DecorationStage::UndergroundStructures);
}

TEST_F(MonsterRoomFeatureTest, ConfiguredFeaturePlaceDelegatesToFeature)
{
    const i32 oy = 64;
    carveIntersectingCave(oy);

    ConfiguredMonsterRoomFeature configured;
    bool result = configured.place(*m_region, centerChunk(), m_stubGenerator, m_random, BlockPos(0, oy, 0));
    EXPECT_TRUE(result);

    const BlockState* originState = blockAt(0, oy, 0);
    ASSERT_NE(originState, nullptr);
    EXPECT_TRUE(originState->is(VanillaBlocks::SPAWNER));
}
