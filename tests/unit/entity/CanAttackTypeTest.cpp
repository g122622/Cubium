/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do any of subject to the following conditions:
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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/basic/PhantomEntity.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// canAttackType 单元测试
// ============================================================================
// 对应 MC 原版 Mob.canAttackType() 及其子类重写的完整行为测试。
// Mob 基类排除恶魂（GHAST），Phantom 重写返回 true，Breeze 白名单模式，
// IronGolem 排除苦力怕和（玩家创建时）玩家。

class CanAttackTypeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaEntities::registerAll();
            s_initialized = true;
        }
    }
};

// ============================================================================
// MobEntity 基类 canAttackType 测试
// ============================================================================

TEST_F(CanAttackTypeTest, MobBase_ExcludesGhast)
{
    // MC 原版 Mob.canAttackType() 排除恶魂
    // 使用 ZombieEntity 作为 MobEntity 的具体子类测试
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(zombie->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "MobEntity should not be able to attack GHAST type";
}

TEST_F(CanAttackTypeTest, MobBase_AllowsOtherTypes)
{
    // Mob 基类允许攻击除恶魂外的所有实体类型
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "MobEntity should be able to attack PLAYER type";
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::ZOMBIE))
        << "MobEntity should be able to attack ZOMBIE type";
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::CREEPER))
        << "MobEntity should be able to attack CREEPER type";
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::SKELETON))
        << "MobEntity should be able to attack SKELETON type";
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::PIG)) << "MobEntity should be able to attack PIG type";
    EXPECT_TRUE(zombie->canAttackType(*VanillaEntityTypeKeys::IRON_GOLEM))
        << "MobEntity should be able to attack IRON_GOLEM type";
}

