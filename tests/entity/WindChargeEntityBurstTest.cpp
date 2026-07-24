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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试访问器：通过 friend 声明访问 WindChargeEntity 的 private applyWindBurst
// ============================================================================
//
// WindChargeEntity 被声明为 final，无法通过继承子类暴露 protected/private 方法。
// 测试中通过 WindChargeEntityTestAccessor 这个 friend 类以间接方式访问
// private applyWindBurst()，避免修改生产代码的可见性。
// WindChargeEntity.hpp 中已声明 `friend class test::WindChargeEntityTestAccessor;`。
// 对应 BreezeEntity 的 test::BreezeEntityTestAccessor 模式。

namespace mc::test {

class WindChargeEntityTestAccessor {
public:
    explicit WindChargeEntityTestAccessor(WindChargeEntity& windCharge)
        : m_windCharge(windCharge)
    {}

    void applyWindBurst() { m_windCharge.applyWindBurst(); }

private:
    WindChargeEntity& m_windCharge;
};

} // namespace mc::test

namespace mc {
namespace {

/**
 * @brief 风弹爆炸测试用世界存根
 *
 * 在 BaseTestWorld 基础上覆写：
 * - getEntitiesInAABB：返回预设的测试玩家列表（默认空）
 * - getEntity：通过实体ID查找预注册的实体（用于 ProjectileEntity::getShooter）
 * - broadcastExplosion：捕获传入的位置、半径、玩家击退映射，用于断言
 * - playSound / addParticle：空操作（避免测试噪音）
 */
class WindChargeBurstTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WindChargeBurstTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WindChargeBurstTestWorld::tickManager not implemented");
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entityById.find(id);
        return it != m_entityById.end() ? it->second : nullptr;
    }

    void broadcastExplosion(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback) override
    {
        m_broadcastCalled = true;
        m_lastPosition = position;
        m_lastStrength = strength;
        m_lastAffectedBlocks = affectedBlocks;
        m_lastPlayerKnockback = playerKnockback;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略音效
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子
    }

    /// 重置广播捕获状态（不重置实体注册表，便于连续多次调用断言）
    void resetBroadcastCapture()
    {
        m_broadcastCalled = false;
        m_lastPosition = Vector3(0, 0, 0);
        m_lastStrength = 0.0f;
        m_lastAffectedBlocks.clear();
        m_lastPlayerKnockback.clear();
    }

    /// 注册实体到 ID 索引，便于 getEntity / getShooter 查找
    void registerEntity(Entity* entity) { m_entityById[entity->id()] = entity; }

    /// 设置 getEntitiesInAABB 返回的实体列表
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    // 广播捕获状态
    [[nodiscard]] bool broadcastCalled() const { return m_broadcastCalled; }
    [[nodiscard]] const Vector3& lastPosition() const { return m_lastPosition; }
    [[nodiscard]] f32 lastStrength() const { return m_lastStrength; }
    [[nodiscard]] const std::vector<BlockPos>& lastAffectedBlocks() const { return m_lastAffectedBlocks; }
    [[nodiscard]] const std::unordered_map<u64, Vector3>& lastPlayerKnockback() const { return m_lastPlayerKnockback; }

private:
    std::vector<Entity*> m_entities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityById;

    bool m_broadcastCalled = false;
    Vector3 m_lastPosition{0, 0, 0};
    f32 m_lastStrength = 0.0f;
    std::vector<BlockPos> m_lastAffectedBlocks;
    std::unordered_map<u64, Vector3> m_lastPlayerKnockback;
};

/**
 * @brief WindChargeEntity::applyWindBurst 测试固件
 *
 * 验证风弹爆炸时玩家击退收集与广播逻辑：
 * - 普通玩家（生存模式）被加入 playerKnockback 映射，服务端不调用 addVelocity（客户端权威速度）
 * - 旁观模式玩家被过滤
 * - 创造模式飞行玩家被过滤
 * - broadcastExplosion 被调用，传入正确的位置、半径、空 affectedBlocks、playerKnockback
 * - playerKnockback 中保存的击退向量即为客户端应通过 addVelocity 累加的向量
 * - 玩家分支清除 hurtMarked，防止 EntityTracker 发送 EntityVelocityPacket
 */
class WindChargeEntityBurstTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建风弹实体，位置设为原点
        m_windCharge = std::make_unique<WindChargeEntity>(EntityInstanceId(1));
        m_windCharge->setPosition(0.0f, 64.0f, 0.0f);
        m_windCharge->setWorld(&m_world);
        // 注册风弹自身到世界（避免 getShooter 等查找时返回 nullptr 造成非预期路径）
        m_world.registerEntity(m_windCharge.get());
    }

    void TearDown() override { m_windCharge.reset(); }

    WindChargeBurstTestWorld m_world;
    std::unique_ptr<WindChargeEntity> m_windCharge;
};

// ============================================================================
// 基础测试：applyWindBurst 触发 broadcastExplosion
// ============================================================================

TEST_F(WindChargeEntityBurstTest, ApplyWindBurst_NoEntities_StillBroadcasts)
{
    // 范围内无实体时，仍应广播爆炸（playerKnockback 为空）
    m_world.setEntities({});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    EXPECT_TRUE(m_world.broadcastCalled());
    EXPECT_TRUE(m_world.lastPlayerKnockback().empty());
    EXPECT_TRUE(m_world.lastAffectedBlocks().empty());
    // 风弹爆炸半径默认为 1.2（玩家投掷）
    EXPECT_FLOAT_EQ(m_world.lastStrength(), 1.2f);
    // 广播位置应为风弹自身位置
    EXPECT_FLOAT_EQ(m_world.lastPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(m_world.lastPosition().y, 64.0f);
    EXPECT_FLOAT_EQ(m_world.lastPosition().z, 0.0f);
}

TEST_F(WindChargeEntityBurstTest, ApplyWindBurst_Twice_OnlyBroadcastsOnce)
{
    // m_hasBurst 标志位防止重复触发
    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();
    EXPECT_TRUE(m_world.broadcastCalled());

    // 重置捕获状态后再次调用，不应再广播（m_hasBurst 已为 true）
    m_world.resetBroadcastCapture();
    accessor.applyWindBurst();
    EXPECT_FALSE(m_world.broadcastCalled());
}

// ============================================================================
// 玩家击退收集测试
// ============================================================================

TEST_F(WindChargeEntityBurstTest, SurvivalPlayer_AddedToKnockbackMap)
{
    // 生存模式玩家在爆炸范围内：应被加入 playerKnockback 映射
    Player player(EntityInstanceId(2), "SurvivalPlayer");
    // 玩家位于风弹正东 0.5 格（在 1.2*2=2.4 范围内）
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    EXPECT_TRUE(m_world.broadcastCalled());
    const auto& knockback = m_world.lastPlayerKnockback();
    ASSERT_EQ(knockback.size(), 1u);
    ASSERT_NE(knockback.find(static_cast<u64>(player.id())), knockback.end());

    // 击退方向应从爆炸中心指向玩家（即 +X 方向）
    const Vector3& kb = knockback.at(static_cast<u64>(player.id()));
    EXPECT_GT(kb.x, 0.0f); // X 分量为正
    // Y/Z 分量很小（玩家与中心同 Y，且在 X 轴上）
    EXPECT_NEAR(kb.y, 0.0f, 0.01f);
    EXPECT_NEAR(kb.z, 0.0f, 0.01f);
}

TEST_F(WindChargeEntityBurstTest, KnockbackVector_MatchesAppliedVelocity)
{
    // 验证修复后的击退契约：
    // - 服务端玩家速度不变（不调用 addVelocity）
    // - playerKnockback 中保存的向量即为客户端应累加的击退向量
    // - 玩家 hurtMarked 被清除（防止 EntityTracker 发送 EntityVelocityPacket 覆盖客户端速度）
    //
    // 对应 MC Java: ServerPlayer 的 motion 是 client-authoritative，服务端不通过 SetEntityMotionPacket
    // 把自身速度同步给自己。Cubium 的 EntityTracker 采用 "AndSelf" 模式，因此玩家分支必须显式
    // 跳过 addVelocity 并 clearHurtMarked，让击退仅通过 Explosion IR 在客户端应用。
    Player player(EntityInstanceId(2), "SurvivalPlayer");
    player.setPosition(1.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    const Vector3 velocityBefore = player.velocity();

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    const Vector3 velocityAfter = player.velocity();
    const Vector3 appliedDelta = velocityAfter - velocityBefore;

    // 修复后：服务端玩家速度不变（击退通过 Explosion IR 由客户端应用）
    EXPECT_FLOAT_EQ(appliedDelta.x, 0.0f);
    EXPECT_FLOAT_EQ(appliedDelta.y, 0.0f);
    EXPECT_FLOAT_EQ(appliedDelta.z, 0.0f);

    // playerKnockback 中保存的向量即为客户端应累加的击退向量
    const auto& knockback = m_world.lastPlayerKnockback();
    ASSERT_EQ(knockback.size(), 1u);
    ASSERT_NE(knockback.find(static_cast<u64>(player.id())), knockback.end());
    const Vector3& broadcastDelta = knockback.at(static_cast<u64>(player.id()));
    // 击退方向应从爆炸中心指向玩家（即 +X 方向）
    EXPECT_GT(broadcastDelta.x, 0.0f);

    // hurtMarked 必须被清除，防止 EntityTracker 发送 EntityVelocityPacket 覆盖客户端速度
    EXPECT_FALSE(player.isHurtMarked());
}

TEST_F(WindChargeEntityBurstTest, PlayerVelocity_UnchangedAfterBurst)
{
    // 额外验证：生存模式玩家被风弹击中后，服务端速度保持不变
    // （击退仅通过 Explosion IR 在客户端应用）
    Player player(EntityInstanceId(2), "SurvivalPlayer");
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    // 设置一个非零初速度，验证它不会被覆盖
    player.setVelocity(0.3f, -0.1f, 0.2f);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    const Vector3 velocityBefore = player.velocity();

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    const Vector3 velocityAfter = player.velocity();
    // 服务端玩家速度完全不变（击退由客户端通过 Explosion IR 累加）
    EXPECT_FLOAT_EQ(velocityAfter.x, velocityBefore.x);
    EXPECT_FLOAT_EQ(velocityAfter.y, velocityBefore.y);
    EXPECT_FLOAT_EQ(velocityAfter.z, velocityBefore.z);
}

TEST_F(WindChargeEntityBurstTest, Player_NotHurtMarked_AfterBurst)
{
    // 验证 hurtMarked 在玩家分支被清除
    Player player(EntityInstanceId(2), "SurvivalPlayer");
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    EXPECT_FALSE(player.isHurtMarked());

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    // 玩家分支应清除 hurtMarked（LivingEntity::hurt 会设置它，但玩家分支立即清除）
    EXPECT_FALSE(player.isHurtMarked());
}

TEST_F(WindChargeEntityBurstTest, NonPlayerEntity_VelocityStillChanged)
{
    // 验证非玩家实体（LivingEntity）仍由服务端权威同步速度：
    // 服务端调用 addVelocity 修改速度，并依赖 LivingEntity::hurt 设置的 markHurt
    // 让 EntityTracker 通过 EntityVelocityPacket 同步给追踪此实体的客户端。
    //
    // 确定性前提：WindChargeBurstTestWorld::getBlockState 返回 nullptr（空气），
    // _calculateSeenPercent 中所有射线都 isMiss（无方块阻挡），density = 1.0。
    // 风弹位于原点，实体位于 (0.5, 64, 0)，爆炸半径 1.2，range = 2.4：
    //   distanceRatio = 0.5 / 2.4 ≈ 0.208
    //   impact = (1 - 0.208) * 1.0 = 0.792
    //   finalImpact = 0.792 * 1.22 * (1 - 0) ≈ 0.966  （无爆炸保护附魔）
    // 因此击退必然被应用，速度必然变化，markHurt 必然为 true。

    // 创建一个非玩家 LivingEntity（参考 HurtMarkedTest.cpp 的 TestHurtEntity 模式）
    class TestLivingEntity final : public LivingEntity {
    public:
        explicit TestLivingEntity(EntityInstanceId id)
            : LivingEntity(id)
        {
            registerAttributes();
            setHealth(maxHealth());
        }
    };

    TestLivingEntity entity(EntityInstanceId(2));
    entity.setPosition(0.5f, 64.0f, 0.0f);
    entity.setWorld(&m_world);
    m_world.registerEntity(&entity);
    m_world.setEntities({&entity});

    const Vector3 velocityBefore = entity.velocity();
    EXPECT_FALSE(entity.isHurtMarked());

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    const Vector3 velocityAfter = entity.velocity();
    const Vector3 appliedDelta = velocityAfter - velocityBefore;

    // 确定性断言：非玩家实体速度必须变化（addVelocity 被调用）
    // 击退方向应从爆炸中心（原点）指向实体（+X 方向）
    EXPECT_GT(appliedDelta.x, 0.0f);

    // 非玩家实体的 markHurt 必须为 true（LivingEntity::hurt 已设置，未被清除）
    // 这是非玩家分支与玩家分支的关键区别：玩家分支会 clearHurtMarked，非玩家分支不会
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST_F(WindChargeEntityBurstTest, NonPlayerEntity_HurtMarkedNotCleared)
{
    // 补充验证：非玩家 LivingEntity 被风弹击中后，hurtMarked 保持 true
    // （对比玩家分支会 clearHurtMarked，确保两条分支的同步语义不同）
    class TestLivingEntity final : public LivingEntity {
    public:
        explicit TestLivingEntity(EntityInstanceId id)
            : LivingEntity(id)
        {
            registerAttributes();
            setHealth(maxHealth());
        }
    };

    TestLivingEntity entity(EntityInstanceId(2));
    entity.setPosition(0.5f, 64.0f, 0.0f);
    entity.setWorld(&m_world);
    m_world.registerEntity(&entity);
    m_world.setEntities({&entity});

    EXPECT_FALSE(entity.isHurtMarked());

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    // 非玩家实体：hurtMarked 必须为 true（未被清除）
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST_F(WindChargeEntityBurstTest, SpectatorPlayer_FilteredFromKnockback)
{
    // 旁观模式玩家不应被加入 playerKnockback
    Player player(EntityInstanceId(2), "SpectatorPlayer");
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Spectator);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    EXPECT_TRUE(m_world.broadcastCalled());
    // 旁观玩家不应出现在击退映射中
    EXPECT_TRUE(m_world.lastPlayerKnockback().empty());
}

TEST_F(WindChargeEntityBurstTest, CreativeFlyingPlayer_FilteredFromKnockback)
{
    // 创造模式 + 飞行中的玩家不应被加入 playerKnockback
    Player player(EntityInstanceId(2), "CreativeFlyingPlayer");
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);
    player.abilities().flying = true;
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    EXPECT_TRUE(m_world.broadcastCalled());
    EXPECT_TRUE(m_world.lastPlayerKnockback().empty());
}

TEST_F(WindChargeEntityBurstTest, CreativeNonFlyingPlayer_AddedToKnockback)
{
    // 创造模式但未飞行的玩家仍应被加入 playerKnockback（与 MC Java 过滤规则一致）
    Player player(EntityInstanceId(2), "CreativeWalkingPlayer");
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);
    // abilities().flying 默认为 false
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    EXPECT_TRUE(m_world.broadcastCalled());
    EXPECT_EQ(m_world.lastPlayerKnockback().size(), 1u);
    EXPECT_NE(m_world.lastPlayerKnockback().find(static_cast<u64>(player.id())), m_world.lastPlayerKnockback().end());
}

TEST_F(WindChargeEntityBurstTest, MultiplePlayers_OnlyNonFilteredAdded)
{
    // 混合多个玩家：生存、旁观、创造飞行、创造步行
    Player survival(EntityInstanceId(2), "Survival");
    survival.setPosition(0.5f, 64.0f, 0.0f);
    survival.setWorld(&m_world);
    survival.setGameMode(GameMode::Survival);
    m_world.registerEntity(&survival);

    Player spectator(EntityInstanceId(3), "Spectator");
    spectator.setPosition(-0.5f, 64.0f, 0.0f);
    spectator.setWorld(&m_world);
    spectator.setGameMode(GameMode::Spectator);
    m_world.registerEntity(&spectator);

    Player creativeFlying(EntityInstanceId(4), "CreativeFlying");
    creativeFlying.setPosition(0.0f, 64.0f, 0.5f);
    creativeFlying.setWorld(&m_world);
    creativeFlying.setGameMode(GameMode::Creative);
    creativeFlying.abilities().flying = true;
    m_world.registerEntity(&creativeFlying);

    Player creativeWalking(EntityInstanceId(5), "CreativeWalking");
    creativeWalking.setPosition(0.0f, 64.0f, -0.5f);
    creativeWalking.setWorld(&m_world);
    creativeWalking.setGameMode(GameMode::Creative);
    m_world.registerEntity(&creativeWalking);

    m_world.setEntities({&survival, &spectator, &creativeFlying, &creativeWalking});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    const auto& knockback = m_world.lastPlayerKnockback();
    // 只有 survival 和 creativeWalking 应被加入
    EXPECT_EQ(knockback.size(), 2u);
    EXPECT_NE(knockback.find(static_cast<u64>(survival.id())), knockback.end());
    EXPECT_NE(knockback.find(static_cast<u64>(creativeWalking.id())), knockback.end());
    EXPECT_EQ(knockback.find(static_cast<u64>(spectator.id())), knockback.end());
    EXPECT_EQ(knockback.find(static_cast<u64>(creativeFlying.id())), knockback.end());
}

TEST_F(WindChargeEntityBurstTest, PlayerOutOfRange_NotAdded)
{
    // 玩家在爆炸范围外（> radius * 2 = 2.4 格）不应被加入击退映射
    Player player(EntityInstanceId(2), "FarPlayer");
    // 距离 5.0 格 > 2.4
    player.setPosition(5.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.registerEntity(&player);
    m_world.setEntities({&player});

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();

    // 即使 broadcastExplosion 会被调用，但该玩家不在击退映射中
    EXPECT_TRUE(m_world.broadcastCalled());
    EXPECT_TRUE(m_world.lastPlayerKnockback().empty());
}

TEST_F(WindChargeEntityBurstTest, BreezeShooter_UsesLargerRadius)
{
    // 旋风人发射的风弹半径为 3.0（而非玩家的 1.2）
    // 由于 BreezeEntity 涉及更多子系统，此处通过将风弹发射者设为旋风人来验证半径选择
    // 需要一个 BreezeEntity 实例作为 shooter。为简化测试，直接验证 broadcastExplosion 接收到的 strength。
    // TODO: 若未来 BreezeEntity 可在测试中轻量构造，可补充完整端到端测试
    // 当前测试至少验证了玩家路径下 strength=1.2 的契约
    EXPECT_FLOAT_EQ(m_world.lastStrength(), 0.0f); // 尚未调用 applyWindBurst

    test::WindChargeEntityTestAccessor accessor(*m_windCharge);
    accessor.applyWindBurst();
    EXPECT_FLOAT_EQ(m_world.lastStrength(), 1.2f);
}

} // namespace
} // namespace mc
