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
 * IMPLIED, NONINFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"

namespace mc {
namespace entity {

/**
 * @brief 测试辅助类，用于访问 FishingBobberEntity 的私有方法
 *
 * 必须在 mc::entity 命名空间中，以便 friend 声明能正确引用。
 */
class FishingBobberTestAccess {
public:
    static bool checkOpenWater(FishingBobberEntity& bobber) { return bobber._checkOpenWater(); }

    static FishingWaterType getOpenWaterTypeForBlock(const FishingBobberEntity& bobber, const BlockPos& pos)
    {
        return bobber._getOpenWaterTypeForBlock(pos);
    }

    static FishingWaterType getOpenWaterTypeForArea(
        const FishingBobberEntity& bobber, const BlockPos& from, const BlockPos& to)
    {
        return bobber._getOpenWaterTypeForArea(from, to);
    }
};

} // namespace entity

// 别名，方便在匿名命名空间的测试中使用
using OpenWaterAccess = entity::FishingBobberTestAccess;

namespace {

using mc::entity::FishingWaterType;

/**
 * @brief 开放水域检测测试世界
 *
 * 重写 getBlockState 和 getFluidState 以提供可控的方块/流体状态，
 * 用于测试 FishingBobberEntity::_checkOpenWater 的分层检测逻辑。
 */
class OpenWaterTestWorld : public mc::test::BaseTestWorld {
public:
    OpenWaterTestWorld() = default;

    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }

    void setFluidStateAt(i32 x, i32 y, i32 z, const fluid::FluidState* state)
    {
        m_fluidStates[BlockPos(x, y, z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_fluidStates.find(BlockPos(x, y, z));
        return it != m_fluidStates.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, const fluid::FluidState*> m_fluidStates;
};

/**
 * @brief 开放水域检测测试
 */
class OpenWaterCheckTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<OpenWaterTestWorld>();
        // 确保方块和流体注册表已初始化
        VanillaBlocks::initialize();
        fluid::Fluids::initialize();
    }

    void TearDown() override { m_world.reset(); }

    /**
     * @brief 在指定位置填充水源方块
     *
     * 设置方块状态为水、流体状态为水源。水源方块无碰撞箱。
     *
     * @param centerX 中心X坐标
     * @param baseY 基准Y坐标（浮标所在层）
     * @param centerZ 中心Z坐标
     * @param layers 要填充的层数（从 baseY-1 开始）
     */
    void fillWaterSource(i32 centerX, i32 baseY, i32 centerZ, i32 layers = 4)
    {
        const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
        const fluid::FluidState* waterFluid = &fluid::Fluids::WATER()->defaultState();

        for (i32 dy = -1; dy < layers - 1; ++dy) {
            for (i32 dx = -2; dx <= 2; ++dx) {
                for (i32 dz = -2; dz <= 2; ++dz) {
                    m_world->setBlockStateAt(centerX + dx, baseY + dy, centerZ + dz, waterState);
                    m_world->setFluidStateAt(centerX + dx, baseY + dy, centerZ + dz, waterFluid);
                }
            }
        }
    }

    /**
     * @brief 在指定层填充空气
     */
    void fillAir(i32 centerX, i32 baseY, i32 centerZ, i32 layerOffset)
    {
        // 空气通过不设置任何方块状态来模拟（getBlockState 返回 nullptr）
        // _getOpenWaterTypeForBlock 中 nullptr 返回 Invalid，
        // 但空气方块的 isAir() 返回 true → AboveWater
        // 需要设置空气方块状态
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        for (i32 dx = -2; dx <= 2; ++dx) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                m_world->setBlockStateAt(centerX + dx, baseY + layerOffset, centerZ + dz, airState);
            }
        }
    }

    std::unique_ptr<OpenWaterTestWorld> m_world;
};

// ============================================================================
// 前置条件测试
// ============================================================================

/**
 * @brief 测试无方块时 _checkOpenWater 返回 false
 *
 * 没有方块和流体时，所有位置返回 nullptr → Invalid → 整体不满足开放水域
 */
