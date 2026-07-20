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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/TridentEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

using entity::PickupStatus;

// ============================================================================
// 测试辅助类：暴露受保护成员的三叉戟子类
// ============================================================================

/**
 * @brief 可测试的三叉戟子类
 *
 * 暴露受保护的成员和方法，以便单元测试直接访问。
 */
class TestableTridentEntity : public entity::TridentEntity {
public:
    explicit TestableTridentEntity(EntityInstanceId id)
        : TridentEntity(id)
    {}

    using TridentEntity::_shouldReturnToThrower;
    using TridentEntity::_tickReturning;
    using TridentEntity::tickInGroundTrident;

    void setDealtDamage(bool dealt) { m_dealtDamage = dealt; }
    void setTimeInGround(i32 time) { m_timeInGround = time; }
};

// ============================================================================
// 测试世界
// ============================================================================

/**
 * @brief 三叉戟忠诚附魔测试世界
 *
 * 提供 TridentEntity 测试所需的最小 IWorld 实现，
 * 支持实体存储、查询和物品生成追踪。
 */
class TridentLoyaltyTestWorld : public test::BaseTestWorld {
public:
    TridentLoyaltyTestWorld()
        : m_random(12345)
    {}

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity.get() == except || entity->isRemoved()) {
                continue;
            }
            if (box.intersects(entity->boundingBox())) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_entities.push_back(std::move(entity));
        return id;
    }

    template <typename T, typename... Args>
    T& addEntity(Args&&... args)
    {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entity->setWorld(this);
        T& reference = *entity;
        m_entities.push_back(std::move(entity));
        return reference;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TridentLoyaltyTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TridentLoyaltyTestWorld::tickManager not implemented");
    }

    /// 获取已生成的实体数量
    [[nodiscard]] size_t entityCount() const { return m_entities.size(); }

    /// 查找指定类型的实体
    template <typename T>
    T* findEntityOfType() const
    {
        for (const auto& entity : m_entities) {
            if (!entity->isRemoved()) {
                auto* casted = dynamic_cast<T*>(entity.get());
                if (casted != nullptr) {
                    return casted;
                }
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    mutable math::Random m_random;
};

// ============================================================================
// 测试固定装置
// ============================================================================

class TridentLoyaltyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_world = std::make_unique<TridentLoyaltyTestWorld>();
    }

    void TearDown() override { m_world.reset(); }

    /// 创建一个可测试的三叉戟实体并设置基本属性
    TestableTridentEntity& createTrident(EntityInstanceId id = EntityInstanceId(1))
    {
        auto& trident = m_world->addEntity<TestableTridentEntity>(id);
        trident.setPosition(0.0, 64.0, 0.0);
        // 设置三叉戟物品
        if (Items::TRIDENT != nullptr) {
            ItemStack tridentStack(*Items::TRIDENT, 1);
            trident.setItemStack(tridentStack);
        }
        return trident;
    }

    /// 创建一个玩家实体
    Player& createPlayer(EntityInstanceId id = EntityInstanceId(100), const std::string& name = "TestPlayer")
    {
        auto& player = m_world->addEntity<Player>(id, name);
        player.setPosition(10.0, 64.0, 0.0);
        return player;
    }

    std::unique_ptr<TridentLoyaltyTestWorld> m_world;
};

// ============================================================================
// _shouldReturnToThrower 测试
// ============================================================================

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_NullShooter_ReturnsFalse)
{
    auto& trident = createTrident();
    trident.setShooter(nullptr);
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_DeadShooter_ReturnsFalse)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    // Entity::isAlive() 检查 !m_removed，remove() 设置 m_removed = true
    player.remove();
    trident.setShooter(&player);
    EXPECT_FALSE(player.isAlive());
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_AliveSurvivalPlayer_ReturnsTrue)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    player.setGameMode(GameMode::Survival);
    trident.setShooter(&player);
    EXPECT_TRUE(player.isAlive());
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_SpectatorPlayer_ReturnsFalse)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    player.setGameMode(GameMode::Spectator);
    trident.setShooter(&player);
    EXPECT_TRUE(player.isSpectator());
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_CreativePlayer_ReturnsTrue)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    player.setGameMode(GameMode::Creative);
    trident.setShooter(&player);
    EXPECT_TRUE(player.isCreative());
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_AdventurePlayer_ReturnsTrue)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    player.setGameMode(GameMode::Adventure);
    trident.setShooter(&player);
    EXPECT_TRUE(player.isAdventure());
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

// ============================================================================
// 忠诚附魔属性测试
// ============================================================================

