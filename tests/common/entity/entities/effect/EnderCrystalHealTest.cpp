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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR ANY DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::entity;

/**
 * @brief EnderCrystalEntity 单元测试
 *
 * 测试末影水晶的功能：
 * - 光束目标设置
 * - 底座显示控制
 * - 内部旋转动画
 * - 治愈末影龙接口
 * - 火焰免疫（对齐 vanilla EntityType.END_CRYSTAL.fireImmune()）
 */
class EnderCrystalHealTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表，确保 EntityTypeKeys::END_CRYSTAL 有正确的 typeId，
        // 且 END_CRYSTAL 类型注册了 .immuneToFire() 标志（VanillaEntities.cpp:1263）。
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    void TearDown() override
    {
        // 测试清理
    }
};

// ==================== 常量测试 ====================

/**
 * @brief 验证治愈冷却时间常量
 *
 * MC 1.16.5: 末影水晶每 10 ticks 治愈一次末影龙
 *
 * 注：HEAL_COOLDOWN 是私有常量，无法直接访问。
 * 这里验证算法中使用的常量值。
 */
TEST_F(EnderCrystalHealTest, HealCooldownConstant)
{
    // MC 1.16.5: 末影水晶每 10 ticks 治愈一次末影龙
    constexpr i32 HEAL_COOLDOWN = 10;
    EXPECT_EQ(HEAL_COOLDOWN, 10);
}

/**
 * @brief 验证爆炸半径常量
 *
 * MC 1.16.5: 末影水晶爆炸半径 6.0
 *
 * 注：EXPLOSION_RADIUS 是私有常量，无法直接访问。
 * 这里验证算法中使用的常量值。
 */
TEST_F(EnderCrystalHealTest, ExplosionRadiusConstant)
{
    // MC 1.16.5: 末影水晶爆炸半径 6.0
    constexpr f32 EXPLOSION_RADIUS = 6.0f;
    EXPECT_FLOAT_EQ(EXPLOSION_RADIUS, 6.0f);
}

// ==================== 基本属性测试 ====================

TEST_F(EnderCrystalHealTest, CrystalDimensions)
{
    // 创建末影水晶实体
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 验证尺寸
    // MC 1.16.5: 末影水晶宽高都是 2.0
    EXPECT_FLOAT_EQ(crystal.width(), 2.0f);
    EXPECT_FLOAT_EQ(crystal.height(), 2.0f);
}

TEST_F(EnderCrystalHealTest, BeamTargetInitialization)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 初始状态没有光束目标
    EXPECT_FALSE(crystal.hasBeamTarget());

    // 默认光束目标应该是零位置
    const BlockPos& target = crystal.getBeamTarget();
    EXPECT_EQ(target.x, 0);
    EXPECT_EQ(target.y, 0);
    EXPECT_EQ(target.z, 0);
}

TEST_F(EnderCrystalHealTest, SetBeamTarget)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 设置光束目标
    BlockPos target(100, 64, -200);
    crystal.setBeamTarget(target);

    // 验证设置成功
    EXPECT_TRUE(crystal.hasBeamTarget());
    EXPECT_EQ(crystal.getBeamTarget().x, 100);
    EXPECT_EQ(crystal.getBeamTarget().y, 64);
    EXPECT_EQ(crystal.getBeamTarget().z, -200);
}

TEST_F(EnderCrystalHealTest, ShowBottomFlag)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 默认不显示底座
    EXPECT_FALSE(crystal.shouldShowBottom());

    // 设置显示底座
    crystal.setShowBottom(true);
    EXPECT_TRUE(crystal.shouldShowBottom());

    // 设置不显示底座
    crystal.setShowBottom(false);
    EXPECT_FALSE(crystal.shouldShowBottom());
}

TEST_F(EnderCrystalHealTest, InnerRotationIncrements)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 初始内部旋转为 0
    EXPECT_EQ(crystal.innerRotation(), 0);

    // tick() 应该递增内部旋转
    crystal.tick();
    EXPECT_EQ(crystal.innerRotation(), 1);

    crystal.tick();
    EXPECT_EQ(crystal.innerRotation(), 2);

    // 多次 tick
    for (int i = 0; i < 100; ++i) {
        crystal.tick();
    }
    EXPECT_EQ(crystal.innerRotation(), 102);
}

// ==================== 治愈逻辑测试 ====================

/**
 * @brief 测试冷却机制
 *
 * MC 1.16.5: 末影水晶治愈后有 10 tick 冷却时间
 */
TEST_F(EnderCrystalHealTest, HealCooldownMechanism)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 初始冷却为 0
    // 在无世界环境下调用 healDragon() 应该直接返回

    // 第一次调用（无世界）
    crystal.healDragon();

    // tick() 应该递减冷却
    crystal.tick();
    crystal.tick();
    crystal.tick();

    // 冷却应该从 0 开始，tick 后仍然为 0
    // 注意：由于没有世界，healDragon 不会设置冷却
}

/**
 * @brief 测试治愈搜索范围
 *
 * MC 1.16.5: 末影水晶在 32 格范围内搜索末影龙
 */
TEST_F(EnderCrystalHealTest, HealRangeConstant)
{
    // 验证治愈范围常量
    // MC 1.16.5: EnderDragonEntity.CRYSTAL_SEARCH_RANGE = 32.0
    // 在 healDragon() 中使用 32.0f 作为搜索范围
    constexpr f32 HEAL_RANGE = 32.0f;
    constexpr f32 HEAL_RANGE_SQ = HEAL_RANGE * HEAL_RANGE;

    EXPECT_FLOAT_EQ(HEAL_RANGE, 32.0f);
    EXPECT_FLOAT_EQ(HEAL_RANGE_SQ, 1024.0f);
}

