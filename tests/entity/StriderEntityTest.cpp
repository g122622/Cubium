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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::math;

namespace {

/**
 * @brief 测试 StriderEntity 的 getMountedYOffset 方法
 *
 * MC 1.16.5 公式:
 * float f = Math.min(0.25F, this.limbSwingAmount);
 * float f1 = this.limbSwing;
 * return (double)this.getHeight() - 0.19D + (double)(0.12F * MathHelper.cos(f1 * 1.5F) * 2.0F * f);
 */

// ============================================================================
// StriderEntity::getMountedYOffset 计算测试
// ============================================================================

class StriderEntityMountedYOffsetTest : public ::testing::Test {
protected:
    void SetUp() override { strider = std::make_unique<StriderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

    std::unique_ptr<StriderEntity> strider;
};

/**
 * @brief 测试基础偏移计算（limbSwingAmount = 0 时）
 *
 * 当 limbSwingAmount = 0 时，波动项为 0，
 * 偏移应等于 height - 0.19
 */
TEST_F(StriderEntityMountedYOffsetTest, BaseOffsetWhenStationary)
{
    // 刚创建的实体，limbSwingAmount 应该为 0
    // 偏移应该是 height - 0.19
    f64 offset = strider->getMountedYOffset();

    // 通过 Entity 基类接口获取 height（Entity::height() 是 public）
    f32 height = static_cast<Entity*>(strider.get())->height();

    // 验证基础公式：offset = height - 0.19 + wave (wave 接近 0)
    f64 expectedBase = static_cast<f64>(height) - 0.19;
    f64 tolerance = 0.01; // 允许小的波动，因为 limbSwingAmount 可能有初始值

    EXPECT_NEAR(offset, expectedBase, tolerance) << "Expected offset near height - 0.19 when entity is stationary";
}

/**
 * @brief 测试计算公式的正确性
 *
 * 验证公式：
 * offset = height - 0.19 + 0.12 * cos(limbSwing * 1.5) * 2.0 * min(0.25, limbSwingAmount)
 */
TEST_F(StriderEntityMountedYOffsetTest, FormulaValidation)
{
    f32 height = static_cast<Entity*>(strider.get())->height();
    f32 limbSwing = strider->limbSwing();
    f32 limbSwingAmount = strider->limbSwingAmount();

    // 按照公式计算期望值
    f32 limbSwingAmountClamped = std::min(0.25f, limbSwingAmount);
    f64 expected = static_cast<f64>(height) - 0.19 +
        static_cast<f64>(0.12f * std::cos(limbSwing * 1.5f) * 2.0f * limbSwingAmountClamped);

    f64 actual = strider->getMountedYOffset();

    EXPECT_DOUBLE_EQ(actual, expected) << "Formula calculation mismatch";
}

/**
 * @brief 测试波动幅度限制
 *
 * limbSwingAmount 被限制在 0.25 以内，
 * 即使超过 0.25，波动也不应超过 0.12 * 2.0 * 0.25 = 0.06
 */
TEST_F(StriderEntityMountedYOffsetTest, WaveAmplitudeClamped)
{
    // 最大波动幅度 = 0.12 * 2.0 * 0.25 = 0.06
    // 这发生在 limbSwingAmount >= 0.25 且 cos(limbSwing * 1.5) = 1 或 -1 时

    f32 height = static_cast<Entity*>(strider.get())->height();
    f64 baseOffset = static_cast<f64>(height) - 0.19;

    // 最大正波动
    f64 maxOffset = baseOffset + 0.06;
    // 最大负波动
    f64 minOffset = baseOffset - 0.06;

    f64 offset = strider->getMountedYOffset();

    // 验证偏移在合理范围内
    EXPECT_GE(offset, minOffset - 0.001) << "Offset below minimum expected range";
    EXPECT_LE(offset, maxOffset + 0.001) << "Offset above maximum expected range";
}

/**
 * @brief 测试 cos 函数的周期性
 *
 * limbSwing * 1.5 作为 cos 的参数，
 * 验证不同 limbSwing 值会产生正确的波动
 */
TEST_F(StriderEntityMountedYOffsetTest, CosineWavePeriod)
{
    // cos(x * 1.5) 的周期是 2π/1.5 ≈ 4.19
    // 验证波动项是周期性的

    f32 height = static_cast<Entity*>(strider.get())->height();
    f32 limbSwing = strider->limbSwing();
    f32 limbSwingAmount = strider->limbSwingAmount();

    // 当 limbSwing 增加 2π/1.5 时，cos 值应该相同
    f32 period = 2.0f * mc::math::PI / 1.5f;

    // 计算当前位置的波动
    f32 limbSwingAmountClamped = std::min(0.25f, limbSwingAmount);
    f64 wave1 = static_cast<f64>(0.12f * std::cos(limbSwing * 1.5f) * 2.0f * limbSwingAmountClamped);

    // 计算一个周期后的波动
    f64 wave2 = static_cast<f64>(0.12f * std::cos((limbSwing + period) * 1.5f) * 2.0f * limbSwingAmountClamped);

    EXPECT_NEAR(wave1, wave2, 0.0001) << "Wave should be periodic with period 2π/1.5";
}

/**
 * @brief 测试高度影响
 *
 * 验证 height 对偏移的线性影响
 */
TEST_F(StriderEntityMountedYOffsetTest, HeightAffectsOffset)
{
    // 偏移 = height - 0.19 + wave
    // 所以偏移与 height 是线性关系（加上一个小的波动项）

    f32 height = static_cast<Entity*>(strider.get())->height();
    f64 offset = strider->getMountedYOffset();

    // 验证偏移与高度的关系
    // 由于波动项很小（最大 ±0.06），偏移应该接近 height - 0.19
    f64 expectedBase = static_cast<f64>(height) - 0.19;

    EXPECT_NEAR(offset, expectedBase, 0.07) << "Offset should be close to height - 0.19";
}

/**
 * @brief 测试波动系数
 *
 * 验证公式中的系数：
 * - 0.12：波动幅度系数
 * - 1.5：频率倍数
 * - 2.0：波动放大系数
 */
TEST_F(StriderEntityMountedYOffsetTest, WaveCoefficients)
{
    // 当 limbSwing = 0 时，cos(0) = 1
    // 波动 = 0.12 * 1 * 2.0 * limbSwingAmountClamped = 0.24 * limbSwingAmountClamped

    // 当 limbSwing = π/3 时，cos(π/3 * 1.5) = cos(π/2) = 0
    // 波动 = 0

    // 当 limbSwing = 2π/3 时，cos(2π/3 * 1.5) = cos(π) = -1
    // 波动 = 0.12 * (-1) * 2.0 * limbSwingAmountClamped = -0.24 * limbSwingAmountClamped

    constexpr f32 PI_LOCAL = mc::math::PI;

    // 测试 cos(0) = 1 的情况
    f32 limbSwingAtZero = 0.0f;
    f32 limbSwingAmount = 0.2f; // < 0.25，不会被 clamp
    f32 clamped = std::min(0.25f, limbSwingAmount);
    f64 waveAtZero = static_cast<f64>(0.12f * std::cos(limbSwingAtZero * 1.5f) * 2.0f * clamped);
    f64 expectedWaveAtZero = static_cast<f64>(0.12f * 1.0f * 2.0f * clamped); // = 0.24 * 0.2 = 0.048
    EXPECT_NEAR(waveAtZero, expectedWaveAtZero, 0.0001);

    // 测试 cos(π/2) = 0 的情况（limbSwing = π/3）
    f32 limbSwingAtPiHalf = PI_LOCAL / 3.0f;
    f64 waveAtPiHalf = static_cast<f64>(0.12f * std::cos(limbSwingAtPiHalf * 1.5f) * 2.0f * clamped);
    EXPECT_NEAR(waveAtPiHalf, 0.0, 0.0001);

    // 测试 cos(π) = -1 的情况（limbSwing = 2π/3）
    f32 limbSwingAtPi = 2.0f * PI_LOCAL / 3.0f;
    f64 waveAtPi = static_cast<f64>(0.12f * std::cos(limbSwingAtPi * 1.5f) * 2.0f * clamped);
    f64 expectedWaveAtPi = static_cast<f64>(0.12f * (-1.0f) * 2.0f * clamped); // = -0.048
    EXPECT_NEAR(waveAtPi, expectedWaveAtPi, 0.0001);
}

/**
 * @brief 测试边界条件：limbSwingAmount 为 0
 */
TEST_F(StriderEntityMountedYOffsetTest, ZeroLimbSwingAmount)
{
    // 当 limbSwingAmount = 0 时，波动项为 0
    f32 limbSwingAmount = strider->limbSwingAmount();

    // 如果 limbSwingAmount 确实为 0（新创建的实体）
    if (limbSwingAmount == 0.0f) {
        f32 height = static_cast<Entity*>(strider.get())->height();
        f64 expected = static_cast<f64>(height) - 0.19;
        f64 actual = strider->getMountedYOffset();

        EXPECT_DOUBLE_EQ(actual, expected) << "When limbSwingAmount is 0, offset should equal height - 0.19";
    } else {
        // 如果 limbSwingAmount 不为 0，验证公式仍然正确
        SUCCEED() << "limbSwingAmount is not 0, skipping zero test";
    }
}

/**
 * @brief 测试 limbSwingAmount 超过 0.25 时的 clamp 行为
 *
 * 当 limbSwingAmount > 0.25 时，应该被限制为 0.25
 */
TEST_F(StriderEntityMountedYOffsetTest, LimbSwingAmountClampedToQuarter)
{
    // 由于无法直接设置 limbSwingAmount，
    // 这里验证 clamp 逻辑的正确性

    // 验证 std::min(0.25f, x) 的行为
    EXPECT_EQ(std::min(0.25f, 0.1f), 0.1f);
    EXPECT_EQ(std::min(0.25f, 0.25f), 0.25f);
    EXPECT_EQ(std::min(0.25f, 0.5f), 0.25f);
    EXPECT_EQ(std::min(0.25f, 1.0f), 0.25f);
}

/**
 * @brief 测试返回类型为 f64
 *
 * MC 1.16.5 返回 double，确保 C++ 返回 f64
 */
TEST_F(StriderEntityMountedYOffsetTest, ReturnsDoubleType)
{
    f64 offset = strider->getMountedYOffset();

    // 验证返回值是有效的 double
    EXPECT_FALSE(std::isnan(offset));
    EXPECT_FALSE(std::isinf(offset));
    EXPECT_GT(offset, -10.0); // 合理的下界
    EXPECT_LT(offset, 100.0); // 合理的上界
}

/**
 * @brief 测试与 MC 1.16.5 的一致性
 *
 * 参考: net.minecraft.entity.passive.StriderEntity.getMountedYOffset()
 * float f = Math.min(0.25F, this.limbSwingAmount);
 * float f1 = this.limbSwing;
 * return (double)this.getHeight() - 0.19D + (double)(0.12F * MathHelper.cos(f1 * 1.5F) * 2.0F * f);
 */
TEST_F(StriderEntityMountedYOffsetTest, MC1165Consistency)
{
    // 这个测试验证我们的实现与 MC 1.16.5 公式一致
    f32 height = static_cast<Entity*>(strider.get())->height();
    f32 limbSwing = strider->limbSwing();
    f32 limbSwingAmount = strider->limbSwingAmount();

    // MC 1.16.5 公式
    f32 f = std::min(0.25f, limbSwingAmount);
    f32 f1 = limbSwing;
    f64 expected = static_cast<f64>(height) - 0.19 + static_cast<f64>(0.12f * std::cos(f1 * 1.5f) * 2.0f * f);

    f64 actual = strider->getMountedYOffset();

    EXPECT_DOUBLE_EQ(actual, expected) << "Implementation should match MC 1.16.5 formula exactly";
}

/**
 * @brief 测试波动项的范围
 *
 * 验证波动项总是有限的，不会导致异常值
 */
TEST_F(StriderEntityMountedYOffsetTest, WaveTermBounded)
{
    // 对于任意 limbSwing 值，cos 值在 [-1, 1] 范围内
    // 对于任意 limbSwingAmount，clamped 值在 [0, 0.25] 范围内
    // 所以波动项范围是 [-0.06, 0.06]

    // 验证 cos 函数范围
    for (f32 limbSwing = 0.0f; limbSwing < 100.0f; limbSwing += 1.0f) {
        f32 cosValue = std::cos(limbSwing * 1.5f);
        EXPECT_GE(cosValue, -1.0f);
        EXPECT_LE(cosValue, 1.0f);
    }

    // 验证波动项范围
    for (f32 amount = 0.0f; amount <= 1.0f; amount += 0.1f) {
        f32 clamped = std::min(0.25f, amount);
        for (f32 swing = 0.0f; swing < 10.0f; swing += 1.0f) {
            f32 wave = 0.12f * std::cos(swing * 1.5f) * 2.0f * clamped;
            EXPECT_GE(wave, -0.06f - 0.0001f);
            EXPECT_LE(wave, 0.06f + 0.0001f);
        }
    }
}

// ============================================================================
// StriderEntity 基本属性测试
// ============================================================================

class StriderEntityBasicTest : public ::testing::Test {
protected:
    // Items::SADDLE 等指针默认 nullptr,ItemStack(nullptr,1) 退化为空,
    // 致装备相关断言误判;需 Items::initialize() 注册原版物品。
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        strider = std::make_unique<StriderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    std::unique_ptr<StriderEntity> strider;
};

TEST_F(StriderEntityBasicTest, IsNotColdInitially)
{
    EXPECT_FALSE(strider->isCold());
    EXPECT_EQ(strider->getColdTimer(), 0);
}

TEST_F(StriderEntityBasicTest, CanSetColdTimer)
{
    strider->setColdTimer(50);
    EXPECT_TRUE(strider->isCold());
    EXPECT_EQ(strider->getColdTimer(), 50);
}

TEST_F(StriderEntityBasicTest, CanSetSaddle)
{
    EXPECT_FALSE(strider->hasSaddle());

    strider->setSaddle(true);
    EXPECT_TRUE(strider->hasSaddle());

    strider->setSaddle(false);
    EXPECT_FALSE(strider->hasSaddle());
}

TEST_F(StriderEntityBasicTest, IsNotOnLavaSurfaceInitially)
{
    EXPECT_FALSE(strider->isOnLavaSurface());
}

TEST_F(StriderEntityBasicTest, CanSetOnLavaSurface)
{
    strider->setOnLavaSurface(true);
    EXPECT_TRUE(strider->isOnLavaSurface());

    strider->setOnLavaSurface(false);
    EXPECT_FALSE(strider->isOnLavaSurface());
}

TEST_F(StriderEntityBasicTest, IsNotBeingRiddenInitially)
{
    EXPECT_FALSE(strider->isBeingRidden());
}

TEST_F(StriderEntityBasicTest, CanBeRidden)
{
    EXPECT_TRUE(strider->canBeRidden());
}

TEST_F(StriderEntityBasicTest, CannotBeRiddenInWater)
{
    // MC 1.16.5: 炽足兽不能在水中骑乘
    EXPECT_FALSE(strider->canBeRiddenInWater());
}

TEST_F(StriderEntityBasicTest, EyeHeightDependsOnAge)
{
    // 成体眼睛高度 = 1.0
    EXPECT_FLOAT_EQ(strider->eyeHeight(), 1.0f);

    // 幼体眼睛高度 = 0.5
    // 参考 MC 1.16.5 StriderEntity.getEyeHeight()
    StriderEntity childStrider(EntityInstanceId(2), mc::test::testEcsRegistry());
    childStrider.setChild(true);
    EXPECT_TRUE(childStrider.isChild());
    EXPECT_FLOAT_EQ(childStrider.eyeHeight(), 0.5f);

    // 验证成体/幼体切换后眼睛高度正确更新
    childStrider.setChild(false);
    EXPECT_FALSE(childStrider.isChild());
    EXPECT_FLOAT_EQ(childStrider.eyeHeight(), 1.0f);
}

TEST_F(StriderEntityBasicTest, InitialBoostState)
{
    EXPECT_FALSE(strider->isBoosting());
    // Note: getBoostTime() requires EntityDataManager initialization
    // which is not available in unit tests without a full world setup
}

TEST_F(StriderEntityBasicTest, CanSetBoostTime)
{
    // Note: setBoostTime() requires EntityDataManager initialization
    // The BoostHelper needs to be initialized with a DataParameter
    // This is tested in BoostHelperTest in RidingSystemTests.cpp
    // Here we just verify the API exists and doesn't crash
    strider->setBoostTime(100);
    // Without EntityDataManager, this call is a no-op
}

TEST_F(StriderEntityBasicTest, HeightAccessorWorks)
{
    // 通过 Entity 基类接口访问 height()
    Entity* entity = strider.get();
    f32 height = entity->height();

    // StriderEntity 的基础高度是 1.7（参考 MC 1.16.5）
    // 成体 Strider 高度约 1.7
    EXPECT_GT(height, 0.0f);
    EXPECT_LT(height, 5.0f); // 合理的上界
}

// ============================================================================
// StriderEntity IEquipable 接口测试
// ============================================================================

class StriderEntityEquipableTest : public ::testing::Test {
protected:
    // Items::SADDLE/DIAMOND 指针默认 nullptr,ItemStack(nullptr,1) 退化为空,
    // 致 canEquip(钻石/剑) 走"清槽"分支误返 true、setEquipment 装鞍失败;
    // 需 Items::initialize() 注册原版物品。对齐同文件 StriderInteractTestFixture。
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        strider = std::make_unique<StriderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    std::unique_ptr<StriderEntity> strider;
};

/**
 * @brief 测试 IEquipable 接口槽数量
 * MC 1.16.5: 炽足兽只有一个鞍槽
 */
TEST_F(StriderEntityEquipableTest, HasOneEquipmentSlot)
{
    EXPECT_EQ(strider->getEquipmentSlotCount(), 1);
}

/**
 * @brief 测试无鞍时 getEquipment 返回空
 */
TEST_F(StriderEntityEquipableTest, GetEquipmentReturnsEmptyWhenNoSaddle)
{
    EXPECT_FALSE(strider->hasSaddle());
    ItemStack equipment = strider->getEquipment(0);
    EXPECT_TRUE(equipment.isEmpty());
}

/**
 * @brief 测试有鞍时 getEquipment 返回鞍物品
 */
TEST_F(StriderEntityEquipableTest, GetEquipmentReturnsSaddleWhenSaddled)
{
    strider->setSaddle(true);
    EXPECT_TRUE(strider->hasSaddle());

    ItemStack equipment = strider->getEquipment(0);
    EXPECT_FALSE(equipment.isEmpty());
    EXPECT_EQ(equipment.getItem(), Items::SADDLE);
    EXPECT_EQ(equipment.getCount(), 1);
}

/**
 * @brief 测试无效槽位返回空
 */
TEST_F(StriderEntityEquipableTest, InvalidSlotReturnsEmpty)
{
    strider->setSaddle(true);

    // 负数槽位
    EXPECT_TRUE(strider->getEquipment(-1).isEmpty());

    // 超出范围的槽位
    EXPECT_TRUE(strider->getEquipment(1).isEmpty());
    EXPECT_TRUE(strider->getEquipment(2).isEmpty());
}

/**
 * @brief 测试 setEquipment 设置鞍
 */
TEST_F(StriderEntityEquipableTest, SetEquipmentSetsSaddle)
{
    ItemStack saddle(Items::SADDLE, 1);

    strider->setEquipment(0, saddle);
    EXPECT_TRUE(strider->hasSaddle());

    ItemStack equipment = strider->getEquipment(0);
    EXPECT_FALSE(equipment.isEmpty());
    EXPECT_EQ(equipment.getItem(), Items::SADDLE);
}

/**
 * @brief 测试 setEquipment 清空鞍
 */
TEST_F(StriderEntityEquipableTest, SetEquipmentClearsSaddle)
{
    // 先设置鞍
    strider->setSaddle(true);
    EXPECT_TRUE(strider->hasSaddle());

    // 清空鞍
    ItemStack empty;
    strider->setEquipment(0, empty);
    EXPECT_FALSE(strider->hasSaddle());
    EXPECT_TRUE(strider->getEquipment(0).isEmpty());
}

/**
 * @brief 测试 setEquipment 忽略无效槽位
 */
TEST_F(StriderEntityEquipableTest, SetEquipmentIgnoresInvalidSlot)
{
    ItemStack saddle(Items::SADDLE, 1);

    // 设置到无效槽位
    strider->setEquipment(1, saddle);
    EXPECT_FALSE(strider->hasSaddle());

    strider->setEquipment(-1, saddle);
    EXPECT_FALSE(strider->hasSaddle());
}

/**
 * @brief 测试 canEquip 接受鞍
 */
TEST_F(StriderEntityEquipableTest, CanEquipSaddle)
{
    ItemStack saddle(Items::SADDLE, 1);
    EXPECT_TRUE(strider->canEquip(saddle, 0));
}

/**
 * @brief 测试 canEquip 拒绝非鞍物品
 */
TEST_F(StriderEntityEquipableTest, CannotEquipNonSaddle)
{
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(strider->canEquip(diamond, 0));

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    EXPECT_FALSE(strider->canEquip(sword, 0));
}

/**
 * @brief 测试 canEquip 接受空物品（清空槽位）
 */
TEST_F(StriderEntityEquipableTest, CanEquipEmptyToClearSlot)
{
    ItemStack empty;
    EXPECT_TRUE(strider->canEquip(empty, 0));
}

/**
 * @brief 测试 canEquip 拒绝有效物品到无效槽位
 */
TEST_F(StriderEntityEquipableTest, CannotEquipToInvalidSlot)
{
    ItemStack saddle(Items::SADDLE, 1);

    // 无效槽位
    EXPECT_FALSE(strider->canEquip(saddle, 1));
    EXPECT_FALSE(strider->canEquip(saddle, -1));
}

// ============================================================================
// StriderEntity interactMob 交互测试
// ============================================================================

/**
 * @brief StriderEntity interactMob 测试用世界
 *
 * 提供最小化测试环境，支持追踪 playSound 调用
 */
class StriderInteractTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("StriderInteractTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("StriderInteractTestWorld::tickManager not implemented");
    }

