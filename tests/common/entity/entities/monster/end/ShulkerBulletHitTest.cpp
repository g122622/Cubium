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

// 潜影贝被潜影弹命中 hitByShulkerBullet 瞬移+繁殖测试。
//
// 验证 ShulkerEntity::hurt（ShulkerEntity.cpp）对齐 vanilla 1.21.11 Shulker.hurtServer:423-430 +
// hitByShulkerBullet:440-455：
//   if (super.hurtServer(...)) {
//       if (health < max*0.5 && random(4)==0) teleportSomewhere();
//       else if (source.is(IS_PROJECTILE) && getDirectEntity().getType()==SHULKER_BULLET)
//           hitByShulkerBullet();
//   }
//   hitByShulkerBullet: 若 !isClosed() && teleportSomewhere() 成功，则统计原碰撞箱外扩8格内
//       存活潜影贝数 i，f=(i-1)/5，random.nextFloat()>=f 时在原位置繁殖一只同色潜影贝。
//
// 此前缺陷：Cubium ShulkerEntity::hurt 缺 else if 潜影弹命中分支，被同类潜影弹命中时既不瞬移
// 也不繁殖——偏离 vanilla（vanilla 潜影贝被潜影弹命中会瞬移并概率繁殖，这是末地城市潜影贝
// 增殖的机制）。补 _hitByShulkerBullet + hurt 内 else if 分支。另修 _tryTeleportToNewPosition
// 缺 setPosition（vanilla teleportSomewhere:391 setPos）致瞬移后实体位置不变的预存缺陷。
//
// 测试设计（单元测试，确定性守卫 + 确定性繁殖）：
//   - ClosedShellDoesNotBreedOnBulletHit：闭壳潜影贝被潜影弹命中 → isShellClosed 守卫，不繁殖。
//   - NonShulkerBulletDoesNotBreed：开壳潜影贝被雪球（非潜影弹）命中 → dynamic_cast 守卫，不繁殖。
//   - TeleportActuallyMovesEntity：开壳潜影贝瞬移成功后 position 改变（验证 setPosition 生效，
//     对齐 vanilla teleportSomewhere:391 setPos）。
//   - BulletHitBreedsSameColorShulker：开壳潜影贝被潜影弹命中，瞬移成功且周围无其他潜影贝
//     （i=0→f=-0.2→必繁殖）→ 在原位繁殖一只同色新潜影贝。
//
// 确定性瞬移：ShulkerHitTestWorld.getBlockState 让 y==0 返回石头（固体，blocksMovement=true）、
//   其余返回空气。潜影贝 home=(0.5,1,0.5)（blockPos=(0,1,0)）。_tryTeleportToNewPosition 随机
//   ±8 找目标，当目标 y==1 时 targetPos 空气、Down 面 y==0 石头可附着 → 瞬移成功。瞬移是否命中
//   y==1 取决于种子的 nextInt(17) 序列，故遍历种子 0..1024 找成功者（nextInt(17)==8 单次概率
//   ~5.9%，5 次尝试约 26%/种子，1024 种子几乎必然命中），结果确定。
//
// 框架陷阱（防坠落）：openShellFully 调 25 次 tick 推进开壳动画。BaseTestWorld 无物理引擎
//   （physicsEngine() 返 nullptr），tick→aiStep→travel 施加重力后 move 无条件累加位置（无碰撞
//   拦截），25 tick 后潜影贝从 y=1 坠落到 y≈-21，瞬移目标全落纯空气区恒失败。生产环境有碰撞
//   引擎支撑潜影贝不坠落，故此为测试基础设施限制。修复：openShellFully 后、瞬移前重置
//   position 回 home，对齐生产环境潜影贝附着不坠落的语义。
//
// 注：ShulkerBulletEntity 构造后未 setTypeId，但 hurt 内用 dynamic_cast<ShulkerBulletEntity*>
//   判定（非 typeId 比较），故无需 setTypeId。DamageSources::mobProjectile(bullet, shooter)
//   构造 IndirectEntityDamageSource(MobProjectile, shooter, bullet)，MobProjectile 在 IS_PROJECTILE，
//   directSource=bullet。开壳潜影贝 hurt 走 MonsterEntity::hurt 扣血（非箭，闭壳免疫不触发），
//   成功后进 else if 分支。
//
// Ref: vanilla Shulker.java:423-455（hurtServer else if + hitByShulkerBullet）
// Ref: ShulkerEntity.cpp（hurt else if 分支 + _hitByShulkerBullet + _tryTeleportToNewPosition setPosition）
// Ref: DamageTypeTags.cpp（IS_PROJECTILE 含 MobProjectile）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/monster/end/ShulkerEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

