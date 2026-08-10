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

#include "common/TestWorldHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/structures/JungleTempleStructure.hpp"

using namespace mc;
using namespace mc::block_registry;
using namespace mc::world::gen::structure;
using namespace mc::world::chunk;

// ============================================================================
// 测试夹具：创建包含 WorldGenRegion 的测试环境
// ============================================================================

class StructurePieceChestDispenserTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 创建 3x3 区块区域（区块半径 1），以 (0,0) 为中心
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 用石头填充 y=60..70 的基础地形
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        for (i32 y = 60; y <= 70; ++y) {
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

// ============================================================================
// WorldGenRegion::getBlockEntity / setBlockEntity / removeBlockEntity 测试
// ============================================================================

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_GetBlockEntityReturnsNullptrForEmpty)
{
    // 位置 (5, 65, 5) 是石头，没有方块实体
    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_SetBlockStateCreatesBlockEntityForChest)
{
    // 放置一个宝箱方块
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    BlockState orientedState = chestState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    bool result = m_region->setBlockState(5, 65, 5, &orientedState);
    EXPECT_TRUE(result);

    // WorldGenRegion::setBlockState 应该自动创建方块实体
    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Chest);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_SetBlockStateCreatesBlockEntityForDispenser)
{
    // 放置一个发射器方块
    const BlockState* dispenserState = VanillaBlocks::getState(VanillaBlocks::DISPENSER);
    ASSERT_NE(dispenserState, nullptr);

    BlockState orientedState = dispenserState->with(BlockStateProperties::FACING(), Direction::Down);
    bool result = m_region->setBlockState(5, 65, 5, &orientedState);
    EXPECT_TRUE(result);

    // WorldGenRegion::setBlockState 应该自动创建方块实体
    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Dispenser);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_SetBlockEntityManually)
{
    // 先放一个宝箱方块（这会自动创建 BlockEntity）
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    BlockState orientedState = chestState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    m_region->setBlockState(5, 65, 5, &orientedState);

    BlockPos pos(5, 65, 5);
    BlockEntity* existingEntity = m_region->getBlockEntity(pos);
    ASSERT_NE(existingEntity, nullptr);

    // 手动设置新的方块实体（替换）
    auto newEntity = std::make_unique<blockentity::ChestEntity>(pos);
    BlockEntity* newEntityPtr = newEntity.get();
    m_region->setBlockEntity(pos, newEntity.release());

    // 验证可以获取到新的方块实体
    BlockEntity* retrievedEntity = m_region->getBlockEntity(pos);
    ASSERT_NE(retrievedEntity, nullptr);
    EXPECT_EQ(retrievedEntity->getType(), BlockEntityType::Chest);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_RemoveBlockEntity)
{
    // 先放一个宝箱方块
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    BlockState orientedState = chestState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    m_region->setBlockState(5, 65, 5, &orientedState);

    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);

    // 移除方块实体
    m_region->removeBlockEntity(pos);

    // 验证方块实体已被移除
    BlockEntity* removedEntity = m_region->getBlockEntity(pos);
    EXPECT_EQ(removedEntity, nullptr);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_ChestLootTableCanBeSet)
{
    // 放置宝箱
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    BlockState orientedState = chestState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    m_region->setBlockState(5, 65, 5, &orientedState);

    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    ASSERT_EQ(entity->getType(), BlockEntityType::Chest);

    // 设置战利品表
    auto* chestEntity = static_cast<blockentity::ChestEntity*>(entity);
    ResourceLocation lootTable("minecraft", "chests/jungle_temple");
    chestEntity->setLootTable(lootTable, 12345LL);

    // 验证战利品表已设置
    EXPECT_EQ(chestEntity->getLootTable(), lootTable);
    EXPECT_EQ(chestEntity->getLootTableSeed(), 12345LL);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_DispenserLootTableCanBeSet)
{
    // 放置发射器
    const BlockState* dispenserState = VanillaBlocks::getState(VanillaBlocks::DISPENSER);
    ASSERT_NE(dispenserState, nullptr);

    BlockState orientedState = dispenserState->with(BlockStateProperties::FACING(), Direction::Down);
    m_region->setBlockState(5, 65, 5, &orientedState);

    BlockPos pos(5, 65, 5);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    ASSERT_EQ(entity->getType(), BlockEntityType::Dispenser);

    // 设置战利品表
    auto* dispenserEntity = static_cast<blockentity::DispenserBlockEntity*>(entity);
    ResourceLocation lootTable("minecraft", "chests/jungle_temple_dispenser");
    dispenserEntity->setLootTable(lootTable, 67890LL);

    // 验证战利品表已设置
    EXPECT_EQ(dispenserEntity->getLootTable(), lootTable);
    EXPECT_EQ(dispenserEntity->getLootTableSeed(), 67890LL);
}

