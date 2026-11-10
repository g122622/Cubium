// ============================================================================
// AgeableEntity::breedingExperience 默认实现
//
// 原版 `Animal.finalizeSpawnChildFromBreeding` 在繁殖成功时生成
// `getRandom().nextInt(7) + 1` 个经验球，每个价值 1。该方法把"个数"抽成可覆写入口，
// 供基岩行为包用 `minecraft:experience_reward.on_bred` 覆盖。
// ============================================================================

#include <gtest/gtest.h>

#include <memory>

#include "common/TestWorldHelper.hpp"
#include "core/Types.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"

namespace {

using mc::EntityInstanceId;
using mc::i32;
using mc::PigEntity;

} // namespace

TEST(BreedingExperienceTest, DefaultIsBetweenOneAndSeven)
{
    // 默认实现即原版语义：nextInt(7) + 1 → 落在 [1, 7] 闭区间。
    // 多取几次以覆盖边界（单次取值无法排除实现被写成常量 1 的情况）。
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    for (i32 i = 0; i < 64; ++i) {
        const i32 experience = pig->breedingExperience();
        EXPECT_GE(experience, 1);
        EXPECT_LE(experience, 7);
    }
}

TEST(BreedingExperienceTest, DefaultIsNotConstant)
{
    // 若实现误写为固定值，上面的范围断言仍会通过；这里确认它确实带随机性。
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());

    const i32 first = pig->breedingExperience();
    bool sawDifferent = false;
    for (i32 i = 0; i < 64; ++i) {
        if (pig->breedingExperience() != first) {
            sawDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(sawDifferent) << "breedingExperience() should vary across calls (nextInt(7) + 1)";
}
