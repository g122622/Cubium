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

#include "entity/core/Entity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/food/FoodStats.hpp"
#include "entity/serialization/EntityNbtKeys.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "world/GlobalPos.hpp"

using namespace mc;

// ============================================================================
// Entity 测试
// ============================================================================

TEST(Entity, Construction)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    EXPECT_EQ(entity.id(), 1u);
    // 直接构造的 Entity 未 setTypeId，entityType() 懒查询返回 nullptr（对齐 EntityCoreTests DefaultTypeIdIsUnknown）
    EXPECT_EQ(entity.entityType(), nullptr);
    EXPECT_FALSE(entity.uuid().empty());
    EXPECT_FALSE(entity.isRemoved());
}

TEST(Entity, Position)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.setPosition(100.5, 64.0, -200.25);
    EXPECT_FLOAT_EQ(entity.x(), 100.5f);
    EXPECT_FLOAT_EQ(entity.y(), 64.0f);
    EXPECT_FLOAT_EQ(entity.z(), -200.25f);

    auto pos = entity.position();
    EXPECT_FLOAT_EQ(pos.x, 100.5f);
    EXPECT_FLOAT_EQ(pos.y, 64.0f);
    EXPECT_FLOAT_EQ(pos.z, -200.25f);
}

TEST(Entity, Rotation)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.setRotation(90.0f, 45.0f);
    EXPECT_FLOAT_EQ(entity.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 45.0f);
}

TEST(Entity, Velocity)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.setVelocity(1.0, 2.0, 3.0);
    auto vel = entity.velocity();
    EXPECT_FLOAT_EQ(vel.x, 1.0f);
    EXPECT_FLOAT_EQ(vel.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.z, 3.0f);
}

TEST(Entity, Move)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setPosition(0.0, 0.0, 0.0);

    entity.move(10.0, 5.0, -3.0);
    EXPECT_FLOAT_EQ(entity.x(), 10.0f);
    EXPECT_FLOAT_EQ(entity.y(), 5.0f);
    EXPECT_FLOAT_EQ(entity.z(), -3.0f);
}

TEST(Entity, Rotate)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setRotation(0.0f, 0.0f);

    entity.rotate(90.0f, 45.0f);
    EXPECT_FLOAT_EQ(entity.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 45.0f);

    // 测试俯仰角限制
    entity.rotate(0.0f, 100.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 90.0f);

    entity.rotate(0.0f, -200.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), -90.0f);
}

TEST(Entity, BoundingBox)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setPosition(0.0, 0.0, 0.0);

    auto box = entity.boundingBox();
    EXPECT_FLOAT_EQ(box.width(), 0.6f);
    EXPECT_FLOAT_EQ(box.height(), 1.8f);
}

TEST(Entity, Flags)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.addFlag(EntityFlags::OnFire);
    EXPECT_TRUE(entity.hasFlag(EntityFlags::OnFire));
    EXPECT_FALSE(entity.hasFlag(EntityFlags::Sprinting));

    entity.addFlag(EntityFlags::Sprinting);
    EXPECT_TRUE(entity.hasFlag(EntityFlags::Sprinting));

    entity.removeFlag(EntityFlags::OnFire);
    EXPECT_FALSE(entity.hasFlag(EntityFlags::OnFire));
    EXPECT_TRUE(entity.hasFlag(EntityFlags::Sprinting));
}

TEST(Entity, Tick)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    EXPECT_EQ(entity.ticksExisted(), 0u);

    entity.tick();
    EXPECT_EQ(entity.ticksExisted(), 1u);

    entity.tick();
    entity.tick();
    EXPECT_EQ(entity.ticksExisted(), 3u);
}

// ============================================================================
// Player 测试
// ============================================================================

TEST(Player, Construction)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.id(), 1u);
    EXPECT_EQ(player.playerId(), 0u); // 默认为0，需要手动设置
    EXPECT_EQ(player.username(), "TestPlayer");
    EXPECT_EQ(player.gameMode(), GameMode::Survival);
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
}

TEST(Player, Health)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    EXPECT_FALSE(player.isDead());

    // 测试 setHealth 和 heal（直接设置，不需要世界）
    player.setHealth(15.0f);
    EXPECT_FLOAT_EQ(player.health(), 15.0f);
    EXPECT_FALSE(player.isDead());

    player.heal(3.0f);
    EXPECT_FLOAT_EQ(player.health(), 18.0f);

    player.setHealth(0.0f);
    EXPECT_FLOAT_EQ(player.health(), 0.0f);
    EXPECT_TRUE(player.isDead());
}

