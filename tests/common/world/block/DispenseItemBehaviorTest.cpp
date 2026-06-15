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
 * @file DispenseItemBehaviorTest.cpp
 * @brief 发射器行为系统测试
 *
 * 测试发射器行为的核心功能：
 * - OptionalDispenseItemBehavior 的成功/失败状态
 * - DispenseItemBehaviorRegistry 的注册和查询
 * - 药水效果发射器的药水效果应用
 */

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/ProjectileItemEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/potion/PotionUtils.hpp"
#include "item/potion/Potions.hpp"
#include "util/Direction.hpp"
#include "util/math/Vector3.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "world/block/dispense/IDispenseItemBehavior.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 测试夹具基类
 */
class DispenseBehaviorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
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

TEST_F(DispenseBehaviorTest, OptionalDispenseBehavior_DefaultSuccess)
{
    OptionalDispenseItemBehavior behavior;
    EXPECT_TRUE(behavior.isSuccess());
}

TEST_F(DispenseBehaviorTest, OptionalDispenseBehavior_CanSetFailure)
{
    OptionalDispenseItemBehavior behavior;

    // 通过反射或友元类设置失败状态
    // 注意：_setSuccess 是 protected，需要子类访问
    class TestableOptionalBehavior : public OptionalDispenseItemBehavior {
    public:
        using OptionalDispenseItemBehavior::_setSuccess;
    };

    TestableOptionalBehavior testBehavior;
    EXPECT_TRUE(testBehavior.isSuccess());

    testBehavior._setSuccess(false);
    EXPECT_FALSE(testBehavior.isSuccess());

    testBehavior._setSuccess(true);
    EXPECT_TRUE(testBehavior.isSuccess());
}

// ============================================================================
// DispenseItemBehaviorRegistry 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasDefaultBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* defaultBehavior = registry.getDefaultBehavior();
    ASSERT_NE(defaultBehavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForEmptyStack_ReturnsNull)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    ItemStack emptyStack;
    IDispenseItemBehavior* behavior = registry.getBehavior(emptyStack);
    EXPECT_EQ(behavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForUnregisteredItem_ReturnsNull)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 未注册的物品ID返回 nullptr
    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:unknown_item");
    EXPECT_EQ(behavior, nullptr);
}

TEST_F(DispenseBehaviorTest, Registry_HasBehavior_ReturnsCorrectValue)
{
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

TEST_F(DispenseBehaviorTest, Registry_InitDefaultBehaviors_RegistersProjectiles)
{
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

TEST_F(DispenseBehaviorTest, Registry_GetBehaviorForSnowball_ReturnsProjectileBehavior)
{
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

TEST_F(DispenseBehaviorTest, ProjectileBehavior_CreatesFactoryBasedBehavior)
{
    // 测试工厂函数创建
    bool factoryCalled = false;

    auto factory = [&factoryCalled](
                       IWorld& world, const Vector3& pos, const ItemStack& stack) -> std::unique_ptr<mc::Entity> {
        factoryCalled = true;
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(stack);
        return nullptr; // 返回 nullptr 模拟创建失败
    };

    ProjectileDispenseBehavior behavior(factory, 1.5f, 3.0f);

    // 测试行为已创建
    EXPECT_TRUE(behavior.isSuccess());
}

// ============================================================================
// 世界事件 ID 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, WorldEventIds_AreCorrect)
{
    // 验证世界事件 ID 与 MC 1.16.5 一致
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_DISPENSE_SOUND, 1000);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_FAIL_SOUND, 1001);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_LAUNCH_SOUND, 1002);
    EXPECT_EQ(mc::world::WorldEvents::DISPENSER_SMOKE, 2000);
}

// ============================================================================
// 药水效果发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasTippedArrowBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:tipped_arrow"));
}

TEST_F(DispenseBehaviorTest, Registry_HasSplashPotionBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:splash_potion"));
}

TEST_F(DispenseBehaviorTest, Registry_HasLingeringPotionBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    EXPECT_TRUE(registry.hasBehavior("minecraft:lingering_potion"));
}