// 潜影贝 home 位置：(0.5, 1, 0.5)，对应 blockPos=(0,1,0)，Down 面 y==0 为石头可附着。
const Vector3 SHULKER_HOME(0.5f, 1.0f, 0.5f);

// 潜影弹命中测试世界：y==0 石头（可附着固体），其余空气；记录 spawn 的实体。
class ShulkerHitTestWorld final : public mc::test::BaseTestWorld {
public:
    ShulkerHitTestWorld() { VanillaBlocks::initialize(); }

    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    // y==0 返回石头（固体，供潜影贝 Down 面附着），其余空气。
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        MC_UNUSED(x);
        MC_UNUSED(z);
        if (y == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawned.size()));
    }

    // 返回预设的附近实体列表（用于繁殖计数），默认空。
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_nearby;
    }

    void setNearbyEntities(std::vector<Entity*> entities) { m_nearby = std::move(entities); }

    [[nodiscard]] size_t spawnedCount() const { return m_spawned.size(); }
    [[nodiscard]] const Entity* spawnedEntity(size_t i) const
    {
        return i < m_spawned.size() ? m_spawned[i].get() : nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
    std::vector<Entity*> m_nearby;
};

// 将潜影贝开壳至 Open 状态（OPEN_DURATION=20 tick），并重置回 home 防止 tick 内重力坠落。
//
// 框架陷阱：BaseTestWorld 无物理引擎，tick 内 aiStep→travel 施加重力、move 无条件累加位置，
// 25 tick 后潜影贝坠落到 y≈-21（见文件头注释）。生产环境潜影贝靠碰撞引擎支撑不坠落，此处
// 在开壳动画推进完成后将位置重置回 home，对齐生产语义。
void openShellFully(ShulkerEntity& shulker)
{
    shulker.openShell();
    for (int i = 0; i < 25; ++i) {
        shulker.tick();
    }
    // 防坠落：tick 推进开壳动画期间重力已使位置下落，重置回 home 以恢复附着姿态。
    shulker.setPosition(SHULKER_HOME);
    ASSERT_TRUE(shulker.isShellOpen());
}

// 构造一个开壳潜影贝并置于 home，固定满血（30），便于复用。
void setupOpenShulker(ShulkerHitTestWorld& world, ShulkerEntity& shulker)
{
    shulker.setWorld(&world);
    shulker.setPosition(SHULKER_HOME);
    shulker.setHealth(30.0f);
    openShellFully(shulker);
}

} // namespace

// ============================================================================
// hitByShulkerBullet 守卫与繁殖测试
// ============================================================================

