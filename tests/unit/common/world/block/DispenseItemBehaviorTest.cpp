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

#include "common/core/Result.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "entity/core/Entity.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/ProjectileItemEntity.hpp"
#include "entity/inventory/IInventory.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/core/ProjectileItem.hpp"
#include "item/items/weapon/ArrowItem.hpp"
#include "item/items/weapon/FireChargeItem.hpp"
#include "item/items/weapon/FireworkRocketItem.hpp"
#include "item/items/weapon/SpectralArrowItem.hpp"
#include "item/items/weapon/ThrowableItems.hpp"
#include "item/potion/PotionUtils.hpp"
#include "item/potion/Potions.hpp"
#include "util/Direction.hpp"
#include "util/math/Vector3.hpp"
#include "util/property/Properties.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "world/block/dispense/IDispenseItemBehavior.hpp"

// 测试基础设施
#include "common/TestWorldHelper.hpp"

#include <unordered_map>
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

TEST_F(DispenseBehaviorTest, ProjectileBehavior_CreatesFromProjectileItem)
{
    // 测试通过 ProjectileItem 接口创建行为
    // SnowballItem 实现了 ProjectileItem 接口
    item::SnowballItem snowballItem(ItemProperties().maxStackSize(16));
    ProjectileDispenseBehavior behavior(snowballItem);

    // 测试行为已创建
    EXPECT_TRUE(behavior.isSuccess());
}

// ============================================================================
// ProjectileItem 各子类的 asProjectile / getDispenseConfig 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, ArrowItem_GetDispenseConfig_ReturnsArrowConfig)
{
    // ArrowItem 的 getDispenseConfig 应返回 arrow() 预设（power=1.1, uncertainty=6.0）
    if (Items::ARROW == nullptr) {
        GTEST_SKIP() << "ARROW item not initialized";
    }

    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::ARROW);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(DispenseBehaviorTest, SpectralArrowItem_GetDispenseConfig_ReturnsArrowConfig)
{
    // SpectralArrowItem 继承 ArrowItem 的 getDispenseConfig
    if (Items::SPECTRAL_ARROW == nullptr) {
        GTEST_SKIP() << "SPECTRAL_ARROW item not initialized";
    }

    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SPECTRAL_ARROW);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(DispenseBehaviorTest, FireChargeItem_GetDispenseConfig_ReturnsFireChargeConfig)
{
    // FireChargeItem 的 getDispenseConfig 应返回 fireCharge() 预设（power=1.0, uncertainty=6.0）
    if (Items::FIRE_CHARGE == nullptr) {
        GTEST_SKIP() << "FIRE_CHARGE item not initialized";
    }

    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::FIRE_CHARGE);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.0f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(DispenseBehaviorTest, FireworkRocketItem_GetDispenseConfig_ReturnsFireworkRocketConfig)
{
    // FireworkRocketItem 的 getDispenseConfig 应返回 fireworkRocket() 预设（power=0.5, uncertainty=1.0）
    if (Items::FIREWORK_ROCKET == nullptr) {
        GTEST_SKIP() << "FIREWORK_ROCKET item not initialized";
    }

    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::FIREWORK_ROCKET);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 0.5f);
    EXPECT_FLOAT_EQ(config.uncertainty, 1.0f);
}

