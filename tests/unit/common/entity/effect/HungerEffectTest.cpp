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
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
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

class HungerEffectTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<EffectTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<EffectTestWorld> m_world;
};

// ============================================================================
// 饥饿效果测试
// ============================================================================

TEST_F(HungerEffectTest, HungerEffectAddExhaustionToPlayer)
{
    // 创建玩家并设置世界
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    // 获取初始饥饿值和饱和度
    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 I (amplifier = 0)
    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 应用效果（每 tick）- 使用 tick 而不是 applyEffect
    hunger.tick(player);

    // 验证饥饿消耗增加
    // MC 1.16.5: exhaustion += 0.005F * (amplifier + 1)
    // 饥饿 I: 0.005 * 1 = 0.005
    f32 expectedExhaustion = initialExhaustion + 0.005f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectIIIncreasesExhaustionMore)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 II (amplifier = 1)
    EffectInstance hunger(EffectType::Hunger, 200, 1, false, true, true);

    hunger.tick(player);

    // 饥饿 II: 0.005 * 2 = 0.01
    f32 expectedExhaustion = initialExhaustion + 0.01f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectIIIIncreasesExhaustionMore)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 III (amplifier = 2)
    EffectInstance hunger(EffectType::Hunger, 200, 2, false, true, true);

    hunger.tick(player);

    // 饥饿 III: 0.005 * 3 = 0.015
    f32 expectedExhaustion = initialExhaustion + 0.015f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectDoesNotAffectNonPlayer)
{
    // 饥饿效果对非玩家实体不应该做任何事情
    // 这个测试验证 tick 不会崩溃
    MobEntity mob(EntityInstanceId(2), mc::test::testEcsRegistry());
    mob.setWorld(m_world.get());
    mob.setPosition(0.0f, 64.0f, 0.0f);

    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 不应该抛出异常
    EXPECT_NO_THROW(hunger.tick(mob));
}

TEST_F(HungerEffectTest, HungerEffectMultipleTicks)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 I
    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 模拟多个 tick
    for (int i = 0; i < 10; ++i) {
        hunger.tick(player);
    }

    // 10 tick 后: 0.005 * 10 = 0.05
    f32 expectedExhaustion = initialExhaustion + 0.05f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

} // namespace
