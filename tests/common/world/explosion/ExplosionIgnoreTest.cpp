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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/ExplosionImmunityContext.hpp"

namespace mc {
namespace {

using namespace mc::world::explosion;

// ============================================================================
// 测试用 Mob 桩
// ============================================================================
//
// VehicleEntity（Boat/Minecart）的 ignoreExplosion 覆写需要判定「爆炸间接源是否为
// Mob」（Java: getIndirectSourceEntity() instanceof Mob）。MobEntity 抽象度低但构造
// 会拉起 controller/navigator 等 AI 子系统，此处用一个最小派生类提供可实例化的 Mob。
// ignoreExplosion 不依赖属性/生命值，故不调用 registerAttributes/setHealth。

class TestMobEntity final : public MobEntity {
public:
    explicit TestMobEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : MobEntity(id, registry)
    {}
};

// ============================================================================
// 上下文构造辅助
// ============================================================================

/// 构造一个 ExplosionImmunityContext，便于在测试中按字段覆写。
ExplosionImmunityContext makeCtx(bool shouldAffectBlocklikeEntities,
    LivingEntity* indirectSource = nullptr,
    Entity* directSource = nullptr,
    bool mobGriefing = false)
{
    ExplosionImmunityContext ctx{};
    ctx.shouldAffectBlocklikeEntities = shouldAffectBlocklikeEntities;
    ctx.indirectSource = indirectSource;
    ctx.directSource = directSource;
    ctx.mobGriefing = mobGriefing;
    return ctx;
}

// ============================================================================
// 固件：初始化方块/物品注册表（ItemEntity 构造需要 ItemStack → ItemRegistry）
// ============================================================================

class ExplosionIgnoreTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    /// 获取 stone 物品，用于构造 ItemEntity 的 ItemStack。
    static const Item* stoneItem() { return ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone")); }
};

// ============================================================================
// ItemEntity
// ============================================================================
//
// Java ItemEntity.ignoreExplosion:
//   return this.shouldAffectBlocklikeEntities(explosion) ? super.ignoreExplosion(explosion) : true;
// Cubium 同构：shouldAffectBlocklikeEntities=true → 基类 false（受影响）；false → true（忽略）。

TEST_F(ExplosionIgnoreTest, ItemEntity_IgnoredWhenNotAffectingBlocklike)
{
    ItemEntity entity(EntityInstanceId(1), ItemStack(stoneItem(), 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // shouldAffectBlocklikeEntities=false（如风爆路径或 mobGriefing 关闭且 mode=None）：
    // 掉落物忽略爆炸。
    const auto ctx = makeCtx(false);
    EXPECT_TRUE(entity.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, ItemEntity_AffectedWhenAffectingBlocklike)
{
    ItemEntity entity(EntityInstanceId(1), ItemStack(stoneItem(), 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // shouldAffectBlocklikeEntities=true：掉落物受爆炸影响（回退基类 false）。
    const auto ctx = makeCtx(true);
    EXPECT_FALSE(entity.ignoreExplosion(ctx));
}

// ============================================================================
// ArmorStandEntity
// ============================================================================
//
// Java ArmorStand.ignoreExplosion:
//   return this.shouldAffectBlocklikeEntities(explosion) && !this.isInvisible()
//       ? super.ignoreExplosion(explosion) : true;
// Cubium 近似：shouldAffectBlocklikeEntities=true → 不可见忽略、可见受影响；false → 忽略。
// 注：vanilla 用 Entity.isInvisible()（共享 flags），Cubium 暂用 ArmorStand 自身 isInvisible()。

TEST_F(ExplosionIgnoreTest, ArmorStand_IgnoredWhenNotAffectingBlocklike)
{
    entity::ArmorStandEntity entity(mc::test::testEcsRegistry());

    const auto ctx = makeCtx(false);
    EXPECT_TRUE(entity.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, ArmorStand_InvisibleIgnoredEvenWhenAffectingBlocklike)
{
    entity::ArmorStandEntity entity(mc::test::testEcsRegistry());
    entity.setInvisible(true);

    const auto ctx = makeCtx(true);
    // 不可见盔甲架忽略爆炸。
    EXPECT_TRUE(entity.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, ArmorStand_VisibleAffectedWhenAffectingBlocklike)
{
    entity::ArmorStandEntity entity(mc::test::testEcsRegistry());
    entity.setInvisible(false);

    const auto ctx = makeCtx(true);
    // 可见盔甲架受爆炸影响（回退基类 false）。
    EXPECT_FALSE(entity.ignoreExplosion(ctx));
}

// ============================================================================
// HangingEntity（PaintingEntity 代表）
// ============================================================================
//
// Java BlockAttachedEntity.ignoreExplosion:
//   return this.shouldAffectBlocklikeEntities(explosion) && !explosion.getDirectSourceEntity().isInWater()
//       ? super.ignoreExplosion(explosion) : true;
// Cubium 同构：directSource 在水中 → 忽略；否则按 shouldAffectBlocklikeEntities 判定。
// 覆盖 Painting/ItemFrame/LeashKnot 三个子类（继承自动生效），此处用 PaintingEntity 代表。

TEST_F(ExplosionIgnoreTest, Hanging_IgnoredWhenNotAffectingBlocklike)
{
    entity::PaintingEntity entity(mc::test::testEcsRegistry());

    const auto ctx = makeCtx(false);
    EXPECT_TRUE(entity.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Hanging_IgnoredWhenDirectSourceInWater)
{
    entity::PaintingEntity entity(mc::test::testEcsRegistry());

    // 直接源在水中：悬挂实体忽略爆炸（避免水下爆炸摧毁画作/展示框）。
    entity::ArmorStandEntity source(mc::test::testEcsRegistry());
    source.setInWater(true);
    const auto ctx = makeCtx(true, nullptr, &source);
    EXPECT_TRUE(entity.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Hanging_AffectedWhenDirectSourceNotInWater)
{
    entity::PaintingEntity entity(mc::test::testEcsRegistry());

    // 直接源不在水中且 shouldAffectBlocklikeEntities=true：受爆炸影响。
    entity::ArmorStandEntity source(mc::test::testEcsRegistry());
    source.setInWater(false);
    const auto ctx = makeCtx(true, nullptr, &source);
    EXPECT_FALSE(entity.ignoreExplosion(ctx));
}

// ============================================================================
// BoatEntity / AbstractMinecartEntity（VehicleEntity 语义）
// ============================================================================
//
// Java VehicleEntity.ignoreExplosion:
//   return explosion.getIndirectSourceEntity() instanceof Mob && !level.getGameRules().mobGriefing
//       ? super.ignoreExplosion(explosion) : true;
// 即：间接源是 Mob 且 mobGriefing 关闭 → 受影响（super false）；否则忽略（true）。
// Cubium 无 VehicleEntity 基类，Boat/Minecart 分别覆写同款逻辑。

// ---------- Boat ----------

TEST_F(ExplosionIgnoreTest, Boat_IgnoredWhenIndirectSourceNotMob)
{
    entity::BoatEntity boat(entity::BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 间接源为空（无追溯到 Mob）：忽略爆炸（载具不被意外摧毁）。
    const auto ctx = makeCtx(false, nullptr);
    EXPECT_TRUE(boat.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Boat_AffectedWhenMobSourceAndMobGriefingOff)
{
    entity::BoatEntity boat(entity::BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 间接源是 Mob（如苦力怕）且 mobGriefing 关闭：受爆炸影响（super false）。
    TestMobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    const auto ctx = makeCtx(false, &mob, nullptr, /*mobGriefing=*/false);
    EXPECT_FALSE(boat.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Boat_IgnoredWhenMobSourceAndMobGriefingOn)
{
    entity::BoatEntity boat(entity::BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 间接源是 Mob 但 mobGriefing 开启：忽略爆炸（怪物破坏被允许，但载具受保护）。
    TestMobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    const auto ctx = makeCtx(false, &mob, nullptr, /*mobGriefing=*/true);
    EXPECT_TRUE(boat.ignoreExplosion(ctx));
}

// ---------- Minecart ----------

TEST_F(ExplosionIgnoreTest, Minecart_IgnoredWhenIndirectSourceNotMob)
{
    entity::RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    const auto ctx = makeCtx(false, nullptr);
    EXPECT_TRUE(minecart.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Minecart_AffectedWhenMobSourceAndMobGriefingOff)
{
    entity::RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    TestMobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    const auto ctx = makeCtx(false, &mob, nullptr, /*mobGriefing=*/false);
    EXPECT_FALSE(minecart.ignoreExplosion(ctx));
}

TEST_F(ExplosionIgnoreTest, Minecart_IgnoredWhenMobSourceAndMobGriefingOn)
{
    entity::RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    TestMobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    const auto ctx = makeCtx(false, &mob, nullptr, /*mobGriefing=*/true);
    EXPECT_TRUE(minecart.ignoreExplosion(ctx));
}

// ============================================================================
// WardenEntity
// ============================================================================
//
// 监守者 ignoreExplosion 应在 Digging/Emerging 姿态下返回 true，但姿态系统未实现，
// 当前回退 MonsterEntity::ignoreExplosion（基类 false）。姿态相关分支待 Pose 系统引入后
// 补充测试，此处不实例化 WardenEntity（其构造依赖怒气/属性/目标注册等重型子系统）。

} // namespace
} // namespace mc
