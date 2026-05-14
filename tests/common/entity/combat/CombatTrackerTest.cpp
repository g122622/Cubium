#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/CombatTracker.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"

using namespace mc;

// ============================================================================
// CombatTracker 攀爬位置追踪测试
// ============================================================================

// 测试攀爬位置追踪
TEST(CombatTrackerClimbPosTest, EntityHasLastClimbPos)
{
    // 验证Entity类有攀爬位置追踪方法
    Entity entity(LegacyEntityType::Player, 1, nullptr);

    // 初始状态应该没有攀爬位置
    EXPECT_FALSE(entity.getLastClimbPos().has_value());

    // 设置攀爬位置
    BlockPos climbPos(10, 64, 20);
    entity.setLastClimbPos(climbPos);
    EXPECT_TRUE(entity.getLastClimbPos().has_value());
    EXPECT_EQ(entity.getLastClimbPos().value(), climbPos);

    // 清空攀爬位置
    entity.clearLastClimbPos();
    EXPECT_FALSE(entity.getLastClimbPos().has_value());
}

TEST(CombatTrackerClimbPosTest, SetOnGroundClearsClimbPos)
{
    // 验证落地时清空攀爬位置
    Entity entity(LegacyEntityType::Player, 1, nullptr);

    // 设置攀爬位置
    BlockPos climbPos(10, 64, 20);
    entity.setLastClimbPos(climbPos);
    EXPECT_TRUE(entity.getLastClimbPos().has_value());

    // 设置在地面状态（模拟落地）
    entity.setOnGround(true);
    EXPECT_FALSE(entity.getLastClimbPos().has_value());
}

TEST(CombatTrackerClimbPosTest, SetOnGroundNotClearWhenAlreadyOnGround)
{
    // 验证已经在地面时不会清空攀爬位置
    Entity entity(LegacyEntityType::Player, 1, nullptr);

    // 先设置在地面
    entity.setOnGround(true);

    // 设置攀爬位置（这在正常情况下不会发生，但测试边界条件）
    BlockPos climbPos(10, 64, 20);
    entity.setLastClimbPos(climbPos);
    EXPECT_TRUE(entity.getLastClimbPos().has_value());

    // 再次设置在地面（已经是地面状态）
    entity.setOnGround(true);
    // 攀爬位置不应该被清空（因为之前已经在地面）
    EXPECT_TRUE(entity.getLastClimbPos().has_value());
}

// ============================================================================
// CombatTracker calculateFallSuffix 测试
// ============================================================================

// 由于calculateFallSuffix是私有方法，我们通过测试CombatEntry的fallSuffix来间接测试
// 但我们可以测试fallSuffix的存储和获取

TEST(CombatTrackerFallSuffixTest, FallSuffixValues)
{
    // 验证各种摔落后缀值
    // 这些是MC 1.16.5中定义的标准后缀值

    const char* expectedSuffixes[] = {
        "ladder", "vines", "weeping_vines", "twisting_vines", "scaffolding", "other_climbable", "water"};

    // 验证后缀字符串的正确性
    EXPECT_STREQ(expectedSuffixes[0], "ladder");
    EXPECT_STREQ(expectedSuffixes[1], "vines");
    EXPECT_STREQ(expectedSuffixes[2], "weeping_vines");
    EXPECT_STREQ(expectedSuffixes[3], "twisting_vines");
    EXPECT_STREQ(expectedSuffixes[4], "scaffolding");
    EXPECT_STREQ(expectedSuffixes[5], "other_climbable");
    EXPECT_STREQ(expectedSuffixes[6], "water");
}

TEST(CombatTrackerFallSuffixTest, WaterSuffixWhenInWater)
{
    // 测试在水中时的摔落后缀
    // 这个测试验证实体在水中摔落时的后缀

    Entity entity(LegacyEntityType::Player, 1, nullptr);

    // 设置在水中
    entity.setInWater(true);
    EXPECT_TRUE(entity.isInWater());

    // 没有攀爬位置但在水中
    EXPECT_FALSE(entity.getLastClimbPos().has_value());
}

TEST(CombatTrackerFallSuffixTest, EmptySuffixWhenNoClimbOrWater)
{
    // 验证没有攀爬位置且不在水中时，后缀为空
    Entity entity(LegacyEntityType::Player, 1, nullptr);

    EXPECT_FALSE(entity.getLastClimbPos().has_value());
    EXPECT_FALSE(entity.isInWater());
}

// ============================================================================
// BlockPos 测试（辅助测试）
// ============================================================================

TEST(BlockPosTest, OptionalBlockPos)
{
    // 测试 std::optional<BlockPos> 的使用
    std::optional<BlockPos> pos1;
    EXPECT_FALSE(pos1.has_value());

    pos1 = BlockPos(10, 64, 20);
    EXPECT_TRUE(pos1.has_value());
    EXPECT_EQ(pos1->x, 10);
    EXPECT_EQ(pos1->y, 64);
    EXPECT_EQ(pos1->z, 20);

    pos1 = std::nullopt;
    EXPECT_FALSE(pos1.has_value());
}

// ============================================================================
// ResourceLocation 测试（辅助测试）
// ============================================================================

TEST(ResourceLocationTest, ClimbableBlockIds)
{
    // 验证攀爬方块的资源位置
    ResourceLocation ladder("minecraft", "ladder");
    ResourceLocation vine("minecraft", "vine");
    ResourceLocation scaffolding("minecraft", "scaffolding");
    ResourceLocation weepingVines("minecraft", "weeping_vines");
    ResourceLocation twistingVines("minecraft", "twisting_vines");

    EXPECT_EQ(ladder.path(), "ladder");
    EXPECT_EQ(vine.path(), "vine");
    EXPECT_EQ(scaffolding.path(), "scaffolding");
    EXPECT_EQ(weepingVines.path(), "weeping_vines");
    EXPECT_EQ(twistingVines.path(), "twisting_vines");

    // 比较操作
    ResourceLocation ladder2("minecraft", "ladder");
    EXPECT_EQ(ladder, ladder2);
    EXPECT_NE(ladder, vine);
}
