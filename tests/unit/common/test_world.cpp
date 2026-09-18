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

#include <cmath>
#include <gtest/gtest.h>

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

using namespace mc;
using namespace mc::world;

// ============================================================================
// 测试固件
// ============================================================================

class ChunkTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// ChunkSection 测试
// ============================================================================

TEST_F(ChunkTest, ChunkSection_Construction)
{
    ChunkSection section;
    EXPECT_EQ(section.getBlockCount(), 0);
    EXPECT_TRUE(section.isEmpty());
}

TEST_F(ChunkTest, ChunkSection_SetGetBlock)
{
    ChunkSection section;

    // 设置方块
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    section.setBlockState(5, 10, 7, stoneState);
    EXPECT_EQ(section.getBlockCount(), 1);
    EXPECT_FALSE(section.isEmpty());

    // 获取方块
    const BlockState* block = section.getBlockState(5, 10, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());

    // 边界检查 - 返回空气
    const BlockState* outOfBounds = section.getBlockState(16, 0, 0);
    EXPECT_EQ(outOfBounds, nullptr);
}

TEST_F(ChunkTest, ChunkSection_FastAccess)
{
    ChunkSection section;
    i32 index = ChunkSection::blockIndex(3, 5, 7);

    u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
    section.setBlockStateIdFast(index, dirtStateId);

    const BlockState* block = section.getBlockState(3, 5, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::DIRT->blockId());
}

TEST_F(ChunkTest, ChunkSection_BlockCount)
{
    ChunkSection section;

    // 添加方块
    section.setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    section.setBlockState(1, 1, 1, &VanillaBlocks::DIRT->defaultState());
    EXPECT_EQ(section.getBlockCount(), 2);

    // 替换方块 (不增加计数)
    section.setBlockState(0, 0, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_EQ(section.getBlockCount(), 2);

    // 移除方块 (设为空气)
    section.setBlockState(0, 0, 0, &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(section.getBlockCount(), 1);

    // 再次移除 (计数不变)
    section.setBlockState(0, 0, 0, &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(section.getBlockCount(), 1);
}

TEST_F(ChunkTest, ChunkSection_RandomTickCounters)
{
    ChunkSection section;

    EXPECT_FALSE(section.needsRandomTickAny());
    EXPECT_FALSE(section.needsRandomTick());

    section.setBlockState(0, 0, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_TRUE(section.needsRandomTickAny());
    EXPECT_TRUE(section.needsRandomTick());
    EXPECT_EQ(section.blockTickRefCount(), 1);

    section.setBlockState(0, 0, 0, &VanillaBlocks::DIRT->defaultState());
    EXPECT_FALSE(section.needsRandomTickAny());
    EXPECT_EQ(section.blockTickRefCount(), 0);
}

TEST_F(ChunkTest, ChunkSection_FastAccessRebuildsRandomTickCounters)
{
    ChunkSection section;
    i32 index = ChunkSection::blockIndex(3, 5, 7);

    section.setBlockStateIdFast(index, VanillaBlocks::GRASS_BLOCK->defaultState().stateId());

    EXPECT_TRUE(section.needsRandomTickAny());
    EXPECT_TRUE(section.needsRandomTick());
    EXPECT_EQ(section.blockTickRefCount(), 1);
}

TEST_F(ChunkTest, ChunkSection_LightAccess)
{
    ChunkSection section;

    // 天空光照
    section.setSkyLight(5, 5, 5, 10);
    EXPECT_EQ(section.getSkyLight(5, 5, 5), 10);

    // 方块光照
    section.setBlockLight(5, 5, 5, 12);
    EXPECT_EQ(section.getBlockLight(5, 5, 5), 12);

    // 边界检查 - 天空光照返回15，方块光照返回0
    EXPECT_EQ(section.getSkyLight(-1, 0, 0), 15);
    EXPECT_EQ(section.getBlockLight(-1, 0, 0), 0);
}

TEST_F(ChunkTest, ChunkSection_Serialization)
{
    ChunkSection original;
    original.setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    original.setBlockState(7, 7, 7, &VanillaBlocks::DIRT->defaultState());
    original.setSkyLight(3, 3, 3, 15);
    original.setBlockLight(3, 3, 3, 10);

    // 序列化
    auto data = original.serialize();
    EXPECT_FALSE(data.empty());

    // 反序列化
    auto result = ChunkSection::deserialize(data.data(), data.size());
    EXPECT_TRUE(result.success());

    auto restored = result.value();
    const BlockState* block0 = restored->getBlockState(0, 0, 0);
    const BlockState* block7 = restored->getBlockState(7, 7, 7);
    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block7, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block7->blockId(), VanillaBlocks::DIRT->blockId());
    EXPECT_EQ(restored->getSkyLight(3, 3, 3), 15);
    EXPECT_EQ(restored->getBlockLight(3, 3, 3), 10);
}

TEST_F(ChunkTest, ChunkSection_Fill)
{
    ChunkSection section;
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section.fill(stoneStateId);

    EXPECT_EQ(section.getBlockCount(), ChunkSection::VOLUME);

    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                const BlockState* block = section.getBlockState(x, y, z);
                ASSERT_NE(block, nullptr);
                EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());
            }
        }
    }
}