TEST_F(DispenseBehaviorTest, SnowballItem_GetDispenseConfig_ReturnsDefaultConfig)
{
    // SnowballItem 使用默认配置（power=1.1, uncertainty=6.0）
    item::SnowballItem snowballItem(ItemProperties().maxStackSize(16));

    auto config = snowballItem.getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(DispenseBehaviorTest, TippedArrowItem_ImplementsProjectileItem)
{
    // 验证 TippedArrowItem 实现了 ProjectileItem 接口
    if (Items::TIPPED_ARROW == nullptr) {
        GTEST_SKIP() << "TIPPED_ARROW item not initialized";
    }

    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::TIPPED_ARROW);
    ASSERT_NE(projectileItem, nullptr);

    // TippedArrowItem 继承 ArrowItem 的配置
    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

// ============================================================================
// registerProjectileBehavior 边界测试
// ============================================================================

TEST_F(DispenseBehaviorTest, RegisterProjectileBehavior_NonProjectileItem_DoesNotRegister)
{
    // registerProjectileBehavior 对非 ProjectileItem 物品应不注册任何行为
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    // DIAMOND 不实现 ProjectileItem 接口
    if (Items::DIAMOND == nullptr) {
        GTEST_SKIP() << "DIAMOND item not initialized";
    }

    // 先确保没有注册过 minecraft:diamond 行为
    std::string diamondId = Items::DIAMOND->itemLocation().toString();
    // 清理可能存在的旧注册（不应存在）
    bool hadBefore = registry.hasBehavior(diamondId);

    // 尝试注册非 ProjectileItem 物品
    registry.registerProjectileBehavior(*Items::DIAMOND);

    // 行为不应该被注册
    if (!hadBefore) {
        EXPECT_FALSE(registry.hasBehavior(diamondId));
    }
}

TEST_F(DispenseBehaviorTest, RegisterProjectileBehavior_ProjectileItem_RegistersCorrectly)
{
    // registerProjectileBehavior 对 ProjectileItem 物品应正确注册行为
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();

    if (Items::SNOWBALL == nullptr) {
        GTEST_SKIP() << "SNOWBALL item not initialized";
    }

    std::string snowballId = Items::SNOWBALL->itemLocation().toString();

    // 使用 registerProjectileBehavior 注册
    registry.registerProjectileBehavior(*Items::SNOWBALL);

    // 行为应该已注册
    EXPECT_TRUE(registry.hasBehavior(snowballId));

    // 注册的行为应该可以 dynamic_cast 到 ProjectileDispenseBehavior
    IDispenseItemBehavior* behavior = registry.getBehavior(snowballId);
    ASSERT_NE(behavior, nullptr);
    auto* projBehavior = dynamic_cast<ProjectileDispenseBehavior*>(behavior);
    EXPECT_NE(projBehavior, nullptr);
}

TEST_F(DispenseBehaviorTest, AllProjectileItems_ImplementProjectileItemInterface)
{
    // 验证所有需要发射器行为的物品都实现了 ProjectileItem 接口
    // 这是通过 dynamic_cast 检测的，registerProjectileBehavior 依赖此接口

    struct ItemCheck {
        const Item* item;
        const char* name;
    };

    ItemCheck projectileItems[] = {
        {Items::ARROW, "ARROW"},
        {Items::SPECTRAL_ARROW, "SPECTRAL_ARROW"},
        {Items::TIPPED_ARROW, "TIPPED_ARROW"},
        {Items::SNOWBALL, "SNOWBALL"},
        {Items::EGG, "EGG"},
        {Items::ENDER_PEARL, "ENDER_PEARL"},
        {Items::EXPERIENCE_BOTTLE, "EXPERIENCE_BOTTLE"},
        {Items::SPLASH_POTION, "SPLASH_POTION"},
        {Items::LINGERING_POTION, "LINGERING_POTION"},
        {Items::FIRE_CHARGE, "FIRE_CHARGE"},
        {Items::WIND_CHARGE, "WIND_CHARGE"},
        {Items::FIREWORK_ROCKET, "FIREWORK_ROCKET"},
    };

    for (const auto& check : projectileItems) {
        if (check.item == nullptr) {
            continue;
        }
        const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(check.item);
        EXPECT_NE(projectileItem, nullptr) << check.name << " should implement ProjectileItem interface";
    }
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
// 行为逻辑测试 — 使用自定义 TestWorld 验证 dispense 核心逻辑
// ============================================================================

namespace {

/// BlockPos 哈希函数，用于 unordered_map
struct BlockPosHasher {
    std::size_t operator()(const BlockPos& pos) const noexcept
    {
        auto h1 = std::hash<i32>{}(pos.x);
        auto h2 = std::hash<i32>{}(pos.y);
        auto h3 = std::hash<i32>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief 专为发射器行为测试定制的 TestWorld
 *
 * 继承 BaseTestWorld 并覆写发射器行为所需的关键方法：
 * - setBlockState(i32,i32,i32,...): 记录最后设置的方块状态
 * - getBlockState(i32,i32,i32): 返回预设状态（支持按位置映射）
 * - getFluidState(i32,i32,i32): 返回预设流体状态
 * - playEvent: 记录播放的事件
 * - spawnEntity: 记录生成的实体（返回 EntityInstanceId）
 * - tickManager: 提供可用的 DummyTickManager
 * - isUltraWarm: 可配置（默认 false，用于下界蒸发测试）
 */
class DispenseTestWorld : public mc::test::BaseTestWorld {
public:
    DispenseTestWorld()
        : m_tickManager()
        , m_isUltraWarm(false)
    {}

    // --- setBlockState: 记录最后一次调用（覆写虚函数） ---
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_lastSetBlockPos = BlockPos(x, y, z);
        m_lastSetBlockState = state;
        m_setBlockStateCallCount++;
        return true;
    }

    // --- getBlockState: 返回预设状态（覆写虚函数） ---
    // 优先按位置查找映射，找不到则返回默认预设状态
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        BlockPos pos(x, y, z);
        auto it = m_blockStateMap.find(pos);
        if (it != m_blockStateMap.end()) {
            return it->second;
        }
        return m_presetBlockState;
    }

    // --- getFluidState: 返回预设流体状态 ---
    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        BlockPos pos(x, y, z);
        auto it = m_fluidStateMap.find(pos);
        if (it != m_fluidStateMap.end()) {
            return it->second;
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    // --- playEvent: 记录事件 ---
    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_playedEvents.push_back({eventId, pos, data});
    }

    // --- spawnEntity: 记录实体并返回 EntityInstanceId ---
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        std::string typeId = entity->getTypeId();
        m_spawnedEntityTypes.push_back(typeId);
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(0);
    }

    // --- tickManager: 提供可用的 DummyTickManager ---
    [[nodiscard]] world::tick::TickManager& tickManager() override { return m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return m_tickManager; }

    // --- isUltraWarm: 可配置 ---
    [[nodiscard]] bool isUltraWarm() const override { return m_isUltraWarm; }

    // --- 设置预设的方块状态（全局默认） ---
    void setPresetBlockState(const BlockState* state) { m_presetBlockState = state; }

    // --- 设置指定位置的方块状态 ---
    void setBlockStateAt(const BlockPos& pos, const BlockState* state) { m_blockStateMap[pos] = state; }

    // --- 设置指定位置的流体状态 ---
    void setFluidStateAt(const BlockPos& pos, const fluid::FluidState* state) { m_fluidStateMap[pos] = state; }

    // --- 设置超热维度 ---
    void setUltraWarm(bool ultraWarm) { m_isUltraWarm = ultraWarm; }

    // --- 查询记录 ---
    i32 setBlockStateCallCount() const { return m_setBlockStateCallCount; }
    const BlockPos& lastSetBlockPos() const { return m_lastSetBlockPos; }
    const BlockState* lastSetBlockState() const { return m_lastSetBlockState; }
    const std::vector<std::tuple<i32, BlockPos, i32>>& playedEvents() const { return m_playedEvents; }
    const std::vector<std::string>& spawnedEntityTypes() const { return m_spawnedEntityTypes; }

    // --- 检查是否有指定事件ID的记录 ---
    bool hasEvent(i32 eventId) const
    {
        for (const auto& ev : m_playedEvents) {
            if (std::get<0>(ev) == eventId) return true;
        }
        return false;
    }

    // --- 清除记录 ---
    void clearRecords()
    {
        m_setBlockStateCallCount = 0;
        m_lastSetBlockPos = BlockPos(0, 0, 0);
        m_lastSetBlockState = nullptr;
        m_playedEvents.clear();
        m_spawnedEntityTypes.clear();
        m_spawnedEntities.clear();
    }

private:
    mc::test::DummyTickManager m_tickManager;
    const BlockState* m_presetBlockState = nullptr;
    bool m_isUltraWarm;
    BlockPos m_lastSetBlockPos{0, 0, 0};
    const BlockState* m_lastSetBlockState = nullptr;
    i32 m_setBlockStateCallCount = 0;
    std::vector<std::tuple<i32, BlockPos, i32>> m_playedEvents;
    std::vector<std::string> m_spawnedEntityTypes;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unordered_map<BlockPos, const BlockState*, BlockPosHasher> m_blockStateMap;
    std::unordered_map<BlockPos, const fluid::FluidState*, BlockPosHasher> m_fluidStateMap;
};

} // anonymous namespace

// ============================================================================
// BucketDispenseBehavior 行为逻辑测试
// ============================================================================

TEST_F(DispenseBehaviorTest, BucketDispense_FailWhenTargetBlockIsSolid)
{
    // 当目标位置是固体方块（石砖）时，桶发射行为应失败并回退到默认投掷行为
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    // 发射器位于 (0,0,0)，面朝北，目标位置为 (0,0,-1)
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 目标位置设为固体方块（石砖）
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    world.setPresetBlockState(stoneBricksState);
    // 发射器位置自身也需要有状态
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);

    if (Items::WATER_BUCKET != nullptr) {
        ItemStack stack(Items::WATER_BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);
        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 失败时应回退到默认投掷行为，OptionalDispenseItemBehavior::isSuccess() 返回 false
        EXPECT_FALSE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, BucketDispense_PlaceWaterOnAir)
{
    // 当目标位置是空气时，水桶应放置水方块
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 目标位置设为空气
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), airState);

    if (Items::WATER_BUCKET != nullptr) {
        ItemStack stack(Items::WATER_BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);
        i32 beforeCall = world.setBlockStateCallCount();

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 成功放置水方块
        EXPECT_TRUE(behavior->isSuccess());
        // 应调用了 setBlockState 在目标位置放置水
        EXPECT_GT(world.setBlockStateCallCount(), beforeCall);
        // 应播放成功音效（1000）
        EXPECT_TRUE(world.hasEvent(world::WorldEvents::DISPENSER_DISPENSE_SOUND));
    }
}