TEST_F(OpenWaterCheckTest, EmptyWorldReturnsFalse)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 空世界，_checkOpenWater 应返回 false
    EXPECT_FALSE(OpenWaterAccess::checkOpenWater(bobber));
}

/**
 * @brief 测试水源方块的默认碰撞箱为空
 *
 * 验证水源方块的 getCollisionShape().isEmpty() 返回 true，
 * 这是 _checkOpenWater 中 InsideWater 类型的必要条件。
 */
TEST_F(OpenWaterCheckTest, WaterSourceBlockHasEmptyCollisionShape)
{
    const BlockState& waterState = VanillaBlocks::WATER->defaultState();
    EXPECT_TRUE(waterState.getCollisionShape().isEmpty()) << "水源方块应该有空碰撞箱";
}

/**
 * @brief 测试水源流体是 source
 *
 * 验证 Fluids::WATER 的默认状态是 source。
 */
TEST_F(OpenWaterCheckTest, WaterFluidIsSource)
{
    const fluid::FluidState& waterFluid = fluid::Fluids::WATER()->defaultState();
    EXPECT_TRUE(waterFluid.isSource()) << "默认水流体应该是 source";
    EXPECT_TRUE(waterFluid.getFluid().isIn(fluid::FluidTags::WATER())) << "默认水流体应该属于 WATER 标签";
}

/**
 * @brief 测试 LILY_PAD 方块识别
 *
 * 验证 NaturalBlocks::LILY_PAD 可正确识别，_checkOpenWater 中
 * 睡莲应被分类为 AboveWater。
 */
TEST_F(OpenWaterCheckTest, LilyPadIsRecognized)
{
    ASSERT_NE(block_registry::NaturalBlocks::LILY_PAD, nullptr) << "NaturalBlocks::LILY_PAD 应该已注册";
}

/**
 * @brief 测试 FishingWaterType 枚举值
 *
 * 验证 FishingWaterType 枚举有正确的值。
 */
TEST_F(OpenWaterCheckTest, WaterTypeEnumValues)
{
    EXPECT_EQ(static_cast<int>(FishingWaterType::AboveWater), 0);
    EXPECT_EQ(static_cast<int>(FishingWaterType::InsideWater), 1);
    EXPECT_EQ(static_cast<int>(FishingWaterType::Invalid), 2);
}

// ============================================================================
// _getOpenWaterTypeForBlock 测试
// ============================================================================

/**
 * @brief 测试 _getOpenWaterTypeForBlock 对空气返回 AboveWater
 */
TEST_F(OpenWaterCheckTest, GetOpenWaterTypeForBlock_Air)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 66.0, 0.0);
    bobber.setWorld(m_world.get());

    // 设置空气方块
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    m_world->setBlockStateAt(0, 66, 0, airState);

    EXPECT_EQ(OpenWaterAccess::getOpenWaterTypeForBlock(bobber, BlockPos(0, 66, 0)), FishingWaterType::AboveWater);
}

/**
 * @brief 测试 _getOpenWaterTypeForBlock 对水源返回 InsideWater
 */
TEST_F(OpenWaterCheckTest, GetOpenWaterTypeForBlock_WaterSource)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 设置水源方块和流体
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    const fluid::FluidState* waterFluid = &fluid::Fluids::WATER()->defaultState();
    m_world->setBlockStateAt(0, 64, 0, waterState);
    m_world->setFluidStateAt(0, 64, 0, waterFluid);

    EXPECT_EQ(OpenWaterAccess::getOpenWaterTypeForBlock(bobber, BlockPos(0, 64, 0)), FishingWaterType::InsideWater);
}

/**
 * @brief 测试 _getOpenWaterTypeForBlock 对石块返回 Invalid
 */
TEST_F(OpenWaterCheckTest, GetOpenWaterTypeForBlock_Stone)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 石头不是空气、不是睡莲、不是水源 → Invalid
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockStateAt(0, 64, 0, stoneState);

    EXPECT_EQ(OpenWaterAccess::getOpenWaterTypeForBlock(bobber, BlockPos(0, 64, 0)), FishingWaterType::Invalid);
}

/**
 * @brief 测试 _getOpenWaterTypeForBlock 对无方块返回 Invalid
 */