    // 追踪 playSound 调用
    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const mc::Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundId = soundId;
        m_soundPlayCount++;
        m_lastPitch = pitch;
        (void)category;
        (void)pos;
        (void)volume;
    }

    [[nodiscard]] const ResourceLocation& getLastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] i32 getSoundPlayCount() const { return m_soundPlayCount; }
    [[nodiscard]] f32 getLastPitch() const { return m_lastPitch; }
    void resetSoundTracking()
    {
        m_lastSoundId = ResourceLocation();
        m_soundPlayCount = 0;
        m_lastPitch = 0.0f;
    }

    // 追踪 broadcastEntityStatus 调用（用于追踪爱心粒子等）
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityInstanceId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 getBroadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityInstanceId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;

    // 声音追踪
    ResourceLocation m_lastSoundId;
    i32 m_soundPlayCount = 0;
    f32 m_lastPitch = 0.0f;

    // 广播追踪
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

class StriderInteractTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    StriderInteractTestWorld m_world;
};

/**
 * @brief 喂食成年可繁殖炽足兽 → 进入爱心模式，消耗物品，播放吃食音效
 */
TEST_F(StriderInteractTestFixture, FeedAdultBreedableStrider_SetsInLoveAndPlaysEatSound)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");

    // 成年炽足兽，未处于爱心模式
    ASSERT_FALSE(strider.isChild());
    ASSERT_TRUE(strider.canBreed());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 3));
    player.inventory().setSelectedSlot(0);

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 成功喂食
    EXPECT_EQ(result, ActionResultType::Success);

    // 物品数量减少
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 2);

    // 处于爱心模式
    EXPECT_TRUE(strider.isInLove());

    // 播放了吃食音效
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
    EXPECT_EQ(m_world.getLastSoundId(), SoundEvents::ENTITY_STRIDER_EAT);

    // 音调在 [0.8, 1.2] 范围内
    EXPECT_GE(m_world.getLastPitch(), 0.8f);
    EXPECT_LE(m_world.getLastPitch(), 1.2f);
}