TEST(Player, GameMode)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.setGameMode(GameMode::Creative);
    EXPECT_EQ(player.gameMode(), GameMode::Creative);
    EXPECT_TRUE(player.abilities().creativeMode);
    EXPECT_TRUE(player.abilities().canFly);

    player.setGameMode(GameMode::Spectator);
    EXPECT_EQ(player.gameMode(), GameMode::Spectator);
    EXPECT_TRUE(player.abilities().invulnerable);
    EXPECT_TRUE(player.abilities().flying);
}

TEST(Player, IsSpectator)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 默认生存模式，不是观察者
    EXPECT_FALSE(player.isSpectator());

    // 设置为观察者模式
    player.setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player.isSpectator());

    // 设置为创造模式
    player.setGameMode(GameMode::Creative);
    EXPECT_FALSE(player.isSpectator());

    // 设置为冒险模式
    player.setGameMode(GameMode::Adventure);
    EXPECT_FALSE(player.isSpectator());

    // 设置回生存模式
    player.setGameMode(GameMode::Survival);
    EXPECT_FALSE(player.isSpectator());
}

TEST(Player, IsCreative)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 默认生存模式
    EXPECT_FALSE(player.isCreative());

    player.setGameMode(GameMode::Creative);
    EXPECT_TRUE(player.isCreative());

    player.setGameMode(GameMode::Spectator);
    EXPECT_FALSE(player.isCreative());
}

TEST(Player, IsSurvival)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_TRUE(player.isSurvival());

    player.setGameMode(GameMode::Creative);
    EXPECT_FALSE(player.isSurvival());
}

TEST(Player, IsAdventure)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_FALSE(player.isAdventure());

    player.setGameMode(GameMode::Adventure);
    EXPECT_TRUE(player.isAdventure());
}

TEST(Player, Experience)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.experienceLevel(), 0);
    EXPECT_FLOAT_EQ(player.experienceProgress(), 0.0f);

    player.addExperience(10);
    EXPECT_GT(player.experienceLevel(), 0);

    player.setExperienceLevel(10);
    EXPECT_EQ(player.experienceLevel(), 10);
}

TEST(Player, ExperienceBarCapacity)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // Level 0-14: 7 + level * 2
    player.setExperienceLevel(0);
    EXPECT_EQ(player.experienceBarCapacity(), 7);

    player.setExperienceLevel(5);
    EXPECT_EQ(player.experienceBarCapacity(), 17);

    // Level 15-29: 37 + (level - 15) * 5
    player.setExperienceLevel(15);
    EXPECT_EQ(player.experienceBarCapacity(), 37);

    player.setExperienceLevel(20);
    EXPECT_EQ(player.experienceBarCapacity(), 62);

    // Level 30+: 112 + (level - 30) * 9
    player.setExperienceLevel(30);
    EXPECT_EQ(player.experienceBarCapacity(), 112);
}

TEST(Player, Food)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 5.0f);

    player.foodStats().addExhaustion(10.0f);
    // 4次消耗触发，每次消耗1饱和度或1饥饿值
    // 注意：消耗是异步的，通过 tick 处理，所以这里只检查不会超过最大值

    player.foodStats().addStats(5, 3.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20); // 最大20
}

TEST(FoodStats, ExhaustionConsumption)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 初始状态：foodLevel=20, saturation=5.0
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 5.0f);

    // 对齐 MC Java 1.21.11 FoodData.tick:35（exhaustionLevel > 4.0F 严格大于才触发，
    // 单次 if 非 while——一次 tick 最多扣一次 4.0，残留留待后续 tick）。
    // addExhaustion(4.0) 后 exhaustion=4.0，严格大于 4.0 不成立，不消耗。
    player.foodStats().addExhaustion(4.0f);
    player.foodStats().tick(player, Difficulty::Normal, false);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 5.0f); // 边界 4.0 不触发，饱和度不变
    EXPECT_EQ(player.foodStats().foodLevel(), 20);

    // 累积到 20.0（残留 4.0 + 新 16.0），严格大于 4.0 成立，扣 4.0 → exhaustion=16.0，
    // saturation>0 扣 1 → saturation=4.0。单次 tick 只扣一次（残留 16.0 留待后续 tick）。
    player.foodStats().addExhaustion(16.0f);
    player.foodStats().tick(player, Difficulty::Normal, false);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 4.0f); // 单次 tick 只扣 1 点饱和度
    EXPECT_EQ(player.foodStats().foodLevel(), 20);

    // 再加 4.0 → exhaustion=20.0，扣 4.0 → exhaustion=16.0，saturation(4.0)>0 扣 1 → saturation=3.0。
    // saturation 仍未归零，故 foodLevel 不变（1.21.11 优先消耗饱和度，saturation>0 时不扣 foodLevel）。
    player.foodStats().addExhaustion(4.0f);
    player.foodStats().tick(player, Difficulty::Normal, false);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 3.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20); // saturation 未归零，foodLevel 不变
}