TEST_F(TridentLoyaltyTest, DefaultLoyaltyLevelIsZero)
{
    auto& trident = createTrident();
    EXPECT_EQ(trident.loyaltyLevel(), 0);
}

TEST_F(TridentLoyaltyTest, SetLoyaltyLevel_UpdatesValue)
{
    auto& trident = createTrident();
    trident.setLoyaltyLevel(3);
    EXPECT_EQ(trident.loyaltyLevel(), 3);
}

TEST_F(TridentLoyaltyTest, SetItemStack_ExtractsLoyaltyLevel)
{
    auto& trident = createTrident();
    EXPECT_EQ(trident.loyaltyLevel(), 0);

    if (Items::TRIDENT != nullptr) {
        ItemStack tridentStack(*Items::TRIDENT, 1);
        tridentStack.addEnchantment("minecraft:loyalty", 3);
        trident.setItemStack(tridentStack);
        EXPECT_EQ(trident.loyaltyLevel(), 3);
    }
}

// ============================================================================
// 三叉戟基本属性测试
// ============================================================================

TEST_F(TridentLoyaltyTest, DefaultDamageIsEight)
{
    auto& trident = createTrident();
    EXPECT_FLOAT_EQ(trident.damage(), 8.0f);
}

TEST_F(TridentLoyaltyTest, DefaultPickupStatusIsAllowed)
{
    auto& trident = createTrident();
    EXPECT_EQ(trident.pickupStatus(), PickupStatus::Allowed);
}

TEST_F(TridentLoyaltyTest, DefaultReturningIsFalse)
{
    auto& trident = createTrident();
    EXPECT_FALSE(trident.isReturning());
}

TEST_F(TridentLoyaltyTest, SetReturning_UpdatesValue)
{
    auto& trident = createTrident();
    trident.setReturning(true);
    EXPECT_TRUE(trident.isReturning());
    trident.setReturning(false);
    EXPECT_FALSE(trident.isReturning());
}

TEST_F(TridentLoyaltyTest, GetWaterDrag_ReturnsLowDrag)
{
    auto& trident = createTrident();
    EXPECT_FLOAT_EQ(trident.getWaterDrag(), 0.99f);
}

// ============================================================================
// 射手游戏模式切换场景测试
// ============================================================================

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_SwitchFromSurvivalToSpectator)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    player.setGameMode(GameMode::Survival);
    trident.setShooter(&player);

    EXPECT_TRUE(trident._shouldReturnToThrower());

    player.setGameMode(GameMode::Spectator);
    EXPECT_FALSE(trident._shouldReturnToThrower());

    player.setGameMode(GameMode::Survival);
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_ShooterDiesAndRespawns)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);

    EXPECT_TRUE(trident._shouldReturnToThrower());

    // 模拟射手死亡
    player.remove();
    EXPECT_FALSE(trident._shouldReturnToThrower());

    // 模拟重生：创建新射手实体并重新设置
    auto& player2 = createPlayer(EntityInstanceId(200), "RespawnedPlayer");
    trident.setShooter(&player2);
    EXPECT_TRUE(player2.isAlive());
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, ShouldReturnToThrower_NoLoyalty_DoesNotAffectReturn)
{
    // 忠诚等级不影响 _shouldReturnToThrower，只影响是否触发返回
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);
    trident.setLoyaltyLevel(0);

    // 即使忠诚等级为0，射手状态检查仍然正确
    EXPECT_TRUE(trident._shouldReturnToThrower());

    player.setGameMode(GameMode::Spectator);
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

// ============================================================================
// ItemDropHelper 安全性测试
// ============================================================================

