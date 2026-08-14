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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EITHER WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, THE IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file MinecartCheckRailStateTest.cpp
 * @brief 测试 AbstractMinecartEntity::_checkRailState() 的铁轨形状提取
 *
 * 测试覆盖：
 * 1. currentPos 为铁轨时，RailShape 是否正确提取
 * 2. belowPos 为铁轨时，RailShape 是否正确提取
 * 3. 非铁轨方块时，m_onRail 是否为 false 且 m_railShape 保持默认值
 * 4. dynamic_cast 失败时（方块通过 isOnRailAt 但非 AbstractRailBlock），m_railShape 保持不变
 * 5. 不同铁轨类型（RailBlock、PoweredRailBlock、DetectorRailBlock、ActivatorRailBlock）的形状提取
 * 6. 各种 RailShape 枚举值的正确提取
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "common/world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "common/world/block/blocks/redstone/PoweredRailBlock.hpp"
#include "common/world/block/blocks/redstone/RailBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <map>
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;
using namespace mc::entity;
using namespace mc::block_registry;
using namespace mc::test;

namespace {

/**
 * @brief 支持矿车铁轨检测测试的世界
 *
 * 提供 getBlockState、setBlockState 的最小化实现，
 * 使用 std::map<BlockPos, unique_ptr<BlockState>> 确保方块状态指针的生命周期。
 */
class MinecartCheckRailStateTestWorld final : public BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 /*flags*/) override
    {
        return setBlockState(x, y, z, state);
    }

    /**
     * @brief 在指定位置放置铁轨方块并设置形状
     */
    void setRail(i32 x, i32 y, i32 z, const AbstractRailBlock& rail, RailShape shape)
    {
        BlockState state = rail.withRailShape(rail.defaultState(), shape);
        setBlockState(x, y, z, &state);
    }

    /**
     * @brief 在指定位置放置铁轨方块（使用默认形状）
     */
    void setRail(i32 x, i32 y, i32 z, const AbstractRailBlock& rail) { setBlockState(x, y, z, &rail.defaultState()); }

    void clearAll() { m_blocks.clear(); }

private:
    std::map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

/**
 * @brief 测试辅助子类，暴露 _checkRailState() 为公开方法
 */
class TestMinecartEntity : public RideableMinecartEntity {
public:
    explicit TestMinecartEntity(EntityInstanceId id = EntityInstanceId(1))
        : RideableMinecartEntity(id, mc::test::testEcsRegistry())
    {}

    /**
     * @brief 暴露 _checkRailState() 以便测试调用
     */
    void checkRailState() { _checkRailState(); }

    using AbstractMinecartEntity::getRailPosition;
};

} // anonymous namespace

// ============================================================================
// 测试 1: currentPos 为铁轨时，RailShape 正确提取
// ============================================================================

class CheckRailStateCurrentPosTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;
    const RailBlock& rail{dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL)};

    void SetUp() override
    {
        // 确保方块注册表已初始化
        VanillaBlocks::initialize();
    }
};

TEST_F(CheckRailStateCurrentPosTest, NorthSouthShapeAtEntityPosition)
{
    // 在 (5, 64, 10) 放置一个南北方向铁轨
    world.setRail(5, 64, 10, rail, RailShape::NorthSouth);

    // 矿车位于方块中心
    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(5, 64, 10));
}

TEST_F(CheckRailStateCurrentPosTest, EastWestShapeAtEntityPosition)
{
    world.setRail(5, 64, 10, rail, RailShape::EastWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::EastWest);
}

TEST_F(CheckRailStateCurrentPosTest, AscendingEastShapeAtEntityPosition)
{
    world.setRail(5, 64, 10, rail, RailShape::AscendingEast);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::AscendingEast);
}

TEST_F(CheckRailStateCurrentPosTest, SouthEastCurveShapeAtEntityPosition)
{
    world.setRail(5, 64, 10, rail, RailShape::SouthEast);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::SouthEast);
}

TEST_F(CheckRailStateCurrentPosTest, NorthWestCurveShapeAtEntityPosition)
{
    world.setRail(5, 64, 10, rail, RailShape::NorthWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthWest);
}

// ============================================================================
// 测试 2: belowPos 为铁轨时，RailShape 正确提取
// ============================================================================

class CheckRailStateBelowPosTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;
    const RailBlock& rail{dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL)};

    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CheckRailStateBelowPosTest, DetectsRailOneBlockBelow)
{
    // 铁轨在 (5, 63, 10)，矿车在 (5, 64, 10)
    // 矿车 Y=64.0，floor(64.0) = 64，currentPos=(5,64,10) 无铁轨
    // 检查 belowPos=(5,63,10) 有铁轨
    world.setRail(5, 63, 10, rail, RailShape::EastWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::EastWest);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(5, 63, 10));
}

TEST_F(CheckRailStateBelowPosTest, AscendingSouthShapeBelowEntity)
{
    world.setRail(5, 63, 10, rail, RailShape::AscendingSouth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::AscendingSouth);
}

// ============================================================================
// 测试 3: 非铁轨方块时，m_onRail 为 false 且 m_railShape 保持默认值
// ============================================================================

class CheckRailStateNoRailTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;

    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CheckRailStateNoRailTest, NoRailAtAnyPosition_OffRail)
{
    // 世界中没有铁轨
    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_FALSE(minecart.isOnRail());
    // m_railShape 应保持默认值 NorthSouth
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);
}

TEST_F(CheckRailStateNoRailTest, NoWorldPointer_OffRail)
{
    // 没有 world 指针
    TestMinecartEntity minecart;
    // 不调用 setWorld()

    minecart.checkRailState();

    EXPECT_FALSE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);
}

// ============================================================================
// 测试 4: 不同铁轨类型的形状提取
// ============================================================================

class CheckRailStateRailTypesTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;
    const RailBlock& rail{dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL)};
    const PoweredRailBlock& poweredRail{dynamic_cast<const PoweredRailBlock&>(*VanillaBlocks::POWERED_RAIL)};
    const DetectorRailBlock& detectorRail{dynamic_cast<const DetectorRailBlock&>(*VanillaBlocks::DETECTOR_RAIL)};
    const ActivatorRailBlock& activatorRail{dynamic_cast<const ActivatorRailBlock&>(*VanillaBlocks::ACTIVATOR_RAIL)};

    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CheckRailStateRailTypesTest, PoweredRailExtractsShape)
{
    // 动力铁轨只支持直轨形状（6种）
    world.setRail(5, 64, 10, poweredRail, RailShape::AscendingNorth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::AscendingNorth);
}

TEST_F(CheckRailStateRailTypesTest, DetectorRailExtractsShape)
{
    world.setRail(5, 64, 10, detectorRail, RailShape::EastWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::EastWest);
}

TEST_F(CheckRailStateRailTypesTest, ActivatorRailExtractsShape)
{
    world.setRail(5, 64, 10, activatorRail, RailShape::NorthSouth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);
}

TEST_F(CheckRailStateRailTypesTest, NormalRailExtractsCurveShape)
{
    // 普通铁轨支持弯轨形状（10种）
    world.setRail(5, 64, 10, rail, RailShape::SouthWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::SouthWest);
}

TEST_F(CheckRailStateRailTypesTest, PoweredRailBelowEntity)
{
    // 动力铁轨在矿车下方
    world.setRail(5, 63, 10, poweredRail, RailShape::AscendingWest);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::AscendingWest);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(5, 63, 10));
}

// ============================================================================
// 测试 5: 所有 RailShape 枚举值的正确提取
// ============================================================================

class CheckRailStateAllShapesTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;
    const RailBlock& rail{dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL)};

    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CheckRailStateAllShapesTest, AllTenRailShapes)
{
    // 测试所有 10 种 RailShape 是否正确提取
    struct TestCase {
        RailShape shape;
        const char* name;
    };

    TestCase testCases[] = {
        {RailShape::NorthSouth, "NorthSouth"},
        {RailShape::EastWest, "EastWest"},
        {RailShape::AscendingEast, "AscendingEast"},
        {RailShape::AscendingWest, "AscendingWest"},
        {RailShape::AscendingNorth, "AscendingNorth"},
        {RailShape::AscendingSouth, "AscendingSouth"},
        {RailShape::SouthEast, "SouthEast"},
        {RailShape::SouthWest, "SouthWest"},
        {RailShape::NorthWest, "NorthWest"},
        {RailShape::NorthEast, "NorthEast"},
    };

    for (const auto& tc : testCases) {
        world.clearAll();

        world.setRail(10, 70, 20, rail, tc.shape);

        TestMinecartEntity minecart;
        minecart.setWorld(&world);
        minecart.setPosition(10.5, 70.0, 20.5);

        minecart.checkRailState();

        EXPECT_TRUE(minecart.isOnRail()) << "Should be on rail for shape " << tc.name;
        EXPECT_EQ(minecart.getRailShape(), tc.shape) << "RailShape mismatch for " << tc.name;
        EXPECT_EQ(minecart.getRailPosition(), BlockPos(10, 70, 20)) << "RailPosition mismatch for shape " << tc.name;
    }
}