TEST_F(CanAttackTypeTest, MobBase_PassiveMobAlsoExcludesGhast)
{
    // 被动生物（如猪）也继承 MobEntity，同样排除恶魂
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(pig->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "Passive mobs (PigEntity) should also exclude GHAST type";
    EXPECT_TRUE(pig->canAttackType(*VanillaEntityTypeKeys::PLAYER)) << "Passive mobs should allow other types";
}

TEST_F(CanAttackTypeTest, MobBase_UnknownTypeAllowed)
{
    // 使用不可能的 ID 测试：Mob 基类允许攻击非 GHAST 的任意类型
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(zombie->canAttackType(entity::EntityType::UNKNOWN))
        << "MobEntity should allow attacking unknown type IDs (except GHAST)";
}

// ============================================================================
// PhantomEntity canAttackType 测试
// ============================================================================

TEST_F(CanAttackTypeTest, Phantom_CanAttackGhast)
{
    // MC 原版 Phantom.canAttackType() 返回 true，覆盖基类排除恶魂的限制
    // 幻翼本身是飞行生物，可以攻击空中目标
    auto phantom = std::make_unique<PhantomEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(phantom->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "PhantomEntity should be able to attack GHAST (overrides base class exclusion)";
}

TEST_F(CanAttackTypeTest, Phantom_CanAttackAllTypes)
{
    // Phantom 返回 true，允许攻击所有类型
    auto phantom = std::make_unique<PhantomEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(phantom->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "PhantomEntity should be able to attack PLAYER";
    EXPECT_TRUE(phantom->canAttackType(*VanillaEntityTypeKeys::ZOMBIE))
        << "PhantomEntity should be able to attack ZOMBIE";
    EXPECT_TRUE(phantom->canAttackType(*VanillaEntityTypeKeys::SKELETON))
        << "PhantomEntity should be able to attack SKELETON";
    EXPECT_TRUE(phantom->canAttackType(*VanillaEntityTypeKeys::CREEPER))
        << "PhantomEntity should be able to attack CREEPER";
    EXPECT_TRUE(phantom->canAttackType(entity::EntityType::UNKNOWN))
        << "PhantomEntity should be able to attack any type (returns true for all)";
}

// ============================================================================
// BreezeEntity canAttackType 测试
// ============================================================================

TEST_F(CanAttackTypeTest, Breeze_CanAttackPlayer)
{
    // MC 原版 Breeze.canAttackType() 白名单模式：仅允许攻击玩家
    auto breeze = std::make_unique<BreezeEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(breeze->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "BreezeEntity should be able to attack PLAYER";
}

TEST_F(CanAttackTypeTest, Breeze_CanAttackIronGolem)
{
    // MC 原版 Breeze.canAttackType() 白名单模式：仅允许攻击铁傀儡
    auto breeze = std::make_unique<BreezeEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(breeze->canAttackType(*VanillaEntityTypeKeys::IRON_GOLEM))
        << "BreezeEntity should be able to attack IRON_GOLEM";
}

TEST_F(CanAttackTypeTest, Breeze_CannotAttackOtherTypes)
{
    // 白名单模式：除玩家和铁傀儡外的类型都不允许攻击
    auto breeze = std::make_unique<BreezeEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(breeze->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "BreezeEntity should NOT be able to attack GHAST (not in whitelist)";
    EXPECT_FALSE(breeze->canAttackType(*VanillaEntityTypeKeys::ZOMBIE))
        << "BreezeEntity should NOT be able to attack ZOMBIE (not in whitelist)";
    EXPECT_FALSE(breeze->canAttackType(*VanillaEntityTypeKeys::SKELETON))
        << "BreezeEntity should NOT be able to attack SKELETON (not in whitelist)";
    EXPECT_FALSE(breeze->canAttackType(*VanillaEntityTypeKeys::CREEPER))
        << "BreezeEntity should NOT be able to attack CREEPER (not in whitelist)";
    EXPECT_FALSE(breeze->canAttackType(*VanillaEntityTypeKeys::PIG))
        << "BreezeEntity should NOT be able to attack PIG (not in whitelist)";
    EXPECT_FALSE(breeze->canAttackType(entity::EntityType::UNKNOWN))
        << "BreezeEntity should NOT be able to attack unknown types (not in whitelist)";
}

// ============================================================================
// IronGolemEntity canAttackType 测试（与基类交互）
// ============================================================================

TEST_F(CanAttackTypeTest, IronGolem_NeverAttacksCreeper)
{
    // 铁傀儡不攻击苦力怕，无论是否玩家创建
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setPlayerCreated(true);
    EXPECT_FALSE(golem->canAttackType(*VanillaEntityTypeKeys::CREEPER))
        << "Player-created IronGolem should NOT attack CREEPER";

    golem->setPlayerCreated(false);
    EXPECT_FALSE(golem->canAttackType(*VanillaEntityTypeKeys::CREEPER)) << "Wild IronGolem should NOT attack CREEPER";
}

TEST_F(CanAttackTypeTest, IronGolem_PlayerCreatedDoesNotAttackPlayer)
{
    // 玩家创建的铁傀儡不攻击玩家
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setPlayerCreated(true);
    EXPECT_FALSE(golem->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "Player-created IronGolem should NOT attack PLAYER";
}

TEST_F(CanAttackTypeTest, IronGolem_WildGolemCanAttackPlayer)
{
    // 野生铁傀儡可以攻击玩家
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setPlayerCreated(false);
    EXPECT_TRUE(golem->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "Wild IronGolem should be able to attack PLAYER";
}

TEST_F(CanAttackTypeTest, IronGolem_ExcludesGhastViaBaseClass)
{
    // 铁傀儡继承 MobEntity 基类排除恶魂的逻辑
    // IronGolem::canAttackType 对非 PLAYER/非 CREEPER 类型委托给 MobEntity::canAttackType
    // 而 MobEntity::canAttackType 排除 GHAST
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setPlayerCreated(false);
    EXPECT_FALSE(golem->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "IronGolem should NOT attack GHAST (inherited from MobEntity base class)";
}

TEST_F(CanAttackTypeTest, IronGolem_CanAttackOtherTypes)
{
    // 铁傀儡可以攻击除苦力怕、恶魂以外的类型
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setPlayerCreated(false);
    EXPECT_TRUE(golem->canAttackType(*VanillaEntityTypeKeys::ZOMBIE)) << "IronGolem should be able to attack ZOMBIE";
    EXPECT_TRUE(golem->canAttackType(*VanillaEntityTypeKeys::SKELETON))
        << "IronGolem should be able to attack SKELETON";
    EXPECT_TRUE(golem->canAttackType(*VanillaEntityTypeKeys::SPIDER)) << "IronGolem should be able to attack SPIDER";
}

// ============================================================================
// ZoglinEntity canAttackType 测试
// ============================================================================
// 对应 MC 原版 Zoglin.isTargetable 的类型过滤逻辑。
// 僵尸疣兽不攻击同类（ZOGLIN）和苦力怕（CREEPER）。

#include "common/entity/entities/monster/nether/NetherEntities.hpp"

TEST_F(CanAttackTypeTest, Zoglin_NeverAttacksCreeper)
{
    // MC 原版 Zoglin.isTargetable 排除 Creeper
    auto zoglin = std::make_unique<ZoglinEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(zoglin->canAttackType(*VanillaEntityTypeKeys::CREEPER))
        << "ZoglinEntity should NOT be able to attack CREEPER";
}

TEST_F(CanAttackTypeTest, Zoglin_NeverAttacksZoglin)
{
    // MC 原版 Zoglin.isTargetable 排除同类 Zoglin
    auto zoglin = std::make_unique<ZoglinEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(zoglin->canAttackType(*VanillaEntityTypeKeys::ZOGLIN))
        << "ZoglinEntity should NOT be able to attack ZOGLIN";
}

TEST_F(CanAttackTypeTest, Zoglin_CanAttackOtherTypes)
{
    // 僵尸疣兽可以攻击除同类和苦力怕以外的所有类型
    auto zoglin = std::make_unique<ZoglinEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(zoglin->canAttackType(*VanillaEntityTypeKeys::PLAYER))
        << "ZoglinEntity should be able to attack PLAYER";
    EXPECT_TRUE(zoglin->canAttackType(*VanillaEntityTypeKeys::ZOMBIE))
        << "ZoglinEntity should be able to attack ZOMBIE";
    EXPECT_TRUE(zoglin->canAttackType(*VanillaEntityTypeKeys::SKELETON))
        << "ZoglinEntity should be able to attack SKELETON";
    EXPECT_TRUE(zoglin->canAttackType(*VanillaEntityTypeKeys::PIG)) << "ZoglinEntity should be able to attack PIG";
    EXPECT_TRUE(zoglin->canAttackType(*VanillaEntityTypeKeys::IRON_GOLEM))
        << "ZoglinEntity should be able to attack IRON_GOLEM";
}

TEST_F(CanAttackTypeTest, Zoglin_ExcludesGhastViaBaseClass)
{
    // ZoglinEntity 继承 MonsterEntity -> MobEntity 基类排除恶魂的逻辑
    auto zoglin = std::make_unique<ZoglinEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(zoglin->canAttackType(*VanillaEntityTypeKeys::GHAST))
        << "ZoglinEntity should NOT attack GHAST (inherited from MobEntity base class)";
}