// ============================================================================
// generateChest / generateDispenser 集成测试
// ============================================================================

namespace {

/**
 * @brief 测试用 StructurePiece 子类，暴露 generateChest/generateDispenser
 */
class TestStructurePiece : public StructurePiece {
public:
    TestStructurePiece(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
        : StructurePiece(0, minX, minY, minZ, maxX, maxY, maxZ)
    {}

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        world::chunk::ChunkPrimer* /*chunk*/,
        IChunkGenerator* /*generator*/) override
    {
        MC_UNUSED(world);
        MC_UNUSED(rng);
        MC_UNUSED(chunkX);
        MC_UNUSED(chunkZ);
        MC_UNUSED(chunkBounds);
    }

    // 暴露基类的 generateChest 和 generateDispenser 方法
    using StructurePiece::generateChest;
    using StructurePiece::generateDispenser;
    using StructurePiece::reorientChest;
};

} // namespace

TEST_F(StructurePieceChestDispenserTest, GenerateChestPlacesBlockAndSetsLootTable)
{
    // 创建一个测试片段，位置在 (5, 65, 5) 到 (20, 80, 20)
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    // 使用 Direction::South 使得坐标变换为加法偏移
    // getXWithOffset(x, z) = m_minX + x = 5 + x
    // getYWithOffset(y) = y + m_minY = y + 65
    // getZWithOffset(x, z) = m_minZ + z = 5 + z
    piece.setCoordBaseMode(Direction::South);

    // 生成边界覆盖整个片段
    StructureBoundingBox bounds(0, 0, 0, 30, 100, 30);
    math::Random rng(42);

    // 在相对位置 (2, 0, 2) 放置宝箱（世界坐标 = 5+2, 0+65, 5+2 = 7, 65, 7）
    piece.generateChest(
        *m_region, bounds, rng, 2, 0, 2, Direction::South, ResourceLocation("minecraft", "chests/jungle_temple"));

    // 验证宝箱方块已放置
    const BlockState* state = m_region->getBlockState(7, 65, 7);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::CHEST));

    // 验证方块实体已创建且战利品表已设置
    BlockPos chestPos(7, 65, 7);
    BlockEntity* entity = m_region->getBlockEntity(chestPos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Chest);

    auto* chestEntity = static_cast<blockentity::ChestEntity*>(entity);
    EXPECT_EQ(chestEntity->getLootTable(), ResourceLocation("minecraft", "chests/jungle_temple"));
}

TEST_F(StructurePieceChestDispenserTest, GenerateDispenserPlacesBlockAndSetsLootTable)
{
    // 创建一个测试片段
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    piece.setCoordBaseMode(Direction::South);

    StructureBoundingBox bounds(0, 0, 0, 30, 100, 30);
    math::Random rng(42);

    // 在相对位置 (3, 1, 3) 放置发射器（世界坐标 = 5+3, 1+65, 5+3 = 8, 66, 8）
    piece.generateDispenser(*m_region,
        bounds,
        rng,
        3,
        1,
        3,
        Direction::Down,
        ResourceLocation("minecraft", "chests/jungle_temple_dispenser"));

    // 验证发射器方块已放置
    const BlockState* state = m_region->getBlockState(8, 66, 8);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::DISPENSER));

    // 验证方块实体已创建且战利品表已设置
    BlockPos dispenserPos(8, 66, 8);
    BlockEntity* entity = m_region->getBlockEntity(dispenserPos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Dispenser);

    auto* dispenserEntity = static_cast<blockentity::DispenserBlockEntity*>(entity);
    EXPECT_EQ(dispenserEntity->getLootTable(), ResourceLocation("minecraft", "chests/jungle_temple_dispenser"));
}

TEST_F(StructurePieceChestDispenserTest, GenerateChestOutOfBoundsDoesNothing)
{
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    piece.setCoordBaseMode(Direction::South);

    // 边界框不包含目标位置（bounds 在 50-60 范围，而 piece 的世界坐标在 5-25 范围）
    StructureBoundingBox bounds(50, 0, 50, 60, 100, 60);
    math::Random rng(42);

    // 相对位置 (2, 0, 2) → 世界坐标 (7, 65, 7)，不在 bounds 中
    piece.generateChest(
        *m_region, bounds, rng, 2, 0, 2, Direction::South, ResourceLocation("minecraft", "chests/simple_dungeon"));

    // 方块应未改变（还是石头）
    const BlockState* state = m_region->getBlockState(7, 65, 7);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::STONE));
}