// ============================================================================
// ChunkData 测试
// ============================================================================

TEST_F(ChunkTest, ChunkData_Construction)
{
    ChunkData chunk(10, 20);
    EXPECT_EQ(chunk.x(), 10);
    EXPECT_EQ(chunk.z(), 20);
    EXPECT_FALSE(chunk.isLoaded());
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_FALSE(chunk.isFullyGenerated());
}

TEST_F(ChunkTest, ChunkData_SetGetBlock)
{
    ChunkData chunk;

    // 设置方块
    chunk.setBlockState(5, 100, 7, &VanillaBlocks::STONE->defaultState());
    EXPECT_TRUE(chunk.isDirty());

    // 获取方块
    const BlockState* block = chunk.getBlockState(5, 100, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());

    // 边界检查
    const BlockState* outOfBounds = chunk.getBlockState(-1, 0, 0);
    // 返回 nullptr 表示越界
    EXPECT_EQ(outOfBounds, nullptr);
    outOfBounds = chunk.getBlockState(0, 500, 0);
    // 返回 nullptr 表示越界
    EXPECT_EQ(outOfBounds, nullptr);
}

TEST_F(ChunkTest, ChunkData_SectionManagement)
{
    ChunkData chunk;

    // 初始没有段
    EXPECT_FALSE(chunk.hasSection(0));
    EXPECT_FALSE(chunk.hasSection(10));

    // 设置方块会创建段。段索引由 toSectionIndex(y) = (y - MIN_BUILD_HEIGHT) / 16 计算，
    // MIN_BUILD_HEIGHT=-64，故 Y=50 落在段 7（覆盖 Y∈[48,64)）。
    constexpr BlockCoord blockY = 50;
    const i32 expectedSection = toSectionIndex(blockY); // = (50-(-64))/16 = 7
    ASSERT_EQ(expectedSection, 7);                      // 防止高度模型再次迁移时静默回归
    chunk.setBlockState(5, blockY, 7, &VanillaBlocks::DIRT->defaultState());

    // 目标段应被创建，其它段（含段 0）不应被创建
    EXPECT_TRUE(chunk.hasSection(expectedSection));
    EXPECT_FALSE(chunk.hasSection(0)); // 段0(Y∈[-64,-48))不应被创建
}

TEST_F(ChunkTest, ChunkData_HeightMap)
{
    ChunkData chunk;

    // 设置一些方块
    chunk.setBlockState(0, 10, 0, &VanillaBlocks::STONE->defaultState());
    chunk.setBlockState(0, 20, 0, &VanillaBlocks::DIRT->defaultState());
    chunk.setBlockState(0, 30, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());

    // 最高方块应该是30
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 30);

    // 移除最高方块
    chunk.setBlockState(0, 30, 0, &VanillaBlocks::AIR->defaultState());

    // 更新高度图
    chunk.updateHeightMap(0, 0);
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 20);
}

TEST_F(ChunkTest, ChunkData_Serialization)
{
    ChunkData original(5, 10);
    original.setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    original.setBlockState(8, 100, 8, &VanillaBlocks::DIRT->defaultState());
    original.setFullyGenerated(true);

    // 序列化
    auto data = original.serialize();
    EXPECT_FALSE(data.empty());

    // 反序列化
    auto result = ChunkData::deserialize(data.data(), data.size());
    EXPECT_TRUE(result.success());

    auto restored = result.value();
    EXPECT_EQ(restored->x(), 5);
    EXPECT_EQ(restored->z(), 10);
    EXPECT_TRUE(restored->isFullyGenerated());

    const BlockState* block0 = restored->getBlockState(0, 0, 0);
    const BlockState* block8 = restored->getBlockState(8, 100, 8);
    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block8, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block8->blockId(), VanillaBlocks::DIRT->blockId());
}

