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

// Player::hurt 难度伤害缩放单元测试。
//
// 对齐 MC Java 1.21.11 Player.hurtServer（Player.java:692-720）的难度缩放逻辑：
//   - Peaceful：amount = 0.0（玩家在和平模式免疫所有受缩放伤害）
//   - Easy：amount = min(amount/2 + 1, amount)（减半但保底 +1，即小伤害不缩减）
//   - Hard：amount = amount * 1.5（伤害增强 50%）
//   - Normal：不调整
//
// 缩放触发条件：source.scalesWithDifficulty()（对齐 vanilla DamageSource.scalesWithDifficulty，
// DamageSource.java:90-96）。依据伤害类型数据包的 scaling 字段动态判定：
//   - ALWAYS：explosion/player_explosion/sonic_boom/bad_respawn_point → 无条件缩放
//   - WHEN_CAUSED_BY_LIVING_NON_PLAYER：其余全部 → causingEntity 是非玩家 LivingEntity 时才缩放
//
// 此前缺陷：Cubium Player::hurt 完全缺失此段难度缩放逻辑。isDifficultyScaled() flag 在 8 个工厂
// 设置但 hurt 链路从未读取——死代码，难度缩放从未生效（Easy 下玩家不减伤、Hard 下怪物伤害不增强）。
// 修复：新增 DamageSource::scalesWithDifficulty()（数据驱动动态判定），Player::hurt 接入缩放。
//
// Ref: D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\world\entity\player\Player.java:692-720
// Ref: D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\world\damagesource\DamageSource.java:90-96
// Ref: src/common/entity/entities/player/Player.cpp（Player::hurt 难度缩放接入）
// Ref: src/common/entity/damage/DamageSource.cpp（scalesWithDifficulty 实现）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 可配置难度的测试用世界。BaseTestWorld::difficulty() 默认返回 Easy，本子类按测试配置难度，
// 以覆盖难度缩放的四种分支（Peaceful/Easy/Normal/Hard）。吸收 hurt 链路的 playSound。
class ScalingTestWorld final : public mc::test::BaseTestWorld {
public:
    explicit ScalingTestWorld(Difficulty difficulty)
        : m_difficulty(difficulty)
    {}

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void addParticle(particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

private:
    Difficulty m_difficulty;
};

// 测试用 LivingEntity 子类：作 mobAttack 的攻击者（非玩家生物）。
// registerAttributes() 注册 ATTACK_DAMAGE 等属性；满血避免死亡分支干扰。
class TestMobEntity final : public LivingEntity {
public:
    TestMobEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

class PlayerDifficultyScalingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。未初始化时标签成员集为空，source.is(标签) 恒返 false。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    // 在指定难度世界内创建一个满血 Player（maxHealth=20）。
    std::unique_ptr<Player> makePlayer(ScalingTestWorld& world)
    {
        auto player = std::make_unique<Player>(EntityInstanceId(2), "Victim", mc::test::testEcsRegistry());
        player->setWorld(&world);
        player->setHealth(player->maxHealth());
        return player;
    }
};

// ============================================================================
// Hard 难度：伤害 ×1.5
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, HardDifficulty_MobAttack_DamageScaledBy1_5)
{
    // 对齐 vanilla Player.hurtServer:712-714：Hard 难度 amount *= 1.5。
    // 此前 Cubium 此段缺失——Hard 下怪物伤害不增强 1.5 倍。
    ScalingTestWorld world(Difficulty::Hard);
    auto player = makePlayer(world);
    TestMobEntity mob;
    mob.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::mobAttack(&mob);
    bool result = player->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    // 10 × 1.5 = 15 伤害，空手无护甲玩家 maxHp 20 → 剩余 5。
    EXPECT_FLOAT_EQ(player->health(), maxHp - 15.0f);
}

// ============================================================================
// Easy 难度：amount = min(amount/2 + 1, amount)
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, EasyDifficulty_MobAttack_DamageHalvedWithFloor)
{
    // 对齐 vanilla Player.hurtServer:708-710：Easy 难度 amount = min(amount/2 + 1, amount)。
    // 10 伤害 → min(6, 10) = 6。此前 Cubium 此段缺失——Easy 下玩家不减伤。
    ScalingTestWorld world(Difficulty::Easy);
    auto player = makePlayer(world);
    TestMobEntity mob;
    mob.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::mobAttack(&mob);
    bool result = player->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    // min(10/2 + 1, 10) = min(6, 10) = 6 伤害。
    EXPECT_FLOAT_EQ(player->health(), maxHp - 6.0f);
}

TEST_F(PlayerDifficultyScalingTest, EasyDifficulty_LargeDamageNotExceedsOriginal)
{
    // Easy 公式 min(amount/2 + 1, amount)：大伤害（如 100）→ min(51, 100) = 51。
    // 验证缩放后伤害不超过原值（min 上界保护）。
    ScalingTestWorld world(Difficulty::Easy);
    auto player = makePlayer(world);
    TestMobEntity mob;
    mob.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::mobAttack(&mob);
    player->hurt(source, 100.0f);

    // min(100/2 + 1, 100) = min(51, 100) = 51 伤害。
    EXPECT_FLOAT_EQ(player->health(), maxHp - 51.0f);
}

