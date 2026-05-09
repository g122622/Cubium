#include <gtest/gtest.h>
#include "common/world/border/WorldBorder.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/util/AxisAlignedBB.hpp"

using namespace mc;
using namespace mc::world::border;

class WorldBorderTest : public ::testing::Test {
protected:
    void SetUp() override {
        border = std::make_unique<WorldBorder>();
    }

    std::unique_ptr<WorldBorder> border;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(WorldBorderTest, DefaultValues) {
    // 默认边界大小为 6000 万格
    EXPECT_DOUBLE_EQ(border->getSize(), 6.0E7);
    EXPECT_DOUBLE_EQ(border->getCenterX(), 0.0);
    EXPECT_DOUBLE_EQ(border->getCenterZ(), 0.0);
    EXPECT_DOUBLE_EQ(border->getDamagePerBlock(), 0.2);
    EXPECT_DOUBLE_EQ(border->getDamageBuffer(), 5.0);
    EXPECT_EQ(border->getWarningTime(), 15);
    EXPECT_EQ(border->getWarningDistance(), 5);
    EXPECT_EQ(border->getStatus(), BorderStatus::Stationary);
}

TEST_F(WorldBorderTest, SetSize) {
    border->setSize(1000.0);
    EXPECT_DOUBLE_EQ(border->getSize(), 1000.0);
    EXPECT_EQ(border->getStatus(), BorderStatus::Stationary);

    // 设置大小应该被限制在最大值
    border->setSize(1.0E10);
    EXPECT_DOUBLE_EQ(border->getSize(), WorldBorder::MAX_SIZE);
}

TEST_F(WorldBorderTest, SetCenter) {
    border->setCenter(100.0, 200.0);
    EXPECT_DOUBLE_EQ(border->getCenterX(), 100.0);
    EXPECT_DOUBLE_EQ(border->getCenterZ(), 200.0);
}

TEST_F(WorldBorderTest, SetDamagePerBlock) {
    border->setDamagePerBlock(0.5);
    EXPECT_DOUBLE_EQ(border->getDamagePerBlock(), 0.5);
}

TEST_F(WorldBorderTest, SetDamageBuffer) {
    border->setDamageBuffer(10.0);
    EXPECT_DOUBLE_EQ(border->getDamageBuffer(), 10.0);
}

TEST_F(WorldBorderTest, SetWarningTime) {
    border->setWarningTime(30);
    EXPECT_EQ(border->getWarningTime(), 30);
}

TEST_F(WorldBorderTest, SetWarningDistance) {
    border->setWarningDistance(10);
    EXPECT_EQ(border->getWarningDistance(), 10);
}

// ============================================================================
// 边界检测测试
// ============================================================================

TEST_F(WorldBorderTest, ContainsPoint) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    // 中心点应该在边界内
    EXPECT_TRUE(border->contains(0.0, 0.0));

    // 边界内的点
    EXPECT_TRUE(border->contains(40.0, 40.0));
    EXPECT_TRUE(border->contains(-40.0, -40.0));

    // 边界外的点
    EXPECT_FALSE(border->contains(60.0, 0.0));  // 超过 minX = -50, maxX = 50
    EXPECT_FALSE(border->contains(0.0, 60.0));  // 超过 minZ = -50, maxZ = 50
}

TEST_F(WorldBorderTest, ContainsBlockPos) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    // 区块坐标 (0, 0, 0) 应该在边界内
    BlockPos pos1(0, 0, 0);
    EXPECT_TRUE(border->contains(pos1));

    // 边界外的方块
    BlockPos pos2(60, 0, 0);
    EXPECT_FALSE(border->contains(pos2));
}

TEST_F(WorldBorderTest, IntersectsAABB) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    // 边界内的 AABB
    AxisAlignedBB box1(0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
    EXPECT_TRUE(border->intersects(box1));

    // 跨越边界的 AABB
    AxisAlignedBB box2(40.0, 0.0, 0.0, 60.0, 10.0, 10.0);
    EXPECT_TRUE(border->intersects(box2));

    // 边界外的 AABB
    AxisAlignedBB box3(60.0, 0.0, 0.0, 70.0, 10.0, 10.0);
    EXPECT_FALSE(border->intersects(box3));
}

