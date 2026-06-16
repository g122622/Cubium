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

#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/structures/StrongholdPieces.hpp"

using namespace mc;
using namespace mc::block_registry;
using namespace mc::world::gen::structure;
using namespace mc::world::chunk;

// ============================================================================
// StrongholdPieceWeight 单元测试
// ============================================================================

TEST(StrongholdPieceWeightTest, UnlimitedType_CanAlwaysSpawn)
{
    // instancesLimit == 0 表示无限制，应始终可以生成
    StrongholdPieceWeight weight(StrongholdPieceTypes::STRAIGHT, 40, 0, 0);
    EXPECT_TRUE(weight.canSpawnMoreStructures());
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(0));
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(100));

    // 即使已生成多次，无限制类型仍可生成
    weight.instancesSpawned = 100;
    EXPECT_TRUE(weight.canSpawnMoreStructures());
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(0));
}

TEST(StrongholdPieceWeightTest, LimitedType_CanSpawnUntilLimit)
{
    // instancesLimit == 2，生成 2 次后不可再生成
    StrongholdPieceWeight weight(StrongholdPieceTypes::LIBRARY, 10, 2, 4);
    EXPECT_TRUE(weight.canSpawnMoreStructures());

    weight.instancesSpawned = 1;
    EXPECT_TRUE(weight.canSpawnMoreStructures());

    weight.instancesSpawned = 2;
    EXPECT_FALSE(weight.canSpawnMoreStructures());
}

TEST(StrongholdPieceWeightTest, DepthRestriction_BlocksShallowDepth)
{
    // Library: minDepth == 4，depth <= 4 时不可生成
    StrongholdPieceWeight library(StrongholdPieceTypes::LIBRARY, 10, 2, 4);
    EXPECT_FALSE(library.canSpawnMoreStructuresOfType(4));
    EXPECT_FALSE(library.canSpawnMoreStructuresOfType(3));
    EXPECT_FALSE(library.canSpawnMoreStructuresOfType(0));

    // depth > 4 时可以生成
    EXPECT_TRUE(library.canSpawnMoreStructuresOfType(5));
    EXPECT_TRUE(library.canSpawnMoreStructuresOfType(10));
}

TEST(StrongholdPieceWeightTest, DepthRestriction_PortalRoom)
{
    // PortalRoom: minDepth == 5，depth <= 5 时不可生成
    StrongholdPieceWeight portal(StrongholdPieceTypes::PORTAL_ROOM, 20, 1, 5);
    EXPECT_FALSE(portal.canSpawnMoreStructuresOfType(5));
    EXPECT_FALSE(portal.canSpawnMoreStructuresOfType(4));
    EXPECT_TRUE(portal.canSpawnMoreStructuresOfType(6));
}

TEST(StrongholdPieceWeightTest, NoMinDepth_AlwaysAllowed)
{
    // minDepth == 0 表示无深度限制
    StrongholdPieceWeight weight(StrongholdPieceTypes::STRAIGHT, 40, 0, 0);
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(0));
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(1));
    EXPECT_TRUE(weight.canSpawnMoreStructuresOfType(100));
}

// ============================================================================
// initializeStrongholdPieceWeights 测试
// ============================================================================

TEST(StrongholdPieceWeightTest, InitializeWeights_CorrectEntryCount)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);
    EXPECT_EQ(weights.size(), 11u);
}

TEST(StrongholdPieceWeightTest, InitializeWeights_AllInstancesSpawnedZero)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);
    for (const auto& w : weights) {
        EXPECT_EQ(w.instancesSpawned, 0);
    }
}