TEST(FoodStats, SaturationCalculation)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 重置到低饥饿值
    player.foodStats().setFoodLevel(10);
    player.foodStats().setSaturationLevel(0.0f);

    // 吃苹果：food=4, modifier=0.3
    // saturation = 4 * 0.3 * 2.0 = 2.4
    player.foodStats().addStats(4, 0.3f);
    EXPECT_EQ(player.foodStats().foodLevel(), 14);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 2.4f);

    // 吃熟牛排：food=8, modifier=0.8
    // saturation = 2.4 + 8 * 0.8 * 2.0 = 2.4 + 12.8 = 15.2
    // 饥饿值上限 20，饱和度上限为 foodLevel (20)，但实际计算结果为 15.2
    player.foodStats().addStats(8, 0.8f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 15.2f);
}

TEST(FoodStats, NeedsFood)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 饱食时不需要食物
    EXPECT_FALSE(player.foodStats().needsFood());

    // 饥饿值降低后需要食物
    player.foodStats().setFoodLevel(15);
    EXPECT_TRUE(player.foodStats().needsFood());

    player.foodStats().setFoodLevel(0);
    EXPECT_TRUE(player.foodStats().needsFood());
}

TEST(FoodStats, FoodTimer)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.foodStats().foodTimer(), 0);

    player.foodStats().setFoodTimer(50);
    EXPECT_EQ(player.foodStats().foodTimer(), 50);
}

TEST(FoodStats, PrevFoodLevel)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.foodStats().prevFoodLevel(), 20);

    player.foodStats().setFoodLevel(15);
    // prevFoodLevel 在 tick() 中更新，不在 setFoodLevel 中更新
    EXPECT_EQ(player.foodStats().prevFoodLevel(), 20);
}

TEST(FoodStats, ExhaustionCap)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 消耗值上限为 40.0
    player.foodStats().addExhaustion(50.0f);
    EXPECT_FLOAT_EQ(player.foodStats().exhaustionLevel(), 40.0f);
}

TEST(FoodStats, FastRegeneration)
{
    // 快速恢复条件：foodLevel >= 20 且 saturation > 0，每 10 ticks 恢复 saturation/6 生命
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 设置满饥饿和饱和度
    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f); // 受伤状态

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 6.0f);
    EXPECT_FLOAT_EQ(player.health(), 10.0f);

    // tick() 需要调用多次触发快速恢复 (每 10 ticks)
    // 注意：tick() 需要 Player 实例，且会检查是否有饥饿效果
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证生命恢复
    EXPECT_GT(player.health(), 10.0f);                     // 生命应该恢复
    EXPECT_LT(player.foodStats().saturationLevel(), 6.0f); // 饱和度应该消耗
}

TEST(FoodStats, SlowRegeneration)
{
    // 慢速恢复条件：foodLevel >= 18 且 saturation == 0，每 80 ticks 恢复 1 生命
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 设置高饥饿值但无饱和度
    player.foodStats().setFoodLevel(18);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(15.0f);

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 18);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 0.0f);
    EXPECT_FLOAT_EQ(player.health(), 15.0f);

    // tick() 需要调用多次触发慢速恢复 (每 80 ticks)
    for (int i = 0; i < 85; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证生命恢复
    EXPECT_GT(player.health(), 15.0f); // 生命应该恢复
    // 慢速恢复会添加 6.0 消耗值，导致饥饿值下降 1 点（18 -> 17）
    EXPECT_EQ(player.foodStats().foodLevel(), 17);
}

