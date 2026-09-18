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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/item/items/potion/LingeringPotionItem.hpp"
#include "common/item/items/potion/SplashPotionItem.hpp"
#include "common/item/items/trial/WindChargeItem.hpp"
#include "common/item/items/weapon/ThrowableItems.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class ProjectileItemTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ProjectileItemTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ProjectileItemTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

private:
    EntityInstanceId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
};

class ProjectileItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    void TearDown() override
    {
        // Items 清理由静态析构处理
    }

    ProjectileItemTestWorld m_world;
};

// ============================================================================
// ProjectileDispenseConfig 测试
// ============================================================================

TEST_F(ProjectileItemTest, DispenseConfig_Defaults)
{
    auto config = item::ProjectileDispenseConfig::defaults();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(ProjectileItemTest, DispenseConfig_Potion)
{
    auto config = item::ProjectileDispenseConfig::potion();
    EXPECT_FLOAT_EQ(config.power, 1.375f);
    EXPECT_FLOAT_EQ(config.uncertainty, 3.0f);
}

TEST_F(ProjectileItemTest, DispenseConfig_WindCharge)
{
    auto config = item::ProjectileDispenseConfig::windCharge();
    EXPECT_FLOAT_EQ(config.power, 1.0f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.6666665f);
}

// ============================================================================
// ThrowableItem 实现 ProjectileItem 接口测试
// ============================================================================

TEST_F(ProjectileItemTest, SnowballItem_IsProjectileItem)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);
}

TEST_F(ProjectileItemTest, SnowballItem_DispenseConfig_IsDefault)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.1f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.0f);
}

TEST_F(ProjectileItemTest, SnowballItem_AsProjectile_CreatesEntity)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::SNOWBALL, 1);
    Vector3 pos(10.0, 64.0, 20.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 实体位置应被设置为传入的位置
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->x()), 10.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->y()), 64.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->z()), 20.0f);

    // 实体应为 SnowballEntity（ProjectileItemEntity 子类）
    auto* snowball = dynamic_cast<entity::SnowballEntity*>(entity.get());
    EXPECT_NE(snowball, nullptr);
}

TEST_F(ProjectileItemTest, EggItem_IsProjectileItem)
{
    ASSERT_NE(Items::EGG, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::EGG);
    ASSERT_NE(projectileItem, nullptr);
}

TEST_F(ProjectileItemTest, EnderPearlItem_IsProjectileItem)
{
    ASSERT_NE(Items::ENDER_PEARL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::ENDER_PEARL);
    ASSERT_NE(projectileItem, nullptr);
}

TEST_F(ProjectileItemTest, ExperienceBottleItem_IsProjectileItem_AndHasPotionConfig)
{
    ASSERT_NE(Items::EXPERIENCE_BOTTLE, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::EXPERIENCE_BOTTLE);
    ASSERT_NE(projectileItem, nullptr);

    // ExperienceBottleItem 应返回药水配置
    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.375f);
    EXPECT_FLOAT_EQ(config.uncertainty, 3.0f);
}

// ============================================================================
// ThrowablePotionItem 实现 ProjectileItem 接口测试
// ============================================================================

TEST_F(ProjectileItemTest, SplashPotionItem_IsProjectileItem_AndHasPotionConfig)
{
    ASSERT_NE(Items::SPLASH_POTION, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SPLASH_POTION);
    ASSERT_NE(projectileItem, nullptr);

    // 喷溅药水应返回药水配置
    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.375f);
    EXPECT_FLOAT_EQ(config.uncertainty, 3.0f);
}

TEST_F(ProjectileItemTest, LingeringPotionItem_IsProjectileItem_AndHasPotionConfig)
{
    ASSERT_NE(Items::LINGERING_POTION, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::LINGERING_POTION);
    ASSERT_NE(projectileItem, nullptr);

    // 滞留药水应返回药水配置
    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.375f);
    EXPECT_FLOAT_EQ(config.uncertainty, 3.0f);
}

TEST_F(ProjectileItemTest, SplashPotionItem_AsProjectile_CreatesPotionEntity)
{
    ASSERT_NE(Items::SPLASH_POTION, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SPLASH_POTION);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::SPLASH_POTION, 1);
    Vector3 pos(0.0, 64.0, 0.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 实体应为 PotionEntity 且非滞留型
    auto* potion = dynamic_cast<entity::PotionEntity*>(entity.get());
    ASSERT_NE(potion, nullptr);
    EXPECT_FALSE(potion->isLingering());
}

TEST_F(ProjectileItemTest, LingeringPotionItem_AsProjectile_CreatesLingeringPotionEntity)
{
    ASSERT_NE(Items::LINGERING_POTION, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::LINGERING_POTION);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::LINGERING_POTION, 1);
    Vector3 pos(0.0, 64.0, 0.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 实体应为 PotionEntity 且为滞留型
    auto* potion = dynamic_cast<entity::PotionEntity*>(entity.get());
    ASSERT_NE(potion, nullptr);
    EXPECT_TRUE(potion->isLingering());
}

