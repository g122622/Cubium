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

// FoodStats 饥饿系统 tick 行为单元测试。
//
// 对齐 MC Java 1.21.11 FoodData.tick（FoodData.java:32-72）。覆盖此前零测试的 tick 链路，
// 并锚定任务 #351 修复的三个偏离 vanilla 的缺陷：
//   1. _consumeExhaustion 用单次 if + 严格 > 4.0（对齐 FoodData.java:35），非旧 while + >=。
//      —— 旧实现一次 tick 扣多点 saturation（消耗过快）+ 边界 4.0 误扣。
//   2. 回血门控移除 !hasHungerEffect（对齐 FoodData.java:45/53 不查 Hunger 效果）。
//      —— 旧实现满饱但有 Hunger 效果时不回血，偏离 vanilla（Hunger 仅加速 exhaustion）。
//   3. 单一 m_foodTimer 共享回血/饿死/else-reset 三分支（对齐 vanilla 单 tickTimer）。
//      —— 旧实现用独立 m_starveTimer，且和平模式有 _handlePeacefulMode 特例（已移除）。
//
// 回归保护：慢回血 80 tick / 饿死 80 tick + 难度血量下限门控。
//
// Ref: src/common/entity/food/FoodStats.cpp（tick / _consumeExhaustion / _perform*）
// Ref: D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\world\food\FoodData.java:32-72

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectManager.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/food/FoodStats.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::effect;

namespace {

/**
 * @brief 可配置难度的测试用世界存根
 *
 * BaseTestWorld::difficulty() 默认返回 Easy，本存根允许按测试配置难度，
 * 以覆盖饿死伤害的难度血量下限门控（Easy=10 / Normal=1 / Hard=0 / Peaceful=不饿）。
 */
class FoodStatsTestWorld final : public mc::test::BaseTestWorld {
public:
    explicit FoodStatsTestWorld(Difficulty difficulty)
        : m_difficulty(difficulty)
    {}

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void addParticle(particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FoodStatsTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FoodStatsTestWorld::tickManager not implemented");
    }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

private:
    Difficulty m_difficulty;
};

class FoodStatsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 默认 Normal 难度（饿死血量下限 1，回血不依赖难度）。个别测试自建世界覆盖难度。
        m_world = std::make_unique<FoodStatsTestWorld>(Difficulty::Normal);
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<FoodStatsTestWorld> m_world;
};

// ============================================================================
// 缺陷 1：exhaustion 单次扣减（对齐 FoodData.java:35 单次 if + 严格 >）
// ============================================================================

// exhaustion=8.0 时，一次 tick 只扣 1 点 saturation（vanilla 单次 if）。
// 旧实现 while+>= 会扣 2 点 saturation（8→4→0 两次循环），消耗过快。
TEST_F(FoodStatsTest, ExhaustionConsumesAtMostOneSaturationPerTick)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(5.0f);
    foodStats.setExhaustionLevel(8.0f);
    // 满血满饱：不触发回血/饿死，tick 只走 _consumeExhaustion，干净验证消耗逻辑。
    player.setHealth(20.0f);

    foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);

    // vanilla: 8.0 > 4.0 触发一次，exhaustion -= 4.0 = 4.0；saturation 5→4。
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), 4.0f);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 4.0f);
}

// exhaustion 恰好 4.0（边界）：vanilla 严格 > 4.0 不触发，saturation/exhaustion 保持。
// 旧实现 >= 4.0 会触发扣 1 点 saturation（边界误扣）。
TEST_F(FoodStatsTest, ExhaustionBoundaryStrictlyGreaterThanDoesNotConsume)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(5.0f);
    foodStats.setExhaustionLevel(4.0f);
    player.setHealth(20.0f);

    foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);

    // vanilla: 4.0 > 4.0 为假，不触发，exhaustion 保持 4.0，saturation 保持 5.0。
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), 4.0f);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 5.0f);
}