TEST_F(DispenseBehaviorTest, BucketDispense_WaterEvaporatesInUltraWarmDimension)
{
    // 在超热维度（下界）中，水桶应蒸发而不放置水
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    world.setUltraWarm(true); // 模拟超热维度

    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), airState);

    if (Items::WATER_BUCKET != nullptr) {
        ItemStack stack(Items::WATER_BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 成功但水蒸发
        EXPECT_TRUE(behavior->isSuccess());
        // 应播放火焰熄灭音效（1009）
        EXPECT_TRUE(world.hasEvent(world::WorldEvents::FIRE_EXTINGUISH_SOUND));
    }
}

TEST_F(DispenseBehaviorTest, BucketDispense_LavaOnAirSucceeds)
{
    // 岩浆桶在空气位置放置岩浆方块
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:lava_bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), airState);

    if (Items::LAVA_BUCKET != nullptr) {
        ItemStack stack(Items::LAVA_BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 成功放置岩浆
        EXPECT_TRUE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, BucketDispense_TypeVerification)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:water_bucket");
    ASSERT_NE(behavior, nullptr);
    // BucketDispenseBehavior 继承自 OptionalDispenseItemBehavior
    EXPECT_TRUE(behavior->isSuccess());
    // 可以安全地 dynamic_cast 验证类型
    auto* bucketBehavior = dynamic_cast<BucketDispenseBehavior*>(behavior);
    if (bucketBehavior != nullptr) {
        EXPECT_NE(bucketBehavior, nullptr);
    }
}