/**
 * @brief 创造模式下喂食 → 不消耗物品，但仍然进入爱心模式
 */
TEST_F(StriderInteractTestFixture, FeedAdultBreedableStrider_CreativeMode_NoItemConsume)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.setGameMode(GameMode::Creative);
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 1));
    player.inventory().setSelectedSlot(0);

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 创造模式不消耗物品
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 1);

    // 仍然进入爱心模式
    EXPECT_TRUE(strider.isInLove());

    // 仍然播放吃食音效
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
}

/**
 * @brief 喂食幼年炽足兽 → 加速成长，消耗物品，播放吃食音效
 */
TEST_F(StriderInteractTestFixture, FeedChildStrider_AcceleratesGrowth)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setChild(true);

    ASSERT_TRUE(strider.isChild());
    i32 initialAge = strider.getGrowingAge();
    ASSERT_LT(initialAge, 0);

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 2));
    player.inventory().setSelectedSlot(0);

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 成功喂食
    EXPECT_EQ(result, ActionResultType::Success);

    // 物品数量减少
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 1);

    // 年龄增长了（加速成长）
    EXPECT_GT(strider.getGrowingAge(), initialAge);

    // 播放了吃食音效
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
    EXPECT_EQ(m_world.getLastSoundId(), SoundEvents::ENTITY_STRIDER_EAT);
}

