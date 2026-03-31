#include <gtest/gtest.h>
#include "entity/entities/orb/ExperienceOrbEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/experience/ExperienceConstants.hpp"
#include "entity/experience/ExperienceUtils.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using mc::i32;
using mc::math::Random;

// 命名空间别名
namespace xp_constants = mc::entity::experience::constants;

// ==================== ExperienceOrbEntity Tests ====================

class ExperienceOrbEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        orb = std::make_unique<ExperienceOrbEntity>(10);  // 10 XP orb
    }

    void TearDown() override {
        orb.reset();
    }

    std::unique_ptr<ExperienceOrbEntity> orb;
};

// ========== 构造函数测试 ==========

TEST_F(ExperienceOrbEntityTest, DefaultConstruction) {
    ExperienceOrbEntity defaultOrb;
    EXPECT_EQ(defaultOrb.getXpValue(), 1);
    EXPECT_EQ(defaultOrb.getAge(), 0);
    EXPECT_EQ(defaultOrb.getPickupDelay(), ExperienceOrbEntity::DEFAULT_PICKUP_DELAY);
    EXPECT_FALSE(defaultOrb.isRemoved());
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithXpValue) {
    ExperienceOrbEntity orb50(50);
    EXPECT_EQ(orb50.getXpValue(), 50);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithMaxValue) {
    // 值应该被限制在 MAX_ORB_SIZE
    ExperienceOrbEntity largeOrb(5000);
    EXPECT_EQ(largeOrb.getXpValue(), ExperienceOrbEntity::MAX_ORB_SIZE);
    EXPECT_EQ(largeOrb.getXpValue(), 2477);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithZeroValue) {
    // 最小值应该是 1
    ExperienceOrbEntity zeroOrb(0);
    EXPECT_EQ(zeroOrb.getXpValue(), 1);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithNegativeValue) {
    ExperienceOrbEntity negativeOrb(-10);
    EXPECT_EQ(negativeOrb.getXpValue(), 1);
}

// ========== 属性测试 ==========

TEST_F(ExperienceOrbEntityTest, Dimensions) {
    EXPECT_FLOAT_EQ(orb->width(), 0.5f);
    EXPECT_FLOAT_EQ(orb->height(), 0.5f);
}

TEST_F(ExperienceOrbEntityTest, SetXpValue) {
    orb->setXpValue(100);
    EXPECT_EQ(orb->getXpValue(), 100);

    orb->setXpValue(5000);
    EXPECT_EQ(orb->getXpValue(), ExperienceOrbEntity::MAX_ORB_SIZE);

    orb->setXpValue(0);
    EXPECT_EQ(orb->getXpValue(), 1);
}

TEST_F(ExperienceOrbEntityTest, Age) {
    EXPECT_EQ(orb->getAge(), 0);

    orb->setAge(100);
    EXPECT_EQ(orb->getAge(), 100);
}

TEST_F(ExperienceOrbEntityTest, PickupDelay) {
    EXPECT_EQ(orb->getPickupDelay(), ExperienceOrbEntity::DEFAULT_PICKUP_DELAY);
    EXPECT_FALSE(orb->canBePickedUp());

    orb->setPickupDelay(0);
    EXPECT_EQ(orb->getPickupDelay(), 0);
    EXPECT_TRUE(orb->canBePickedUp());

    orb->setPickupDelay(100);
    EXPECT_FALSE(orb->canBePickedUp());
}

// ========== 经验球大小测试 ==========

TEST_F(ExperienceOrbEntityTest, GetOrbSize) {
    // 根据经验值获取球大小等级
    // 1-2: 等级 0
    // 3-6: 等级 1
    // 7-16: 等级 2
    // ...

    ExperienceOrbEntity orb1(1);
    EXPECT_EQ(orb1.getOrbSize(), 0);

    ExperienceOrbEntity orb2(2);
    EXPECT_EQ(orb2.getOrbSize(), 0);

    ExperienceOrbEntity orb3(3);
    EXPECT_EQ(orb3.getOrbSize(), 1);

    ExperienceOrbEntity orb7(7);
    EXPECT_EQ(orb7.getOrbSize(), 2);

    ExperienceOrbEntity orb17(17);
    EXPECT_EQ(orb17.getOrbSize(), 3);

    ExperienceOrbEntity orbMax(2477);
    EXPECT_EQ(orbMax.getOrbSize(), 10);  // 最大球大小
}

// ========== 静态方法测试 ==========

TEST_F(ExperienceOrbEntityTest, GetXPSplit) {
    // 静态方法测试经验分割
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(1), 1);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(2), 1);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(3), 3);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(7), 7);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(10), 7);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(100), 73);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(1000), 617);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(3000), 2477);  // 最大分割值
}

// ========== 合并测试 ==========

TEST_F(ExperienceOrbEntityTest, CanMergeWithSelf) {
    // 不能和自己合并
    EXPECT_FALSE(orb->canMergeWith(*orb));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithDifferentValues) {
    ExperienceOrbEntity orb1(10);
    ExperienceOrbEntity orb2(20);

    // 在同一位置应该可以合并
    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    // 由于距离检查，需要足够近
    // MERGE_DISTANCE_SQ = 0.5 * 0.5 = 0.25
    // 但 canMergeWith 需要距离检查，而且还要检查合并后是否超过最大值

    // 总值 10 + 20 = 30，小于 MAX_ORB_SIZE (2477)
    EXPECT_TRUE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithExceedsMax) {
    ExperienceOrbEntity orb1(2000);
    ExperienceOrbEntity orb2(1000);

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    // 2000 + 1000 = 3000 > 2477，不应该合并
    EXPECT_FALSE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithDistance) {
    ExperienceOrbEntity orb1(10);
    ExperienceOrbEntity orb2(20);

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(100, 0, 0);  // 距离太远

    EXPECT_FALSE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, TryMerge) {
    ExperienceOrbEntity orb1(10);
    ExperienceOrbEntity orb2(20);

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    bool merged = orb1.tryMergeWith(orb2);

    EXPECT_TRUE(merged);
    EXPECT_EQ(orb1.getXpValue(), 30);  // 合并后的值
    EXPECT_TRUE(orb2.isRemoved());  // 被合并的球应该被移除
}