// exhaustion=8.0 但 saturation=0（非和平）：一次 tick 只扣 1 点 foodLevel（vanilla 单次 if）。
// 旧实现 while 会扣 2 点 foodLevel。
TEST_F(FoodStatsTest, ExhaustionConsumesAtMostOneFoodLevelPerTick)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(0.0f);
    foodStats.setExhaustionLevel(8.0f);
    player.setHealth(20.0f);

    foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);

    // vanilla: 8.0 > 4.0 触发一次，exhaustion = 4.0；saturation=0 故扣 foodLevel 20→19。
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), 4.0f);
    EXPECT_EQ(foodStats.foodLevel(), 19);
}

// 和平难度下 saturation=0 时 exhaustion 不扣 foodLevel（对齐 FoodData.java:39）。
TEST_F(FoodStatsTest, PeacefulDoesNotConsumeFoodLevelWhenSaturationZero)
{
    auto peacefulWorld = std::make_unique<FoodStatsTestWorld>(Difficulty::Peaceful);
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(peacefulWorld.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(0.0f);
    foodStats.setExhaustionLevel(8.0f);
    player.setHealth(20.0f);

    foodStats.tick(player, Difficulty::Peaceful, /*naturalRegeneration=*/true);

    // 和平：exhaustion 仍扣 4.0（8→4），但 foodLevel 不扣（vanilla 第 39 行 PEACEFUL 门控）。
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), 4.0f);
    EXPECT_EQ(foodStats.foodLevel(), 20);
}

// ============================================================================
// 缺陷 2：Hunger 效果不阻止回血（对齐 FoodData.java:45/53 不查 Hunger）
// ============================================================================

// 满饱(food=20, sat=6) + 受伤(health=10) + Hunger 效果 + naturalRegen=true：
// vanilla 每 10 tick 快回血 min(6,6)/6=1 HP。Hunger 效果不阻止回血。
// 旧实现 !hasHungerEffect 门控会阻止回血（health 保持 10）。
TEST_F(FoodStatsTest, HungerEffectDoesNotBlockFastRegeneration)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(6.0f);
    foodStats.setExhaustionLevel(0.0f);
    player.setHealth(10.0f);

    // 施加 Hunger 效果（amplifier=0，duration=600）。
    EffectInstance hunger(EffectType::Hunger, 600, 0, false, true, true);
    player.effectManager().addEffect(std::move(hunger), player);
    ASSERT_TRUE(player.hasEffect(EffectType::Hunger));

    // 跑 10 tick：快回血分支每 10 tick 触发一次（timer 从 0 累积到 10）。
    // 注意：Hunger 效果每 tick addExhaustion 0.005，10 tick 累积 0.05（远未到 4.0），
    //       不干扰 saturation，快回血仍用 sat=6 回 1 HP。
    for (int i = 0; i < 10; ++i) {
        foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    }

    // vanilla：10 tick 后快回血触发，health 10→11（heal(6/6=1.0)）。
    // 旧实现：!hasHungerEffect 阻止回血，health 保持 10。
    EXPECT_FLOAT_EQ(player.health(), 11.0f);
}

// 慢回血分支同样不查 Hunger 效果：food=19(>=18) sat=0 受伤 + Hunger，80 tick 回 1 HP。
TEST_F(FoodStatsTest, HungerEffectDoesNotBlockSlowRegeneration)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(19);
    foodStats.setSaturationLevel(0.0f);
    foodStats.setExhaustionLevel(0.0f);
    player.setHealth(10.0f);

    EffectInstance hunger(EffectType::Hunger, 600, 0, false, true, true);
    player.effectManager().addEffect(std::move(hunger), player);
    ASSERT_TRUE(player.hasEffect(EffectType::Hunger));

    // 80 tick 慢回血（timer 0→80）。Hunger 每 tick 0.005 exhaustion，80 tick=0.4（<4.0 不扣 saturation/food）。
    // 但慢回血每 80 tick addExhaustion(6.0) 会触发消耗——此处只验证首次 80 tick 回血发生。
    for (int i = 0; i < 80; ++i) {
        foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    }

    // vanilla：80 tick 慢回血触发，health 10→11。旧实现被 !hasHungerEffect 阻止，health=10。
    EXPECT_FLOAT_EQ(player.health(), 11.0f);
}

// ============================================================================
// 回归保护：慢回血 / 饿死的 tick 周期与难度门控
// ============================================================================

