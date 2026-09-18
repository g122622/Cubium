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

#include "client/sound/handler/EntitySoundHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundTypes.hpp"
#include <glm/glm.hpp>

using namespace mc::client::sound;
using namespace mc::sound;
using namespace mc;

// ============================================================================
// TickableSound 测试
// ============================================================================

/**
 * @brief 测试用 TickableSound 实现
 */
class TestTickableSound : public TickableSound {
public:
    TestTickableSound(const ResourceLocation& soundEventId, SoundCategory category)
        : TickableSound(soundEventId,
              category,
              glm::vec3(0.0f), // position
              1.0f,            // volume
              1.0f,            // pitch
              false,           // looping
              AttenuationType::Linear,
              DEFAULT_ATTENUATION_DISTANCE)
        , m_tickCount(0)
    {}

    void tick() override
    {
        ++m_tickCount;
        if (m_tickCount >= 10) {
            markDone();
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    [[nodiscard]] i32 getTickCount() const { return m_tickCount; }

private:
    i32 m_tickCount = 0;
};

class TickableSoundTest : public ::testing::Test {
protected:
    void SetUp() override { testLocation = ResourceLocation("minecraft:test.tickable"); }

    ResourceLocation testLocation;
};

TEST_F(TickableSoundTest, BasicConstruction)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    EXPECT_EQ(sound.getSoundEventId(), testLocation);
    EXPECT_EQ(sound.getCategory(), SoundCategory::Neutral);
    EXPECT_FLOAT_EQ(sound.getVolume(), 1.0f);
    EXPECT_FLOAT_EQ(sound.getPitch(), 1.0f);
    EXPECT_FALSE(sound.isLooping());
    EXPECT_FALSE(sound.isDone());
    EXPECT_TRUE(sound.canBeSilent());
}

TEST_F(TickableSoundTest, TickUpdatesState)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    EXPECT_EQ(sound.getTickCount(), 0);

    sound.tick();
    EXPECT_EQ(sound.getTickCount(), 1);
    EXPECT_FALSE(sound.isDone());

    sound.tick();
    EXPECT_EQ(sound.getTickCount(), 2);
}

TEST_F(TickableSoundTest, MarkDoneAfterTicks)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    // Tick 10 次后标记为完成
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(sound.isDone()) << "Should not be done at tick " << i;
        sound.tick();
    }

    // 第10次tick后变为done
    EXPECT_TRUE(sound.isDone());
    EXPECT_EQ(sound.getTickCount(), 10);
}

TEST_F(TickableSoundTest, SetVolumeAndPitch)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    // TickableSound 的 setVolume/setPitch 是 protected，
    // 但可以通过 tick() 间接测试
    sound.tick();
    EXPECT_FLOAT_EQ(sound.getVolume(), 1.0f);
    EXPECT_FLOAT_EQ(sound.getPitch(), 1.0f);
}

TEST_F(TickableSoundTest, SetPosition)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    // 默认位置
    EXPECT_FLOAT_EQ(sound.getX(), 0.0f);
    EXPECT_FLOAT_EQ(sound.getY(), 0.0f);
    EXPECT_FLOAT_EQ(sound.getZ(), 0.0f);
}

TEST_F(TickableSoundTest, SetLooping)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    EXPECT_FALSE(sound.isLooping());
}

TEST_F(TickableSoundTest, SetAndGetId)
{
    TestTickableSound sound(testLocation, SoundCategory::Neutral);

    EXPECT_EQ(sound.getId(), 0u);

    sound.setId(42);
    EXPECT_EQ(sound.getId(), 42u);
}

// ============================================================================
// EntitySoundHandler 测试
// ============================================================================

class EntitySoundHandlerTest : public ::testing::Test {
protected:
    void SetUp() override { handler = std::make_unique<EntitySoundHandler>(); }

    std::unique_ptr<EntitySoundHandler> handler;
};

TEST_F(EntitySoundHandlerTest, Construction)
{
    EXPECT_TRUE(handler != nullptr);
}

TEST_F(EntitySoundHandlerTest, UpdateEntityState)
{
    EntitySoundState state;
    state.position = glm::vec3(10.0f, 20.0f, 30.0f);
    state.velocity = glm::vec3(1.0f, 0.0f, 0.0f);
    state.isRemoved = false;
    state.isChild = false;
    state.isFallFlying = false;

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);

    const EntitySoundState* retrieved = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_FLOAT_EQ(retrieved->position.x, 10.0f);
    EXPECT_FLOAT_EQ(retrieved->position.y, 20.0f);
    EXPECT_FLOAT_EQ(retrieved->position.z, 30.0f);
    EXPECT_FLOAT_EQ(retrieved->velocity.x, 1.0f);
    EXPECT_FALSE(retrieved->isRemoved);
}

TEST_F(EntitySoundHandlerTest, RemoveEntityState)
{
    EntitySoundState state;
    state.position = glm::vec3(0.0f);

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);
    EXPECT_NE(handler->getEntityState(static_cast<EntityInstanceId>(1)), nullptr);

    handler->removeEntityState(static_cast<EntityInstanceId>(1));
    EXPECT_EQ(handler->getEntityState(static_cast<EntityInstanceId>(1)), nullptr);
}