// 验证 WorldSurface 高度图在 serialize/deserialize 往返中保持一致
TEST_F(ChunkTest, ChunkData_Serialization_PreservesWorldSurfaceHeightmap)
{
    ChunkData original(3, 7);
    original.setBlockState(0, 10, 0, &VanillaBlocks::STONE->defaultState());
    original.setBlockState(0, 20, 0, &VanillaBlocks::DIRT->defaultState());
    original.setBlockState(5, 50, 5, &VanillaBlocks::GRASS_BLOCK->defaultState());

    // 序列化 + 反序列化
    auto data = original.serialize();
    ASSERT_FALSE(data.empty());
    auto result = ChunkData::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());
    auto restored = result.value();

    // WorldSurface 高度图应保留：方块 Y=20 是 (0,0) 列最高，Y=50 是 (5,5) 列最高
    EXPECT_EQ(restored->getHighestBlock(0, 0), 20);
    EXPECT_EQ(restored->getHighestBlock(5, 5), 50);
    // 未放置方块的列应回退为 0
    EXPECT_EQ(restored->getHighestBlock(10, 10), 0);
}

// 验证所有 7 种高度图类型在 serialize/deserialize 往返中保持一致
TEST_F(ChunkTest, ChunkData_Serialization_PreservesAllHeightmapTypes)
{
    ChunkData original(2, 3);

    // 通过 setHeightmapFromStorage 模拟从存档加载各类型高度图
    std::array<BlockCoord, Heightmap::SIZE> worldSurfaceHeights{};
    std::array<BlockCoord, Heightmap::SIZE> oceanFloorHeights{};
    std::array<BlockCoord, Heightmap::SIZE> motionBlockingHeights{};
    std::array<BlockCoord, Heightmap::SIZE> motionBlockingNoLeavesHeights{};
    std::array<BlockCoord, Heightmap::SIZE> worldSurfaceWGHeights{};
    std::array<BlockCoord, Heightmap::SIZE> oceanFloorWGHeights{};
    std::array<BlockCoord, Heightmap::SIZE> lightBlockingHeights{};

    // 设置一些不同的高度值，确保每种类型都能正确往返
    // 内部存储语义：Y+1（Heightmap::NO_BLOCK_SENTINEL 表示无方块）
    const BlockCoord noBlock = Heightmap::NO_BLOCK_SENTINEL;
    for (i32 z = 0; z < 16; ++z) {
        for (i32 x = 0; x < 16; ++x) {
            const i32 index = z * 16 + x;
            // WorldSurface: 高度随 x+z 递增
            worldSurfaceHeights[static_cast<size_t>(index)] = static_cast<BlockCoord>(x + z + 1);
            // OceanFloor: 比 WorldSurface 低 5
            oceanFloorHeights[static_cast<size_t>(index)] =
                static_cast<BlockCoord>(x + z - 4 > 0 ? x + z - 4 : noBlock);
            // MotionBlocking: 与 WorldSurface 相同
            motionBlockingHeights[static_cast<size_t>(index)] = static_cast<BlockCoord>(x + z + 1);
            // MotionBlockingNoLeaves: 比 WorldSurface 低 2
            motionBlockingNoLeavesHeights[static_cast<size_t>(index)] =
                static_cast<BlockCoord>(x + z - 1 > 0 ? x + z - 1 : noBlock);
            // WorldSurfaceWG: 比 WorldSurface 高 3
            worldSurfaceWGHeights[static_cast<size_t>(index)] = static_cast<BlockCoord>(x + z + 4);
            // OceanFloorWG: 比 OceanFloor 高 1
            oceanFloorWGHeights[static_cast<size_t>(index)] =
                static_cast<BlockCoord>(x + z - 3 > 0 ? x + z - 3 : noBlock);
            // LightBlocking: 比 WorldSurface 低 1
            lightBlockingHeights[static_cast<size_t>(index)] = static_cast<BlockCoord>(x + z > 0 ? x + z : noBlock);
        }
    }

    original.setHeightmapFromStorage(HeightmapType::WorldSurface, worldSurfaceHeights);
    original.setHeightmapFromStorage(HeightmapType::OceanFloor, oceanFloorHeights);
    original.setHeightmapFromStorage(HeightmapType::MotionBlocking, motionBlockingHeights);
    original.setHeightmapFromStorage(HeightmapType::MotionBlockingNoLeaves, motionBlockingNoLeavesHeights);
    original.setHeightmapFromStorage(HeightmapType::WorldSurfaceWG, worldSurfaceWGHeights);
    original.setHeightmapFromStorage(HeightmapType::OceanFloorWG, oceanFloorWGHeights);
    original.setHeightmapFromStorage(HeightmapType::LightBlocking, lightBlockingHeights);

    // 序列化 + 反序列化
    auto data = original.serialize();
    ASSERT_FALSE(data.empty());
    auto result = ChunkData::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());
    auto restored = result.value();

    // 验证每种类型的高度图都能正确还原
    // 注意 getTopBlockY 返回最高方块 Y（即内部存储 Y+1 - 1），无方块返回 MIN_BUILD_HEIGHT
    const BlockCoord expectedWs = 5 + 5;         // x=5, z=5: internal=11, Y=10
    const BlockCoord expectedOf = 5 + 5 - 4 - 1; // internal=6, Y=5
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::WorldSurface, 5, 5), expectedWs);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::OceanFloor, 5, 5), expectedOf);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::MotionBlocking, 5, 5), expectedWs);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::MotionBlockingNoLeaves, 5, 5), 5 + 5 - 1 - 1);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::WorldSurfaceWG, 5, 5), 5 + 5 + 4 - 1);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::OceanFloorWG, 5, 5), 5 + 5 - 3 - 1);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::LightBlocking, 5, 5), 5 + 5 - 1);

    // 边界：x=0, z=0 时 internal=1，Y=0
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::WorldSurface, 0, 0), 0);
    // OceanFloor at (0,0): internal=noBlock → 无方块 → MIN_BUILD_HEIGHT
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::OceanFloor, 0, 0), mc::world::MIN_BUILD_HEIGHT);
}