TEST_F(WorldBorderTest, IntersectsChunk) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    // 区块 (0, 0) 应该与边界相交
    EXPECT_TRUE(border->intersectsChunk(0, 0));

    // 远处的区块
    EXPECT_FALSE(border->intersectsChunk(100, 100));
}

TEST_F(WorldBorderTest, GetClosestDistance) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    // 中心点的距离应该是 50（半径）
    EXPECT_DOUBLE_EQ(border->getClosestDistance(0.0, 0.0), 50.0);

    // 靠近边界的点
    EXPECT_NEAR(border->getClosestDistance(45.0, 0.0), 5.0, 0.0001);

    // 边界外的点（负距离）
    EXPECT_NEAR(border->getClosestDistance(55.0, 0.0), -5.0, 0.0001);
}

// ============================================================================
// 渐变测试
// ============================================================================

TEST_F(WorldBorderTest, LerpSizeStartsTransition) {
    border->setSize(1000.0);
    border->setSizeLerp(1000.0, 500.0, 1000);  // 1秒过渡

    // 状态应该是 Shrinking
    EXPECT_EQ(border->getStatus(), BorderStatus::Shrinking);

    // 目标大小应该是 500
    EXPECT_DOUBLE_EQ(border->getTargetSize(), 500.0);

    // 过渡速度应该是正数
    EXPECT_GT(border->getResizeSpeed(), 0.0);
}

TEST_F(WorldBorderTest, LerpSizeGrowing) {
    border->setSize(500.0);
    border->setSizeLerp(500.0, 1000.0, 1000);

    // 状态应该是 Growing
    EXPECT_EQ(border->getStatus(), BorderStatus::Growing);
}

TEST_F(WorldBorderTest, LerpSizeImmediateWhenZeroTime) {
    border->setSize(1000.0);
    border->setSizeLerp(1000.0, 500.0, 0);

    // 时间为 0 应该立即完成
    EXPECT_DOUBLE_EQ(border->getSize(), 500.0);
    EXPECT_EQ(border->getStatus(), BorderStatus::Stationary);
}