TEST(FoodStats, StarvationDamage)
{
    // 饥饿伤害条件：foodLevel <= 0，每 80 ticks 造成 1 点伤害
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 设置零饥饿值
    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(20.0f);

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 0);
    EXPECT_FLOAT_EQ(player.health(), 20.0f);

    // tick() 需要调用多次触发饥饿伤害 (每 80 ticks)
    for (int i = 0; i < 85; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证饥饿伤害
    EXPECT_LT(player.health(), 20.0f); // 应该受到伤害
}

TEST(FoodStats, StarvationDamageEasyMode)
{
    // 简单模式：饥饿伤害最低保留 10 点生命
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(12.0f); // 12 点生命

    // 触发多次饥饿伤害
    for (int i = 0; i < 170; ++i) { // 足够触发多次伤害
        player.foodStats().tick(player, Difficulty::Easy, true);
    }

    // 简单模式：生命最低 10
    EXPECT_GE(player.health(), 10.0f);
}

TEST(FoodStats, StarvationDamageNormalMode)
{
    // 普通模式：饥饿伤害最低保留 1 点生命
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(5.0f);

    // 触发多次饥饿伤害
    for (int i = 0; i < 500; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 普通模式：生命最低 1
    EXPECT_GE(player.health(), 1.0f);
}

TEST(FoodStats, PeacefulMode)
{
    // 对齐 MC Java 1.21.11 FoodData.tick:44-71：和平模式**无自动恢复特例**。
    // 回血门控为 flag(naturalRegeneration) && (saturation>0 && foodLevel>=20 || foodLevel>=18) && isHurt，
    // 不区分难度——和平模式 foodLevel=10 不满足任一回血分支（<18），走 else 归零计时器，
    // 不回血、不回饥饿值。1.16.5 的"和平模式自动回血/回饥饿"特例在 1.21.11 已移除。
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(10);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(15.0f);

    // 和平模式 tick（naturalRegeneration=true 模拟 NATURAL_HEALTH_REGENERATION 游戏规则开启）
    for (int i = 0; i < 25; ++i) {
        player.foodStats().tick(player, Difficulty::Peaceful, true);
    }

    // 和平模式 foodLevel=10 不满足回血分支（需 >=18 慢速或 >=20 快速），不回血
    EXPECT_FLOAT_EQ(player.health(), 15.0f);
    // 和平模式不自动恢复饥饿值（1.21.11 无此特例）
    EXPECT_EQ(player.foodStats().foodLevel(), 10);
}

TEST(FoodStats, PeacefulModeNoStarvation)
{
    // 对齐 MC Java 1.21.11 FoodData.tick:60-68：和平模式 foodLevel<=0 走饿死分支，
    // tickTimer 攒到 80 触发，但伤害门控 getHealth()>10 || HARD || getHealth()>1 && NORMAL
    // 和平模式不满足任一条件→不造成伤害（tickTimer 仍归零）。且和平模式不自动恢复饥饿值。
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(20.0f);

    // 和平模式 tick 多次（远超 80 tick 饿死间隔）
    for (int i = 0; i < 200; ++i) {
        player.foodStats().tick(player, Difficulty::Peaceful, true);
    }

    // 和平模式：不会受到饥饿伤害
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    // 和平模式不自动恢复饥饿值（1.21.11 无此特例，foodLevel 保持 0）
    EXPECT_EQ(player.foodStats().foodLevel(), 0);
}

TEST(FoodStats, NoRegenerationWithHungerEffect)
{
    // 有饥饿效果时不恢复生命
    // 注意：此测试需要 Player 支持 addEffect() 方法
    // 目前仅验证基础逻辑框架
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f);

    // 正常情况下应该恢复
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证在无饥饿效果时可以恢复
    EXPECT_GT(player.health(), 10.0f);
}

TEST(FoodStats, NaturalRegenerationDisabled)
{
    // naturalRegeneration=false 时不应恢复生命
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f);

    // naturalRegeneration=false
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, false);
    }

    // 禁用自然恢复时，生命不应恢复
    EXPECT_FLOAT_EQ(player.health(), 10.0f);
}

TEST(Player, PoseHeight)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 站立
    player.setPose(EntityPose::Standing);
    EXPECT_FLOAT_EQ(player.height(), 1.8f);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 1.62f);

    // 潜行
    player.setPose(EntityPose::Crouching);
    EXPECT_FLOAT_EQ(player.height(), 1.5f);

    // 游泳
    player.setPose(EntityPose::Swimming);
    EXPECT_FLOAT_EQ(player.height(), 0.6f);

    // 睡觉
    player.setPose(EntityPose::Sleeping);
    EXPECT_FLOAT_EQ(player.height(), 0.2f);
}

