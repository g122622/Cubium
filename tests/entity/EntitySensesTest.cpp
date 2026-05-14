#include <gtest/gtest.h>

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

TEST(EntitySensesTest, VisibleEntityIsCachedWithinSameTick)
{
    MobEntity observer(LegacyEntityType::Zombie, 1);
    MobEntity target(LegacyEntityType::Zombie, 2);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(4.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_TRUE(observer.senses()->canSee(target));

    target.setPosition(128.0f, 64.0f, 0.0f);
    EXPECT_TRUE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, CacheClearsOnTick)
{
    MobEntity observer(LegacyEntityType::Zombie, 3);
    MobEntity target(LegacyEntityType::Zombie, 4);

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
    MobEntity observer(LegacyEntityType::Zombie, 5);
    MobEntity target(LegacyEntityType::Zombie, 6);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(128.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_FALSE(observer.senses()->canSee(target));

    target.setPosition(4.0f, 64.0f, 0.0f);
    EXPECT_FALSE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, LookControllerIdlePitchResetHonorsHook)
{
    MobEntity resetMob(LegacyEntityType::Zombie, 7);
    resetMob.setRotation(45.0f, 15.0f);

    entity::ai::controller::LookController resetController(&resetMob);
    resetController.tick();

    EXPECT_FLOAT_EQ(resetMob.yaw(), 45.0f);
    // MC 1.16.5: 俯仰角重置为0.0f（当shouldResetPitch返回true时）
    EXPECT_FLOAT_EQ(resetMob.pitch(), 0.0f);

    MobEntity lockedMob(LegacyEntityType::Zombie, 8);
    lockedMob.setRotation(45.0f, 15.0f);

    NoResetLookController lockedController(&lockedMob);
    lockedController.tick();

    EXPECT_FLOAT_EQ(lockedMob.yaw(), 45.0f);
    // NoResetLookController不重置俯仰角，保持原值
    EXPECT_FLOAT_EQ(lockedMob.pitch(), 15.0f);
}

} // namespace
} // namespace mc