TEST(StrongholdPieceWeightTest, InitializeWeights_SpecificEntries)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    // 查找各关键条目并验证
    const StrongholdPieceWeight* straight = nullptr;
    const StrongholdPieceWeight* library = nullptr;
    const StrongholdPieceWeight* portal = nullptr;
    const StrongholdPieceWeight* prison = nullptr;

    for (const auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::STRAIGHT) straight = &w;
        if (w.pieceType == StrongholdPieceTypes::LIBRARY) library = &w;
        if (w.pieceType == StrongholdPieceTypes::PORTAL_ROOM) portal = &w;
        if (w.pieceType == StrongholdPieceTypes::PRISON) prison = &w;
    }

    ASSERT_NE(straight, nullptr);
    EXPECT_EQ(straight->weight, 40);
    EXPECT_EQ(straight->instancesLimit, 0); // 无限制
    EXPECT_EQ(straight->minDepth, 0);

    ASSERT_NE(library, nullptr);
    EXPECT_EQ(library->weight, 10);
    EXPECT_EQ(library->instancesLimit, 2);
    EXPECT_EQ(library->minDepth, 4);

    ASSERT_NE(portal, nullptr);
    EXPECT_EQ(portal->weight, 20);
    EXPECT_EQ(portal->instancesLimit, 1);
    EXPECT_EQ(portal->minDepth, 5);

    ASSERT_NE(prison, nullptr);
    EXPECT_EQ(prison->weight, 5);
    EXPECT_EQ(prison->instancesLimit, 5);
    EXPECT_EQ(prison->minDepth, 0);
}

// ============================================================================
// canAddStructurePieces 测试
// ============================================================================

TEST(StrongholdPieceWeightTest, CanAddStructurePieces_InitialWeights_ReturnsTrue)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    i32 totalWeight = 0;
    EXPECT_TRUE(canAddStructurePieces(weights, totalWeight));
    EXPECT_EQ(totalWeight, 145); // 40+5+20+20+10+5+5+5+5+10+20
}

TEST(StrongholdPieceWeightTest, CanAddStructurePieces_AllLimitedExhausted_ReturnsFalse)
{
    // 所有限制类型都达到上限时，canAdd 应返回 false
    // 即使无限类型（STRAIGHT, LEFT_TURN, RIGHT_TURN）仍可生成
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    // 将所有限制类型的 instancesSpawned 设为 instancesLimit
    for (auto& w : weights) {
        if (w.instancesLimit > 0) {
            w.instancesSpawned = w.instancesLimit;
        }
    }

    i32 totalWeight = 0;
    // MC Java 的 updatePieceWeight 在所有限制类型耗尽时也返回 false
    EXPECT_FALSE(canAddStructurePieces(weights, totalWeight));
    // 注意：canAddStructurePieces 只跳过 weight==0 的条目，
    // 当仅设置 instancesSpawned=instancesLimit 而未将 weight 设为 0 时，
    // 这些条目仍会贡献其权重到 totalWeight（这与 MC Java 一致，
    // 因为 MC Java 的 updatePieceWeight 也对列表中所有条目求和）
    EXPECT_EQ(totalWeight, 145);
}

TEST(StrongholdPieceWeightTest, CanAddStructurePieces_ZeroWeightSkipped)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    // 将 PRISON 的权重设为 0
    for (auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::PRISON) {
            w.weight = 0;
            break;
        }
    }

    i32 totalWeight = 0;
    EXPECT_TRUE(canAddStructurePieces(weights, totalWeight));
    // 总权重应减少 PRISON 的 5
    EXPECT_EQ(totalWeight, 140);
}

TEST(StrongholdPieceWeightTest, CanAddStructurePieces_SomeLimitedStillAvailable)
{
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    // 只将部分限制类型耗尽
    for (auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::PRISON) {
            w.instancesSpawned = w.instancesLimit; // PRISON 耗尽
        }
        if (w.pieceType == StrongholdPieceTypes::CROSSING) {
            w.instancesSpawned = w.instancesLimit; // CROSSING 耗尽
        }
    }

    i32 totalWeight = 0;
    EXPECT_TRUE(canAddStructurePieces(weights, totalWeight)); // 其他限制类型仍可用
}

// ============================================================================
// StrongholdStartStairs imposedPieceType 测试
// ============================================================================

