#include <gtest/gtest.h>
#include "world/chunk/ChunkData.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/block/VanillaBlocks.hpp"

using namespace mc;

// 测试用的简单方块实体
class TestBlockEntity : public BlockEntity {
public:
    TestBlockEntity(BlockEntityType type, const BlockPos& pos)
        : BlockEntity(type, pos)
        , m_testValue(0) {}

    void setTestValue(i32 value) { m_testValue = value; }
    i32 testValue() const { return m_testValue; }

    std::unique_ptr<BlockEntity> clone() const override {
        auto cloned = std::make_unique<TestBlockEntity>(m_type, m_pos);
        cloned->m_testValue = m_testValue;
        return cloned;
    }

    bool needsTick() const override { return m_needsTick; }
    void setNeedsTick(bool needs) { m_needsTick = needs; }

private:
    i32 m_testValue;
    bool m_needsTick = false;
};

// ============================================================================
// ChunkData 方块实体测试固件
// ============================================================================

class ChunkDataBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 创建一个测试区块
        chunk = std::make_unique<ChunkData>(0, 0);
    }

    void TearDown() override {
        chunk.reset();
    }

    std::unique_ptr<ChunkData> chunk;
};

// ============================================================================
// getBlockEntity 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, GetBlockEntity_ReturnsNullptrWhenEmpty) {
    BlockPos pos(5, 64, 10);
    EXPECT_EQ(chunk->getBlockEntity(pos), nullptr);
}

TEST_F(ChunkDataBlockEntityTest, GetBlockEntity_ReturnsNullptrForPositionOutsideChunk) {
    // 位置 (20, 64, 30) 不在区块 (0, 0) 内
    BlockPos outsidePos(20, 64, 30);
    EXPECT_EQ(chunk->getBlockEntity(outsidePos), nullptr);
}

TEST_F(ChunkDataBlockEntityTest, GetBlockEntity_ReturnsNullptrForNegativePosition) {
    BlockPos negativePos(-5, 64, -10);
    EXPECT_EQ(chunk->getBlockEntity(negativePos), nullptr);
}

// ============================================================================
// setBlockEntity 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_StoresEntityAtCorrectPosition) {
    BlockPos pos(5, 64, 10);
    auto entity = std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos);

    auto oldEntity = chunk->setBlockEntity(pos, std::move(entity));

    EXPECT_EQ(oldEntity, nullptr);  // 没有旧实体

    BlockEntity* stored = chunk->getBlockEntity(pos);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->getType(), BlockEntityType::Chest);
    EXPECT_EQ(stored->getPos(), pos);
}

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_ReplacesExistingEntity) {
    BlockPos pos(5, 64, 10);

    // 设置第一个实体
    auto entity1 = std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos);
    static_cast<TestBlockEntity*>(entity1.get())->setTestValue(100);
    chunk->setBlockEntity(pos, std::move(entity1));

    // 设置第二个实体替换第一个
    auto entity2 = std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos);
    static_cast<TestBlockEntity*>(entity2.get())->setTestValue(200);
    auto oldEntity = chunk->setBlockEntity(pos, std::move(entity2));

    // 应该返回旧实体
    ASSERT_NE(oldEntity, nullptr);
    EXPECT_EQ(oldEntity->getType(), BlockEntityType::Chest);
    EXPECT_EQ(static_cast<TestBlockEntity*>(oldEntity.get())->testValue(), 100);

    // 当前存储的应该是新实体
    BlockEntity* stored = chunk->getBlockEntity(pos);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(static_cast<TestBlockEntity*>(stored)->testValue(), 200);
}

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_RejectsNullptr) {
    BlockPos pos(5, 64, 10);

    auto result = chunk->setBlockEntity(pos, nullptr);
    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(chunk->getBlockEntity(pos), nullptr);
}

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_PositionOutsideChunkReturnsEntity) {
    // 位置 (20, 64, 30) 不在区块 (0, 0) 内
    BlockPos outsidePos(20, 64, 30);
    auto entity = std::make_unique<TestBlockEntity>(BlockEntityType::Chest, outsidePos);

    // 设置应该返回实体本身，因为位置不在区块内
    auto result = chunk->setBlockEntity(outsidePos, std::move(entity));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getType(), BlockEntityType::Chest);
}

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_MultipleEntitiesAtDifferentPositions) {
    BlockPos pos1(1, 64, 2);
    BlockPos pos2(5, 70, 8);
    BlockPos pos3(15, 80, 12);

    chunk->setBlockEntity(pos1, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos1));
    chunk->setBlockEntity(pos2, std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos2));
    chunk->setBlockEntity(pos3, std::make_unique<TestBlockEntity>(BlockEntityType::Hopper, pos3));

    EXPECT_NE(chunk->getBlockEntity(pos1), nullptr);
    EXPECT_NE(chunk->getBlockEntity(pos2), nullptr);
    EXPECT_NE(chunk->getBlockEntity(pos3), nullptr);

    EXPECT_EQ(chunk->getBlockEntity(pos1)->getType(), BlockEntityType::Chest);
    EXPECT_EQ(chunk->getBlockEntity(pos2)->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(chunk->getBlockEntity(pos3)->getType(), BlockEntityType::Hopper);
}

// ============================================================================
// removeBlockEntity 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, RemoveBlockEntity_ReturnsStoredEntity) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    auto removed = chunk->removeBlockEntity(pos);

    ASSERT_NE(removed, nullptr);
    EXPECT_EQ(removed->getType(), BlockEntityType::Chest);
    EXPECT_EQ(chunk->getBlockEntity(pos), nullptr);
}