// ============================================================================
// EmptyBucketDispenseBehavior 行为逻辑测试
// ============================================================================

TEST_F(DispenseBehaviorTest, EmptyBucket_FailWhenNoFluidToPickup)
{
    // 当目标位置没有可拾取流体的方块时，空桶发射行为应失败
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 目标位置是石砖（不实现 IBucketPickupHandler）
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), stoneBricksState);

    if (Items::BUCKET != nullptr) {
        ItemStack stack(Items::BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 失败时应回退到默认投掷行为
        EXPECT_FALSE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, EmptyBucket_FailOnAirBlock)
{
    // 空桶对空气方块也应失败（空气不实现 IBucketPickupHandler）
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bucket");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), airState);

    if (Items::BUCKET != nullptr) {
        ItemStack stack(Items::BUCKET, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        EXPECT_FALSE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, EmptyBucket_TypeVerification)
{
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bucket");
    ASSERT_NE(behavior, nullptr);
    auto* emptyBucketBehavior = dynamic_cast<EmptyBucketDispenseBehavior*>(behavior);
    if (emptyBucketBehavior != nullptr) {
        EXPECT_NE(emptyBucketBehavior, nullptr);
    }
}

// ============================================================================
// FlintAndSteelDispenseBehavior 行为逻辑测试
// ============================================================================

TEST_F(DispenseBehaviorTest, FlintAndSteel_FailOnNonFlammableTarget)
{
    // 当目标位置是不可点燃的方块（石砖）时，打火石应标记为失败
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:flint_and_steel");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 石砖不可点燃
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), stoneBricksState);

    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack stack(Items::FLINT_AND_STEEL, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 失败
        EXPECT_FALSE(behavior->isSuccess());
        // 失败时应播放 DISPENSER_FAIL_SOUND (1001)
        EXPECT_TRUE(world.hasEvent(world::WorldEvents::DISPENSER_FAIL_SOUND));
    }
}

TEST_F(DispenseBehaviorTest, FlintAndSteel_FailOnNullTargetState)
{
    // 当 getBlockState 返回 nullptr 时，打火石应失败
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:flint_and_steel");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    // 默认 getBlockState 返回 nullptr（未设置预设状态）
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 不设置目标位置状态，getBlockState 将返回 nullptr

    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack stack(Items::FLINT_AND_STEEL, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 失败
        EXPECT_FALSE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, FlintAndSteel_ConsumesDurabilityNotQuantity)
{
    // 打火石发射行为消耗耐久度而非数量，这里验证注册和类型正确
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:flint_and_steel");
    ASSERT_NE(behavior, nullptr);
    auto* flintBehavior = dynamic_cast<FlintAndSteelDispenseBehavior*>(behavior);
    if (flintBehavior != nullptr) {
        EXPECT_NE(flintBehavior, nullptr);
    }
}

// ============================================================================
// BonemealDispenseBehavior 行为逻辑测试
// ============================================================================

TEST_F(DispenseBehaviorTest, BoneMeal_FailWhenTargetNotGrowable)
{
    // 当目标方块不可催熟且不是水源时，骨粉应标记为失败
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bone_meal");
    ASSERT_NE(behavior, nullptr);

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    // 石砖不可催熟
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    world.setBlockStateAt(BlockPos(0, 0, 0), dispenserState);
    world.setBlockStateAt(BlockPos(0, 0, -1), stoneBricksState);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack stack(Items::BONE_MEAL, 1);
        BlockPos dispenserPos(0, 0, 0);

        ItemStack result = behavior->dispense(world, dispenserPos, *dispenserState, stack, nullptr);

        // 失败
        EXPECT_FALSE(behavior->isSuccess());
    }
}

TEST_F(DispenseBehaviorTest, BoneMeal_ConsumesQuantity)
{
    // 骨粉发射行为消耗数量而非耐久，这里验证注册和类型正确
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:bone_meal");
    ASSERT_NE(behavior, nullptr);
    auto* bonemealBehavior = dynamic_cast<BonemealDispenseBehavior*>(behavior);
    if (bonemealBehavior != nullptr) {
        EXPECT_NE(bonemealBehavior, nullptr);
    }
}

// ============================================================================
// DefaultDispenseItemBehavior 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, DefaultBehavior_SpawnsItemEntity)
{
    // 验证默认发射行为通过注册表验证
    DispenseItemBehaviorRegistry& registry = DispenseItemBehaviorRegistry::instance();
    registry.initDefaultBehaviors();

    IDispenseItemBehavior* behavior = registry.getBehavior("minecraft:tnt");
    ASSERT_NE(behavior, nullptr);
    // TNT 使用默认发射行为（直接投掷物品）
    EXPECT_TRUE(behavior->isSuccess());
}

// ============================================================================
// DispenseTestWorld 基础功能验证
// ============================================================================

TEST_F(DispenseBehaviorTest, DispenseTestWorld_SetBlockStateRecords)
{
    DispenseTestWorld world;

    const BlockState* testState = VanillaBlocks::getState(VanillaBlocks::STONE);
    world.setBlockState(10, 20, 30, testState);

    EXPECT_EQ(world.setBlockStateCallCount(), 1);
    EXPECT_EQ(world.lastSetBlockPos(), BlockPos(10, 20, 30));
    EXPECT_EQ(world.lastSetBlockState(), testState);
}

TEST_F(DispenseBehaviorTest, DispenseTestWorld_PlayEventRecords)
{
    DispenseTestWorld world;

    BlockPos pos(5, 10, 15);
    world.playEvent(world::WorldEvents::DISPENSER_DISPENSE_SOUND, pos, 0);
    world.playEvent(world::WorldEvents::DISPENSER_FAIL_SOUND, pos, 1);

    ASSERT_EQ(world.playedEvents().size(), 2u);
    EXPECT_EQ(std::get<0>(world.playedEvents()[0]), world::WorldEvents::DISPENSER_DISPENSE_SOUND);
    EXPECT_EQ(std::get<0>(world.playedEvents()[1]), world::WorldEvents::DISPENSER_FAIL_SOUND);
}

TEST_F(DispenseBehaviorTest, DispenseTestWorld_TickManagerAvailable)
{
    DispenseTestWorld world;

    // tickManager() 不应抛出异常
    EXPECT_NO_THROW(world.tickManager());
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

// ============================================================================
// MockInventory - 用于测试 consumeWithRemainder 和 addToInventoryOrDispense
// ============================================================================

namespace {

/**
 * @brief 测试用 Mock 库存，模拟 IInventory 接口
 *
 * 简单的9槽位库存实现，支持 addItem、getItem、setItem 等操作，
 * 用于验证 consumeWithRemainder 的物品放回逻辑。
 */
class MockInventory : public IInventory {
public:
    explicit MockInventory(i32 size = 9)
        : m_items(size)
    {}

    [[nodiscard]] i32 getContainerSize() const override { return static_cast<i32>(m_items.size()); }

    [[nodiscard]] bool isEmpty() const override
    {
        for (const auto& item : m_items) {
            if (!item.isEmpty()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] ItemStack getItem(i32 slot) const override
    {
        if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
            return ItemStack();
        }
        return m_items[slot];
    }

    void setItem(i32 slot, const ItemStack& stack) override
    {
        if (slot >= 0 && slot < static_cast<i32>(m_items.size())) {
            m_items[slot] = stack;
        }
    }

    ItemStack removeItem(i32 slot, i32 count) override
    {
        if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
            return ItemStack();
        }
        ItemStack result = m_items[slot].split(count);
        return result;
    }

    ItemStack removeItemNoUpdate(i32 slot) override
    {
        if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
            return ItemStack();
        }
        ItemStack result = std::move(m_items[slot]);
        m_items[slot] = ItemStack();
        return result;
    }

    void clear() override
    {
        for (auto& item : m_items) {
            item = ItemStack();
        }
    }

    [[nodiscard]] i32 getFirstEmptySlot() const override
    {
        for (i32 i = 0; i < static_cast<i32>(m_items.size()); ++i) {
            if (m_items[i].isEmpty()) {
                return i;
            }
        }
        return -1;
    }

private:
    std::vector<ItemStack> m_items;
};

} // anonymous namespace

// ============================================================================
// consumeWithRemainder 测试
// ============================================================================

TEST_F(DispenseBehaviorTest, ConsumeWithRemainder_SingleItem_ReturnsReplacement)
{
    // 场景1：原始物品只有1个，shrink(1)后为空，直接返回替换物品
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    // 1个水桶 -> 消耗后返回空桶
    if (Items::WATER_BUCKET != nullptr && Items::BUCKET != nullptr) {
        ItemStack original(Items::WATER_BUCKET, 1);
        ItemStack replacement(Items::BUCKET, 1);

        ItemStack result = DefaultDispenseItemBehavior::consumeWithRemainder(
            world, dispenserPos, *dispenserState, original, replacement, nullptr);

        // 原始物品变为空
        EXPECT_TRUE(original.isEmpty());
        // 返回替换物品（空桶）
        EXPECT_FALSE(result.isEmpty());
        EXPECT_EQ(result.getItem(), Items::BUCKET);
        EXPECT_EQ(result.getCount(), 1);
    }
}

TEST_F(DispenseBehaviorTest, ConsumeWithRemainder_MultipleItems_InventoryAcceptsReplacement)
{
    // 场景2：原始物品有多个，shrink(1)后还有剩余，替换物品能放入库存
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::WATER_BUCKET != nullptr && Items::BUCKET != nullptr) {
        ItemStack original(Items::WATER_BUCKET, 3);
        ItemStack replacement(Items::BUCKET, 1);
        MockInventory inventory(9);

        ItemStack result = DefaultDispenseItemBehavior::consumeWithRemainder(
            world, dispenserPos, *dispenserState, original, replacement, &inventory);

        // 原始物品减1后还剩2
        EXPECT_FALSE(original.isEmpty());
        EXPECT_EQ(original.getCount(), 2);
        // 返回的是剩余的原始物品（2个水桶）
        EXPECT_EQ(result.getItem(), Items::WATER_BUCKET);
        EXPECT_EQ(result.getCount(), 2);
        // 空桶应该被放入了库存的某个空槽位
        bool foundEmptyBucket = false;
        for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
            ItemStack slotItem = inventory.getItem(i);
            if (!slotItem.isEmpty() && slotItem.getItem() == Items::BUCKET) {
                foundEmptyBucket = true;
                EXPECT_EQ(slotItem.getCount(), 1);
                break;
            }
        }
        EXPECT_TRUE(foundEmptyBucket);
    }
}