TEST_F(OpenWaterCheckTest, GetOpenWaterTypeForBlock_Nullptr)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 未设置方块 → nullptr → Invalid
    EXPECT_EQ(OpenWaterAccess::getOpenWaterTypeForBlock(bobber, BlockPos(0, 64, 0)), FishingWaterType::Invalid);
}

// ============================================================================
// _checkOpenWater 集成测试
// ============================================================================

/**
 * @brief 测试完整开放水域：4层全是水源
 *
 * Y-1 到 Y+2 全部为水源 → InsideWater → 返回 true
 */
TEST_F(OpenWaterCheckTest, CheckOpenWater_FullWaterSource)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 填充 4 层水源（Y-1 到 Y+2）
    fillWaterSource(0, 64, 0, 4);

    EXPECT_TRUE(OpenWaterAccess::checkOpenWater(bobber));
}

/**
 * @brief 测试开放水域：3层水 + 1层空气
 *
 * Y-1 到 Y+1 为水源(InsideWater)，Y+2 为空气(AboveWater)
 * 这是合法的分层：InsideWater → AboveWater
 */
TEST_F(OpenWaterCheckTest, CheckOpenWater_WaterWithAirAbove)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 填充 Y-1 到 Y+1 为水源（3层）
    fillWaterSource(0, 64, 0, 3);

    // Y+2 为空气
    fillAir(0, 64, 0, 2);

    EXPECT_TRUE(OpenWaterAccess::checkOpenWater(bobber));
}

/**
 * @brief 测试非开放水域：底层是空气
 *
 * Y-1 为空气(AboveWater)但 prevType=Invalid → 不满足 AboveWater 前面不能有 Invalid → 返回 false
 */
TEST_F(OpenWaterCheckTest, CheckOpenWater_AirAtBottomReturnsFalse)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // Y-1 为空气，Y+0 到 Y+2 为水源
    fillAir(0, 64, 0, -1);        // Y-1
    fillWaterSource(0, 64, 0, 3); // Y+0 到 Y+2

    // Y-1 层是 AboveWater，但 prevType = Invalid（初始值），
    // AboveWater 不能出现在 prevType=Invalid 之后 → false
    EXPECT_FALSE(OpenWaterAccess::checkOpenWater(bobber));
}

/**
 * @brief 测试非开放水域：中间有石头
 *
 * 石头为 Invalid → 直接返回 false
 */
TEST_F(OpenWaterCheckTest, CheckOpenWater_StoneInWaterReturnsFalse)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 填充 4 层水源
    fillWaterSource(0, 64, 0, 4);

    // 在 Y+0 中心放一个石头，使其变为 Invalid
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockStateAt(0, 64, 0, stoneState);

    EXPECT_FALSE(OpenWaterAccess::checkOpenWater(bobber));
}

/**
 * @brief 测试非开放水域：从空气回到水
 *
 * Y-1 到 Y+0 为水源(InsideWater)，Y+1 为空气(AboveWater)，Y+2 为水源(InsideWater)
 * InsideWater → AboveWater → InsideWater 是非法过渡 → 返回 false
 */
TEST_F(OpenWaterCheckTest, CheckOpenWater_WaterAirWaterReturnsFalse)
{
    entity::FishingBobberEntity bobber(EntityInstanceId(1), mc::test::testEcsRegistry());
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // Y-1, Y+0 为水源
    fillWaterSource(0, 64, 0, 2);

    // Y+1 为空气
    fillAir(0, 64, 0, 1);

    // Y+2 为水源
    fillWaterSource(0, 64, 0, 1);
    // 只覆盖 Y+2 层，需要单独设置
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    const fluid::FluidState* waterFluid = &fluid::Fluids::WATER()->defaultState();
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dz = -2; dz <= 2; ++dz) {
            m_world->setBlockStateAt(dx, 66, dz, waterState);
            m_world->setFluidStateAt(dx, 66, dz, waterFluid);
        }
    }

    // InsideWater → AboveWater → InsideWater 是非法过渡
    EXPECT_FALSE(OpenWaterAccess::checkOpenWater(bobber));
}

} // namespace
} // namespace mc