TEST(StrongholdStartStairsTest, ImposedPieceType_DefaultIsNegativeOne)
{
    // 默认无强制片段
    math::Random rng(42);
    StrongholdStartStairs start(rng, 0, 0);
    EXPECT_EQ(start.imposedPieceType(), -1);
}

TEST(StrongholdStartStairsTest, ImposedPieceType_SetAndGet)
{
    math::Random rng(42);
    StrongholdStartStairs start(rng, 0, 0);

    start.setImposedPieceType(StrongholdPieceTypes::CROSSING);
    EXPECT_EQ(start.imposedPieceType(), StrongholdPieceTypes::CROSSING);

    start.setImposedPieceType(-1);
    EXPECT_EQ(start.imposedPieceType(), -1);
}

TEST(StrongholdStartStairsTest, WeightsInitializedInConstructor)
{
    math::Random rng(42);
    StrongholdStartStairs start(rng, 0, 0);

    const auto& weights = start.weights();
    EXPECT_EQ(weights.size(), 11u);

    // 验证所有 instancesSpawned 为 0
    for (const auto& w : weights) {
        EXPECT_EQ(w.instancesSpawned, 0);
    }
}

// ============================================================================
// generatePieceFromSmallDoor imposedPiece 测试
// ============================================================================

class GeneratePieceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        math::Random rng(42);
        m_start = std::make_unique<StrongholdStartStairs>(rng, 100, 200);
    }

    std::unique_ptr<StrongholdStartStairs> m_start;
    std::vector<std::unique_ptr<StructurePiece>> m_pieces;
};

TEST_F(GeneratePieceTest, ImposedPiece_CrossingIsCreated)
{
    // 设置强制片段类型为 CROSSING
    m_start->setImposedPieceType(StrongholdPieceTypes::CROSSING);

    math::Random rng(42);
    auto& weights = m_start->weights();
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);

    // 强制片段应被创建
    ASSERT_NE(piece, nullptr);
    EXPECT_EQ(piece->type(), StrongholdPieceTypes::CROSSING);

    // 强制片段类型应被消费
    EXPECT_EQ(m_start->imposedPieceType(), -1);

    // 对应权重的 instancesSpawned 应增加
    bool found = false;
    for (const auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::CROSSING) {
            EXPECT_EQ(w.instancesSpawned, 1);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GeneratePieceTest, ImposedPiece_ConsumedAfterUse)
{
    m_start->setImposedPieceType(StrongholdPieceTypes::CROSSING);

    math::Random rng(42);
    auto& weights = m_start->weights();
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    // 第一次调用应使用强制片段
    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);
    ASSERT_NE(piece, nullptr);
    EXPECT_EQ(m_start->imposedPieceType(), -1); // 已消费

    // 第二次调用应不再使用强制片段（正常随机选择）
    StrongholdPiece* piece2 = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 2, weights, lastPlaced);
    // piece2 可能为 nullptr（碰撞等原因）或为其他类型，但不应是强制片段
    if (piece2 != nullptr) {
        // 只要不是因为强制片段创建的就行
        EXPECT_EQ(m_start->imposedPieceType(), -1);
    }
}

TEST_F(GeneratePieceTest, ImposedPiece_WeightZeroedWhenLimitReached)
{
    // PortalRoom 的限制为 1，设置强制片段为 PORTAL_ROOM
    m_start->setImposedPieceType(StrongholdPieceTypes::PORTAL_ROOM);

    math::Random rng(42);
    auto& weights = m_start->weights();
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    // 先确保深度足够（PortalRoom minDepth == 5）
    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 10, weights, lastPlaced);

    if (piece != nullptr) {
        // 找到 PORTAL_ROOM 的权重
        for (const auto& w : weights) {
            if (w.pieceType == StrongholdPieceTypes::PORTAL_ROOM) {
                // instancesLimit == 1，已生成 1 个，权重应被设为 0
                EXPECT_EQ(w.instancesSpawned, 1);
                EXPECT_EQ(w.weight, 0);
            }
        }
    }
}