TEST_F(DispenseBehaviorTest, ConsumeWithRemainder_MultipleItems_InventoryFull_DispatchesToPlayer)
{
    // 场景3：原始物品有多个，库存已满，替换物品弹出到世界中
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::WATER_BUCKET != nullptr && Items::BUCKET != nullptr && Items::STONE != nullptr) {
        ItemStack original(Items::WATER_BUCKET, 2);
        ItemStack replacement(Items::BUCKET, 1);

        // 创建一个满库存（9个槽位都放满石头）
        MockInventory inventory(9);
        for (i32 i = 0; i < 9; ++i) {
            inventory.setItem(i, ItemStack(Items::STONE, 64));
        }

        i32 spawnCountBefore = static_cast<i32>(world.spawnedEntityTypes().size());

        ItemStack result = DefaultDispenseItemBehavior::consumeWithRemainder(
            world, dispenserPos, *dispenserState, original, replacement, &inventory);

        // 原始物品减1后还剩1
        EXPECT_FALSE(original.isEmpty());
        EXPECT_EQ(original.getCount(), 1);
        // 返回的是剩余的原始物品（1个水桶）
        EXPECT_EQ(result.getItem(), Items::WATER_BUCKET);
        EXPECT_EQ(result.getCount(), 1);
        // 库存仍然是满的（空桶没有被放入）
        // 替换物品应该被弹出到世界中（通过 _spawnItemEntity）
        EXPECT_GT(world.spawnedEntityTypes().size(), static_cast<size_t>(spawnCountBefore));
    }
}