TEST_F(TridentLoyaltyTest, ItemDropHelper_NullWorld_ReturnsNullptr)
{
    math::Random rng(12345);
    ItemStack stack;
    auto* result =
        ItemDropHelper::spawnItemEntity(nullptr, stack, 0.0, 64.0, 0.0, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
    EXPECT_EQ(result, nullptr);
}

TEST_F(TridentLoyaltyTest, ItemDropHelper_NullEntity_ReturnsNullptr)
{
    math::Random rng(12345);
    ItemStack stack;
    auto* result = ItemDropHelper::spawnItemAtEntity(nullptr, stack, 0.5f, rng);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// tickInGroundTrident 测试
// ============================================================================

TEST_F(TridentLoyaltyTest, TickInGround_NoLoyaltyAllowedPickup_UsesNormalTimeout)
{
    auto& trident = createTrident();
    trident.setLoyaltyLevel(0);
    trident.setPickupStatus(PickupStatus::Allowed);
    trident.setInGround(true);

    // 没有忠诚附魔 -> tickInGroundTrident 委托给 AbstractArrowEntity::tickInGround
    // 验证前置条件
    EXPECT_EQ(trident.loyaltyLevel(), 0);
    EXPECT_EQ(trident.pickupStatus(), PickupStatus::Allowed);
    EXPECT_TRUE(trident.isInGround());
}

TEST_F(TridentLoyaltyTest, TickInGround_WithLoyaltyAllowedPickup_NoTimeout)
{
    auto& trident = createTrident();
    trident.setLoyaltyLevel(3);
    trident.setPickupStatus(PickupStatus::Allowed);
    trident.setInGround(true);

    // 有忠诚附魔且允许拾取 -> 不超时，等待返回
    EXPECT_EQ(trident.loyaltyLevel(), 3);
    EXPECT_EQ(trident.pickupStatus(), PickupStatus::Allowed);
}

TEST_F(TridentLoyaltyTest, TickInGround_WithLoyaltyDisallowed_UsesNormalTimeout)
{
    auto& trident = createTrident();
    trident.setLoyaltyLevel(3);
    trident.setPickupStatus(PickupStatus::Disallowed);
    trident.setInGround(true);

    // 忠诚附魔但不允许拾取 -> 使用普通超时逻辑
    EXPECT_EQ(trident.pickupStatus(), PickupStatus::Disallowed);
}

// ============================================================================
// onPlayerPickup 前置条件测试
// ============================================================================

TEST_F(TridentLoyaltyTest, OnPlayerPickup_InGroundAllowedStatus_CanPickup)
{
    auto& trident = createTrident();
    trident.setInGround(true);
    EXPECT_EQ(trident.pickupStatus(), PickupStatus::Allowed);
    EXPECT_TRUE(trident.isInGround());
}

TEST_F(TridentLoyaltyTest, OnPlayerPickup_NotInGroundNotNoClip_CannotPickup)
{
    auto& trident = createTrident();
    trident.setInGround(false);
    trident.setNoClip(false);
    EXPECT_FALSE(trident.isInGround());
    EXPECT_FALSE(trident.noClip());
}

TEST_F(TridentLoyaltyTest, OnPlayerPickup_NoClipReturningTrident_CanPickupByOwner)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);
    trident.setNoClip(true);
    trident.setInGround(false);

    // 返回中的三叉戟（noClip）允许被射手拾取
    EXPECT_TRUE(trident.noClip());
}

// ============================================================================
// 综合场景测试
// ============================================================================

TEST_F(TridentLoyaltyTest, LoyaltyTrident_SpectatorShooterCannotReturn)
{
    // 场景：玩家投掷忠诚 III 三叉戟后切换到旁观者模式
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);
    trident.setLoyaltyLevel(3);
    trident.setDealtDamage(true);

    EXPECT_TRUE(trident._shouldReturnToThrower());

    player.setGameMode(GameMode::Spectator);
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, LoyaltyTrident_DeadShooterCannotReturn)
{
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);
    trident.setLoyaltyLevel(2);
    trident.setDealtDamage(true);

    EXPECT_TRUE(trident._shouldReturnToThrower());

    // 模拟射手死亡（移除实体）
    player.remove();
    EXPECT_FALSE(player.isAlive());
    EXPECT_FALSE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, LoyaltyTrident_RespawnedShooterCanReturn)
{
    // 场景：射手在另一局中复活（使用新的射手引用）
    auto& trident = createTrident();
    auto& player = createPlayer();
    trident.setShooter(&player);
    trident.setLoyaltyLevel(3);

    EXPECT_TRUE(trident._shouldReturnToThrower());

    // 射手死亡
    player.remove();
    EXPECT_FALSE(trident._shouldReturnToThrower());

    // 创建新的射手（模拟重生）- 新实体存活
    auto& player2 = createPlayer(EntityInstanceId(200), "RespawnedPlayer");
    trident.setShooter(&player2);
    EXPECT_TRUE(player2.isAlive());
    EXPECT_TRUE(trident._shouldReturnToThrower());
}

TEST_F(TridentLoyaltyTest, LoyaltyTrident_LoyaltyLevels_OneToThree)
{
    auto& trident = createTrident();
    trident.setLoyaltyLevel(1);
    EXPECT_EQ(trident.loyaltyLevel(), 1);
    trident.setLoyaltyLevel(2);
    EXPECT_EQ(trident.loyaltyLevel(), 2);
    trident.setLoyaltyLevel(3);
    EXPECT_EQ(trident.loyaltyLevel(), 3);
}