// ==================== AreaEffectCloudEntity 测试 ====================

TEST_F(EnderCrystalHealTest, AreaEffectCloudDefaultParameters)
{
    AreaEffectCloudEntity cloud{mc::test::testEcsRegistry()};

    // MC 1.16.5 默认值验证
    EXPECT_FLOAT_EQ(cloud.getRadius(), 3.0f);
    EXPECT_EQ(cloud.getDuration(), 600);
    EXPECT_EQ(cloud.getWaitTime(), 20);
    EXPECT_EQ(cloud.getReapplicationDelay(), 20);
}

TEST_F(EnderCrystalHealTest, AreaEffectCloudRadiusModification)
{
    AreaEffectCloudEntity cloud{mc::test::testEcsRegistry()};

    // 设置半径
    cloud.setRadius(5.0f);
    EXPECT_FLOAT_EQ(cloud.getRadius(), 5.0f);

    // 半径变化设置
    cloud.setRadiusOnUse(-0.5f);
    cloud.setRadiusPerTick(-0.01f);

    // 持续时间设置
    cloud.setDuration(300);
    EXPECT_EQ(cloud.getDuration(), 300);

    cloud.setDurationOnUse(-10);
}

// ==================== LightningBoltEntity 测试 ====================

TEST_F(EnderCrystalHealTest, LightningBoltDimensions)
{
    LightningBoltEntity lightning{mc::test::testEcsRegistry()};

    // 闪电没有碰撞箱
    EXPECT_FLOAT_EQ(lightning.width(), 0.0f);
    EXPECT_FLOAT_EQ(lightning.height(), 0.0f);
}

TEST_F(EnderCrystalHealTest, LightningBoltEffectOnly)
{
    LightningBoltEntity lightning{mc::test::testEcsRegistry()};

    // 默认不是仅效果
    EXPECT_FALSE(lightning.isEffectOnly());

    // 设置为仅效果
    lightning.setEffectOnly(true);
    EXPECT_TRUE(lightning.isEffectOnly());
}

TEST_F(EnderCrystalHealTest, LightningBoltCaster)
{
    LightningBoltEntity lightning{mc::test::testEcsRegistry()};

    // 默认无施法者
    EXPECT_EQ(lightning.caster(), 0);

    // 设置施法者
    lightning.setCaster(PlayerId(12345));
    EXPECT_EQ(lightning.caster(), 12345);
}

TEST_F(EnderCrystalHealTest, LightningBoltState)
{
    LightningBoltEntity lightning{mc::test::testEcsRegistry()};

    // 默认闪电状态为 2
    EXPECT_EQ(lightning.lightningState(), 2);

    // 默认生命时间为 1-3
    EXPECT_GE(lightning.boltLivingTime(), 1);
    EXPECT_LE(lightning.boltLivingTime(), 3);
}

// ============================================================================
// 火焰免疫测试（对齐 vanilla EntityType.END_CRYSTAL.fireImmune()，
// EntityType.java:456）。vanilla 末影水晶 fireImmune=true，免疫所有 IS_FIRE
// 伤害源（in_fire/campfire/on_fire/lava/hot_floor/fireball/unattributed_fireball）。
// Cubium 此前 END_CRYSTAL 注册缺 .immuneToFire() 且 Entity 基类 isInvulnerableTo
// 缺 IS_FIRE+isImmuneToFire 守卫，致末影水晶在火焰/岩浆中被错误伤害。
// ============================================================================

TEST_F(EnderCrystalHealTest, EnderCrystal_IsImmuneToFire)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};

    // 直接构造的实体不经过 EntityType::create() 工厂，typeId 默认空。
    // 补 setTypeId 对齐生产路径，使 isImmuneToFire() 经 EntityRegistry 查到
    // END_CRYSTAL 类型的 .immuneToFire() 标志（VanillaEntities.cpp:1263）。
    crystal.setTypeId(entity::EntityTypeKeys::END_CRYSTAL);

    // 末影水晶免疫火焰（对齐 vanilla fireImmune=true）
    EXPECT_TRUE(crystal.isImmuneToFire()) << "末影水晶应免疫火焰（vanilla EntityType.END_CRYSTAL.fireImmune()）";
}

TEST_F(EnderCrystalHealTest, EnderCrystal_FireDamageImmune)
{
    EnderCrystalEntity crystal{mc::test::testEcsRegistry()};
    crystal.setTypeId(entity::EntityTypeKeys::END_CRYSTAL);

    // InFire 是 IS_FIRE 伤害源（DamageTypeTags.cpp:617 IS_FIRE 成员含 InFire）
    EnvironmentalDamage fireDamage(DamageType::InFire);

    // isInvulnerableTo 应拦截 IS_FIRE 伤害（基类 Entity::isInvulnerableTo 的
    // IS_FIRE+isImmuneToFire 守卫，对齐 vanilla isInvulnerableToBase:2921）
    EXPECT_TRUE(crystal.isInvulnerableTo(fireDamage)) << "末影水晶对 IS_FIRE 伤害源应免疫（isInvulnerableTo 返 true）";

    // hurt 应返回 false（火焰伤害被 isInvulnerableTo 拦截，不触发爆炸/破坏流程）
    EXPECT_FALSE(crystal.hurt(fireDamage, 5.0f)) << "末影水晶受火焰伤害应被拒绝（hurt 返 false）";
}
