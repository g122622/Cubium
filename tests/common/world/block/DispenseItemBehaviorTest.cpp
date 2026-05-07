/**
 * @file DispenseItemBehaviorTest.cpp
 * @brief 发射器行为系统测试
 *
 * 测试发射器行为的核心功能：
 * - OptionalDispenseItemBehavior 的成功/失败状态
 * - DispenseItemBehaviorRegistry 的注册和查询
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
#include "entity/core/Entity.hpp"

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 测试夹具基类
 */
class DispenseBehaviorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表（如果需要）
        VanillaBlocks::initialize();
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

} // namespace test
} // namespace blocks
} // namespace mc
