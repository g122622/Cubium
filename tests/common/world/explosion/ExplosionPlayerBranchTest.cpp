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
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::world::explosion;

namespace mc {
namespace {

/**
 * @brief 爆炸玩家分支集成测试用世界存根
 *
 * 在 BaseTestWorld 基础上覆写：
 * - getEntitiesInAABB：返回预设的实体列表（忽略 AABB 与 source 过滤）
 * - playSound / addParticle：空操作（避免测试噪音）
 *
 * 注意：Explosion::explode() 不会调用 IWorld::broadcastExplosion（只有
 * ServerWorld::createExplosion* 才会），因此本测试世界不需要捕获 broadcastExplosion。
 * 测试通过直接读取 Explosion::playerKnockback() 验证击退向量。
 */
class ExplosionTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ExplosionTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ExplosionTestWorld::tickManager not implemented");
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
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

    /// 设置 getEntitiesInAABB 返回的实体列表
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

private:
    std::vector<Entity*> m_entities;
};

/**
 * @brief Explosion 玩家分支集成测试固件
 *
 * 验证 Explosion 类在玩家分支（_calculateAffectedEntities 中 Player* 命中的路径）的修复：
 * - 玩家速度不变（不调用 addVelocity）
 * - 玩家 hurtMarked 被清除（LivingEntity::hurt 设置后立即 clearHurtMarked）
 * - playerKnockback 映射保存击退向量
 * - 非玩家 LivingEntity 分支仍由服务端权威同步速度（addVelocity + 保留 hurtMarked）
 *
 * 对应 src/common/world/explosion/README.md 第 #10 节「玩家击退的双重应用防范」。
 */
class ExplosionPlayerBranchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 爆炸中心位于原点
        m_explosionPos = Vector3(0.0f, 64.0f, 0.0f);
    }

    ExplosionTestWorld m_world;
    Vector3 m_explosionPos{0.0f, 64.0f, 0.0f};
};

// ============================================================================
// 玩家分支测试
// ============================================================================