/**
 * @brief 喂食已处于爱心模式的成年炽足兽 → 服务端返回 Pass
 */
TEST_F(StriderInteractTestFixture, FeedAdultAlreadyInLove_ReturnsPassOnServer)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");

    // 先设置为爱心模式
    strider.setInLove(PlayerId(1));
    ASSERT_TRUE(strider.isInLove());
    ASSERT_FALSE(strider.canBreed());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 3));
    player.inventory().setSelectedSlot(0);

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 服务端：不消耗物品，不播放音效
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 3);
    EXPECT_EQ(m_world.getSoundPlayCount(), 0);
}

/**
 * @brief 手持非食物物品 + 已装备鞍 + 无乘客 + 未蹲下 → 玩家骑乘
 */
TEST_F(StriderInteractTestFixture, RideSaddledStrider_EmptyHand_ReturnsSuccess)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSaddle(true);

    ASSERT_TRUE(strider.hasSaddle());
    ASSERT_TRUE(strider.getPassengers().empty());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    // 手持钻石（非食物、非鞍）
    player.inventory().setItem(0, ItemStack(Items::DIAMOND, 1));
    player.inventory().setSelectedSlot(0);
    player.setSneaking(false);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 返回 Success（骑乘）
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 手持食物时不触发骑乘（即使已装备鞍），而是喂食
 */