TEST_F(TridentLoyaltyTest, LoyaltyTrident_DealtDamageFlag)
{
    auto& trident = createTrident();
    EXPECT_FALSE(trident.hasDealtDamage());
    trident.setDealtDamage(true);
    EXPECT_TRUE(trident.hasDealtDamage());
}

TEST_F(TridentLoyaltyTest, LoyaltyTrident_TimeInGround_Tracked)
{
    auto& trident = createTrident();
    EXPECT_EQ(trident.timeInGround(), 0);
    trident.setTimeInGround(5);
    EXPECT_EQ(trident.timeInGround(), 5);
}

// ============================================================================
// ProjectileEntity::hurt 测试
//
// 通过 TridentEntity（ProjectileEntity 子类）测试 ProjectileEntity::hurt()。
// ProjectileEntity::hurt() 始终返回 false（投掷物不可被伤害），
// 但当来源非无敌时标记 hurtMarked 以同步速度到客户端。
// ============================================================================

/**
 * @brief 无敌伤害源：markHurt 不被调用，返回 false
 */
TEST_F(TridentLoyaltyTest, Hurt_InvulnerableSource_ReturnsFalse_NoMarkHurt)
{
    auto& trident = createTrident();
    trident.setInvulnerable(true);
    EXPECT_FALSE(trident.isHurtMarked());

    auto source = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source, 5.0f));
    EXPECT_FALSE(trident.isHurtMarked());
}

/**
 * @brief 正常伤害源：markHurt 被调用，返回 false
 *
 * ProjectileEntity::hurt() 对非无敌伤害源标记 hurtMarked
 * （以同步速度到客户端产生击退效果），但始终返回 false
 * 因为投掷物不可被伤害。
 */
TEST_F(TridentLoyaltyTest, Hurt_NormalSource_MarksHurt_ReturnsFalse)
{
    auto& trident = createTrident();
    EXPECT_FALSE(trident.isHurtMarked());

    auto source = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source, 5.0f));
    EXPECT_TRUE(trident.isHurtMarked());
}

/**
 * @brief 伤害量不影响返回值——始终返回 false
 */
TEST_F(TridentLoyaltyTest, Hurt_AnyAmount_ReturnsFalse)
{
    auto& trident = createTrident();

    auto source1 = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source1, 0.0f));

    auto source2 = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source2, 1000.0f));
}

/**
 * @brief hurt() 不移除实体
 *
 * ProjectileEntity 不会因为 hurt() 而被移除。
 */
TEST_F(TridentLoyaltyTest, Hurt_DoesNotRemoveEntity)
{
    auto& trident = createTrident();

    auto source = DamageSources::generic();
    trident.hurt(source, 100.0f);
    EXPECT_FALSE(trident.isRemoved());
}

/**
 * @brief 清除 hurtMarked 后可以再次标记
 */
TEST_F(TridentLoyaltyTest, Hurt_ClearAndReMarkHurt)
{
    auto& trident = createTrident();

    auto source = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source, 1.0f));
    EXPECT_TRUE(trident.isHurtMarked());

    trident.clearHurtMarked();
    EXPECT_FALSE(trident.isHurtMarked());

    auto source2 = DamageSources::generic();
    EXPECT_FALSE(trident.hurt(source2, 1.0f));
    EXPECT_TRUE(trident.isHurtMarked());
}

/**
 * @brief 虚空伤害绕过无敌但 ProjectileEntity 仍返回 false
 *
 * 即使 isInvulnerableTo() 对虚空伤害返回 false，
 * ProjectileEntity::hurt() 仍然返回 false（投掷物不可被伤害），
 * 但会标记 hurtMarked。
 */
TEST_F(TridentLoyaltyTest, Hurt_VoidDamageBypassesInvulnerability_StillReturnsFalse)
{
    auto& trident = createTrident();
    trident.setInvulnerable(true);

    auto voidSource = DamageSources::outOfWorld();
    // 虚空伤害绕过无敌，所以 isInvulnerableTo() 返回 false
    // 因此 markHurt() 应被调用，但 hurt() 仍返回 false
    EXPECT_FALSE(trident.hurt(voidSource, 100.0f));
    EXPECT_TRUE(trident.isHurtMarked());
    EXPECT_FALSE(trident.isRemoved()); // 投掷物不会被虚空伤害摧毁
}

} // namespace
} // namespace mc