TEST_F(WorldBorderTest, LerpSizeImmediateWhenSameSize) {
    border->setSize(1000.0);
    border->setSizeLerp(1000.0, 1000.0, 1000);

    // 大小相同应该立即完成
    EXPECT_DOUBLE_EQ(border->getSize(), 1000.0);
    EXPECT_EQ(border->getStatus(), BorderStatus::Stationary);
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(WorldBorderTest, SerializeDeserialize) {
    border->setSize(2000.0);
    border->setCenter(100.0, 200.0);
    border->setDamagePerBlock(0.3);
    border->setDamageBuffer(8.0);
    border->setWarningTime(20);
    border->setWarningDistance(10);

    auto data = border->serialize();

    WorldBorder newBorder;
    newBorder.deserialize(data);

    EXPECT_DOUBLE_EQ(newBorder.getSize(), 2000.0);
    EXPECT_DOUBLE_EQ(newBorder.getCenterX(), 100.0);
    EXPECT_DOUBLE_EQ(newBorder.getCenterZ(), 200.0);
    EXPECT_DOUBLE_EQ(newBorder.getDamagePerBlock(), 0.3);
    EXPECT_DOUBLE_EQ(newBorder.getDamageBuffer(), 8.0);
    EXPECT_EQ(newBorder.getWarningTime(), 20);
    EXPECT_EQ(newBorder.getWarningDistance(), 10);
}

// ============================================================================
// 边界范围测试
// ============================================================================

TEST_F(WorldBorderTest, GetMinXMaxX) {
    border->setSize(100.0);
    border->setCenter(0.0, 0.0);

    EXPECT_DOUBLE_EQ(border->getMinX(), -50.0);
    EXPECT_DOUBLE_EQ(border->getMaxX(), 50.0);
    EXPECT_DOUBLE_EQ(border->getMinZ(), -50.0);
    EXPECT_DOUBLE_EQ(border->getMaxZ(), 50.0);
}

TEST_F(WorldBorderTest, GetMinXMaxXWithOffset) {
    border->setSize(100.0);
    border->setCenter(100.0, 200.0);

    EXPECT_DOUBLE_EQ(border->getMinX(), 50.0);
    EXPECT_DOUBLE_EQ(border->getMaxX(), 150.0);
    EXPECT_DOUBLE_EQ(border->getMinZ(), 150.0);
    EXPECT_DOUBLE_EQ(border->getMaxZ(), 250.0);
}

// ============================================================================
// 监听器测试
// ============================================================================

class MockListener : public IBorderListener {
public:
    void onSizeChanged(double newSize) override {
        sizeChanged = true;
        lastSize = newSize;
    }
    void onTransitionStarted(double oldSize, double newSize, u64 timeMs) override {
        transitionStarted = true;
        lastOldSize = oldSize;
        lastNewSize = newSize;
        lastTimeMs = timeMs;
    }
    void onCenterChanged(double x, double z) override {
        centerChanged = true;
        lastCenterX = x;
        lastCenterZ = z;
    }
    void onWarningTimeChanged(i32 warningTime) override {
        warningTimeChanged = true;
        lastWarningTime = warningTime;
    }
    void onWarningDistanceChanged(i32 warningDistance) override {
        warningDistanceChanged = true;
        lastWarningDistance = warningDistance;
    }
    void onDamageBufferChanged(double damageBuffer) override {
        damageBufferChanged = true;
        lastDamageBuffer = damageBuffer;
    }
    void onDamagePerBlockChanged(double damagePerBlock) override {
        damagePerBlockChanged = true;
        lastDamagePerBlock = damagePerBlock;
    }

    void reset() {
        sizeChanged = false;
        transitionStarted = false;
        centerChanged = false;
        warningTimeChanged = false;
        warningDistanceChanged = false;
        damageBufferChanged = false;
        damagePerBlockChanged = false;
    }

    bool sizeChanged = false;
    bool transitionStarted = false;
    bool centerChanged = false;
    bool warningTimeChanged = false;
    bool warningDistanceChanged = false;
    bool damageBufferChanged = false;
    bool damagePerBlockChanged = false;

    double lastSize = 0.0;
    double lastOldSize = 0.0;
    double lastNewSize = 0.0;
    u64 lastTimeMs = 0;
    double lastCenterX = 0.0;
    double lastCenterZ = 0.0;
    i32 lastWarningTime = 0;
    i32 lastWarningDistance = 0;
    double lastDamageBuffer = 0.0;
    double lastDamagePerBlock = 0.0;
};

TEST_F(WorldBorderTest, ListenerOnSizeChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setSize(1000.0);

    EXPECT_TRUE(listener->sizeChanged);
    EXPECT_DOUBLE_EQ(listener->lastSize, 1000.0);
}

TEST_F(WorldBorderTest, ListenerOnTransitionStarted) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setSizeLerp(1000.0, 500.0, 10000);

    EXPECT_TRUE(listener->transitionStarted);
    EXPECT_DOUBLE_EQ(listener->lastOldSize, 1000.0);
    EXPECT_DOUBLE_EQ(listener->lastNewSize, 500.0);
    EXPECT_EQ(listener->lastTimeMs, 10000u);
}

TEST_F(WorldBorderTest, ListenerOnCenterChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setCenter(100.0, 200.0);

    EXPECT_TRUE(listener->centerChanged);
    EXPECT_DOUBLE_EQ(listener->lastCenterX, 100.0);
    EXPECT_DOUBLE_EQ(listener->lastCenterZ, 200.0);
}

TEST_F(WorldBorderTest, ListenerOnWarningTimeChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setWarningTime(30);

    EXPECT_TRUE(listener->warningTimeChanged);
    EXPECT_EQ(listener->lastWarningTime, 30);
}

TEST_F(WorldBorderTest, ListenerOnWarningDistanceChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setWarningDistance(15);

    EXPECT_TRUE(listener->warningDistanceChanged);
    EXPECT_EQ(listener->lastWarningDistance, 15);
}

TEST_F(WorldBorderTest, ListenerOnDamageBufferChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setDamageBuffer(10.0);

    EXPECT_TRUE(listener->damageBufferChanged);
    EXPECT_DOUBLE_EQ(listener->lastDamageBuffer, 10.0);
}

TEST_F(WorldBorderTest, ListenerOnDamagePerBlockChanged) {
    auto listener = std::make_shared<MockListener>();
    border->addListener(listener);

    border->setDamagePerBlock(0.5);

    EXPECT_TRUE(listener->damagePerBlockChanged);
    EXPECT_DOUBLE_EQ(listener->lastDamagePerBlock, 0.5);
}