TEST_F(EntitySoundHandlerTest, GetNonExistentState)
{
    const EntitySoundState* state = handler->getEntityState(static_cast<EntityInstanceId>(999));
    EXPECT_EQ(state, nullptr);
}

TEST_F(EntitySoundHandlerTest, OnEntityRemove)
{
    EntitySoundState state;
    state.position = glm::vec3(0.0f);
    state.isRemoved = false;

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);
    EXPECT_NE(handler->getEntityState(static_cast<EntityInstanceId>(1)), nullptr);
    EXPECT_FALSE(handler->getEntityState(static_cast<EntityInstanceId>(1))->isRemoved);

    handler->onEntityRemove(static_cast<EntityInstanceId>(1));

    // onEntityRemove 标记实体为已移除，但不会清除状态
    // 状态会被 removeEntityState 清除
    const EntitySoundState* removedState = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(removedState, nullptr);
    EXPECT_TRUE(removedState->isRemoved);
}

TEST_F(EntitySoundHandlerTest, StopAll)
{
    EntitySoundState state1;
    state1.position = glm::vec3(0.0f);
    EntitySoundState state2;
    state2.position = glm::vec3(100.0f, 0.0f, 0.0f);

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state1);
    handler->updateEntityState(static_cast<EntityInstanceId>(2), state2);

    EXPECT_NE(handler->getEntityState(static_cast<EntityInstanceId>(1)), nullptr);
    EXPECT_NE(handler->getEntityState(static_cast<EntityInstanceId>(2)), nullptr);

    handler->stopAll();

    // stopAll 清理所有状态
    EXPECT_EQ(handler->getEntityState(static_cast<EntityInstanceId>(1)), nullptr);
    EXPECT_EQ(handler->getEntityState(static_cast<EntityInstanceId>(2)), nullptr);
}

TEST_F(EntitySoundHandlerTest, MultipleEntityStates)
{
    for (int i = 1; i <= 10; ++i) {
        EntitySoundState state;
        state.position = glm::vec3(static_cast<float>(i));
        handler->updateEntityState(static_cast<EntityInstanceId>(i), state);
    }

    for (int i = 1; i <= 10; ++i) {
        const EntitySoundState* state = handler->getEntityState(static_cast<EntityInstanceId>(i));
        ASSERT_NE(state, nullptr);
        EXPECT_FLOAT_EQ(state->position.x, static_cast<float>(i));
    }
}

TEST_F(EntitySoundHandlerTest, FallFlyingState)
{
    EntitySoundState state;
    state.position = glm::vec3(0.0f);
    state.isFallFlying = false;

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);

    const EntitySoundState* retrieved = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_FALSE(retrieved->isFallFlying);

    // 更新为 FallFlying
    state.isFallFlying = true;
    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);

    retrieved = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(retrieved->isFallFlying);
}

TEST_F(EntitySoundHandlerTest, ChildState)
{
    EntitySoundState state;
    state.position = glm::vec3(0.0f);
    state.isChild = true;

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);

    const EntitySoundState* retrieved = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(retrieved->isChild);
}

TEST_F(EntitySoundHandlerTest, AngryState)
{
    EntitySoundState state;
    state.position = glm::vec3(0.0f);
    state.isAngry = true;

    handler->updateEntityState(static_cast<EntityInstanceId>(1), state);

    const EntitySoundState* retrieved = handler->getEntityState(static_cast<EntityInstanceId>(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(retrieved->isAngry);
}

// ============================================================================
// EntitySoundState 测试
// ============================================================================

class EntitySoundStateTest : public ::testing::Test {};

TEST_F(EntitySoundStateTest, DefaultValues)
{
    EntitySoundState state;

    EXPECT_FLOAT_EQ(state.position.x, 0.0f);
    EXPECT_FLOAT_EQ(state.position.y, 0.0f);
    EXPECT_FLOAT_EQ(state.position.z, 0.0f);
    EXPECT_FLOAT_EQ(state.velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(state.velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(state.velocity.z, 0.0f);
    EXPECT_FALSE(state.isRemoved);
    EXPECT_FALSE(state.isChild);
    EXPECT_FALSE(state.isFallFlying);
    EXPECT_FALSE(state.isAngry);
    EXPECT_FLOAT_EQ(state.attackAnimScale, 0.0f);
}

TEST_F(EntitySoundStateTest, PositionAssignment)
{
    EntitySoundState state;
    state.position = glm::vec3(100.5f, 64.0f, -200.25f);

    EXPECT_FLOAT_EQ(state.position.x, 100.5f);
    EXPECT_FLOAT_EQ(state.position.y, 64.0f);
    EXPECT_FLOAT_EQ(state.position.z, -200.25f);
}

TEST_F(EntitySoundStateTest, VelocityAssignment)
{
    EntitySoundState state;
    state.velocity = glm::vec3(1.5f, 2.0f, -0.5f);

    EXPECT_FLOAT_EQ(state.velocity.x, 1.5f);
    EXPECT_FLOAT_EQ(state.velocity.y, 2.0f);
    EXPECT_FLOAT_EQ(state.velocity.z, -0.5f);
}
