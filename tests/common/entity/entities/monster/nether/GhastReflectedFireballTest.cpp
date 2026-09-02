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

// 恶魂反弹火球秒杀机制单元测试。
//
// 验证 Cubium GhastEntity 对齐 MC Java 1.21.11 Ghast 的三项核心 override：
//   1. isReflectedFireball(DamageSource)：判定伤害源是否为"被玩家反弹的大型火球"
//      （directSource instanceof FireballEntity && getEntity() instanceof Player）。
//   2. isInvulnerableTo(DamageSource)：反弹火球绕过常规免疫判定（第二支 !isReflectedFireball
//      为 false 使整支 false，即反弹火球不走 super.isInvulnerableTo 的火焰免疫/无敌帧等判定）。
//   3. hurt(DamageSource, amount)：反弹火球施加 1000 伤害秒杀（满血 10 必死）。
//
// vanilla 参考（Ghast.java:81-113）：
//   private static boolean isReflectedFireball(DamageSource p_238408_) {
//       return p_238408_.getDirectEntity() instanceof LargeFireball && p_238408_.getEntity() instanceof Player;
//   }
//   public boolean isInvulnerableTo(ServerLevel p_376822_, DamageSource p_238289_) {
//       return this.isInvulnerable() && !p_238289_.is(DamageTypeTags.BYPASSES_INVULNERABILITY)
//           || !isReflectedFireball(p_238289_) && super.isInvulnerableTo(p_376822_, p_238289_);
//   }
//   public boolean hurtServer(ServerLevel p_376618_, DamageSource p_376819_, float p_376363_) {
//       if (isReflectedFireball(p_376819_)) {
//           super.hurtServer(p_376618_, p_376819_, 1000.0F);
//           return true;
//       } else {
//           return this.isInvulnerableTo(p_376618_, p_376819_) ? false : super.hurtServer(p_376618_, p_376819_,
//           p_376363_);
//       }
//   }
//
// 伤害源构造：DamageSources::fireball(fireball, shooter) →
//   IndirectEntityDamageSource(Fireball, source=shooter, directSource=fireball)
//   - directSource() = fireball（FireballEntity 实例）
//   - getEntity() = shooter（Player 实例，entityType()==PLAYER）
// 火球被玩家反弹时 setShooter 更新为玩家（ProjectileDeflection.cpp:53/75/90），故反弹火球的
// getEntity() 为 Player，满足 isReflectedFireball 条件。
//
// 注：恶魂默认 isInvulnerable()=false（无 /data 设 invulnerable 标志），故 isInvulnerableTo
// 第一支恒 false，反弹火球的免疫判定完全由第二支决定（!isReflectedFireball && super）。
// 本测试不设 invulnerable 标志，聚焦反弹火球秒杀核心机制。
//
// Ref: Ghast.java:81-113（isReflectedFireball / isInvulnerableTo / hurtServer）
// Ref: NetherEntities.cpp（GhastEntity 三项 override）
// Ref: AbstractFireballEntity.cpp:127（FireballEntity::onEntityHit 构造 fireball 伤害源）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

// 恶魂反弹火球测试世界：最小 IWorld 实现，记录 spawn 的实体（死亡掉落物等）。
class GhastFireballTestWorld final : public mc::test::BaseTestWorld {
public:
    GhastFireballTestWorld() { VanillaBlocks::initialize(); }

    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return &VanillaBlocks::AIR->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawned.size()));
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
};

} // namespace

// ============================================================================
// hurt 反弹秒杀测试
// ============================================================================

// 玩家反弹的大型火球（FireballEntity）命中恶魂 → 1000 伤害秒杀（满血 10 必死）。
//
// 对齐 vanilla Ghast.hurtServer 反弹分支：isReflectedFireball(source) 为 true 时调
// super.hurtServer(source, 1000.0F)。Cubium GhastEntity::hurt 反弹分支调 LivingEntity::hurt
// (source, 1000.0f)，恶魂满血 10，1000 必死。
//
// 伤害源：DamageSources::fireball(&fireball, player) → directSource=fireball(FireballEntity),
//   getEntity=player(entityType==PLAYER)，满足 isReflectedFireball。
//
// 判定：hurt 返回 true + 恶魂 isDead()（health<=0）。
TEST(GhastReflectedFireballTest, ReflectedFireballOneShotsGhast)
{
    GhastFireballTestWorld world;
    auto ghast = std::make_unique<GhastEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ghast->setWorld(&world);
    ghast->setHealth(10.0f);
    ASSERT_FLOAT_EQ(ghast->health(), 10.0f);

    // 玩家（反弹者）：Player 构造自动 setTypeId(PLAYER)，entityType()==PLAYER。
    auto player = std::make_unique<Player>(EntityInstanceId(2), "Reflector", mc::test::testEcsRegistry());
    player->setWorld(&world);

    // 大型火球（被玩家反弹，shooter 已更新为玩家）。
    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());

    // 反弹火球伤害源：directSource=fireball, source(shooter)=player。
    auto src = DamageSources::fireball(&fireball, player.get());

    const bool hurtResult = ghast->hurt(src, 6.0f); // 原始伤害量 6 被忽略，反弹分支强制 1000
    EXPECT_TRUE(hurtResult);
    // 恶魂被秒杀：health<=0，isDead()=true。
    EXPECT_LE(ghast->health(), 0.0f);
    EXPECT_TRUE(ghast->isDead());
}