// 慢回血 80 tick 周期：food=19(>=18) sat=0 受伤，79 tick 不回血，80 tick 回 1 HP。
TEST_F(FoodStatsTest, SlowRegenerationHealsAtEightyTicks)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(19);
    foodStats.setSaturationLevel(0.0f);
    foodStats.setExhaustionLevel(0.0f);
    player.setHealth(10.0f);

    for (int i = 0; i < 79; ++i) {
        foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    }
    EXPECT_FLOAT_EQ(player.health(), 10.0f) << "79 ticks should not yet heal";

    foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    EXPECT_FLOAT_EQ(player.health(), 11.0f) << "80th tick should heal 1 HP";
}

// 饿死 80 tick 周期 + Easy 难度血量下限 10：food=0 health=20，80 tick 扣到 19；
// 继续饿到 health=10 后不再扣（Easy 下限 10）。
//
// 测试夹具说明：本测试只调 foodStats.tick，不调 player.tick，导致 hurt 后设置的无敌帧
// （m_hurtResistantTime=20）不会自然递减，后续 starve 伤害会被无敌帧吞掉（amount<=m_lastDamage）。
// 真实游戏里 player.tick 每帧递减 m_hurtResistantTime，饿死每 80 tick 一次远超 20 tick 无敌帧，
// 故每次饿死时无敌帧早已归零。此处每 tick 前 setHurtResistantTime(0) 显式模拟该归零，
// 聚焦验证 FoodStats 的饿死周期与难度血量下限门控，而非 hurt 管线。
TEST_F(FoodStatsTest, StarvationDamagesAtEightyTicksAndRespectsEasyMinHealth)
{
    auto easyWorld = std::make_unique<FoodStatsTestWorld>(Difficulty::Easy);
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(easyWorld.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(0);
    foodStats.setSaturationLevel(0.0f);
    foodStats.setExhaustionLevel(0.0f);
    player.setHealth(20.0f);

    // 80 tick 饿死一次：health 20→19（Easy health>10 扣）。
    for (int i = 0; i < 80; ++i) {
        player.setHurtResistantTime(0);
        foodStats.tick(player, Difficulty::Easy, /*naturalRegeneration=*/false);
    }
    EXPECT_FLOAT_EQ(player.health(), 19.0f) << "80 ticks starvation should deal 1 damage";

    // 继续饿到 health=10（再扣 9 次 = 9*80=720 tick）。
    for (int i = 0; i < 9 * 80; ++i) {
        player.setHurtResistantTime(0);
        foodStats.tick(player, Difficulty::Easy, /*naturalRegeneration=*/false);
    }
    EXPECT_FLOAT_EQ(player.health(), 10.0f) << "Easy starvation floor is 10 HP";

    // 再饿 80 tick：health==10 不 > 10，不再扣（Easy 下限）。
    for (int i = 0; i < 80; ++i) {
        player.setHurtResistantTime(0);
        foodStats.tick(player, Difficulty::Easy, /*naturalRegeneration=*/false);
    }
    EXPECT_FLOAT_EQ(player.health(), 10.0f) << "Easy starvation should not drop below 10 HP";
}

// ============================================================================
// 缺陷 3：单一计时器（对齐 vanilla 单 tickTimer）
// ============================================================================

// 单计时器在状态切换时归零：满饱回血积累 timer=5 后，foodLevel 降到 17（<18 不回血也不饿），
// 走 else 分支归零 timer。再回到满饱回血时从 0 重新积累（非从 5 继续）。
TEST_F(FoodStatsTest, SingleTimerResetsOnStateChangeToElse)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(20);
    foodStats.setSaturationLevel(6.0f);
    foodStats.setExhaustionLevel(0.0f);
    player.setHealth(10.0f);

    // 5 tick 满饱快回血：timer 累积到 5（未到 10 不回血）。
    for (int i = 0; i < 5; ++i) {
        foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    }
    EXPECT_EQ(foodStats.foodTimer(), 5);

    // foodLevel 降到 17（1..17 区间，非回血非饿死）→ else 归零 timer。
    foodStats.setFoodLevel(17);
    foodStats.tick(player, Difficulty::Normal, /*naturalRegeneration=*/true);
    EXPECT_EQ(foodStats.foodTimer(), 0) << "timer should reset when entering else branch";
}

} // namespace
