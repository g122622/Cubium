#include <gtest/gtest.h>
#include <cmath>

#include "common/world/chunk/ChunkData.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::world;

// ============================================================================
// 测试固件
// ============================================================================

class ChunkTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

// ============================================================================
// ChunkSection 测试
// ============================================================================

TEST_F(ChunkTest, ChunkSection_Construction) {
    ChunkSection section;
    EXPECT_EQ(section.getBlockCount(), 0);
    EXPECT_TRUE(section.isEmpty());
}

TEST_F(ChunkTest, ChunkSection_SetGetBlock) {
    ChunkSection section;

    // 设置方块
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    section.setBlock(5, 10, 7, stoneState);
    EXPECT_EQ(section.getBlockCount(), 1);
    EXPECT_FALSE(section.isEmpty());

    // 获取方块
    const BlockState* block = section.getBlock(5, 10, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());

    // 边界检查 - 返回空气
    const BlockState* outOfBounds = section.getBlock(16, 0, 0);
    EXPECT_EQ(outOfBounds, nullptr);
}

TEST_F(ChunkTest, ChunkSection_FastAccess) {
    ChunkSection section;
    i32 index = ChunkSection::blockIndex(3, 5, 7);

    u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
    section.setBlockStateIdFast(index, dirtStateId);

    const BlockState* block = section.getBlock(3, 5, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::DIRT->blockId());
}

TEST_F(ChunkTest, ChunkSection_BlockCount) {
    ChunkSection section;

    // 添加方块
    section.setBlock(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    section.setBlock(1, 1, 1, &VanillaBlocks::DIRT->defaultState());
    EXPECT_EQ(section.getBlockCount(), 2);

    // 替换方块 (不增加计数)
    section.setBlock(0, 0, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_EQ(section.getBlockCount(), 2);

    // 移除方块 (设为空气)
    section.setBlock(0, 0, 0, &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(section.getBlockCount(), 1);

    // 再次移除 (计数不变)
    section.setBlock(0, 0, 0, &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(section.getBlockCount(), 1);
}

TEST_F(ChunkTest, ChunkSection_LightAccess) {
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

TEST_F(ChunkTest, ChunkSection_Serialization) {
    ChunkSection original;
    original.setBlock(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    original.setBlock(7, 7, 7, &VanillaBlocks::DIRT->defaultState());
    original.setSkyLight(3, 3, 3, 15);
    original.setBlockLight(3, 3, 3, 10);

    // 序列化
    auto data = original.serialize();
    EXPECT_FALSE(data.empty());

    // 反序列化
    auto result = ChunkSection::deserialize(data.data(), data.size());
    EXPECT_TRUE(result.success());

    auto restored = result.value();
    const BlockState* block0 = restored->getBlock(0, 0, 0);
    const BlockState* block7 = restored->getBlock(7, 7, 7);
    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block7, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block7->blockId(), VanillaBlocks::DIRT->blockId());
    EXPECT_EQ(restored->getSkyLight(3, 3, 3), 15);
    EXPECT_EQ(restored->getBlockLight(3, 3, 3), 10);
}

TEST_F(ChunkTest, ChunkSection_Fill) {
    ChunkSection section;
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section.fill(stoneStateId);

    EXPECT_EQ(section.getBlockCount(), ChunkSection::VOLUME);

    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                const BlockState* block = section.getBlock(x, y, z);
                ASSERT_NE(block, nullptr);
                EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());
            }
        }
    }
}

// ============================================================================
// ChunkData 测试
// ============================================================================

TEST_F(ChunkTest, ChunkData_Construction) {
    ChunkData chunk(10, 20);
    EXPECT_EQ(chunk.x(), 10);
    EXPECT_EQ(chunk.z(), 20);
    EXPECT_FALSE(chunk.isLoaded());
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_FALSE(chunk.isFullyGenerated());
}

TEST_F(ChunkTest, ChunkData_SetGetBlock) {
    ChunkData chunk;

    // 设置方块
    chunk.setBlock(5, 100, 7, &VanillaBlocks::STONE->defaultState());
    EXPECT_TRUE(chunk.isDirty());

    // 获取方块
    const BlockState* block = chunk.getBlock(5, 100, 7);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());

    // 边界检查
    const BlockState* outOfBounds = chunk.getBlock(-1, 0, 0);
    // 返回 nullptr 表示越界
    EXPECT_EQ(outOfBounds, nullptr);
    outOfBounds = chunk.getBlock(0, 500, 0);
    // 返回 nullptr 表示越界
    EXPECT_EQ(outOfBounds, nullptr);
}

TEST_F(ChunkTest, ChunkData_SectionManagement) {
    ChunkData chunk;

    // 初始没有段
    EXPECT_FALSE(chunk.hasSection(0));
    EXPECT_FALSE(chunk.hasSection(10));

    // 设置方块会创建段
    // Y=50, sectionIndex = 50 / CHUNK_SECTION_HEIGHT(16) = 3
    chunk.setBlock(5, 50, 7, &VanillaBlocks::DIRT->defaultState());

    // 段3应该被创建
    EXPECT_TRUE(chunk.hasSection(3));
    EXPECT_FALSE(chunk.hasSection(0));  // 段0不应该被创建
}

TEST_F(ChunkTest, ChunkData_HeightMap) {
    ChunkData chunk;

    // 设置一些方块
    chunk.setBlock(0, 10, 0, &VanillaBlocks::STONE->defaultState());
    chunk.setBlock(0, 20, 0, &VanillaBlocks::DIRT->defaultState());
    chunk.setBlock(0, 30, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());

    // 最高方块应该是30
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 30);

    // 移除最高方块
    chunk.setBlock(0, 30, 0, &VanillaBlocks::AIR->defaultState());

    // 更新高度图
    chunk.updateHeightMap(0, 0);
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 20);
}

TEST_F(ChunkTest, ChunkData_Serialization) {
    ChunkData original(5, 10);
    original.setBlock(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    original.setBlock(8, 100, 8, &VanillaBlocks::DIRT->defaultState());
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

    const BlockState* block0 = restored->getBlock(0, 0, 0);
    const BlockState* block8 = restored->getBlock(8, 100, 8);
    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block8, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block8->blockId(), VanillaBlocks::DIRT->blockId());
}