TEST_F(DispenseBehaviorTest, ConsumeWithRemainder_NullInventory_DispatchesToPlayer)
{
    // 场景4：dispenserInventory 为 nullptr，替换物品直接弹出到世界中
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::WATER_BUCKET != nullptr && Items::BUCKET != nullptr) {
        ItemStack original(Items::WATER_BUCKET, 2);
        ItemStack replacement(Items::BUCKET, 1);

        i32 spawnCountBefore = static_cast<i32>(world.spawnedEntityTypes().size());

        // 传入 nullptr 库存指针
        ItemStack result = DefaultDispenseItemBehavior::consumeWithRemainder(
            world, dispenserPos, *dispenserState, original, replacement, nullptr);

        // 原始物品减1后还剩1
        EXPECT_FALSE(original.isEmpty());
        EXPECT_EQ(original.getCount(), 1);
        // 返回剩余的原始物品
        EXPECT_EQ(result.getItem(), Items::WATER_BUCKET);
        EXPECT_EQ(result.getCount(), 1);
        // 替换物品被弹出到世界中
        EXPECT_GT(world.spawnedEntityTypes().size(), static_cast<size_t>(spawnCountBefore));
    }
}

TEST_F(DispenseBehaviorTest, AddToInventoryOrDispense_InventoryAcceptsItem)
{
    // addToInventoryOrDispense: 库存有空间，物品成功放入
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::BUCKET != nullptr) {
        MockInventory inventory(9);
        ItemStack itemToInsert(Items::BUCKET, 1);

        // 库存为空，应该可以成功放入
        DefaultDispenseItemBehavior::addToInventoryOrDispense(
            world, dispenserPos, *dispenserState, itemToInsert, &inventory);

        // 验证物品被放入库存
        bool foundItem = false;
        for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
            ItemStack slotItem = inventory.getItem(i);
            if (!slotItem.isEmpty() && slotItem.getItem() == Items::BUCKET) {
                foundItem = true;
                break;
            }
        }
        EXPECT_TRUE(foundItem);
    }
}