TEST_F(GeneratePieceTest, NoImposedPiece_NormalWeightSelection)
{
    // 不设置强制片段类型，应正常使用权重选择
    EXPECT_EQ(m_start->imposedPieceType(), -1);

    math::Random rng(42);
    auto& weights = m_start->weights();
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);

    // 不验证具体类型（随机），但应不是强制片段
    // 强制片段类型应保持 -1
    EXPECT_EQ(m_start->imposedPieceType(), -1);
}

// ============================================================================
// canAddStructurePieces 与 generatePieceFromSmallDoor 的交互测试
// ============================================================================

TEST_F(GeneratePieceTest, AllLimitedExhausted_ReturnsNullptr)
{
    auto& weights = m_start->weights();

    // 将所有限制类型的 instancesSpawned 设为 instancesLimit
    for (auto& w : weights) {
        if (w.instancesLimit > 0) {
            w.instancesSpawned = w.instancesLimit;
        }
    }

    math::Random rng(42);
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    // canAddStructurePieces 应返回 false
    i32 totalWeight = 0;
    EXPECT_FALSE(canAddStructurePieces(weights, totalWeight));

    // generatePieceFromSmallDoor 在无强制片段且 canAdd 返回 false 时应返回 nullptr
    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);
    EXPECT_EQ(piece, nullptr);
}

TEST_F(GeneratePieceTest, ImposedPieceOverridesExhaustedWeights)
{
    auto& weights = m_start->weights();

    // 将所有限制类型的 instancesSpawned 设为 instancesLimit
    for (auto& w : weights) {
        if (w.instancesLimit > 0) {
            w.instancesSpawned = w.instancesLimit;
        }
    }

    // 设置强制片段类型（即使 canAdd 返回 false，强制片段仍应被创建）
    m_start->setImposedPieceType(StrongholdPieceTypes::STRAIGHT);

    math::Random rng(42);
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    // 强制片段应在 canAdd 返回 false 时仍被创建
    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);

    // 强制片段类型为 STRAIGHT（无限制类型），应该能创建
    ASSERT_NE(piece, nullptr);
    EXPECT_EQ(piece->type(), StrongholdPieceTypes::STRAIGHT);
}

// ============================================================================
// 权重置零逻辑测试
// ============================================================================

TEST_F(GeneratePieceTest, WeightZeroedWhenLimitReached)
{
    auto& weights = m_start->weights();

    // 将 PRISON 的 instancesSpawned 设为比限制少 1
    for (auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::PRISON) {
            w.instancesSpawned = w.instancesLimit - 1; // 限制 5，已生成 4
            break;
        }
    }

    // 设置强制片段为 PRISON
    m_start->setImposedPieceType(StrongholdPieceTypes::PRISON);

    math::Random rng(42);
    StrongholdPieceWeight* lastPlaced = m_start->lastPlaced();

    StrongholdPiece* piece = generatePieceFromSmallDoor(
        m_start.get(), m_pieces, rng, 100, 50, 200, Direction::South, 1, weights, lastPlaced);

    if (piece != nullptr) {
        // PRISON 的 instancesSpawned 应该达到限制，权重应被置 0
        for (const auto& w : weights) {
            if (w.pieceType == StrongholdPieceTypes::PRISON) {
                EXPECT_EQ(w.instancesSpawned, w.instancesLimit);
                EXPECT_EQ(w.weight, 0);
            }
        }
    }
}

TEST_F(GeneratePieceTest, UnlimitedTypeWeightNeverZeroed)
{
    // STRAIGHT 是无限制类型，即使多次生成也不应将权重置 0
    auto& weights = m_start->weights();
    StrongholdPieceWeight* straightWeight = nullptr;
    for (auto& w : weights) {
        if (w.pieceType == StrongholdPieceTypes::STRAIGHT) {
            straightWeight = &w;
            break;
        }
    }
    ASSERT_NE(straightWeight, nullptr);
    EXPECT_EQ(straightWeight->instancesLimit, 0); // 无限制

    // 模拟多次生成
    straightWeight->instancesSpawned = 100;
    EXPECT_TRUE(straightWeight->canSpawnMoreStructures()); // 仍可生成
    EXPECT_EQ(straightWeight->weight, 40);                 // 权重不变
}