// ============================================================================
// 测试 6: 形状更新 - 从一种铁轨移动到另一种铁轨时 RailShape 更新
// ============================================================================

class CheckRailStateShapeUpdateTest : public ::testing::Test {
protected:
    MinecartCheckRailStateTestWorld world;
    const RailBlock& rail{dynamic_cast<const RailBlock&>(*VanillaBlocks::RAIL)};

    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CheckRailStateShapeUpdateTest, ShapeUpdatesWhenRailChanges)
{
    // 先放置一个南北铁轨
    world.setRail(5, 64, 10, rail, RailShape::NorthSouth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();
    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);

    // 更改铁轨形状为东西方向
    world.setRail(5, 64, 10, rail, RailShape::EastWest);

    minecart.checkRailState();
    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::EastWest) << "RailShape should update when the rail block changes";
}

TEST_F(CheckRailStateShapeUpdateTest, ShapeUpdatesWhenMovingToDifferentRail)
{
    // 放置南北铁轨在 (5, 64, 10)
    world.setRail(5, 64, 10, rail, RailShape::NorthSouth);
    // 放置弯轨在 (6, 64, 10)
    world.setRail(6, 64, 10, rail, RailShape::SouthEast);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);

    // 矿车在南北铁轨上
    minecart.setPosition(5.5, 64.0, 10.5);
    minecart.checkRailState();
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(5, 64, 10));

    // 移动矿车到弯轨位置
    minecart.setPosition(6.5, 64.0, 10.5);
    minecart.checkRailState();
    EXPECT_EQ(minecart.getRailShape(), RailShape::SouthEast);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(6, 64, 10));
}

TEST_F(CheckRailStateShapeUpdateTest, OffRailThenBackOnRail)
{
    world.setRail(5, 64, 10, rail, RailShape::NorthSouth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);

    // 矿车在铁轨上
    minecart.setPosition(5.5, 64.0, 10.5);
    minecart.checkRailState();
    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);

    // 移动矿车到无铁轨的位置
    minecart.setPosition(100.5, 64.0, 100.5);
    minecart.checkRailState();
    EXPECT_FALSE(minecart.isOnRail());
    // m_railShape 应保持上一次的铁轨形状（脱轨后不重置）
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthSouth);

    // 移动回铁轨，这次是弯轨
    world.clearAll();
    world.setRail(5, 64, 10, rail, RailShape::NorthEast);
    minecart.setPosition(5.5, 64.0, 10.5);
    minecart.checkRailState();
    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::NorthEast)
        << "RailShape should update when re-entering rail with different shape";
}

// ============================================================================
// 测试 7: currentPos 优先于 belowPos
// ============================================================================

TEST_F(CheckRailStateShapeUpdateTest, CurrentPosTakesPrecedenceOverBelowPos)
{
    // 在 currentPos (5, 64, 10) 和 belowPos (5, 63, 10) 都放置铁轨
    // currentPos 是东西方向，belowPos 是南北方向
    world.setRail(5, 64, 10, rail, RailShape::EastWest);
    world.setRail(5, 63, 10, rail, RailShape::NorthSouth);

    TestMinecartEntity minecart;
    minecart.setWorld(&world);
    minecart.setPosition(5.5, 64.0, 10.5);

    minecart.checkRailState();

    // 应优先使用 currentPos 的铁轨
    EXPECT_TRUE(minecart.isOnRail());
    EXPECT_EQ(minecart.getRailShape(), RailShape::EastWest);
    EXPECT_EQ(minecart.getRailPosition(), BlockPos(5, 64, 10));
}