// ============================================================================
// WindChargeItem 实现 ProjectileItem 接口测试
// ============================================================================

TEST_F(ProjectileItemTest, WindChargeItem_IsProjectileItem)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::WIND_CHARGE);
    ASSERT_NE(projectileItem, nullptr);
}

TEST_F(ProjectileItemTest, WindChargeItem_DispenseConfig)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::WIND_CHARGE);
    ASSERT_NE(projectileItem, nullptr);

    auto config = projectileItem->getDispenseConfig();
    EXPECT_FLOAT_EQ(config.power, 1.0f);
    EXPECT_FLOAT_EQ(config.uncertainty, 6.6666665f);
}

TEST_F(ProjectileItemTest, WindChargeItem_AsProjectile_CreatesWindChargeEntity)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::WIND_CHARGE);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::WIND_CHARGE, 1);
    Vector3 pos(5.0, 70.0, 10.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 实体位置应被设置
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->x()), 5.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->y()), 70.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(entity->z()), 10.0f);

    // 实体应为 WindChargeEntity
    auto* windCharge = dynamic_cast<entity::WindChargeEntity*>(entity.get());
    EXPECT_NE(windCharge, nullptr);

    // 风弹在 asProjectile 中已根据方向预设初速度
    // 向下方向 (-1.0) 乘以 power (1.0) = -1.0
    EXPECT_LT(entity->velocityY(), 0.0f);
}

TEST_F(ProjectileItemTest, WindChargeItem_Shoot_IsNoop)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::WIND_CHARGE);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::WIND_CHARGE, 1);
    Vector3 pos(0.0, 64.0, 0.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 记录 shoot 前的速度
    f32 preShootVelocityX = entity->velocityX();
    f32 preShootVelocityY = entity->velocityY();
    f32 preShootVelocityZ = entity->velocityZ();

    // WindChargeItem::shoot() 为空操作，不应改变速度
    projectileItem->shoot(*entity, 0.0f, -1.0f, 0.0f, 1.5f, 1.0f);

    EXPECT_FLOAT_EQ(entity->velocityX(), preShootVelocityX);
    EXPECT_FLOAT_EQ(entity->velocityY(), preShootVelocityY);
    EXPECT_FLOAT_EQ(entity->velocityZ(), preShootVelocityZ);
}

// ============================================================================
// 非 ProjectileItem 物品测试
// ============================================================================

TEST_F(ProjectileItemTest, NonProjectileItem_ReturnsNullOnDynamicCast)
{
    // 铁锭不是 ProjectileItem
    ASSERT_NE(Items::IRON_INGOT, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::IRON_INGOT);
    EXPECT_EQ(projectileItem, nullptr);
}

// ============================================================================
// ProjectileItem 默认 shoot 行为测试
// ============================================================================

TEST_F(ProjectileItemTest, ThrowableItem_DefaultShoot_CallsProjectileShoot)
{
    // 雪球的 shoot() 应调用 ProjectileEntity::shoot()
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::SNOWBALL, 1);
    Vector3 pos(0.0, 64.0, 0.0);

    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, -1.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 初始时弹射物没有设置速度（asProjectile 不调用 shoot）
    // 调用 shoot 后应设置速度
    projectileItem->shoot(*entity, 0.0f, -1.0f, 0.0f, 1.1f, 6.0f);

    // shoot 后应有向下的速度
    EXPECT_LT(entity->velocityY(), 0.0f);
}

// ============================================================================
// asProjectile 空物品栈测试
// ============================================================================

TEST_F(ProjectileItemTest, AsProjectile_WithEmptyStack_StillCreatesEntity)
{
    // asProjectile 不依赖物品栈内容来创建实体类型（实体类型由子类决定）
    // 即使物品栈为空，也应能创建弹射物
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack emptyStack;
    Vector3 pos(0.0, 64.0, 0.0);

    auto entity = projectileItem->asProjectile(m_world, pos, emptyStack, 0.0f, -1.0f, 0.0f);
    // 即使物品栈为空，仍应创建实体（实体类型由动态类型决定，不依赖物品栈内容）
    EXPECT_NE(entity, nullptr);
}

// ============================================================================
// asProjectile 零方向向量测试
// ============================================================================

TEST_F(ProjectileItemTest, AsProjectile_ZeroDirection_StillCreatesEntity)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(Items::SNOWBALL);
    ASSERT_NE(projectileItem, nullptr);

    ItemStack stack(Items::SNOWBALL, 1);
    Vector3 pos(0.0, 64.0, 0.0);

    // 零方向向量不应导致崩溃
    auto entity = projectileItem->asProjectile(m_world, pos, stack, 0.0f, 0.0f, 0.0f);
    EXPECT_NE(entity, nullptr);
}

} // namespace
} // namespace mc