TEST(Player, SprintingSneaking)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    player.setSprinting(true);
    EXPECT_TRUE(player.isSprinting());
    EXPECT_TRUE(player.hasFlag(EntityFlags::Sprinting));

    player.setSprinting(false);
    EXPECT_FALSE(player.isSprinting());

    player.setSneaking(true);
    EXPECT_TRUE(player.isSneaking());
    EXPECT_TRUE(player.hasFlag(EntityFlags::Crouching));
    EXPECT_EQ(player.pose(), EntityPose::Crouching);
}

TEST(Player, Respawn)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    auto genericSource = DamageSources::generic();
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());

    player.respawn();
    EXPECT_FALSE(player.isDead());
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
}

TEST(Player, LastDeathLocationDieSetsPosition)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(100.5f, 64.0f, -200.25f);
    player.setDimension(0); // 主世界

    // 死亡前应该没有记录
    EXPECT_FALSE(player.getLastDeathLocation().has_value());

    // 致命伤害触发死亡
    auto genericSource = DamageSources::generic();
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());

    // 死亡后应该记录位置
    auto deathLoc = player.getLastDeathLocation();
    ASSERT_TRUE(deathLoc.has_value());
    EXPECT_EQ(deathLoc->getDimensionId(), 0); // 主世界
    // onPos() 返回脚下方块位置：floor(y) - 1
    EXPECT_EQ(deathLoc->x(), 100);
    EXPECT_EQ(deathLoc->z(), -201);
}

TEST(Player, LastDeathLocationSetterGetter)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 初始状态应该没有记录
    EXPECT_FALSE(player.getLastDeathLocation().has_value());

    // 设置死亡位置
    GlobalPos deathPos(-1, BlockPos(50, 30, -100)); // 下界
    player.setLastDeathLocation(deathPos);
    ASSERT_TRUE(player.getLastDeathLocation().has_value());
    EXPECT_EQ(player.getLastDeathLocation()->getDimensionId(), -1);
    EXPECT_EQ(player.getLastDeathLocation()->x(), 50);
    EXPECT_EQ(player.getLastDeathLocation()->y(), 30);
    EXPECT_EQ(player.getLastDeathLocation()->z(), -100);

    // 清除死亡位置
    player.setLastDeathLocation(std::nullopt);
    EXPECT_FALSE(player.getLastDeathLocation().has_value());
}

TEST(Player, LastDeathLocationNbtSerialization)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(100.5f, 64.0f, -200.25f);
    player.setDimension(1); // 末地

    // 触发死亡以设置 lastDeathLocation
    auto genericSource = DamageSources::generic();
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());
    ASSERT_TRUE(player.getLastDeathLocation().has_value());
    EXPECT_EQ(player.getLastDeathLocation()->getDimensionId(), 1);

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    player.addAdditionalSaveData(tag);

    // 验证 LastDeathLocation 在 NBT 中
    using namespace mc::entity::serialization;
    auto* deathTag = nbt_helper::tryGetCompound(tag, nbt_keys::LAST_DEATH_LOCATION);
    ASSERT_NE(deathTag, nullptr);

    auto dimStr = nbt_helper::tryGetString(*deathTag, nbt_keys::LAST_DEATH_LOCATION_DIMENSION);
    EXPECT_TRUE(dimStr.has_value());
    if (dimStr.has_value()) {
        EXPECT_EQ(*dimStr, "minecraft:the_end");
    }

    auto posList = nbt_helper::getIntList(*deathTag, nbt_keys::LAST_DEATH_LOCATION_POS);
    ASSERT_GE(posList.size(), 3u);

    // 反序列化到新玩家
    Player restored(2, "RestoredPlayer", mc::test::testEcsRegistry());
    auto result = restored.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    ASSERT_TRUE(restored.getLastDeathLocation().has_value());
    EXPECT_EQ(restored.getLastDeathLocation()->getDimensionId(), 1);
    EXPECT_EQ(restored.getLastDeathLocation()->x(), posList[0]);
    EXPECT_EQ(restored.getLastDeathLocation()->y(), posList[1]);
    EXPECT_EQ(restored.getLastDeathLocation()->z(), posList[2]);
}