TEST_F(StructurePieceChestDispenserTest, GenerateDispenserOutOfBoundDoesNothing)
{
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    piece.setCoordBaseMode(Direction::South);

    // 边界框不包含目标位置
    StructureBoundingBox bounds(50, 0, 50, 60, 100, 60);
    math::Random rng(42);

    piece.generateDispenser(*m_region,
        bounds,
        rng,
        3,
        1,
        3,
        Direction::Down,
        ResourceLocation("minecraft", "chests/jungle_temple_dispenser"));

    // 方块应未改变（还是石头）
    const BlockState* state = m_region->getBlockState(8, 66, 8);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::STONE));
}

TEST_F(StructurePieceChestDispenserTest, GenerateChestReplacesExistingBlock)
{
    // 先在目标位置放石头（已在 SetUp 中放置）
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    piece.setCoordBaseMode(Direction::South);

    StructureBoundingBox bounds(0, 0, 0, 30, 100, 30);
    math::Random rng(42);

    // 目标位置 (0, 0, 0) → 世界坐标 (5, 65, 5)，原本是石头
    const BlockState* beforeState = m_region->getBlockState(5, 65, 5);
    ASSERT_NE(beforeState, nullptr);
    EXPECT_TRUE(beforeState->is(VanillaBlocks::STONE));

    // 在相对位置 (0, 0, 0) 放置宝箱
    piece.generateChest(
        *m_region, bounds, rng, 0, 0, 0, Direction::West, ResourceLocation("minecraft", "chests/spawn_bonus_chest"));

    // 验证宝箱已替换石头
    const BlockState* afterState = m_region->getBlockState(5, 65, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_TRUE(afterState->is(VanillaBlocks::CHEST));
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_GetBlockEntityOutsideRegionReturnsNullptr)
{
    // 请求区域外的方块实体应返回 nullptr
    BlockPos outsidePos(100, 65, 100); // 超出 3x3 区块区域
    BlockEntity* entity = m_region->getBlockEntity(outsidePos);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_RemoveBlockEntityOutsideRegionDoesNotCrash)
{
    // 移除区域外的方块实体不应崩溃
    BlockPos outsidePos(100, 65, 100);
    m_region->removeBlockEntity(outsidePos); // 不应崩溃
}

TEST_F(StructurePieceChestDispenserTest, WorldGenRegion_SetBlockEntityOutsideRegionDoesNotCrash)
{
    // 设置区域外的方块实体不应崩溃
    BlockPos outsidePos(100, 65, 100);
    auto entity = std::make_unique<blockentity::ChestEntity>(outsidePos);
    m_region->setBlockEntity(outsidePos, entity.release()); // 不应崩溃
}

// ============================================================================
// reorientChest 测试
// ============================================================================

TEST_F(StructurePieceChestDispenserTest, ReorientChest_NoAdjacentBlocks_ReturnsNorthFacing)
{
    // 没有相邻方块时，宝箱默认朝向北
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    // 清除宝箱位置及其四个水平邻居的石头（SetUp 填充了 y=60..70 的石头）
    const BlockState* airState = &BaseBlocks::AIR->defaultState();
    BlockPos chestPos(5, 65, 5);
    m_region->setBlockState(5, 65, 5, airState, 2);
    m_region->setBlockState(4, 65, 5, airState, 2);
    m_region->setBlockState(6, 65, 5, airState, 2);
    m_region->setBlockState(5, 65, 4, airState, 2);
    m_region->setBlockState(5, 65, 6, airState, 2);

    // 在空气中放置宝箱（周围都是空气）
    m_region->setBlockState(5, 65, 5, chestState, 2);

    const BlockState* result = StructurePiece::reorientChest(*m_region, chestPos, chestState);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is(VanillaBlocks::CHEST));

    // 没有相邻实心方块，应返回朝北的宝箱
    auto facing = result->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::North);
}

TEST_F(StructurePieceChestDispenserTest, ReorientChest_OneSolidBlock_FacesAwayFromSolid)
{
    // 北侧有一个实心方块时，宝箱应朝向南（远离实心方块）
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    // 清除宝箱位置及其四个水平邻居的石头（SetUp 填充了 y=60..70 的石头）
    const BlockState* airState = &BaseBlocks::AIR->defaultState();
    BlockPos chestPos(5, 65, 5);
    m_region->setBlockState(5, 65, 5, airState, 2);
    m_region->setBlockState(4, 65, 5, airState, 2);
    m_region->setBlockState(6, 65, 5, airState, 2);
    m_region->setBlockState(5, 65, 4, airState, 2);
    m_region->setBlockState(5, 65, 6, airState, 2);

    // 只在宝箱北侧放置石头（宝箱在 (5, 65, 5)，北侧是 (5, 65, 4)）
    m_region->setBlockState(5, 65, 4, &VanillaBlocks::STONE->defaultState(), 2);

    const BlockState* result = StructurePiece::reorientChest(*m_region, chestPos, chestState);
    ASSERT_NE(result, nullptr);

    auto facing = result->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::South);
}