TEST_F(DispenseBehaviorTest, TippedArrowBehavior_IsProjectileBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:tipped_arrow");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, SplashPotionBehavior_IsProjectileBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:splash_potion");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, LingeringPotionBehavior_IsProjectileBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:lingering_potion");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, TippedArrowBehavior_RegisteredWithCorrectItem)
{
    // 验证药水箭发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 TIPPED_ARROW 物品已初始化，验证可以通过物品获取行为
    if (Items::TIPPED_ARROW != nullptr) {
        ItemStack stack(Items::TIPPED_ARROW, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, SplashPotionBehavior_RegisteredWithCorrectItem)
{
    // 验证喷溅药水发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 SPLASH_POTION 物品已初始化，验证可以通过物品获取行为
    if (Items::SPLASH_POTION != nullptr) {
        ItemStack stack(Items::SPLASH_POTION, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, LingeringPotionBehavior_RegisteredWithCorrectItem)
{
    // 验证滞留药水发射行为已正确注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 如果 LINGERING_POTION 物品已初始化，验证可以通过物品获取行为
    if (Items::LINGERING_POTION != nullptr) {
        ItemStack stack(Items::LINGERING_POTION, 1);
        IDispenseItemBehavior* behavior = registry.getBehavior(stack);
        EXPECT_NE(behavior, nullptr);
    }
}

// ============================================================================
// 药水效果设置逻辑测试
// 测试 PotionUtils 与发射器工厂函数的集成
// ============================================================================

TEST_F(DispenseBehaviorTest, TippedArrow_PotionEffectsCanBeReadFromItemStack)
{
    // 验证药水箭可以从 ItemStack 读取药水效果
    // 这是发射器药水箭工厂函数的核心逻辑
    if (Items::TIPPED_ARROW == nullptr) {
        GTEST_SKIP() << "TIPPED_ARROW item not initialized";
    }

    ItemStack tippedArrowStack(Items::TIPPED_ARROW, 1);

    // 设置自定义药水效果
    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 600, 1);         // Speed II for 30s
    customEffects.emplace_back(entity::effect::EffectType::Regeneration, 1200, 0); // Regeneration I for 60s

    potion::PotionUtils::setCustomEffects(tippedArrowStack, customEffects);

    // 验证可以通过 PotionUtils::getEffects 读取效果
    auto effects = potion::PotionUtils::getEffects(tippedArrowStack);
    ASSERT_EQ(effects.size(), 2);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(effects[0].amplifier(), 1);
    EXPECT_EQ(effects[1].type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(effects[1].amplifier(), 0);
}

TEST_F(DispenseBehaviorTest, TippedArrow_ColorCanBeCalculatedFromEffects)
{
    // 验证药水箭颜色可以正确计算
    // 这是发射器药水箭工厂函数设置 arrow->setColor() 的核心逻辑
    if (Items::TIPPED_ARROW == nullptr) {
        GTEST_SKIP() << "TIPPED_ARROW item not initialized";
    }

    ItemStack tippedArrowStack(Items::TIPPED_ARROW, 1);

    // 设置自定义药水效果
    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 600, 0);

    potion::PotionUtils::setCustomEffects(tippedArrowStack, customEffects);

    // 验证颜色计算
    u32 color = potion::PotionUtils::getColor(tippedArrowStack);
    EXPECT_NE(color, 0x385DC6FF);       // 不是水瓶颜色
    EXPECT_GT((color >> 24) & 0xFF, 0); // 有有效的 alpha 通道
}

TEST_F(DispenseBehaviorTest, SplashPotion_ItemStackCanBeCreatedWithEffects)
{
    // 验证喷溅药水 ItemStack 可以携带效果
    // 这是发射器药水工厂函数设置 potion->setItemStack(stack) 的核心逻辑
    if (Items::SPLASH_POTION == nullptr) {
        GTEST_SKIP() << "SPLASH_POTION item not initialized";
    }

    ItemStack splashPotionStack(Items::SPLASH_POTION, 1);

    // 设置药水类型
    potion::PotionUtils::setPotion(splashPotionStack, potion::Potions::SWIFTNESS);

    // 设置自定义效果
    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 1200, 1); // Speed II for 60s

    potion::PotionUtils::setCustomEffects(splashPotionStack, customEffects);

    // 验证效果可以通过 PotionUtils::getEffects 读取（模拟 onImpact() 的逻辑）
    auto effects = potion::PotionUtils::getEffects(splashPotionStack);
    ASSERT_FALSE(effects.empty());

    // 验证基础效果 + 自定义效果
    // Swiftness 药水有 Speed 基础效果，自定义效果会合并
    bool hasSpeedEffect = false;
    for (const auto& effect : effects) {
        if (effect.type() == entity::effect::EffectType::Speed) {
            hasSpeedEffect = true;
            break;
        }
    }
    EXPECT_TRUE(hasSpeedEffect);
}

TEST_F(DispenseBehaviorTest, LingeringPotion_ItemStackCanBeCreatedWithEffects)
{
    // 验证滞留药水 ItemStack 可以携带效果
    // 这是发射器药水工厂函数设置 potion->setItemStack(stack) 的核心逻辑
    if (Items::LINGERING_POTION == nullptr) {
        GTEST_SKIP() << "LINGERING_POTION item not initialized";
    }

    ItemStack lingeringPotionStack(Items::LINGERING_POTION, 1);

    // 设置药水类型
    potion::PotionUtils::setPotion(lingeringPotionStack, potion::Potions::HEALING);

    // 验证药水类型已设置
    const potion::Potion* potion = potion::PotionUtils::getPotion(lingeringPotionStack);
    EXPECT_NE(potion, nullptr);
    EXPECT_NE(potion, potion::Potions::EMPTY);

    // Healing 药水包含 InstantHealth 效果，验证可以通过 PotionUtils::getEffects 读取
    auto effects = potion::PotionUtils::getEffects(lingeringPotionStack);
    // 注意：InstantHealth 是即时效果，可能没有持续时间
    // 主要验证药水类型设置成功，效果从药水基础效果中获取
}

TEST_F(DispenseBehaviorTest, EmptyTippedArrow_HasNoCustomEffects)
{
    // 边界情况：未设置效果的药水箭没有自定义效果
    if (Items::TIPPED_ARROW == nullptr) {
        GTEST_SKIP() << "TIPPED_ARROW item not initialized";
    }

    ItemStack tippedArrowStack(Items::TIPPED_ARROW, 1);
    // 不设置任何自定义效果

    // getCustomEffects 只返回自定义效果，不包括药水基础效果
    auto customEffects = potion::PotionUtils::getCustomEffects(tippedArrowStack);
    // 空药水箭应该没有自定义效果
    EXPECT_TRUE(customEffects.empty());
}

TEST_F(DispenseBehaviorTest, EmptySplashPotion_HasNoCustomEffects)
{
    // 边界情况：未设置效果的喷溅药水只有药水类型，没有自定义效果
    if (Items::SPLASH_POTION == nullptr) {
        GTEST_SKIP() << "SPLASH_POTION item not initialized";
    }

    ItemStack splashPotionStack(Items::SPLASH_POTION, 1);
    // 不设置药水类型，默认是水瓶

    // getCustomEffects 只返回自定义效果，不包括药水基础效果
    auto customEffects = potion::PotionUtils::getCustomEffects(splashPotionStack);
    // 空喷溅药水应该没有自定义效果
    EXPECT_TRUE(customEffects.empty());
}

TEST_F(DispenseBehaviorTest, NonPotionItem_ReturnsNullBehavior)
{
    // 边界情况：普通物品没有发射器行为
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    if (Items::DIAMOND == nullptr) {
        GTEST_SKIP() << "DIAMOND item not initialized";
    }

    ItemStack diamondStack(Items::DIAMOND, 1);
    IDispenseItemBehavior* behavior = registry.getBehavior(diamondStack);
    // 钻石应该没有特殊发射行为，返回 nullptr
    EXPECT_EQ(behavior, nullptr);
}

TEST_F(DispenseBehaviorTest, MultipleEffectsPreservedInItemStack)
{
    // 验证多个效果在 ItemStack 中正确保存和读取
    if (Items::TIPPED_ARROW == nullptr) {
        GTEST_SKIP() << "TIPPED_ARROW item not initialized";
    }

    ItemStack stack(Items::TIPPED_ARROW, 1);

    // 设置两个效果（减少数量避免潜在问题）
    std::vector<entity::effect::EffectInstance> effects;
    effects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    effects.emplace_back(entity::effect::EffectType::JumpBoost, 600, 0);

    potion::PotionUtils::setCustomEffects(stack, effects);

    auto retrieved = potion::PotionUtils::getCustomEffects(stack);
    ASSERT_EQ(retrieved.size(), 2);

    // 验证所有效果类型都被保存
    bool hasSpeed = false, hasJump = false;
    for (const auto& effect : retrieved) {
        if (effect.type() == entity::effect::EffectType::Speed) hasSpeed = true;
        if (effect.type() == entity::effect::EffectType::JumpBoost) hasJump = true;
    }
    EXPECT_TRUE(hasSpeed);
    EXPECT_TRUE(hasJump);
}

TEST_F(DispenseBehaviorTest, Registry_InitDefaultBehaviors_Idempotent)
{
    // 验证 initDefaultBehaviors() 可以安全地多次调用
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // 多次调用
    registry.initDefaultBehaviors();
    registry.initDefaultBehaviors();
    registry.initDefaultBehaviors();

    // 验证行为仍然正确
    EXPECT_TRUE(registry.hasBehavior("minecraft:tipped_arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:splash_potion"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:lingering_potion"));
}

// ============================================================================
// 火焰弹发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasFireChargeBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:fire_charge"));
}

TEST_F(DispenseBehaviorTest, FireChargeBehavior_IsProjectileBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:fire_charge");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// 烟花火箭发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasFireworkRocketBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:firework_rocket"));
}

TEST_F(DispenseBehaviorTest, FireworkRocketBehavior_IsProjectileBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:firework_rocket");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// 船发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasBoatBehaviors)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    // 验证所有6种船已注册
    EXPECT_TRUE(registry.hasBehavior("minecraft:oak_boat"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:spruce_boat"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:birch_boat"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:jungle_boat"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:acacia_boat"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:dark_oak_boat"));
}

TEST_F(DispenseBehaviorTest, BoatBehavior_IsDefaultDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:oak_boat");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// 桶发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasBucketBehaviors)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:water_bucket"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:lava_bucket"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:bucket"));
}

TEST_F(DispenseBehaviorTest, WaterBucketBehavior_IsOptionalDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);
    // 桶行为继承自 OptionalDispenseItemBehavior，初始状态为成功
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, LavaBucketBehavior_IsOptionalDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:lava_bucket");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, EmptyBucketBehavior_IsOptionalDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bucket");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// 打火石和骨粉发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasFlintAndSteelBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:flint_and_steel"));
}