// ============================================================================
// Normal 难度：不缩放
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, NormalDifficulty_MobAttack_NoScaling)
{
    // Normal 难度不对伤害做任何调整（对齐 vanilla Player.hurtServer:703-714 无 Normal 分支）。
    ScalingTestWorld world(Difficulty::Normal);
    auto player = makePlayer(world);
    TestMobEntity mob;
    mob.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::mobAttack(&mob);
    bool result = player->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    // Normal 不缩放，10 伤害直接扣除。
    EXPECT_FLOAT_EQ(player->health(), maxHp - 10.0f);
}

// ============================================================================
// Peaceful 难度：受缩放伤害归零，hurt 返回 false
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, PeacefulDifficulty_MobAttack_DamageZeroed)
{
    // 对齐 vanilla Player.hurtServer:704-706：Peaceful 难度 amount = 0.0，hurt 返回 false。
    // 玩家在和平模式免疫所有受缩放伤害（mobAttack 等）。
    ScalingTestWorld world(Difficulty::Peaceful);
    auto player = makePlayer(world);
    TestMobEntity mob;
    mob.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::mobAttack(&mob);
    bool result = player->hurt(source, 10.0f);

    EXPECT_FALSE(result) << "Peaceful 缩放归零后 hurt 应返回 false";
    EXPECT_FLOAT_EQ(player->health(), maxHp) << "Peaceful 下受缩放伤害不应扣血";
}

// ============================================================================
// ALWAYS 缩放类型（sonic_boom）：Hard 下无条件缩放 ×1.5
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, HardDifficulty_SonicBoom_AlwaysScaled)
{
    // sonic_boom 数据包 scaling=ALWAYS，无条件缩放（不受 causingEntity 影响）。
    // Hard 下 10 伤害 → 15 伤害。
    ScalingTestWorld world(Difficulty::Hard);
    auto player = makePlayer(world);
    TestMobEntity guardian;
    guardian.setWorld(&world);

    const f32 maxHp = player->maxHealth();
    // sonicBoom(guardian, target) 是 IndirectEntityDamageSource，scaling=ALWAYS：
    // scalesWithDifficulty() 走 Always 分支直接返 true，无视 causingEntity 是否为 Player。
    auto source = DamageSources::sonicBoom(&guardian, player.get());
    bool result = player->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(player->health(), maxHp - 15.0f);
}

// ============================================================================
// 玩家造成的伤害（playerAttack / 玩家射的 mobProjectile）：不缩放
// 修正旧 flag 机制偏差——旧 flag 对 mobProjectile 工厂 setDifficultyScaled，
// 但 vanilla WHEN_CAUSED_BY_LIVING_NON_PLAYER 在 causingEntity 是 Player 时返 false。
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, PlayerAttack_NotScaledInHard)
{
    // 玩家攻击玩家（playerAttack）的 causingEntity 是 Player，不应受难度缩放。
    // Hard 下 10 伤害 playerAttack → 仍 10 伤害（不 ×1.5）。
    ScalingTestWorld world(Difficulty::Hard);
    auto victim = makePlayer(world);
    Player attacker(EntityInstanceId(3), "Attacker", mc::test::testEcsRegistry());
    attacker.setWorld(&world);
    attacker.setHealth(attacker.maxHealth());

    const f32 maxHp = victim->maxHealth();
    auto source = DamageSources::playerAttack(&attacker);
    bool result = victim->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    // PlayerAttack 不缩放，10 伤害直接扣除。
    EXPECT_FLOAT_EQ(victim->health(), maxHp - 10.0f);
}

// ============================================================================
// 纯环境伤害（无 causingEntity，如 Fall）：Hard 下不缩放
// WHEN_CAUSED_BY_LIVING_NON_PLAYER 在 causingEntity==nullptr 时返 false。
// ============================================================================

TEST_F(PlayerDifficultyScalingTest, FallDamage_NotScaledInHard)
{
    // 摔落伤害（Fall）无 causingEntity，WHEN_CAUSED_BY_LIVING_NON_PLAYER 返 false → 不缩放。
    // Hard 下 10 伤害 Fall → 仍 10 伤害（不 ×1.5）。
    ScalingTestWorld world(Difficulty::Hard);
    auto player = makePlayer(world);

    const f32 maxHp = player->maxHealth();
    auto source = DamageSources::fall();
    bool result = player->hurt(source, 10.0f);

    EXPECT_TRUE(result);
    // Fall 不缩放，10 伤害直接扣除。
    EXPECT_FLOAT_EQ(player->health(), maxHp - 10.0f);
}
