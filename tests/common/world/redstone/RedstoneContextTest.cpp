#include <gtest/gtest.h>
#include "world/redstone/RedstoneContext.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;
using namespace mc::world::redstone;

/**
 * @brief RedstoneContext 单元测试
 *
 * 测试递归防护功能。
 */
class RedstoneContextTest : public ::testing::Test {
protected:
    RedstoneContext context;

    void SetUp() override {
        context.clear();
    }
};

// ========== 基本功能测试 ==========

TEST_F(RedstoneContextTest, InitiallyNotUpdating) {
    BlockPos pos(0, 0, 0);
    EXPECT_FALSE(context.isUpdating(pos));
}

TEST_F(RedstoneContextTest, BeginEndUpdate) {
    BlockPos pos(10, 20, 30);

    EXPECT_FALSE(context.isUpdating(pos));

    context.beginUpdate(pos);
    EXPECT_TRUE(context.isUpdating(pos));

    context.endUpdate(pos);
    EXPECT_FALSE(context.isUpdating(pos));
}

TEST_F(RedstoneContextTest, MultiplePositions) {
    BlockPos pos1(0, 0, 0);
    BlockPos pos2(1, 0, 0);
    BlockPos pos3(0, 1, 0);

    context.beginUpdate(pos1);
    context.beginUpdate(pos2);

    EXPECT_TRUE(context.isUpdating(pos1));
    EXPECT_TRUE(context.isUpdating(pos2));
    EXPECT_FALSE(context.isUpdating(pos3));

    context.endUpdate(pos1);
    EXPECT_FALSE(context.isUpdating(pos1));
    EXPECT_TRUE(context.isUpdating(pos2));

    context.endUpdate(pos2);
    EXPECT_FALSE(context.isUpdating(pos2));
}

TEST_F(RedstoneContextTest, SamePositionMultipleBegin) {
    BlockPos pos(0, 0, 0);

    // 多次 beginUpdate 应该是幂等的
    context.beginUpdate(pos);
    context.beginUpdate(pos);
    context.beginUpdate(pos);

    EXPECT_TRUE(context.isUpdating(pos));
    EXPECT_EQ(context.updatingCount(), 1);

    // 只需要一次 endUpdate
    context.endUpdate(pos);
    EXPECT_FALSE(context.isUpdating(pos));
}

// ========== 深度限制测试 ==========

TEST_F(RedstoneContextTest, DepthInitiallyZero) {
    EXPECT_EQ(context.depth(), 0);
}

TEST_F(RedstoneContextTest, PushPopDepth) {
    context.pushDepth();
    EXPECT_EQ(context.depth(), 1);

    context.pushDepth();
    EXPECT_EQ(context.depth(), 2);

    context.popDepth();
    EXPECT_EQ(context.depth(), 1);

    context.popDepth();
    EXPECT_EQ(context.depth(), 0);
}

TEST_F(RedstoneContextTest, CanPushDepth) {
    // 初始可以增加深度
    EXPECT_TRUE(context.canPushDepth());

    // 增加到最大深度
    for (i32 i = 0; i < RedstoneContext::MAX_DEPTH; ++i) {
        context.pushDepth();
    }

    EXPECT_FALSE(context.canPushDepth());

    // 减少后可以再增加
    context.popDepth();
    EXPECT_TRUE(context.canPushDepth());
}

TEST_F(RedstoneContextTest, PopDepthNoUnderflow) {
    // 在深度为0时pop不应该变成负数
    context.popDepth();
    EXPECT_EQ(context.depth(), 0);

    context.popDepth();
    EXPECT_EQ(context.depth(), 0);
}

// ========== 清空测试 ==========

TEST_F(RedstoneContextTest, Clear) {
    BlockPos pos1(0, 0, 0);
    BlockPos pos2(1, 0, 0);

    context.beginUpdate(pos1);
    context.beginUpdate(pos2);
    context.pushDepth();
    context.pushDepth();

    context.clear();

    EXPECT_FALSE(context.isUpdating(pos1));
    EXPECT_FALSE(context.isUpdating(pos2));
    EXPECT_EQ(context.depth(), 0);
    EXPECT_EQ(context.updatingCount(), 0);
}

// ========== 更新计数测试 ==========

TEST_F(RedstoneContextTest, UpdatingCount) {
    EXPECT_EQ(context.updatingCount(), 0);

    context.beginUpdate(BlockPos(0, 0, 0));
    EXPECT_EQ(context.updatingCount(), 1);

    context.beginUpdate(BlockPos(1, 0, 0));
    EXPECT_EQ(context.updatingCount(), 2);

    context.endUpdate(BlockPos(0, 0, 0));
    EXPECT_EQ(context.updatingCount(), 1);

    context.endUpdate(BlockPos(1, 0, 0));
    EXPECT_EQ(context.updatingCount(), 0);
}

// ========== 边界条件测试 ==========

TEST_F(RedstoneContextTest, LargeCoordinates) {
    BlockPos pos(1000000, -100, 2000000);

    context.beginUpdate(pos);
    EXPECT_TRUE(context.isUpdating(pos));
    context.endUpdate(pos);
    EXPECT_FALSE(context.isUpdating(pos));
}

TEST_F(RedstoneContextTest, NegativeCoordinates) {
    BlockPos pos(-100, -50, -200);

    context.beginUpdate(pos);
    EXPECT_TRUE(context.isUpdating(pos));
    context.endUpdate(pos);
    EXPECT_FALSE(context.isUpdating(pos));
}

// ========== 并发安全测试（基础） ==========

TEST_F(RedstoneContextTest, ThreadSafetyBasic) {
    // 简单测试 mutex 保护不会死锁
    BlockPos pos(0, 0, 0);

    context.beginUpdate(pos);
    EXPECT_TRUE(context.isUpdating(pos)); // 读操作
    EXPECT_TRUE(context.isUpdating(pos)); // 另一个读操作
    context.endUpdate(pos);
    EXPECT_FALSE(context.isUpdating(pos)); // 读操作
}
