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

#include "entity/attribute/AttributeMap.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/LivingEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 测试固定装置
// ============================================================================

class LivingEntityAbsorptionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_living = std::make_unique<LivingEntity>(EntityInstanceId(1));
        m_living->registerData();
        m_living->registerAttributes();
        m_living->setHealth(m_living->maxHealth());
    }

    void TearDown() override { m_living.reset(); }

    std::unique_ptr<LivingEntity> m_living;
};

// ============================================================================
// absorptionAmount 默认值测试
// ============================================================================

TEST_F(LivingEntityAbsorptionTest, DefaultAbsorptionIsZero)
{
    // 默认吸收值应为 0
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

// ============================================================================
// setAbsorptionAmount 正常值测试
// ============================================================================

TEST_F(LivingEntityAbsorptionTest, SetPositiveValue)
{
    // 默认 max_absorption 为 0.0，需要先设置属性才能设置吸收值
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);
    m_living->setAbsorptionAmount(4.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 4.0f);
}

TEST_F(LivingEntityAbsorptionTest, SetLargeValue)
{
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 100.0);
    m_living->setAbsorptionAmount(100.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 100.0f);
}

TEST_F(LivingEntityAbsorptionTest, SetZeroValue)
{
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);
    // 先设置非零值，再设为0
    m_living->setAbsorptionAmount(4.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 4.0f);

    m_living->setAbsorptionAmount(0.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

TEST_F(LivingEntityAbsorptionTest, SetSmallValue)
{
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);
    // 设置非常小的正值
    m_living->setAbsorptionAmount(0.01f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.01f);
}

// ============================================================================
// setAbsorptionAmount 边界 clamping 测试
// ============================================================================

TEST_F(LivingEntityAbsorptionTest, NegativeValueClampedToZero)
{
    // 负值应被限制为 0
    m_living->setAbsorptionAmount(-5.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

TEST_F(LivingEntityAbsorptionTest, LargeNegativeValueClampedToZero)
{
    // 很大的负值也应被限制为 0
    m_living->setAbsorptionAmount(-1000.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

TEST_F(LivingEntityAbsorptionTest, ValueClampedByMaxAbsorptionAttribute)
{
    // 默认 max_absorption 属性值为 0.0，因此任何正值应被限制为 0
    // 这是 MC 原版行为：没有设置 max_absorption 属性时，吸收值无法增加
    m_living->setAbsorptionAmount(4.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

TEST_F(LivingEntityAbsorptionTest, ValueClampedWhenMaxAbsorptionIsSet)
{
    // 设置 max_absorption 属性为 20.0 后，吸收值不应超过 20.0
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);

    m_living->setAbsorptionAmount(4.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 4.0f);

    // 超过最大值的吸收值应被限制
    m_living->setAbsorptionAmount(25.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 20.0f);

    // 正好等于最大值
    m_living->setAbsorptionAmount(20.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 20.0f);
}

TEST_F(LivingEntityAbsorptionTest, NegativeValueClampedEvenWithMaxAbsorptionSet)
{
    // 设置 max_absorption 后，负值仍然被限制为 0
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);

    m_living->setAbsorptionAmount(-5.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}

// ============================================================================
// absorptionAmount 递减测试（模拟伤害消耗）
// ============================================================================

TEST_F(LivingEntityAbsorptionTest, DecreaseAbsorption)
{
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);

    // 设置初始吸收值
    m_living->setAbsorptionAmount(8.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 8.0f);

    // 消耗部分吸收值（模拟伤害吸收）
    const f32 absorbed = 3.0f;
    m_living->setAbsorptionAmount(m_living->absorptionAmount() - absorbed);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 5.0f);
}

TEST_F(LivingEntityAbsorptionTest, AbsorptionFullyConsumed)
{
    m_living->attributes().setBaseValue(entity::attribute::Attributes::MAX_ABSORPTION, 20.0);

    // 设置初始吸收值
    m_living->setAbsorptionAmount(4.0f);

    // 消耗所有吸收值
    m_living->setAbsorptionAmount(m_living->absorptionAmount() - 4.0f);
    EXPECT_FLOAT_EQ(m_living->absorptionAmount(), 0.0f);
}