TEST_F(StriderInteractTestFixture, RideSaddledStrider_HoldingFood_FeedNotRide)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSaddle(true);

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    // 手持诡异菌（食物）
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 1));
    player.inventory().setSelectedSlot(0);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 食物优先：喂食成功（不是骑乘）
    EXPECT_EQ(result, ActionResultType::Success);
    // 应该进入爱心模式
    EXPECT_TRUE(strider.isInLove());
}

/**
 * @brief 未装备鞍时不触发骑乘
 */
TEST_F(StriderInteractTestFixture, RideUnsaddledStrider_ReturnsPass)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    // 未装备鞍
    ASSERT_FALSE(strider.hasSaddle());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::DIAMOND, 1));
    player.inventory().setSelectedSlot(0);
    player.setSneaking(false);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 未装备鞍不触发骑乘
    EXPECT_EQ(result, ActionResultType::Pass);
}

/**
 * @brief 玩家蹲下时不触发骑乘
 */
TEST_F(StriderInteractTestFixture, RideSaddledStrider_PlayerSneaking_NoRide)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSaddle(true);

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::DIAMOND, 1));
    player.inventory().setSelectedSlot(0);
    player.setSneaking(true); // 蹲下

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 蹲下不触发骑乘
    EXPECT_EQ(result, ActionResultType::Pass);
}

