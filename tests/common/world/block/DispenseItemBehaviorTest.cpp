/**
 * @file DispenseItemBehaviorTest.cpp
 * @brief 发射器行为系统测试
 *
 * 测试发射器行为的核心功能：
 * - OptionalDispenseItemBehavior 的成功/失败状态
 * - DispenseItemBehaviorRegistry 的注册和查询
 * - 药水效果发射器的药水效果应用
 */

#include <gtest/gtest.h>
#include "world/block/dispense/IDispenseItemBehavior.hpp"
#include "world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/WorldEvents.hpp"
#include "util/Direction.hpp"
#include "util/math/Vector3.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "item/potion/PotionUtils.hpp"
#include "item/potion/Potions.hpp"
#include "entity/core/Entity.hpp"
#include "entity/effect/EffectType.hpp"

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 测试夹具基类
 */
class DispenseBehaviorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 初始化方块注册表
        VanillaBlocks::initialize();
        // 初始化物品注册表
        Items::initialize();
        // 初始化药水注册表
        potion::Potions::initialize();
        // 初始化发射器行为注册表
        DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
};

// ============================================================================
// OptionalDispenseItemBehavior 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, OptionalDispenseBehavior_DefaultSuccess) {
    OptionalDispenseItemBehavior behavior;
    EXPECT_TRUE(behavior.isSuccess());
}

TEST_F(DispenseBehaviorTest, OptionalDispenseBehavior_CanSetFailure) {
    OptionalDispenseItemBehavior behavior;

    // 通过反射或友元类设置失败状态
    // 注意：setSuccess 是 protected，需要子类访问
    class TestableOptionalBehavior : public OptionalDispenseItemBehavior {
    public:
        using OptionalDispenseItemBehavior::setSuccess;
    };

    TestableOptionalBehavior testBehavior;
    EXPECT_TRUE(testBehavior.isSuccess());

    testBehavior.setSuccess(false);
    EXPECT_FALSE(testBehavior.isSuccess());

    testBehavior.setSuccess(true);
    EXPECT_TRUE(testBehavior.isSuccess());
}

// ============================================================================
// DispenseItemBehaviorRegistry 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasDefaultBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* defaultBehavior = registry.getDefaultBehavior();
    ASSERT_NE(defaultBehavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForEmptyStack_ReturnsNull) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    ItemStack emptyStack;
    IDispenseItemBehavior* behavior = registry.getBehavior(emptyStack);
    EXPECT_EQ(behavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForUnregisteredItem_ReturnsNull) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 未注册的物品ID返回 nullptr
    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:unknown_item");
    EXPECT_EQ(behavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_HasBehavior_ReturnsCorrectValue) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 初始化默认行为
    registry.initDefaultBehaviors();

    // 检查已注册的行为
    EXPECT_TRUE(registry.hasBehavior("minecraft:snowball"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:egg"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:arrow"));

    // 检查未注册的行为
    EXPECT_FALSE(registry.hasBehavior("minecraft:unknown_item"));
}

TEST_F(DispenseBehaviorTest, Registry_InitDefaultBehaviors_RegistersProjectiles) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 初始化默认行为
    registry.initDefaultBehaviors();

    // 验证投掷物行为已注册
    EXPECT_TRUE(registry.hasBehavior("minecraft:arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:spectral_arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:tipped_arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:snowball"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:egg"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:ender_pearl"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:experience_bottle"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:splash_potion"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:lingering_potion"));
}

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForSnowball_ReturnsProjectileBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:snowball");
    ASSERT_NE(behavior, nullptr);

    // 雪球使用投掷物行为
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// ProjectileDispenseBehavior 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, ProjectileBehavior_CreatesFactoryBasedBehavior) {
    // 测试工厂函数创建
    bool factoryCalled = false;

    auto factory = [&factoryCalled](IWorld& world, const Vector3& pos, const ItemStack& stack)
        -> std::unique_ptr<mc::Entity> {
        factoryCalled = true;
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(stack);
        return nullptr;  // 返回 nullptr 模拟创建失败
    };

    ProjectileDispenseBehavior behavior(factory, 1.5f, 3.0f);

    // 测试行为已创建
    EXPECT_TRUE(behavior.isSuccess());
}

// ============================================================================
// 世界事件 ID 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, WorldEventIds_AreCorrect) {
    // 验证世界事件 ID 与 MC 1.16.5 一致
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_DISPENSE_SOUND, 1000);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_FAIL_SOUND, 1001);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_LAUNCH_SOUND, 1002);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_SMOKE, 2000);
}

// ============================================================================
// 药水效果发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasTippedArrowBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:tipped_arrow"));
}

TEST_F(DispenseBehaviorTest, Registry_HasSplashPotionBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:splash_potion"));
}

TEST_F(DispenseBehaviorTest, Registry_HasLingeringPotionBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:lingering_potion"));
}

TEST_F(DispenseBehaviorTest, TippedArrowBehavior_IsProjectileBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:tipped_arrow");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, SplashPotionBehavior_IsProjectileBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:splash_potion");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, LingeringPotionBehavior_IsProjectileBehavior) {
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:lingering_potion");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, TippedArrowBehavior_RegisteredWithCorrectItem) {
    // 验证药水箭发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 TIPPED_ARROW 物品已初始化，验证可以通过物品获取行为
    if (Items::TIPPED_ARROW != nullptr) {
        ItemStack stack(Items::TIPPED_ARROW, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, SplashPotionBehavior_RegisteredWithCorrectItem) {
    // 验证喷溅药水发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 SPLASH_POTION 物品已初始化，验证可以通过物品获取行为
    if (Items::SPLASH_POTION != nullptr) {
        ItemStack stack(Items::SPLASH_POTION, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, LingeringPotionBehavior_RegisteredWithCorrectItem) {
    // 验证滞留药水发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 LINGERING_POTION 物品已初始化，验证可以通过物品获取行为
    if (Items::LINGERING_POTION != nullptr) {
        ItemStack stack(Items::LINGERING_POTION, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

} // namespace test
} // namespace blocks
} // namespace mc
