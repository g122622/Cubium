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
#include "common/core/Constants.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/Items.hpp"
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
class ThrowableTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ThrowableTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ThrowableTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        // 不实际存储实体，返回临时ID
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

class ThrowableItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    void TearDown() override
    {
        // Items 清理由静态析构处理
    }

    ThrowableTestWorld m_world;
};

// ============================================================================
// SnowballItem 测试
// ============================================================================

TEST_F(ThrowableItemTest, SnowballItem_Registered_HasCorrectProperties)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    EXPECT_EQ(Items::SNOWBALL->maxStackSize(), 16);

    auto* snowball = dynamic_cast<const item::SnowballItem*>(Items::SNOWBALL);
    ASSERT_NE(snowball, nullptr);
    EXPECT_FLOAT_EQ(snowball->getThrowVelocity(), 1.5f);
    EXPECT_FLOAT_EQ(snowball->getThrowInaccuracy(), 0.0f);
    EXPECT_EQ(snowball->getUseDuration(ItemStack::EMPTY), 0);
}

TEST_F(ThrowableItemTest, SnowballItem_OnRightClick_SpawnsEntity)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::SNOWBALL, 16);
    player.getHeldItem(Hand::MainHand) = stack;

    // 调用 onItemRightClick 应该生成实体
    ItemActionResult result = Items::SNOWBALL->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    // 在创造模式下不应该消耗物品
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 16);
    // 应该生成了实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// EggItem 测试
// ============================================================================

TEST_F(ThrowableItemTest, EggItem_Registered_HasCorrectProperties)
{
    ASSERT_NE(Items::EGG, nullptr);
    EXPECT_EQ(Items::EGG->maxStackSize(), 16);

    auto* egg = dynamic_cast<const item::EggItem*>(Items::EGG);
    ASSERT_NE(egg, nullptr);
    EXPECT_FLOAT_EQ(egg->getThrowVelocity(), 1.5f);
    EXPECT_FLOAT_EQ(egg->getThrowInaccuracy(), 0.0f);
}

TEST_F(ThrowableItemTest, EggItem_OnRightClick_SpawnsEntity)
{
    ASSERT_NE(Items::EGG, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::EGG, 16);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::EGG->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 16);
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// EnderPearlItem 测试
// ============================================================================

TEST_F(ThrowableItemTest, EnderPearlItem_Registered_HasCorrectProperties)
{
    ASSERT_NE(Items::ENDER_PEARL, nullptr);
    EXPECT_EQ(Items::ENDER_PEARL->maxStackSize(), 16);

    auto* pearl = dynamic_cast<const item::EnderPearlItem*>(Items::ENDER_PEARL);
    ASSERT_NE(pearl, nullptr);
    EXPECT_FLOAT_EQ(pearl->getThrowVelocity(), 1.5f);
    EXPECT_FLOAT_EQ(pearl->getThrowInaccuracy(), 0.0f);
}

TEST_F(ThrowableItemTest, EnderPearlItem_OnRightClick_SpawnsEntity)
{
    ASSERT_NE(Items::ENDER_PEARL, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::ENDER_PEARL, 16);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::ENDER_PEARL->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 16);
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// ExperienceBottleItem 测试
// ============================================================================

TEST_F(ThrowableItemTest, ExperienceBottleItem_Registered_HasCorrectProperties)
{
    ASSERT_NE(Items::EXPERIENCE_BOTTLE, nullptr);
    EXPECT_EQ(Items::EXPERIENCE_BOTTLE->maxStackSize(), 64);

    auto* expBottle = dynamic_cast<const item::ExperienceBottleItem*>(Items::EXPERIENCE_BOTTLE);
    ASSERT_NE(expBottle, nullptr);
    EXPECT_FLOAT_EQ(expBottle->getThrowVelocity(), 1.5f);
    EXPECT_FLOAT_EQ(expBottle->getThrowInaccuracy(), 0.0f);
}

TEST_F(ThrowableItemTest, ExperienceBottleItem_OnRightClick_SpawnsEntity)
{
    ASSERT_NE(Items::EXPERIENCE_BOTTLE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::EXPERIENCE_BOTTLE, 64);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::EXPERIENCE_BOTTLE->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 64);
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// Survival Mode 消耗测试
// ============================================================================

TEST_F(ThrowableItemTest, SnowballItem_SurvivalMode_ConsumesItem)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival); // 生存模式

    ItemStack stack(Items::SNOWBALL, 10);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::SNOWBALL->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    // 在生存模式下应该消耗一个物品
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 9);
}

// ============================================================================
// 投掷物品实体创建测试
// ============================================================================

TEST_F(ThrowableItemTest, SnowballEntity_CanBeCreated)
{
    entity::SnowballEntity snowball(EntityInstanceId(1));
    EXPECT_TRUE(snowball.isAlive());
}

TEST_F(ThrowableItemTest, EggEntity_CanBeCreated)
{
    entity::EggEntity egg(EntityInstanceId(1));
    EXPECT_TRUE(egg.isAlive());
}

TEST_F(ThrowableItemTest, EnderPearlEntity_CanBeCreated)
{
    entity::EnderPearlEntity pearl(EntityInstanceId(1));
    EXPECT_TRUE(pearl.isAlive());
}

TEST_F(ThrowableItemTest, ExperienceBottleEntity_CanBeCreated)
{
    entity::ExperienceBottleEntity bottle(EntityInstanceId(1));
    EXPECT_TRUE(bottle.isAlive());
}

// ============================================================================
// 默认物品测试
// ============================================================================

TEST_F(ThrowableItemTest, SnowballEntity_GetDefaultItem)
{
    Items::initialize();
    entity::SnowballEntity snowball(EntityInstanceId(1));
    EXPECT_EQ(snowball.getDefaultItem(), Items::SNOWBALL);
}

TEST_F(ThrowableItemTest, EggEntity_GetDefaultItem)
{
    Items::initialize();
    entity::EggEntity egg(EntityInstanceId(1));
    EXPECT_EQ(egg.getDefaultItem(), Items::EGG);
}

TEST_F(ThrowableItemTest, EnderPearlEntity_GetDefaultItem)
{
    Items::initialize();
    entity::EnderPearlEntity pearl(EntityInstanceId(1));
    EXPECT_EQ(pearl.getDefaultItem(), Items::ENDER_PEARL);
}

TEST_F(ThrowableItemTest, ExperienceBottleEntity_GetDefaultItem)
{
    Items::initialize();
    entity::ExperienceBottleEntity bottle(EntityInstanceId(1));
    EXPECT_EQ(bottle.getDefaultItem(), Items::EXPERIENCE_BOTTLE);
}

} // namespace
} // namespace mc