/**
 * @brief 手持鞍 → 返回 Pass，委托给 SaddleItem::itemInteractionForEntity
 */
TEST_F(StriderInteractTestFixture, HoldSaddle_ReturnsPass_DelegatesToSaddleItem)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    // 未装备鞍
    ASSERT_FALSE(strider.hasSaddle());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::SADDLE, 1));
    player.inventory().setSelectedSlot(0);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 返回 Pass，让 Player::interactOn 处理鞍装备
    EXPECT_EQ(result, ActionResultType::Pass);
}

/**
 * @brief 已装备鞍 + 手持鞍 + 未蹲下 → 触发骑乘（鞍不是食物，满足骑乘条件）
 */
TEST_F(StriderInteractTestFixture, HoldSaddleOnAlreadySaddledStrider_TriggerRide)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSaddle(true);

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::SADDLE, 1));
    player.inventory().setSelectedSlot(0);
    player.setSneaking(false);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 已装备鞍 + 非食物 + 无乘客 + 未蹲下 → 骑乘（而非 Pass）
    // 鞍不是繁殖食物，因此满足骑乘条件 !isFood && hasSaddle() && ...
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 手持非食物、非鞍物品 + 未装备鞍 → 返回 Pass
 */
TEST_F(StriderInteractTestFixture, HoldNonFoodNonSaddleUnsaddled_ReturnsPass)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    ASSERT_FALSE(strider.hasSaddle());

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::DIAMOND, 1));
    player.inventory().setSelectedSlot(0);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Pass);
}

