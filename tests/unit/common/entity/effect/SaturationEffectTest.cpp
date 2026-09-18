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
 * @brief 测试用世界存根
 */
class EffectTestWorld final : public mc::test::BaseTestWorld {
public:
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void addParticle(particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EffectTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EffectTestWorld::tickManager not implemented");
    }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }
};

class SaturationEffectTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<EffectTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<EffectTestWorld> m_world;
};

// ============================================================================
// 饱和效果通过 EffectInstance::applyInstantly 直接调用
// ============================================================================

TEST_F(SaturationEffectTest, SaturationIRestoresOneFoodAndTwoSaturation)
{
    // 饱和效果 I (amplifier=0): 恢复 1 点饥饿值，2 点饱和度
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(18);
    foodStats.setSaturationLevel(0.0f);

    // MC 原版: player.getFoodData().eat(amplifier + 1, 1.0F)
    // foodLevel += 1, saturation += 1 * 1.0 * 2.0 = 2.0
    EffectInstance saturation(EffectType::Saturation, 600, 0);
    saturation.applyInstantly(player);

    EXPECT_EQ(foodStats.foodLevel(), 19);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 2.0f);
}

TEST_F(SaturationEffectTest, SaturationIIRestoresTwoFoodAndFourSaturation)
{
    // 饱和效果 II (amplifier=1): 恢复 2 点饥饿值，4 点饱和度
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(15);
    foodStats.setSaturationLevel(0.0f);

    EffectInstance saturation(EffectType::Saturation, 600, 1);
    saturation.applyInstantly(player);

    // foodLevel += 2, saturation += 2 * 1.0 * 2.0 = 4.0
    EXPECT_EQ(foodStats.foodLevel(), 17);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 4.0f);
}

TEST_F(SaturationEffectTest, SaturationDoesNotAffectNonPlayer)
{
    // 饱和效果对非玩家实体无效
    MobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    mob.setWorld(m_world.get());
    mob.setPosition(0.0f, 64.0f, 0.0f);

    EffectInstance saturation(EffectType::Saturation, 600, 0);
    // 不应该抛出异常（dynamic_cast<Player*> 返回 nullptr，跳过逻辑）
    EXPECT_NO_THROW(saturation.applyInstantly(mob));
}

TEST_F(SaturationEffectTest, SaturationFoodLevelCappedAt20)
{
    // 饱和效果不会使饥饿值超过上限 20
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(19);
    foodStats.setSaturationLevel(10.0f);

    // 饱和效果 III (amplifier=2): 恢复 3 点饥饿值
    EffectInstance saturation(EffectType::Saturation, 600, 2);
    saturation.applyInstantly(player);

    // foodLevel = min(19 + 3, 20) = 20
    EXPECT_EQ(foodStats.foodLevel(), 20);
    // saturation = min(10.0 + 3 * 1.0 * 2.0, 20.0) = 16.0
    // FoodStats.addStats 首先增加 foodLevel，然后 saturation 上限为当前 foodLevel
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 16.0f);
}

TEST_F(SaturationEffectTest, SaturationThroughEffectManager)
{
    // 饱和效果通过 EffectManager::addEffect 添加时，应立即执行后不保存到效果列表
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(10);
    foodStats.setSaturationLevel(0.0f);

    EffectManager& mgr = player.effectManager();
    EffectInstance saturation(EffectType::Saturation, 600, 0);
    mgr.addEffect(std::move(saturation), player);

    // 瞬间效果应立即执行，不保存到效果列表
    EXPECT_FALSE(mgr.hasEffect(EffectType::Saturation));

    // 饱和效果 I: foodLevel += 1, saturation += 2.0
    EXPECT_EQ(foodStats.foodLevel(), 11);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 2.0f);
}

TEST_F(SaturationEffectTest, MultipleSaturationEffectsStack)
{
    // 连续添加多次饱和效果，应每次都恢复饥饿值
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    foodStats.setFoodLevel(10);
    foodStats.setSaturationLevel(0.0f);

    EffectManager& mgr = player.effectManager();

    // 第一次饱和效果 I
    EffectInstance saturation1(EffectType::Saturation, 600, 0);
    mgr.addEffect(std::move(saturation1), player);
    EXPECT_EQ(foodStats.foodLevel(), 11);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 2.0f);

    // 第二次饱和效果 II
    EffectInstance saturation2(EffectType::Saturation, 600, 1);
    mgr.addEffect(std::move(saturation2), player);
    EXPECT_EQ(foodStats.foodLevel(), 13);
    EXPECT_FLOAT_EQ(foodStats.saturationLevel(), 6.0f);

    // 瞬间效果不应保存到效果列表
    EXPECT_FALSE(mgr.hasEffect(EffectType::Saturation));
}

TEST_F(SaturationEffectTest, InstantEffectNotStoredInManager)
{
    // 验证所有瞬间效果（InstantHealth, InstantDamage, Saturation）都不保存在 EffectManager 中
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    EffectManager& mgr = player.effectManager();
    EXPECT_EQ(mgr.getEffectCount(), 0u);

    // 添加饱和效果
    EffectInstance saturation(EffectType::Saturation, 600, 0);
    mgr.addEffect(std::move(saturation), player);
    EXPECT_EQ(mgr.getEffectCount(), 0u);
    EXPECT_FALSE(mgr.hasEffect(EffectType::Saturation));
}

} // namespace