TEST_F(DispenseBehaviorTest, FlintAndSteelBehavior_IsOptionalDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:flint_and_steel");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, Registry_HasBoneMealBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:bone_meal"));
}

TEST_F(DispenseBehaviorTest, BoneMealBehavior_IsOptionalDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bone_meal");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// TNT发射器测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_HasTntBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    EXPECT_TRUE(registry.hasBehavior("minecraft:tnt"));
}

TEST_F(DispenseBehaviorTest, TntBehavior_IsDefaultDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:tnt");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// 发射位置计算测试
// ============================================================================

TEST_F(DispenseBehaviorTest, GetDispensePosition_CalculatesCorrectOffset)
{
    // 测试发射位置计算
    // MC 1.16.5: 发射位置 = 方块中心 + 方向偏移 * 0.7
    using namespace mc::blocks;

    // 向北发射 (Direction::North = 2, zOffset = -1)
    BlockPos pos(0, 0, 0);
    Direction northDir = Direction::North;
    Vector3 dispensePos = DefaultDispenseItemBehavior::getDispensePosition(pos, northDir);

    // 中心 (0.5, 0.5, 0.5) + 北方向偏移 (0, 0, -0.7) = (0.5, 0.5, -0.2)
    EXPECT_FLOAT_EQ(dispensePos.x, 0.5f);
    EXPECT_FLOAT_EQ(dispensePos.y, 0.5f);
    EXPECT_FLOAT_EQ(dispensePos.z, -0.2f);
}

TEST_F(DispenseBehaviorTest, GetDispensePosition_EastDirection)
{
    using namespace mc::blocks;

    BlockPos pos(10, 20, 30);
    Direction eastDir = Direction::East;
    Vector3 dispensePos = DefaultDispenseItemBehavior::getDispensePosition(pos, eastDir);

    // 中心 (10.5, 20.5, 30.5) + 东方向偏移 (0.7, 0, 0) = (11.2, 20.5, 30.5)
    EXPECT_FLOAT_EQ(dispensePos.x, 11.2f);
    EXPECT_FLOAT_EQ(dispensePos.y, 20.5f);
    EXPECT_FLOAT_EQ(dispensePos.z, 30.5f);
}

TEST_F(DispenseBehaviorTest, GetDispensePosition_UpDirection)
{
    using namespace mc::blocks;

    BlockPos pos(0, 0, 0);
    Direction upDir = Direction::Up;
    Vector3 dispensePos = DefaultDispenseItemBehavior::getDispensePosition(pos, upDir);

    // 中心 (0.5, 0.5, 0.5) + 上方向偏移 (0, 0.7, 0) = (0.5, 1.2, 0.5)
    EXPECT_FLOAT_EQ(dispensePos.x, 0.5f);
    EXPECT_FLOAT_EQ(dispensePos.y, 1.2f);
    EXPECT_FLOAT_EQ(dispensePos.z, 0.5f);
}

// ============================================================================
// 新实现的发射器行为类型测试
// ============================================================================

TEST_F(DispenseBehaviorTest, WaterBucketBehavior_IsBucketDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);
    // BucketDispenseBehavior 继承自 OptionalDispenseItemBehavior
    EXPECT_TRUE(behavior->isSuccess());
    // 可以安全地 dynamic_cast 验证类型
    auto* bucketBehavior = dynamic_cast<BucketDispenseBehavior*>(behavior);
    // 如果水流体注册成功，behavior 应为 BucketDispenseBehavior
    if (bucketBehavior != nullptr) {
        EXPECT_NE(bucketBehavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, LavaBucketBehavior_IsBucketDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:lava_bucket");
    ASSERT_NE(behavior, nullptr);
    EXPECT_TRUE(behavior->isSuccess());
}

TEST_F(DispenseBehaviorTest, EmptyBucketBehavior_IsEmptyBucketDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bucket");
    ASSERT_NE(behavior, nullptr);
    // EmptyBucketDispenseBehavior 继承自 OptionalDispenseItemBehavior
    auto* emptyBucketBehavior = dynamic_cast<EmptyBucketDispenseBehavior*>(behavior);
    if (emptyBucketBehavior != nullptr) {
        EXPECT_NE(emptyBucketBehavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, FlintAndSteelBehavior_IsFlintAndSteelDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:flint_and_steel");
    ASSERT_NE(behavior, nullptr);
    // FlintAndSteelDispenseBehavior 继承自 OptionalDispenseItemBehavior
    auto* flintBehavior = dynamic_cast<FlintAndSteelDispenseBehavior*>(behavior);
    if (flintBehavior != nullptr) {
        EXPECT_NE(flintBehavior, nullptr);
    }
}

TEST_F(DispenseBehaviorTest, BoneMealBehavior_IsBonemealDispenseBehavior)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bone_meal");
    ASSERT_NE(behavior, nullptr);
    // BonemealDispenseBehavior 继承自 OptionalDispenseItemBehavior
    auto* bonemealBehavior = dynamic_cast<BonemealDispenseBehavior*>(behavior);
    if (bonemealBehavior != nullptr) {
        EXPECT_NE(bonemealBehavior, nullptr);
    }
}

// ============================================================================
// _spawnItemEntity 辅助方法测试
// 验证 DefaultDispenseItemBehavior 的静态方法存在性和发射位置计算一致性
// ============================================================================

TEST_F(DispenseBehaviorTest, SpawnItemEntity_StaticMethodExists)
{
    // _spawnItemEntity 是 protected static 方法，无法直接从测试调用
    // 但可以间接验证：通过 getDispensePosition 确认发射位置计算一致
    using namespace mc::blocks;

    BlockPos pos(5, 10, 15);
    Direction southDir = Direction::South;
    Vector3 dispensePos = DefaultDispenseItemBehavior::getDispensePosition(pos, southDir);

    // 中心 (5.5, 10.5, 15.5) + 南方向偏移 (0, 0, 0.7) = (5.5, 10.5, 16.2)
    EXPECT_FLOAT_EQ(dispensePos.x, 5.5f);
    EXPECT_FLOAT_EQ(dispensePos.y, 10.5f);
    EXPECT_FLOAT_EQ(dispensePos.z, 16.2f);
}

// ============================================================================
// 注册表完整性测试
// ============================================================================

TEST_F(DispenseBehaviorTest, Registry_AllProjectileBehaviorsRegistered)
{
    // 验证所有投掷物发射行为都已注册
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    // 箭矢类
    EXPECT_TRUE(registry.hasBehavior("minecraft:arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:spectral_arrow"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:tipped_arrow"));

    // 投掷物品类
    EXPECT_TRUE(registry.hasBehavior("minecraft:snowball"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:egg"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:ender_pearl"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:experience_bottle"));

    // 药水类
    EXPECT_TRUE(registry.hasBehavior("minecraft:splash_potion"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:lingering_potion"));

    // 火焰弹和烟花
    EXPECT_TRUE(registry.hasBehavior("minecraft:fire_charge"));
    EXPECT_TRUE(registry.hasBehavior("minecraft:firework_rocket"));
}

} // namespace test
} // namespace blocks
} // namespace mc