TEST(Player, LastDeathLocationNbtEmptyRoundTrip)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    // 不设置死亡位置

    // 序列化
    nbt::tags::compound_tag tag;
    player.addAdditionalSaveData(tag);

    // LastDeathLocation 不应该出现在 NBT 中（因为 optional 为空）
    using namespace mc::entity::serialization;
    auto* deathTag = nbt_helper::tryGetCompound(tag, nbt_keys::LAST_DEATH_LOCATION);
    EXPECT_EQ(deathTag, nullptr);

    // 反序列化
    Player restored(2, "RestoredPlayer", mc::test::testEcsRegistry());
    auto result = restored.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(restored.getLastDeathLocation().has_value());
}

TEST(Player, LastDeathLocationSecondDeathOverwrites)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(100.0f, 64.0f, -200.0f);
    player.setDimension(0); // 主世界

    // 第一次死亡
    auto genericSource = DamageSources::generic();
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());
    auto firstDeath = player.getLastDeathLocation();
    ASSERT_TRUE(firstDeath.has_value());
    EXPECT_EQ(firstDeath->getDimensionId(), 0);

    // 复活
    player.setHealth(20.0f);
    player.setHurtResistantTime(0); // 清除无敌帧，允许再次受伤
    EXPECT_FALSE(player.isDead());

    // 移动到下界位置
    player.setPosition(50.0f, 30.0f, -100.0f);
    player.setDimension(-1); // 下界

    // 第二次死亡
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());
    auto secondDeath = player.getLastDeathLocation();
    ASSERT_TRUE(secondDeath.has_value());
    // 第二次死亡位置应该覆盖第一次
    EXPECT_EQ(secondDeath->getDimensionId(), -1); // 下界
    EXPECT_EQ(secondDeath->x(), 50);
    EXPECT_EQ(secondDeath->y(), 29); // floor(30) - 1
    EXPECT_EQ(secondDeath->z(), -100);
}

TEST(Player, LastDeathLocationNetherDimension)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(200.0f, 50.0f, 300.0f);
    player.setDimension(-1); // 下界

    // 在下界死亡
    auto genericSource = DamageSources::generic();
    player.hurt(genericSource, 30.0f);
    EXPECT_TRUE(player.isDead());

    auto deathLoc = player.getLastDeathLocation();
    ASSERT_TRUE(deathLoc.has_value());
    EXPECT_EQ(deathLoc->getDimensionId(), -1);
    EXPECT_EQ(deathLoc->x(), 200);
    EXPECT_EQ(deathLoc->y(), 49); // floor(50) - 1
    EXPECT_EQ(deathLoc->z(), 300);

    // 序列化并验证 NBT 中的维度字符串
    nbt::tags::compound_tag tag;
    player.addAdditionalSaveData(tag);

    using namespace mc::entity::serialization;
    auto* deathTag = nbt_helper::tryGetCompound(tag, nbt_keys::LAST_DEATH_LOCATION);
    ASSERT_NE(deathTag, nullptr);
    auto dimStr = nbt_helper::tryGetString(*deathTag, nbt_keys::LAST_DEATH_LOCATION_DIMENSION);
    ASSERT_TRUE(dimStr.has_value());
    EXPECT_EQ(dimStr.value(), "minecraft:the_nether");
}

// ============================================================================
// Portal Timing Tests
// ============================================================================

TEST(Entity, PortalCooldown)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 初始状态：冷却为0，可以传送
    EXPECT_EQ(entity.portalCooldown(), 0);
    EXPECT_TRUE(entity.canTeleport());

    // 触发传送冷却
    entity.triggerPortalCooldown();
    EXPECT_EQ(entity.portalCooldown(), 300); // 默认冷却 300 tick (15秒)
    EXPECT_FALSE(entity.canTeleport());

    // 冷却递减
    entity.setPortalCooldown(100);
    EXPECT_EQ(entity.portalCooldown(), 100);

    // 手动设置冷却
    entity.setPortalCooldown(0);
    EXPECT_TRUE(entity.canTeleport());
}

TEST(Entity, PortalTime)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 初始状态：传送门时间为0
    EXPECT_EQ(entity.portalTime(), 0);
    EXPECT_FALSE(entity.isInPortal());

    // 设置传送门状态
    entity.setInPortal(true);
    EXPECT_TRUE(entity.isInPortal());

    // 设置传送门时间
    entity.setPortalTime(50);
    EXPECT_EQ(entity.portalTime(), 50);

    // 重置传送门时间
    entity.resetPortalTime();
    EXPECT_EQ(entity.portalTime(), 0);
}