// ============================================================================
// StrongholdLibrary 第二个宝箱测试
// ============================================================================

class StrongholdLibraryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 创建 3x3 区块区域
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 用石头填充 y=0..70
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        for (i32 y = 0; y <= 70; ++y) {
                            chunk->setBlockState(x, y, z, &VanillaBlocks::STONE->defaultState());
                        }
                    }
                }

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(StrongholdLibraryTest, LargeRoom_HasTwoChests)
{
    // 大型图书馆 (maxY - minY > 6) 应有两个宝箱
    // m_isLargeRoom = (maxY - minY) > 6
    // 创建一个大型图书馆：minY=40, maxY=50 → 高度差 10 > 6
    math::Random rng(42);

    // StrongholdLibrary 构造函数签名
    StrongholdLibrary library(StrongholdPieceTypes::LIBRARY,
        rng,
        0,
        40,
        0, // minX, minY, minZ
        14,
        50,
        14, // maxX, maxY, maxZ
        Direction::South);

    // 验证是大型房间
    EXPECT_TRUE(library.isLargeRoom());

    // 生成结构
    StructureBoundingBox bounds(-16, 0, -16, 32, 80, 32);
    library.generate(*m_region, rng, 0, 0, bounds);

    // 第一个宝箱在 (3, 3, 5)（相对坐标）
    // 方向为南时，世界坐标 = minX + 3, minY + 3, minZ + 5 = 3, 43, 5
    // 实际坐标取决于坐标变换，这里验证是否有宝箱方块实体

    // 统计生成的宝箱数量
    i32 chestCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::CHEST)) {
                    chestCount++;
                }
            }
        }
    }

    // 大型图书馆应有 2 个宝箱
    EXPECT_EQ(chestCount, 2);
}

TEST_F(StrongholdLibraryTest, SmallRoom_HasOneChest)
{
    // 小型图书馆 (maxY - minY <= 6) 应只有一个宝箱
    math::Random rng(42);

    // minY=40, maxY=46 → 高度差 6，不大于 6，所以是小房间
    StrongholdLibrary library(StrongholdPieceTypes::LIBRARY, rng, 0, 40, 0, 14, 46, 14, Direction::South);

    // 验证不是大型房间
    EXPECT_FALSE(library.isLargeRoom());

    StructureBoundingBox bounds(-16, 0, -16, 32, 80, 32);
    library.generate(*m_region, rng, 0, 0, bounds);

    // 统计生成的宝箱数量
    i32 chestCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::CHEST)) {
                    chestCount++;
                }
            }
        }
    }

    // 小型图书馆应有 1 个宝箱
    EXPECT_EQ(chestCount, 1);
}

TEST_F(StrongholdLibraryTest, LargeRoomChestsHaveLootTables)
{
    // 验证大型图书馆的宝箱都设置了正确的战利品表
    math::Random rng(42);

    StrongholdLibrary library(StrongholdPieceTypes::LIBRARY, rng, 0, 40, 0, 14, 50, 14, Direction::South);

    StructureBoundingBox bounds(-16, 0, -16, 32, 80, 32);
    library.generate(*m_region, rng, 0, 0, bounds);

    // 检查所有宝箱方块实体的战利品表
    ResourceLocation expectedLootTable("minecraft", "chests/stronghold_library");
    i32 chestsWithLootTable = 0;

    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::CHEST)) {
                    BlockEntity* entity = m_region->getBlockEntity(BlockPos(x, y, z));
                    if (entity != nullptr && entity->getType() == BlockEntityType::Chest) {
                        auto* chestEntity = static_cast<blockentity::ChestEntity*>(entity);
                        if (chestEntity->getLootTable() == expectedLootTable) {
                            chestsWithLootTable++;
                        }
                    }
                }
            }
        }
    }

    // 两个宝箱都应有正确的战利品表
    EXPECT_EQ(chestsWithLootTable, 2);
}