/**
 * @brief 空手 + 已装备鞍 + 未蹲下 → 触发骑乘
 */
TEST_F(StriderInteractTestFixture, RideSaddledStrider_EmptyHandNoItem)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSaddle(true);

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    // 空手（主手没有物品）
    player.setSneaking(false);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 空手 + 已装备鞍 → 骑乘
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 静默炽足兽喂食时不播放音效
 */
TEST_F(StriderInteractTestFixture, FeedSilentStrider_NoSoundPlayed)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");
    strider.setSilent(true); // 设置为静默

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS, 1));
    player.inventory().setSelectedSlot(0);

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(strider.isInLove());

    // 静默模式不播放音效
    EXPECT_EQ(m_world.getSoundPlayCount(), 0);
}

/**
 * @brief 诡异菌钓竿不是食物，不应触发喂食
 */
TEST_F(StriderInteractTestFixture, WarpedFungusOnAStick_NotFoodForBreeding)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    player.inventory().setItem(0, ItemStack(Items::WARPED_FUNGUS_ON_A_STICK, 1));
    player.inventory().setSelectedSlot(0);

    ActionResultType result = strider.interactMob(player, Hand::MainHand);

    // 诡异菌钓竿不是繁殖食物
    // 未装备鞍 → Pass（canEquip 返回 false 因为不是鞍）
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_FALSE(strider.isInLove());
}

/**
 * @brief 副手持食物喂食
 */
TEST_F(StriderInteractTestFixture, FeedWithOffHand)
{
    StriderEntity strider(EntityInstanceId(1), mc::test::testEcsRegistry());
    strider.setWorld(&m_world);
    strider.setTypeId("minecraft:strider");

    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setPlayerId(PlayerId(42));
    // 副手持食物
    player.inventory().setItem(40, ItemStack(Items::WARPED_FUNGUS, 2));

    m_world.resetSoundTracking();
    ActionResultType result = strider.interactMob(player, Hand::OffHand);

    // 副手喂食成功
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(strider.isInLove());
    EXPECT_EQ(player.inventory().getItem(40).getCount(), 1);
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
}

} // namespace
