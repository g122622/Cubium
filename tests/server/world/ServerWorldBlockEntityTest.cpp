#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/block/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::server;

// 测试用的简单方块实体
class TestBlockEntity : public BlockEntity {
public:
    TestBlockEntity(const BlockPos& pos)
        : BlockEntity(BlockEntityType::Chest, pos) {}

    void tick(IWorld& world) override { MC_UNUSED(world); }
    [[nodiscard]] bool needsTick() const override { return false; }
    bool load(const nlohmann::json& data) override { MC_UNUSED(data); return true; }
    void save(nlohmann::json& data) const override { MC_UNUSED(data); }
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override {
        return std::make_unique<TestBlockEntity>(getPos());
    }
};

class ServerWorldBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表（ServerWorld需要）
        VanillaBlocks::initialize();

        // 配置ServerWorld但不调用initialize()以避免数据库依赖
        ServerWorldConfig config;
        config.viewDistance = 3;
        config.dimension = 0;
        world = std::make_unique<ServerWorld>(config);

        // 使用getChunkSync创建区块，而不是依赖initialize()
        // 这样可以避免RocksDB ZSTD压缩链接问题
    }

    void TearDown() override {
        world.reset();
    }

    std::unique_ptr<ServerWorld> world;
};

// ========== getBlockEntity 测试 ==========

TEST_F(ServerWorldBlockEntityTest, GetBlockEntity_ReturnsNullptrWhenNotFound) {
    // 先创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);

    // 在没有设置方块实体的位置，应该返回 nullptr
    BlockEntity* entity = world->getBlockEntity(pos);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(ServerWorldBlockEntityTest, GetBlockEntity_ReturnsNullptrForUnloadedChunk) {
    // 不创建区块

    BlockPos pos(1000, 64, 1000);

    BlockEntity* entity = world->getBlockEntity(pos);
    EXPECT_EQ(entity, nullptr);
}

// ========== setBlockEntity 测试 ==========

TEST_F(ServerWorldBlockEntityTest, SetBlockEntity_StoresEntity) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);
    auto entity = std::make_unique<TestBlockEntity>(pos);
    TestBlockEntity* rawPtr = entity.get();

    // 设置方块实体
    world->setBlockEntity(pos, entity.release());

    // 获取并验证
    BlockEntity* retrieved = world->getBlockEntity(pos);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getPos(), pos);
    EXPECT_EQ(retrieved, rawPtr);
}

TEST_F(ServerWorldBlockEntityTest, SetBlockEntity_SetsWorldReference) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);
    auto entity = std::make_unique<TestBlockEntity>(pos);

    // 设置方块实体（会设置世界引用）
    world->setBlockEntity(pos, entity.release());

    // 验证世界引用
    BlockEntity* retrieved = world->getBlockEntity(pos);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getWorld(), world.get());
}

TEST_F(ServerWorldBlockEntityTest, SetBlockEntity_ReplacesExistingEntity) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);

    // 设置第一个方块实体
    auto entity1 = std::make_unique<TestBlockEntity>(pos);
    world->setBlockEntity(pos, entity1.release());

    // 设置第二个方块实体（替换第一个）
    auto entity2 = std::make_unique<TestBlockEntity>(pos);
    TestBlockEntity* rawPtr2 = entity2.get();
    world->setBlockEntity(pos, entity2.release());

    // 验证获取的是第二个
    BlockEntity* retrieved = world->getBlockEntity(pos);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, rawPtr2);
}

TEST_F(ServerWorldBlockEntityTest, SetBlockEntity_HandlesNullptr) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);

    // 设置 nullptr 应该不会崩溃
    EXPECT_NO_THROW(world->setBlockEntity(pos, nullptr));
}

// ========== removeBlockEntity 测试 ==========

TEST_F(ServerWorldBlockEntityTest, RemoveBlockEntity_RemovesExistingEntity) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);
    auto entity = std::make_unique<TestBlockEntity>(pos);
    world->setBlockEntity(pos, entity.release());

    // 移除方块实体
    world->removeBlockEntity(pos);

    // 验证已被移除
    BlockEntity* retrieved = world->getBlockEntity(pos);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(ServerWorldBlockEntityTest, RemoveBlockEntity_HandlesNonExistentEntity) {
    // 创建区块
    world->getChunkSync(0, 0);

    BlockPos pos(10, 64, 10);

    // 移除不存在的方块实体应该不会崩溃
    EXPECT_NO_THROW(world->removeBlockEntity(pos));
}

TEST_F(ServerWorldBlockEntityTest, RemoveBlockEntity_HandlesUnloadedChunk) {
    // 不创建区块

    BlockPos pos(1000, 64, 1000);

    // 移除未加载区块的方块实体应该不会崩溃
    EXPECT_NO_THROW(world->removeBlockEntity(pos));
}

// ========== 边界情况测试 ==========

TEST_F(ServerWorldBlockEntityTest, SetAndGetEntityAtWorldBoundary) {
    // 创建区块（包含原点）
    world->getChunkSync(0, 0);

    // 测试在边界位置
    BlockPos pos(0, 0, 0);
    auto entity = std::make_unique<TestBlockEntity>(pos);
    world->setBlockEntity(pos, entity.release());

    BlockEntity* retrieved = world->getBlockEntity(pos);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getPos(), pos);
}

TEST_F(ServerWorldBlockEntityTest, MultipleBlockEntitiesInSameChunk) {
    // 创建区块
    world->getChunkSync(0, 0);

    // 在同一个区块内设置多个方块实体
    BlockPos pos1(5, 64, 5);
    BlockPos pos2(10, 64, 10);
    BlockPos pos3(15, 64, 15);

    auto entity1 = std::make_unique<TestBlockEntity>(pos1);
    auto entity2 = std::make_unique<TestBlockEntity>(pos2);
    auto entity3 = std::make_unique<TestBlockEntity>(pos3);

    world->setBlockEntity(pos1, entity1.release());
    world->setBlockEntity(pos2, entity2.release());
    world->setBlockEntity(pos3, entity3.release());

    // 验证所有方块实体都可以正确获取
    EXPECT_NE(world->getBlockEntity(pos1), nullptr);
    EXPECT_NE(world->getBlockEntity(pos2), nullptr);
    EXPECT_NE(world->getBlockEntity(pos3), nullptr);

    // 移除中间的
    world->removeBlockEntity(pos2);

    // 验证第一个和第三个还在，中间的已移除
    EXPECT_NE(world->getBlockEntity(pos1), nullptr);
    EXPECT_EQ(world->getBlockEntity(pos2), nullptr);
    EXPECT_NE(world->getBlockEntity(pos3), nullptr);
}