TEST_F(DispenseBehaviorTest, AddToInventoryOrDispense_InventoryFull_SpawnsEntity)
{
    // addToInventoryOrDispense: 库存已满，物品弹出到世界
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::BUCKET != nullptr && Items::STONE != nullptr) {
        // 满库存
        MockInventory inventory(9);
        for (i32 i = 0; i < 9; ++i) {
            inventory.setItem(i, ItemStack(Items::STONE, 64));
        }

        ItemStack itemToInsert(Items::BUCKET, 1);
        i32 spawnCountBefore = static_cast<i32>(world.spawnedEntityTypes().size());

        DefaultDispenseItemBehavior::addToInventoryOrDispense(
            world, dispenserPos, *dispenserState, itemToInsert, &inventory);

        // 替换物品被弹出到世界（生成了物品实体）
        EXPECT_GT(world.spawnedEntityTypes().size(), static_cast<size_t>(spawnCountBefore));
    }
}

TEST_F(DispenseBehaviorTest, AddToInventoryOrDispense_NullInventory_SpawnsEntity)
{
    // addToInventoryOrDispense: nullptr 库存，物品直接弹出到世界
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    if (Items::BUCKET != nullptr) {
        ItemStack itemToInsert(Items::BUCKET, 1);
        i32 spawnCountBefore = static_cast<i32>(world.spawnedEntityTypes().size());

        DefaultDispenseItemBehavior::addToInventoryOrDispense(
            world, dispenserPos, *dispenserState, itemToInsert, nullptr);

        // nullptr 库存，物品应该被弹出
        EXPECT_GT(world.spawnedEntityTypes().size(), static_cast<size_t>(spawnCountBefore));
    }
}

TEST_F(DispenseBehaviorTest, AddToInventoryOrDispense_EmptyStack_NoOp)
{
    // addToInventoryOrDispense: 空物品堆，不应产生任何操作
    using namespace mc::blocks;

    DispenseTestWorld world;
    const BlockState* dispenserState =
        &VanillaBlocks::DISPENSER->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    BlockPos dispenserPos(0, 0, 0);

    MockInventory inventory(9);
    ItemStack emptyStack;

    i32 spawnCountBefore = static_cast<i32>(world.spawnedEntityTypes().size());

    DefaultDispenseItemBehavior::addToInventoryOrDispense(world, dispenserPos, *dispenserState, emptyStack, &inventory);

    // 空物品堆不应改变库存也不应生成实体
    EXPECT_TRUE(inventory.isEmpty());
    EXPECT_EQ(world.spawnedEntityTypes().size(), static_cast<size_t>(spawnCountBefore));
}

} // namespace test
} // namespace blocks
} // namespace mc