TEST_F(ChunkDataBlockEntityTest, RemoveBlockEntity_ReturnsNullptrWhenNoEntity) {
    BlockPos pos(5, 64, 10);

    auto removed = chunk->removeBlockEntity(pos);

    EXPECT_EQ(removed, nullptr);
}

TEST_F(ChunkDataBlockEntityTest, RemoveBlockEntity_OnlyRemovesTargetPosition) {
    BlockPos pos1(1, 64, 2);
    BlockPos pos2(5, 70, 8);

    chunk->setBlockEntity(pos1, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos1));
    chunk->setBlockEntity(pos2, std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos2));

    chunk->removeBlockEntity(pos1);

    EXPECT_EQ(chunk->getBlockEntity(pos1), nullptr);
    EXPECT_NE(chunk->getBlockEntity(pos2), nullptr);
}

// ============================================================================
// hasBlockEntity 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, HasBlockEntity_ReturnsFalseWhenEmpty) {
    BlockPos pos(5, 64, 10);
    EXPECT_FALSE(chunk->hasBlockEntity(pos));
}

TEST_F(ChunkDataBlockEntityTest, HasBlockEntity_ReturnsTrueAfterSet) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    EXPECT_TRUE(chunk->hasBlockEntity(pos));
}

TEST_F(ChunkDataBlockEntityTest, HasBlockEntity_ReturnsFalseAfterRemove) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));
    chunk->removeBlockEntity(pos);

    EXPECT_FALSE(chunk->hasBlockEntity(pos));
}

// ============================================================================
// getAllBlockEntities 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, GetAllBlockEntities_ReturnsEmptyWhenNoEntities) {
    auto entities = chunk->getAllBlockEntities();
    EXPECT_TRUE(entities.empty());
}

TEST_F(ChunkDataBlockEntityTest, GetAllBlockEntities_ReturnsAllEntities) {
    BlockPos pos1(1, 64, 2);
    BlockPos pos2(5, 70, 8);
    BlockPos pos3(15, 80, 12);

    chunk->setBlockEntity(pos1, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos1));
    chunk->setBlockEntity(pos2, std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos2));
    chunk->setBlockEntity(pos3, std::make_unique<TestBlockEntity>(BlockEntityType::Hopper, pos3));

    auto entities = chunk->getAllBlockEntities();
    EXPECT_EQ(entities.size(), 3);
}

TEST_F(ChunkDataBlockEntityTest, GetAllBlockEntities_ConstVersion) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    const ChunkData& constChunk = *chunk;
    auto entities = constChunk.getAllBlockEntities();

    EXPECT_EQ(entities.size(), 1);
}

// ============================================================================
// blockEntityCount 测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, BlockEntityCount_StartsAtZero) {
    EXPECT_EQ(chunk->blockEntityCount(), 0);
}

TEST_F(ChunkDataBlockEntityTest, BlockEntityCount_IncreasesOnSet) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    EXPECT_EQ(chunk->blockEntityCount(), 1);

    BlockPos pos2(1, 70, 2);
    chunk->setBlockEntity(pos2, std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos2));

    EXPECT_EQ(chunk->blockEntityCount(), 2);
}

TEST_F(ChunkDataBlockEntityTest, BlockEntityCount_DecreasesOnRemove) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    EXPECT_EQ(chunk->blockEntityCount(), 1);

    chunk->removeBlockEntity(pos);

    EXPECT_EQ(chunk->blockEntityCount(), 0);
}

TEST_F(ChunkDataBlockEntityTest, BlockEntityCount_UnchangedOnReplace) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    EXPECT_EQ(chunk->blockEntityCount(), 1);

    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Furnace, pos));

    // 替换不应该增加计数
    EXPECT_EQ(chunk->blockEntityCount(), 1);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, PositionAtChunkBoundary) {
    // 区块 (0, 0) 的边界位置
    BlockPos cornerPos(15, 100, 15);  // 最大有效位置
    chunk->setBlockEntity(cornerPos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, cornerPos));

    EXPECT_NE(chunk->getBlockEntity(cornerPos), nullptr);
    EXPECT_EQ(chunk->getBlockEntity(cornerPos)->getPos(), cornerPos);
}

TEST_F(ChunkDataBlockEntityTest, PositionAtMinHeight) {
    BlockPos minPos(5, -64, 10);  // 假设 MIN_BUILD_HEIGHT 是 -64
    chunk->setBlockEntity(minPos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, minPos));

    EXPECT_NE(chunk->getBlockEntity(minPos), nullptr);
}

TEST_F(ChunkDataBlockEntityTest, PositionAtMaxHeight) {
    BlockPos maxPos(5, 319, 10);  // 假设 MAX_BUILD_HEIGHT 是 320
    chunk->setBlockEntity(maxPos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, maxPos));

    EXPECT_NE(chunk->getBlockEntity(maxPos), nullptr);
}

// ============================================================================
// 区块标记测试
// ============================================================================

TEST_F(ChunkDataBlockEntityTest, SetBlockEntity_MarksChunkAsDirty) {
    chunk->setDirty(false);  // 先清除脏标记

    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));

    EXPECT_TRUE(chunk->isDirty());
}

TEST_F(ChunkDataBlockEntityTest, RemoveBlockEntity_MarksChunkAsDirty) {
    BlockPos pos(5, 64, 10);
    chunk->setBlockEntity(pos, std::make_unique<TestBlockEntity>(BlockEntityType::Chest, pos));
    chunk->setDirty(false);  // 清除脏标记

    chunk->removeBlockEntity(pos);

    EXPECT_TRUE(chunk->isDirty());
}