TEST(Entity, GetMaxInPortalTime)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    // MC 1.16.5: 非玩家实体基类返回 0
    // 检查条件 portalCounter++ >= 0 第一次进入就满足
    // 实际效果：非玩家实体需要 1 tick 传送
    EXPECT_EQ(entity.getMaxInPortalTime(), 0);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    // 玩家需要 80 tick (4秒)
    EXPECT_EQ(player.getMaxInPortalTime(), 80);
}

// TODO(ECS迁移): tickPortal() 逻辑已迁入 ecs::PortalTickSystem（System 全 registry 扫描），
// 原栈实体 + 共享静态 registry 的单实体调用模式与 System 架构不兼容（UAF/污染风险）。
// 需改写为驱动 PortalTickSystem::tick(testEcsRegistry()) 并校验组件状态后恢复测试。
TEST(Entity, TickPortalNotInPortal)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setPortalTime(10);

    // 不在传送门中时，传送门时间递减
    entity.setInPortal(false);
    bool shouldTeleport = entity.tickPortal();

    EXPECT_FALSE(shouldTeleport);
    // 递减 4
    EXPECT_EQ(entity.portalTime(), 6);
#endif
}

// TODO(ECS迁移): tickPortal() 迁入 PortalTickSystem，本用例需按 System 架构重写。
TEST(Entity, TickPortalNotInPortalZero)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setPortalTime(2);

    // 传送门时间不会低于0
    entity.setInPortal(false);
    entity.tickPortal();
    EXPECT_EQ(entity.portalTime(), 0);

    // 已经是0时保持0
    entity.tickPortal();
    EXPECT_EQ(entity.portalTime(), 0);
#endif
}

// TODO(ECS迁移): tickPortal() 迁入 PortalTickSystem，本用例需按 System 架构重写。
TEST(Entity, TickPortalInPortal)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // MC 1.16.5: 非玩家实体基类 getMaxInPortalTime() 返回 0
    // 检查条件 portalCounter++ > maxPortalTime
    // 第一次进入：portalTime 从 0 变为 1，然后 1 > 0 成立
    // 实际效果：非玩家实体需要 1 tick 传送
    entity.setInPortal(true);
    entity.triggerPortalCooldown(); // 设置冷却
    entity.setPortalCooldown(0);    // 清除冷却以允许传送

    bool shouldTeleport = entity.tickPortal();
    EXPECT_TRUE(shouldTeleport);
    // 传送后 portalTime 被设置为 maxPortalTime（即 0）
    EXPECT_EQ(entity.portalTime(), 0);
#endif
}

// TODO(ECS迁移): tickPortal() 迁入 PortalTickSystem，本用例需按 System 架构重写。
TEST(Entity, TickPortalInPortalWithCooldown)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 有冷却时不能传送
    entity.setInPortal(true);
    entity.setPortalCooldown(100); // 冷却中

    // tickPortal 会重置 inPortal = false
    bool shouldTeleport = entity.tickPortal();
    EXPECT_FALSE(shouldTeleport);      // 冷却中，不传送
    EXPECT_FALSE(entity.isInPortal()); // inPortal 被重置
    EXPECT_EQ(entity.portalTime(), 0); // 时间不增加（因为冷却阻止了传送）
#endif
}

// TODO(ECS迁移): tickPortal() 迁入 PortalTickSystem，本用例需按 System 架构重写。
TEST(Entity, TickPortalPlayer)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 玩家需要 80 tick (4秒)
    // MC 1.16.5 行为：
    // 1. NetherPortalBlock.onEntityCollision 每帧设置 inPortal = true
    // 2. tickPortal 重置 inPortal = false
    // 所以要持续在传送门中，每帧都需要重新设置 inPortal = true

    player.setPortalCooldown(0);

    // 79 ticks 后不传送
    for (int i = 0; i < 79; ++i) {
        player.setInPortal(true); // 模拟 onEntityCollision 每帧设置
        bool shouldTeleport = player.tickPortal();
        EXPECT_FALSE(shouldTeleport);
        // tickPortal 内部已重置 inPortal = false
    }
    EXPECT_EQ(player.portalTime(), 79);

    // 第 80 tick 传送
    player.setInPortal(true);
    bool shouldTeleport = player.tickPortal();
    EXPECT_TRUE(shouldTeleport);
    EXPECT_EQ(player.portalTime(), 80);