// 闭壳潜影贝被潜影弹命中不繁殖（isShellClosed 前置守卫）。
TEST(ShulkerBulletHitTest, ClosedShellDoesNotBreedOnBulletHit)
{
    ShulkerHitTestWorld world;
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    shulker.setWorld(&world);
    shulker.setPosition(SHULKER_HOME);
    shulker.setHealth(30.0f);
    ASSERT_TRUE(shulker.isShellClosed()); // 默认闭壳

    entity::ShulkerBulletEntity bullet(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto src = DamageSources::mobProjectile(&bullet, nullptr); // MobProjectile 在 IS_PROJECTILE, directSource=bullet

    EXPECT_TRUE(shulker.hurt(src, 5.0f));
    // 闭壳守卫：_hitByShulkerBullet 内 isShellClosed() return，不繁殖
    EXPECT_EQ(world.spawnedCount(), 0u);
}

// 开壳潜影贝被雪球（非潜影弹）命中不繁殖（dynamic_cast<ShulkerBulletEntity*> 守卫）。
TEST(ShulkerBulletHitTest, NonShulkerBulletDoesNotBreed)
{
    ShulkerHitTestWorld world;
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    setupOpenShulker(world, shulker);

    // 雪球作直接来源（SnowballEntity 非 ShulkerBulletEntity，但 thrown 在 IS_PROJECTILE）
    entity::SnowballEntity snowball(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto src = DamageSources::thrown(&snowball, nullptr);

    EXPECT_TRUE(shulker.hurt(src, 5.0f));
    // 雪球非潜影弹 → 不进 _hitByShulkerBullet → 不繁殖
    EXPECT_EQ(world.spawnedCount(), 0u);
}

// 瞬移成功后实体 position 改变（验证 _tryTeleportToNewPosition 补 setPosition 对齐
// vanilla teleportSomewhere:391 setPos）。
//
// 确定性：遍历种子 0..1024 找使 teleport() 成功（return true）的种子。瞬移成功后 position
// 必然离开 home（vanilla setPos 到新 blockpos1 中心）。若setPosition 缺失（旧缺陷），position
// 仍为 home，本测试失败暴露缺陷。
TEST(ShulkerBulletHitTest, TeleportActuallyMovesEntity)
{
    bool found = false;
    for (u64 seed = 0; seed < 1024 && !found; ++seed) {
        ShulkerHitTestWorld world;
        ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
        setupOpenShulker(world, shulker);

        world.getRandom().setSeed(seed);
        const Vector3 posBefore = shulker.position();
        if (!shulker.teleport()) {
            continue; // 该种子瞬移未命中可附着位置，换下一个
        }
        // 瞬移成功且 position 改变 → 验证 setPosition 生效
        EXPECT_NE(shulker.position().x, posBefore.x);
        EXPECT_NE(shulker.position().z, posBefore.z);
        found = true;
    }
    EXPECT_TRUE(found) << "1024 种子内无瞬移成功者，瞬移逻辑或测试世界可附着条件异常";
}

// 开壳潜影贝被潜影弹命中，瞬移成功且周围无其他潜影贝 → 繁殖一只同色潜影贝。
//
// 确定性说明：_tryTeleportToNewPosition 用 world.getRandom()（固定算法 Xoroshiro128++），
// 瞬移成功取决于随机命中的 targetPos 是否落在"空气 + 相邻固体可附着"位置。本测试世界
// getBlockState 让 y==0 石头、其余空气，潜影贝 home y=1，仅 targetY==1（Down 面 y=0 固体）
// 可附着。固定种子下 5 次 nextInt(17) 是否含偏移0（targetY=1）取决于种子。故遍历种子
// 0..1024，对每个种子用独立实体重现完整 hurt 链路，找到使瞬移成功（繁殖 spawn 1 只）的
// 种子后断言。种子搜索在测试内自适应执行，结果确定（1024 种子必含成功者）。
TEST(ShulkerBulletHitTest, BulletHitBreedsSameColorShulker)
{
    bool found = false;
    for (u64 seed = 0; seed < 1024 && !found; ++seed) {
        ShulkerHitTestWorld world;
        ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
        shulker.setWorld(&world);
        shulker.setPosition(SHULKER_HOME);
        shulker.setHealth(30.0f);
        shulker.setColor(ShulkerEntity::ShulkerColor::Red);
        openShellFully(shulker);
        world.setNearbyEntities({}); // 周围无其他潜影贝 → i=0 → f=-0.2 → 必繁殖

        world.getRandom().setSeed(seed);

        entity::ShulkerBulletEntity bullet(EntityInstanceId(2), mc::test::testEcsRegistry());
        auto src = DamageSources::mobProjectile(&bullet, nullptr);
        if (!shulker.hurt(src, 5.0f)) {
            continue; // hurt 失败（不应发生），换种子
        }

        if (world.spawnedCount() == 1) {
            found = true;
            const Entity* baby = world.spawnedEntity(0);
            ASSERT_NE(baby, nullptr);
            const auto* babyShulker = dynamic_cast<const ShulkerEntity*>(baby);
            ASSERT_NE(babyShulker, nullptr);
            // 繁殖继承颜色（vanilla setVariant）
            EXPECT_EQ(babyShulker->getColor(), ShulkerEntity::ShulkerColor::Red);
            // 新潜影贝生成在原位置（vanilla snapTo(vec3)，vec3=瞬移前 position=home）
            EXPECT_FLOAT_EQ(baby->position().x, SHULKER_HOME.x);
            EXPECT_FLOAT_EQ(baby->position().y, SHULKER_HOME.y);
            EXPECT_FLOAT_EQ(baby->position().z, SHULKER_HOME.z);
        }
    }
    EXPECT_TRUE(found) << "未找到使潜影弹命中繁殖成功的种子（1024 种子内无瞬移成功者）";
}

} // namespace mc