TEST_F(ExplosionPlayerBranchTest, PlayerVelocity_UnchangedAfterExplosion)
{
    // 验证修复后：爆炸中玩家服务端速度不变（击退通过 Explosion IR 由客户端应用）
    Player player(EntityInstanceId(2), "SurvivalPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    // 设置非零初速度，验证它不会被覆盖
    player.setVelocity(0.3f, -0.1f, 0.2f);
    m_world.setEntities({&player});

    const Vector3 velocityBefore = player.velocity();

    // 半径 4.0，ExplosionMode::None 不破坏方块，专注实体击退逻辑
    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    const Vector3 velocityAfter = player.velocity();
    // 服务端玩家速度完全不变（击退由客户端通过 Explosion IR 累加）
    EXPECT_FLOAT_EQ(velocityAfter.x, velocityBefore.x);
    EXPECT_FLOAT_EQ(velocityAfter.y, velocityBefore.y);
    EXPECT_FLOAT_EQ(velocityAfter.z, velocityBefore.z);
}

TEST_F(ExplosionPlayerBranchTest, Player_HurtMarkedClearedAfterExplosion)
{
    // 验证玩家分支清除 hurtMarked（防止 EntityTracker 发送 EntityVelocityPacket）
    Player player(EntityInstanceId(2), "SurvivalPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.setEntities({&player});

    EXPECT_FALSE(player.isHurtMarked());

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    // 玩家分支应清除 hurtMarked（LivingEntity::hurt 会设置它，但玩家分支立即清除）
    EXPECT_FALSE(player.isHurtMarked());
}

TEST_F(ExplosionPlayerBranchTest, Player_AddedToKnockbackMap)
{
    // 验证 playerKnockback 映射保存击退向量
    Player player(EntityInstanceId(2), "SurvivalPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.setEntities({&player});

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    const auto& knockback = explosion.playerKnockback();
    ASSERT_EQ(knockback.size(), 1u);
    ASSERT_NE(knockback.find(static_cast<u64>(player.id())), knockback.end());

    // 击退方向应从爆炸中心指向玩家（即 +X 方向）
    const Vector3& kb = knockback.at(static_cast<u64>(player.id()));
    EXPECT_GT(kb.x, 0.0f);
}

TEST_F(ExplosionPlayerBranchTest, SpectatorPlayer_NotInKnockbackMap)
{
    // 旁观模式玩家不受击退也不受伤害
    Player player(EntityInstanceId(2), "SpectatorPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Spectator);
    m_world.setEntities({&player});

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    // 旁观玩家不应出现在击退映射中
    EXPECT_TRUE(explosion.playerKnockback().empty());
}

TEST_F(ExplosionPlayerBranchTest, CreativeFlyingPlayer_NotInKnockbackMap)
{
    // 创造模式飞行中玩家不受击退
    Player player(EntityInstanceId(2), "CreativeFlyingPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);
    player.abilities().flying = true;
    m_world.setEntities({&player});

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    // 创造飞行玩家不应出现在击退映射中
    EXPECT_TRUE(explosion.playerKnockback().empty());
    // hurtMarked 仍应被清除（hurt 调用已设置它，玩家分支会清除）
    EXPECT_FALSE(player.isHurtMarked());
}

// ============================================================================
// 非玩家实体分支测试
// ============================================================================

class TestLivingEntity final : public LivingEntity {
public:
    explicit TestLivingEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : LivingEntity(id, nullptr, registry)
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

TEST_F(ExplosionPlayerBranchTest, NonPlayerLivingEntity_VelocityChangedAndHurtMarkedPreserved)
{
    // 验证非玩家 LivingEntity 仍由服务端权威同步速度：
    // - addVelocity 被调用（速度变化）
    // - hurtMarked 保持 true（LivingEntity::hurt 设置，非玩家分支不清除）
    //
    // 确定性前提：ExplosionTestWorld::getBlockState 返回 nullptr（空气），
    // _getBlockDensity 中所有射线 isMiss，density = 1.0。
    // 爆炸半径 4.0，range = 8.0，实体位于 (0.5, 64, 0)：
    //   distanceRatio = 0.5 / 8.0 = 0.0625
    //   impact = (1 - 0.0625) * 1.0 = 0.9375
    //   knockback = 0.9375（无爆炸保护附魔）
    // 因此击退必然被应用。
    TestLivingEntity entity(EntityInstanceId(2), mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 64.0f, 0.0f);
    entity.setWorld(&m_world);
    m_world.setEntities({&entity});

    const Vector3 velocityBefore = entity.velocity();
    EXPECT_FALSE(entity.isHurtMarked());

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    const Vector3 velocityAfter = entity.velocity();
    const Vector3 appliedDelta = velocityAfter - velocityBefore;

    // 速度必须变化（addVelocity 被调用），方向 +X
    EXPECT_GT(appliedDelta.x, 0.0f);

    // hurtMarked 必须为 true（LivingEntity::hurt 设置，非玩家分支不清除）
    // 这与玩家分支形成对比：玩家分支会 clearHurtMarked，非玩家分支不会
    EXPECT_TRUE(entity.isHurtMarked());

    // 非玩家实体不应出现在 playerKnockback 映射中
    EXPECT_TRUE(explosion.playerKnockback().empty());
}

// ============================================================================
// EPF 一致性测试
// ============================================================================

TEST_F(ExplosionPlayerBranchTest, PlayerKnockback_UsesEPFReducedVector)
{
    // 验证 playerKnockback 中的向量使用了 EPF 衰减后的值。
    // 对应 MC Java ServerExplosion.hurtEntities 第 197 行：vec32 = vec31.scale(d2) 单次计算后
    // push 与 hitPlayers 共用同一向量。
    //
    // 由于本测试世界不初始化 Items / EnchantmentRegistry（避免重型依赖），
    // 玩家未装备爆炸保护附魔，EPF = 0，knockback = impact（无衰减）。
    // 这里验证 playerKnockback 向量与非玩家分支 addVelocity 应用的向量一致（两者都使用 impact）。
    //
    // 完整的 EPF 衰减数值测试由 CombatRulesEPFTest 与 ExplosionIntegrationTest 中的
    // 公式测试覆盖；本测试聚焦于「两条分支使用同一计算」的契约。

    Player player(EntityInstanceId(2), "SurvivalPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.5f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    m_world.setEntities({&player});

    TestLivingEntity nonPlayer(EntityInstanceId(3), mc::test::testEcsRegistry());
    nonPlayer.setPosition(0.5f, 64.0f, 0.0f);
    nonPlayer.setWorld(&m_world);
    m_world.setEntities({&player, &nonPlayer});

    const Vector3 nonPlayerVelocityBefore = nonPlayer.velocity();

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None);
    explosion.explode();

    // 玩家击退向量（将通过 Explosion IR 发送）
    const auto& knockback = explosion.playerKnockback();
    ASSERT_EQ(knockback.size(), 1u);
    const Vector3& playerKnockbackVec = knockback.at(static_cast<u64>(player.id()));

    // 非玩家实体实际应用的速度增量（addVelocity 应用）
    const Vector3 nonPlayerAppliedDelta = nonPlayer.velocity() - nonPlayerVelocityBefore;

    // 两者位置相同，impact 相同，EPF 均为 0，因此向量应一致
    // （验证两条分支使用同一个 knockback 计算）
    EXPECT_FLOAT_EQ(playerKnockbackVec.x, nonPlayerAppliedDelta.x);
    EXPECT_FLOAT_EQ(playerKnockbackVec.y, nonPlayerAppliedDelta.y);
    EXPECT_FLOAT_EQ(playerKnockbackVec.z, nonPlayerAppliedDelta.z);
}

} // namespace
} // namespace mc
