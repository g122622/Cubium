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
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/trial/WindChargeItem.hpp"
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
 * @brief 风弹物品测试用世界存根
 */
class WindChargeTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WindChargeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WindChargeTestWorld::tickManager not implemented");
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

class WindChargeItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    void TearDown() override
    {
        // Items 清理由静态析构处理
    }

    WindChargeTestWorld m_world;
};

// ============================================================================
// WindChargeItem 注册和属性测试
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeItem_Registered_HasCorrectProperties)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);
    // 风弹最大堆叠64
    EXPECT_EQ(Items::WIND_CHARGE->maxStackSize(), 64);

    // 验证 WindChargeItem 常量
    auto* windCharge = dynamic_cast<const item::WindChargeItem*>(Items::WIND_CHARGE);
    ASSERT_NE(windCharge, nullptr);
    EXPECT_EQ(windCharge->COOLDOWN_TICKS, 10);
    EXPECT_FLOAT_EQ(windCharge->DAMAGE, 1.0f);
    EXPECT_FLOAT_EQ(windCharge->WIND_BURST_INNER_RADIUS, 4.0f);
    EXPECT_FLOAT_EQ(windCharge->WIND_BURST_MIDDLE_RADIUS, 8.0f);
    EXPECT_FLOAT_EQ(windCharge->WIND_BURST_OUTER_RADIUS, 24.0f);
    EXPECT_FLOAT_EQ(windCharge->THROW_VELOCITY, 1.5f);
    EXPECT_FLOAT_EQ(windCharge->THROW_INACCURACY, 1.0f);
}

// ============================================================================
// WindChargeItem 右键使用测试（创造模式）
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeItem_OnRightClick_SpawnsEntity_CreativeMode)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::WIND_CHARGE, 64);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    // 创造模式下不应该消耗物品
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 64);
    // 应该生成了实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// WindChargeItem 右键使用测试（生存模式 - 消耗物品）
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeItem_OnRightClick_ConsumesItem_SurvivalMode)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::WIND_CHARGE, 10);
    player.getHeldItem(Hand::MainHand) = stack;

    ItemActionResult result = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccessOrConsume());
    // 生存模式下应该消耗1个物品
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 9);
    // 应该生成了实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

// ============================================================================
// WindChargeItem 冷却测试
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeItem_OnRightClick_SetsCooldown)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::WIND_CHARGE, 64);
    player.getHeldItem(Hand::MainHand) = stack;

    // 第一次使用应该成功
    ItemActionResult result1 = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(result1.isSuccessOrConsume());

    // 使用后应该有冷却
    EXPECT_TRUE(player.hasItemCooldown(Items::WIND_CHARGE));
}

TEST_F(WindChargeItemTest, WindChargeItem_OnRightClick_FailsDuringCooldown)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::WIND_CHARGE, 64);
    player.getHeldItem(Hand::MainHand) = stack;

    // 第一次使用应该成功
    ItemActionResult result1 = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(result1.isSuccessOrConsume());
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);

    // 冷却中第二次使用应该失败
    ItemActionResult result2 = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(result2.isFail());
    // 不应生成新实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
    // 创造模式下不消耗物品
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 64);
}

// ============================================================================
// WindChargeItem 冷却过期后可再次使用
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeItem_CanUseAgain_AfterCooldownExpires)
{
    ASSERT_NE(Items::WIND_CHARGE, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::WIND_CHARGE, 64);
    player.getHeldItem(Hand::MainHand) = stack;

    // 第一次使用
    Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(player.hasItemCooldown(Items::WIND_CHARGE));

    // 模拟冷却过期（tick 足够次数）
    for (i32 i = 0; i < item::WindChargeItem::COOLDOWN_TICKS + 1; ++i) {
        player.cooldownTracker().tick();
    }

    // 冷却应该已过期
    EXPECT_FALSE(player.hasItemCooldown(Items::WIND_CHARGE));

    // 应该可以再次使用
    ItemActionResult result = Items::WIND_CHARGE->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(result.isSuccessOrConsume());
}

// ============================================================================
// WindChargeEntity 创建测试
// ============================================================================

TEST_F(WindChargeItemTest, WindChargeEntity_CanBeCreated)
{
    entity::WindChargeEntity windCharge(EntityInstanceId(1));
    EXPECT_TRUE(windCharge.isAlive());
}

TEST_F(WindChargeItemTest, WindChargeEntity_GravityIsCorrect)
{
    entity::WindChargeEntity windCharge(EntityInstanceId(1));
    // 风弹重力应与 ThrowableEntity 默认值一致
    EXPECT_FLOAT_EQ(windCharge.getGravity(), 0.03f);
}

} // namespace
} // namespace mc