// ========== 常量验证测试 ==========

TEST_F(ExperienceOrbEntityTest, ConstantsValidation) {
    // 验证经验球常量与 ExperienceConstants 一致
    EXPECT_EQ(ExperienceOrbEntity::MAX_ORB_SIZE, xp_constants::MAX_ORB_VALUE);
    EXPECT_EQ(ExperienceOrbEntity::MAX_AGE, xp_constants::MAX_ORB_AGE);
    EXPECT_EQ(ExperienceOrbEntity::DEFAULT_PICKUP_DELAY, xp_constants::DEFAULT_PICKUP_DELAY);
    EXPECT_EQ(ExperienceOrbEntity::TRACKING_RANGE, xp_constants::ORB_TRACKING_RANGE);
}

// ========== 追踪玩家测试 ==========

TEST_F(ExperienceOrbEntityTest, TrackingPlayer) {
    // 初始状态不追踪任何玩家
    EXPECT_FALSE(orb->isBeingTracked());
    EXPECT_EQ(orb->getTrackingPlayer(), nullptr);
}

// ========== 实体类型测试 ==========

TEST_F(ExperienceOrbEntityTest, EntityType) {
    EXPECT_EQ(orb->legacyType(), LegacyEntityType::ExperienceOrb);
}

// ==================== ExperienceOrbEntity Integration Tests ====================

class ExperienceOrbEntityIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExperienceOrbEntityIntegrationTest, XPSplitConsistency) {
    // 验证分割值和球大小一致性
    for (int xp = 1; xp <= 100; ++xp) {
        i32 split = ExperienceOrbEntity::getXPSplit(xp);

        // 分割值必须是有效的大小
        bool validSplit = false;
        for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
            if (xp_constants::XP_SPLIT_VALUES[i] == split) {
                validSplit = true;
                break;
            }
        }
        EXPECT_TRUE(validSplit) << "Invalid split for XP " << xp << ": " << split;

        // 分割值不能超过原始值
        EXPECT_LE(split, xp);

        // 创建对应大小的球
        ExperienceOrbEntity orb(split);
        EXPECT_EQ(orb.getXpValue(), split);
    }
}

TEST_F(ExperienceOrbEntityIntegrationTest, OrbSizeConsistency) {
    // 验证每个分割值对应的球大小
    for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
        i32 value = xp_constants::XP_SPLIT_VALUES[i];
        ExperienceOrbEntity orb(value);
        i32 size = orb.getOrbSize();

        // 更大的分割值应该有更大的或相等的球大小
        if (i < xp_constants::XP_SPLIT_COUNT - 1) {
            i32 nextValue = xp_constants::XP_SPLIT_VALUES[i + 1];
            ExperienceOrbEntity nextOrb(nextValue);
            i32 nextSize = nextOrb.getOrbSize();

            EXPECT_GE(size, nextSize) << "Inconsistent orb sizes: " << value
                                       << " has size " << size << ", " << nextValue
                                       << " has size " << nextSize;
        }
    }
}

TEST_F(ExperienceOrbEntityIntegrationTest, MergeSimulation) {
    // 模拟多个经验球合并
    std::vector<std::unique_ptr<ExperienceOrbEntity>> orbs;

    // 创建多个小经验球
    for (int i = 0; i < 10; ++i) {
        orbs.push_back(std::make_unique<ExperienceOrbEntity>(10));  // 每个10点经验
        orbs.back()->setPosition(0, 0, 0);
    }

    // 尝试合并
    i32 totalMerged = orbs[0]->getXpValue();
    for (size_t i = 1; i < orbs.size(); ++i) {
        if (orbs[0]->canMergeWith(*orbs[i])) {
            orbs[0]->tryMergeWith(*orbs[i]);
            totalMerged += 10;
        }
    }

    // 验证合并后的值
    EXPECT_EQ(orbs[0]->getXpValue(), totalMerged);
    EXPECT_EQ(orbs[0]->getXpValue(), 100);  // 10个球，每个10点
}

TEST_F(ExperienceOrbEntityIntegrationTest, EnderDragonXP) {
    // 末影龙掉落12000经验
    // 验证分割后球的值总和正确
    i32 totalXP = xp_constants::ENDER_DRAGON_XP;
    std::vector<i32> orbs;

    i32 remaining = totalXP;
    while (remaining > 0) {
        i32 split = ExperienceOrbEntity::getXPSplit(remaining);
        orbs.push_back(split);
        remaining -= split;
    }

    // 验证总和
    i32 sum = 0;
    for (i32 v : orbs) {
        sum += v;
    }
    EXPECT_EQ(sum, totalXP);

    // 验证每个球的值有效
    for (i32 v : orbs) {
        EXPECT_LE(v, ExperienceOrbEntity::MAX_ORB_SIZE);
        EXPECT_GE(v, 1);
    }

    // 球的数量应该合理
    EXPECT_LT(orbs.size(), 20u);  // 12000经验应该分成约10个球
    EXPECT_GT(orbs.size(), 5u);
}