#endif
}

// TODO(ECS迁移): tickPortal() 迁入 PortalTickSystem，本用例需按 System 架构重写。
TEST(Entity, TickPortalPlayerInterrupted)
{
    GTEST_SKIP() << "TODO: tickPortal 迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 玩家在传送门中 40 tick
    player.setPortalCooldown(0);
    for (int i = 0; i < 40; ++i) {
        player.setInPortal(true); // 每帧设置 inPortal
        player.tickPortal();
        // tickPortal 内部已重置 inPortal = false
    }
    EXPECT_EQ(player.portalTime(), 40);

    // 离开传送门（不再设置 inPortal）
    // tickPortal 会检测到 inPortal=false 并递减时间
    player.tickPortal();
    // 注意：循环最后一次 setInPortal(true) 后 tickPortal 重置为 false
    // 所以这里 isInPortal() 应该是 false
    EXPECT_FALSE(player.isInPortal());
    EXPECT_EQ(player.portalTime(), 36); // 40 - 4 = 36

    // 再过几帧不在传送门
    for (int i = 0; i < 5; ++i) {
        player.tickPortal();
    }
    EXPECT_EQ(player.portalTime(), 16); // 36 - 4*5 = 16

    // 再次进入传送门
    for (int i = 0; i < 10; ++i) {
        player.setInPortal(true);
        player.tickPortal();
    }
    EXPECT_EQ(player.portalTime(), 26); // 16 + 10 = 26
#endif
}

TEST(Entity, PortalPos)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    BlockPos portalPos(100, 64, 200);

    entity.setPortalPos(portalPos);
    EXPECT_EQ(entity.portalPos().x, 100);
    EXPECT_EQ(entity.portalPos().y, 64);
    EXPECT_EQ(entity.portalPos().z, 200);
}

// TODO(ECS迁移): portal 冷却递减逻辑已从 baseTick() 迁入 ecs::PortalTickSystem，
// 本用例断言 baseTick 后冷却 300→299 不再成立，需改写为驱动 PortalTickSystem::tick 校验。
TEST(Entity, TickPortalCooldownDecrement)
{
    GTEST_SKIP() << "TODO: 冷却递减迁入 PortalTickSystem 后需按 System 架构重写本用例";
#if 0
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 设置冷却
    entity.triggerPortalCooldown();
    EXPECT_EQ(entity.portalCooldown(), 300);

    // 冷却在 baseTick 中递减
    entity.baseTick();
    EXPECT_EQ(entity.portalCooldown(), 299);

    // 在 tick 中调用 baseTick
    entity.tick();
    EXPECT_EQ(entity.portalCooldown(), 298);
#endif
}

TEST(Entity, OnPortalTriggered)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 设置传送门状态
    entity.setInPortal(true);
    entity.setPortalTime(10);

    // 触发传送回调
    bool result = entity.onPortalTriggered();

    // 基类实现返回 false，但重置状态
    EXPECT_FALSE(result);
    EXPECT_FALSE(entity.isInPortal());
    EXPECT_EQ(entity.portalTime(), 0);
    EXPECT_EQ(entity.portalCooldown(), 300); // 触发冷却
}

TEST(Entity, RemoveMarksEntityAsRemoved)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isRemoved());

    entity.remove();
    EXPECT_TRUE(entity.isRemoved());
}

TEST(Entity, DiscardMarksEntityAsRemoved)
{
    // discard() 与 remove() 一样将实体标记为已移除，
    // 但不触发掉落物、经验等死亡相关逻辑。
    // 对应 MC Java 的 Entity.discard()。
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isRemoved());
    EXPECT_TRUE(entity.isAlive());

    entity.discard();
    EXPECT_TRUE(entity.isRemoved());
    EXPECT_FALSE(entity.isAlive());
}

TEST(Entity, RemoveAndDiscardBothMarkRemoved)
{
    // remove() 和 discard() 都应将 isRemoved() 设为 true、isAlive() 设为 false
    Entity entity1(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    Entity entity2(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());

    entity1.remove();
    entity2.discard();

    EXPECT_TRUE(entity1.isRemoved());
    EXPECT_TRUE(entity2.isRemoved());
    EXPECT_FALSE(entity1.isAlive());
    EXPECT_FALSE(entity2.isAlive());
}