TEST_F(StructurePieceChestDispenserTest, ReorientChest_TwoSolidBlocks_KeepsDefaultFacing)
{
    // 两个方向有实心方块时，进入回退逻辑，从默认朝向开始寻找非实心方向
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    // 清除宝箱位置及其四个水平邻居的石头（SetUp 填充了 y=60..70 的石头）
    const BlockState* airState = &BaseBlocks::AIR->defaultState();
    BlockPos chestPos(5, 65, 5);
    m_region->setBlockState(5, 65, 5, airState, 2);
    m_region->setBlockState(4, 65, 5, airState, 2);
    m_region->setBlockState(6, 65, 5, airState, 2);
    m_region->setBlockState(5, 65, 4, airState, 2);
    m_region->setBlockState(5, 65, 6, airState, 2);

    // 北侧和南侧都放置石头，东西两侧是空气
    m_region->setBlockState(5, 65, 4, &VanillaBlocks::STONE->defaultState(), 2);
    m_region->setBlockState(5, 65, 6, &VanillaBlocks::STONE->defaultState(), 2);

    const BlockState* result = StructurePiece::reorientChest(*m_region, chestPos, chestState);
    ASSERT_NE(result, nullptr);

    // 两个实心方向（北和南），回退逻辑：
    // 1. 默认朝向北 → 北侧实心 → 翻转为南
    // 2. 南侧也实心 → 顺时针旋转为西
    // 3. 西侧是空气 → 停止，最终朝向为西
    auto facing = result->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::West);
}

TEST_F(StructurePieceChestDispenserTest, ReorientChest_AdjacentChest_KeepsDefaultFacing)
{
    // 相邻有宝箱时（用于双箱合并），保持默认朝向
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    ASSERT_NE(chestState, nullptr);

    // 清除宝箱位置及其四个水平邻居的石头（SetUp 填充了 y=60..70 的石头）
    const BlockState* airState = &BaseBlocks::AIR->defaultState();
    BlockPos chestPos(5, 65, 5);
    m_region->setBlockState(5, 65, 5, airState, 2);
    m_region->setBlockState(4, 65, 5, airState, 2);
    m_region->setBlockState(6, 65, 5, airState, 2);
    m_region->setBlockState(5, 65, 4, airState, 2);
    m_region->setBlockState(5, 65, 6, airState, 2);

    // 在宝箱东侧放置另一个宝箱
    m_region->setBlockState(6, 65, 5, chestState, 2);

    const BlockState* result = StructurePiece::reorientChest(*m_region, chestPos, chestState);
    ASSERT_NE(result, nullptr);

    // 相邻有宝箱，保持默认状态
    EXPECT_EQ(result, chestState);
}

TEST_F(StructurePieceChestDispenserTest, ReorientChest_NullDefaultState_ReturnsNullptr)
{
    // 传入 nullptr 的 defaultState 应返回 nullptr
    BlockPos chestPos(5, 65, 5);
    const BlockState* result = StructurePiece::reorientChest(*m_region, chestPos, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(StructurePieceChestDispenserTest, AutoFacingGenerateChest_PlacesChestWithCorrectOrientation)
{
    // 测试自动朝向版本的 generateChest
    TestStructurePiece piece(5, 65, 5, 20, 80, 20);
    piece.setCoordBaseMode(Direction::South);

    StructureBoundingBox bounds(0, 0, 0, 30, 100, 30);
    math::Random rng(42);

    // 清除宝箱位置及其四个水平邻居的石头（SetUp 填充了 y=60..70 的石头）
    const BlockState* airState = &BaseBlocks::AIR->defaultState();
    m_region->setBlockState(5, 65, 5, airState, 2);
    m_region->setBlockState(4, 65, 5, airState, 2);
    m_region->setBlockState(6, 65, 5, airState, 2);
    m_region->setBlockState(5, 65, 4, airState, 2);
    m_region->setBlockState(5, 65, 6, airState, 2);

    // 只在宝箱北侧放置石头，宝箱应朝向南
    m_region->setBlockState(5, 65, 4, &VanillaBlocks::STONE->defaultState(), 2);

    // 使用自动朝向版本的 generateChest
    piece.generateChest(*m_region, bounds, rng, 0, 0, 0, ResourceLocation("minecraft", "chests/stronghold_crossing"));

    // 验证宝箱已放置
    const BlockState* afterState = m_region->getBlockState(5, 65, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_TRUE(afterState->is(VanillaBlocks::CHEST));

    // 验证宝箱朝向南（远离北侧的石头）
    auto facing = afterState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::South);

    // 验证战利品表已设置
    BlockEntity* entity = m_region->getBlockEntity(BlockPos(5, 65, 5));
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Chest);
}
