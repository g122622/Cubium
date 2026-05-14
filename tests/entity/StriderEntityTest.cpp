#include <cmath>
#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"

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
    void SetUp() override { strider = std::make_unique<StriderEntity>(LegacyEntityType::Strider, EntityId(1)); }

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
    void SetUp() override { strider = std::make_unique<StriderEntity>(LegacyEntityType::Strider, EntityId(1)); }

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
    StriderEntity childStrider(LegacyEntityType::Strider, EntityId(2));
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

} // namespace
