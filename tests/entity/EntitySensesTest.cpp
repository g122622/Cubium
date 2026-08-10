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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/EntitySenses.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/core/MobEntity.hpp"

namespace mc {
namespace {

class NoResetLookController final : public entity::ai::controller::LookController {
public:
    using LookController::LookController;

protected:
    [[nodiscard]] bool shouldResetPitch() const override { return false; }
};

// Entity::canSee 经 raycastBlocks(*m_world) 做视线检测，无 world 时直接返回 false。
// EntitySensesTestWorld 提供 getBlockState=nullptr（空气）的空世界，使视线无阻挡。
class EntitySensesTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
};

TEST(EntitySensesTest, VisibleEntityIsCachedWithinSameTick)
{
    EntitySensesTestWorld world;
    MobEntity observer(EntityInstanceId(1), mc::test::testEcsRegistry());
    MobEntity target(EntityInstanceId(2), mc::test::testEcsRegistry());
    observer.setWorld(&world);
    target.setWorld(&world);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(4.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_TRUE(observer.senses()->canSee(target));

    target.setPosition(128.0f, 64.0f, 0.0f);
    EXPECT_TRUE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, CacheClearsOnTick)
{
    EntitySensesTestWorld world;
    MobEntity observer(EntityInstanceId(3), mc::test::testEcsRegistry());
    MobEntity target(EntityInstanceId(4), mc::test::testEcsRegistry());
    observer.setWorld(&world);
    target.setWorld(&world);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(4.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_TRUE(observer.senses()->canSee(target));

    target.setPosition(128.0f, 64.0f, 0.0f);
    observer.tick();

    EXPECT_FALSE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, InvisibleEntityIsCachedWithinSameTick)
{
    EntitySensesTestWorld world;
    MobEntity observer(EntityInstanceId(5), mc::test::testEcsRegistry());
    MobEntity target(EntityInstanceId(6), mc::test::testEcsRegistry());
    observer.setWorld(&world);
    target.setWorld(&world);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(128.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_FALSE(observer.senses()->canSee(target));

    target.setPosition(4.0f, 64.0f, 0.0f);
    EXPECT_FALSE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, LookControllerIdlePitchResetHonorsHook)
{
    MobEntity resetMob(EntityInstanceId(7), mc::test::testEcsRegistry());
    resetMob.setRotation(45.0f, 15.0f);

    entity::ai::controller::LookController resetController(&resetMob);
    resetController.tick();

    EXPECT_FLOAT_EQ(resetMob.yaw(), 45.0f);
    // MC 1.16.5: 俯仰角重置为0.0f（当shouldResetPitch返回true时）
    EXPECT_FLOAT_EQ(resetMob.pitch(), 0.0f);

    MobEntity lockedMob(EntityInstanceId(8), mc::test::testEcsRegistry());
    lockedMob.setRotation(45.0f, 15.0f);

    NoResetLookController lockedController(&lockedMob);
    lockedController.tick();

    EXPECT_FLOAT_EQ(lockedMob.yaw(), 45.0f);
    // NoResetLookController不重置俯仰角，保持原值
    EXPECT_FLOAT_EQ(lockedMob.pitch(), 15.0f);
}

} // namespace
} // namespace mc