// 验证未初始化的高度图类型在反序列化后仍回退到 WorldSurface
TEST_F(ChunkTest, ChunkData_Serialization_UninitializedHeightmapsFallBackToWorldSurface)
{
    ChunkData original(1, 1);
    original.setBlockState(0, 30, 0, &VanillaBlocks::STONE->defaultState());

    // 只填充 WorldSurface（setBlockState 会自动维护）
    // 其它类型不填充

    auto data = original.serialize();
    ASSERT_FALSE(data.empty());
    auto result = ChunkData::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());
    auto restored = result.value();

    // WorldSurface 应为 Y=30
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::WorldSurface, 0, 0), 30);
    // OceanFloor 等未初始化类型应回退到 WorldSurface 的值
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::OceanFloor, 0, 0), 30);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::MotionBlocking, 0, 0), 30);
    EXPECT_EQ(restored->getTopBlockY(HeightmapType::LightBlocking, 0, 0), 30);
}

// 验证 setHeightmapFromStorage 直接写入数据并标记初始化
TEST_F(ChunkTest, ChunkData_SetHeightmapFromStorage_LoadsDataAndMarksInitialized)
{
    ChunkData chunk(0, 0);

    std::array<BlockCoord, Heightmap::SIZE> heights{};
    // 未设置的列用 NO_BLOCK_SENTINEL 填充（语义为无方块）
    heights.fill(Heightmap::NO_BLOCK_SENTINEL);
    // 设置 (5, 5) 列高度为 Y+1=42（即 Y=41）
    heights[5 * 16 + 5] = 42;

    chunk.setHeightmapFromStorage(HeightmapType::OceanFloor, heights);

    // getTopBlockY 应返回 41（Y+1 - 1）
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::OceanFloor, 5, 5), 41);
    // 未设置的列应为 MIN_BUILD_HEIGHT（NO_BLOCK_SENTINEL 表示无方块）
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::OceanFloor, 0, 0), mc::world::MIN_BUILD_HEIGHT);
}
