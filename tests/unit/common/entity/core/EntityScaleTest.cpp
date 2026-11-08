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

/**
 * @file EntityScaleTest.cpp
 * @brief 生物尺寸缩放（scale 属性）测试
 *
 * 原版 `LivingEntity.getDimensions(pose)` 对非睡眠姿态返回
 * `getDefaultDimensions(pose).scale(getScale())`，其中 `getScale()` 读 SCALE 属性。
 * 睡眠姿态直接用固定的 `SLEEPING_DIMENSIONS`，与实体类型和缩放都无关。
 *
 * 此前 `scale` 属性只被注册、没有任何消费点，尺寸完全由 EntitySize 决定，
 * 因此 `/attribute ... minecraft:scale` 对碰撞箱毫无影响。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"

namespace mc {
namespace {

class EntityScaleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(EntityScaleTest, DefaultScaleLeavesDimensionsUntouched)
{
    PigEntity pig(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(pig.getScale(), 1.0f);

    const entity::EntitySize defaultDims = pig.getDimensions(EntityPose::Standing);

    // 显式把 scale 设回 1.0，结果必须与"未设置"完全一致——即默认值就是 1.0 语义，
    // 且 1.0 的缩放不会改动任何一维（EntitySize::scale 对 1.0 直接返回自身）。
    ASSERT_TRUE(pig.attributes().setBaseValue(entity::attribute::Attributes::SCALE, 1.0));
    const entity::EntitySize explicitDims = pig.getDimensions(EntityPose::Standing);

    EXPECT_FLOAT_EQ(explicitDims.width(), defaultDims.width());
    EXPECT_FLOAT_EQ(explicitDims.height(), defaultDims.height());
    EXPECT_FLOAT_EQ(explicitDims.eyeHeight(), defaultDims.eyeHeight());
}

TEST_F(EntityScaleTest, ScaleAttributeMultipliesDimensions)
{
    PigEntity pig(EntityInstanceId(2), mc::test::testEcsRegistry());

    const entity::EntitySize base = pig.getDimensions(EntityPose::Standing);
    ASSERT_TRUE(pig.attributes().setBaseValue(entity::attribute::Attributes::SCALE, 2.0));

    EXPECT_FLOAT_EQ(pig.getScale(), 2.0f);

    const entity::EntitySize scaled = pig.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(scaled.width(), base.width() * 2.0f);
    EXPECT_FLOAT_EQ(scaled.height(), base.height() * 2.0f);
    EXPECT_FLOAT_EQ(scaled.eyeHeight(), base.eyeHeight() * 2.0f);
}

TEST_F(EntityScaleTest, FractionalScaleShrinksDimensions)
{
    PigEntity pig(EntityInstanceId(3), mc::test::testEcsRegistry());

    const entity::EntitySize base = pig.getDimensions(EntityPose::Standing);
    ASSERT_TRUE(pig.attributes().setBaseValue(entity::attribute::Attributes::SCALE, 0.5));

    const entity::EntitySize scaled = pig.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(scaled.width(), base.width() * 0.5f);
    EXPECT_FLOAT_EQ(scaled.height(), base.height() * 0.5f);
}

TEST_F(EntityScaleTest, SleepingDimensionsIgnoreScale)
{
    // 睡眠尺寸是所有生物共用的固定常量 (0.2, 0.2, eye 0.2)，不参与缩放。
    PigEntity pig(EntityInstanceId(4), mc::test::testEcsRegistry());
    ASSERT_TRUE(pig.attributes().setBaseValue(entity::attribute::Attributes::SCALE, 3.0));

    const entity::EntitySize sleeping = pig.getDimensions(EntityPose::Sleeping);
    EXPECT_FLOAT_EQ(sleeping.width(), 0.2f);
    EXPECT_FLOAT_EQ(sleeping.height(), 0.2f);
    EXPECT_FLOAT_EQ(sleeping.eyeHeight(), 0.2f);
}

TEST_F(EntityScaleTest, RefreshDimensionsAppliesScale)
{
    PigEntity pig(EntityInstanceId(5), mc::test::testEcsRegistry());

    // 先刷新一次把 m_dimensions 落到当前姿态的真实尺寸，再取基准值，
    // 避免拿构造期的注册表初值当参照。
    pig.refreshDimensions();
    const f32 unscaledWidth = pig.currentDimensions().width();

    ASSERT_TRUE(pig.attributes().setBaseValue(entity::attribute::Attributes::SCALE, 2.0));

    pig.refreshDimensions();
    EXPECT_FLOAT_EQ(pig.currentDimensions().width(), unscaledWidth * 2.0f);
}

} // namespace
} // namespace mc