// 非玩家发射的大型火球命中恶魂 → 不触发反弹秒杀，走标准链路按火球伤害量扣血。
//
// 对齐 vanilla Ghast.hurtServer else 分支：isReflectedFireball 为 false（getEntity 非 Player）
// 时走 super.hurtServer(source, amount) 标准扣血。
//
// 伤害源：DamageSources::fireball(&fireball, ghastShooter)，shooter 为另一只恶魂（非 Player），
//   isReflectedFireball 返回 false（getEntity().entityType != PLAYER）。
//
// 判定：恶魂受 6 伤害（10→4），未死。验证反弹判定正确排除非玩家发射者。
TEST(GhastReflectedFireballTest, NonPlayerFireballDoesNormalDamage)
{
    GhastFireballTestWorld world;

    auto ghast = std::make_unique<GhastEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ghast->setWorld(&world);
    ghast->setHealth(10.0f);

    // 发射者：另一只恶魂（非 Player）。
    auto shooter = std::make_unique<GhastEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    shooter->setWorld(&world);

    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());

    // 非反弹火球：directSource=fireball, source=ghast（非 Player）。
    auto src = DamageSources::fireball(&fireball, shooter.get());

    const bool hurtResult = ghast->hurt(src, 6.0f);
    EXPECT_TRUE(hurtResult);
    // 标准扣血 6：10→4，未死。
    EXPECT_FLOAT_EQ(ghast->health(), 4.0f);
    EXPECT_FALSE(ghast->isDead());
}

// 玩家反弹的小火球（SmallFireballEntity）命中恶魂 → 不触发反弹秒杀。
//
// 对齐 vanilla isReflectedFireball：仅 LargeFireball 算反弹判定（instanceof LargeFireball），
// 小火球（SmallFireball）不算。Cubium 用 dynamic_cast<FireballEntity*> 判定，SmallFireballEntity
// 非 FireballEntity 派生（两者都继承 AbstractFireballEntity 但平行），dynamic_cast 返回 nullptr。
//
// 伤害源：DamageSources::fireball(&smallFireball, player)，directSource=SmallFireballEntity
//   （非 FireballEntity），isReflectedFireball 返回 false。
//
// 判定：恶魂走标准链路扣血，未秒杀。验证 dynamic_cast 正确排除小型火球。
TEST(GhastReflectedFireballTest, ReflectedSmallFireballDoesNotOneShot)
{
    GhastFireballTestWorld world;

    auto ghast = std::make_unique<GhastEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ghast->setWorld(&world);
    ghast->setHealth(10.0f);

    auto player = std::make_unique<Player>(EntityInstanceId(2), "Reflector", mc::test::testEcsRegistry());
    player->setWorld(&world);

    // 小火球（SmallFireballEntity，非 FireballEntity）。
    entity::SmallFireballEntity smallFireball(EntityInstanceId(3), mc::test::testEcsRegistry());

    auto src = DamageSources::fireball(&smallFireball, player.get());

    const bool hurtResult = ghast->hurt(src, 5.0f); // SmallFireball 默认伤害 5
    EXPECT_TRUE(hurtResult);
    // 小火球非反弹判定 → 标准扣血 5：10→5，未秒杀。
    EXPECT_FLOAT_EQ(ghast->health(), 5.0f);
    EXPECT_FALSE(ghast->isDead());
}

// ============================================================================
// isInvulnerableTo 反弹火球绕过免疫判定测试
// ============================================================================

// 反弹火球伤害源对恶魂 isInvulnerableTo 返回 false（绕过常规免疫判定）。
//
// 对齐 vanilla Ghast.isInvulnerableTo 第二支：!isReflectedFireball 为 false 时整支 false，
// 即反弹火球不走 super.isInvulnerableTo 的火焰免疫/无敌帧等判定，返回 false（不免疫）。
// 恶魂默认 isInvulnerable()=false，第一支恒 false，故整体由第二支决定。
//
// 判定：反弹火球伤害源 → isInvulnerableTo 返回 false（不免疫，允许 1000 伤害穿透）。
TEST(GhastReflectedFireballTest, ReflectedFireballBypassesImmunityCheck)
{
    GhastFireballTestWorld world;
    auto ghast = std::make_unique<GhastEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ghast->setWorld(&world);

    auto player = std::make_unique<Player>(EntityInstanceId(2), "Reflector", mc::test::testEcsRegistry());
    player->setWorld(&world);

    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());
    auto reflectedSrc = DamageSources::fireball(&fireball, player.get());

    // 反弹火球：isInvulnerableTo 返回 false（绕过常规免疫判定，允许 1000 伤害穿透）。
    EXPECT_FALSE(ghast->isInvulnerableTo(reflectedSrc));

    // 对照：非反弹火球（shooter=恶魂）走基类 MonsterEntity::isInvulnerableTo 正常判定。
    // 恶魂未 override isImmuneToFire（默认 false），不免疫火球伤害 → 基类返回 false。
    // 此对照验证反弹火球的 false 来自 isReflectedFireball 短路，与非反弹火球的基类判定区分。
    auto shooter = std::make_unique<GhastEntity>(EntityInstanceId(4), mc::test::testEcsRegistry());
    shooter->setWorld(&world);
    auto nonReflectedSrc = DamageSources::fireball(&fireball, shooter.get());
    EXPECT_FALSE(ghast->isInvulnerableTo(nonReflectedSrc));
}

} // namespace mc
