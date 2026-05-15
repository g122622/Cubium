/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so condition, to the following conditions:
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
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/mob/TurtleEggBlock.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

using namespace blocks;

// ============================================================================
// 基础功能测试
// 这些测试不依赖世界交互，仅验证海龟实体的状态管理
// ============================================================================

class TurtleEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

// ========== 步高测试 ==========

TEST_F(TurtleEntityTest, StepHeightIsOne)
{
    // MC 1.16.5: TurtleEntity 构造函数中设置 stepHeight = 1.0F
    TurtleEntity turtle(LegacyEntityType::Turtle, 0);
    EXPECT_FLOAT_EQ(turtle.stepHeight(), 1.0f);
}

// ========== 基础属性测试 ==========

TEST_F(TurtleEntityTest, Create_HasCorrectProperties)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    // 验证初始状态
    EXPECT_FALSE(turtle.hasEgg());
    EXPECT_FALSE(turtle.isLayingEgg());
    EXPECT_FALSE(turtle.hasHomePos());
    EXPECT_FALSE(turtle.isGoingHome());
    EXPECT_FALSE(turtle.isTravelling());
}

TEST_F(TurtleEntityTest, HomePos_CanBeSetAndRetrieved)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    BlockPos homePos(100, 64, -200);
    turtle.setHomePos(homePos);

    EXPECT_TRUE(turtle.hasHomePos());
    EXPECT_EQ(turtle.getHomePos(), homePos);
}

TEST_F(TurtleEntityTest, EyeHeight_DiffersByAge)
{
    TurtleEntity adult(LegacyEntityType::Unknown, 1);
    adult.setChild(false);

    TurtleEntity baby(LegacyEntityType::Unknown, 2);
    baby.setChild(true);

    // MC 1.16.5: 成体眼睛高度 0.4f，幼体 0.2f
    EXPECT_FLOAT_EQ(adult.eyeHeight(), 0.4f);
    EXPECT_FLOAT_EQ(baby.eyeHeight(), 0.2f);
}

TEST_F(TurtleEntityTest, SetHasEgg_ChangesState)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(turtle.hasEgg());

    turtle.setHasEgg(true);
    EXPECT_TRUE(turtle.hasEgg());

    turtle.setHasEgg(false);
    EXPECT_FALSE(turtle.hasEgg());
}

TEST_F(TurtleEntityTest, SetLayingEgg_ChangesState)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(turtle.isLayingEgg());

    turtle.setLayingEgg(true);
    EXPECT_TRUE(turtle.isLayingEgg());

    turtle.setLayingEgg(false);
    EXPECT_FALSE(turtle.isLayingEgg());
}

TEST_F(TurtleEntityTest, StartLayEgg_ResetsTimerAndState)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    // 设置为下蛋状态
    turtle.setHasEgg(true);
    turtle.startLayEgg();

    // 验证状态
    EXPECT_TRUE(turtle.isLayingEgg());
    EXPECT_TRUE(turtle.hasEgg());
}

TEST_F(TurtleEntityTest, SetGoingHome_ChangesState)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(turtle.isGoingHome());

    turtle.setGoingHome(true);
    EXPECT_TRUE(turtle.isGoingHome());

    turtle.setGoingHome(false);
    EXPECT_FALSE(turtle.isGoingHome());
}

TEST_F(TurtleEntityTest, SetTravelling_ChangesState)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(turtle.isTravelling());

    turtle.setTravelling(true);
    EXPECT_TRUE(turtle.isTravelling());

    turtle.setTravelling(false);
    EXPECT_FALSE(turtle.isTravelling());
}

// ========== 水陆状态测试 ==========

TEST_F(TurtleEntityTest, IsOnLand_ReturnsOppositeOfIsInWater)
{
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);

    // 初始状态不在水中
    EXPECT_FALSE(turtle.isInWater());
    EXPECT_TRUE(turtle.isOnLand());
}

// ============================================================================
// layEgg() 单元测试
// 注意：由于 BlockTags 系统需要在完整环境中初始化，
// 这里只测试状态变化逻辑，方块放置测试在集成测试中进行
// ============================================================================

/**
 * @brief 测试用 Mock 世界 - 用于测试 layEgg() 中的状态变化
 *
 * 注意：由于 BlockTags::SAND() 需要完整的标签系统初始化，
 * 本测试不验证方块放置，只验证状态重置逻辑
 */
class TurtleLayEggTestWorld final : public test::BaseTestWorld {
public:
    // 播放的音效记录
    struct PlayedSound
    {
        ResourceLocation soundId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_playedSounds.push_back({soundEventId, category, position, volume, pitch});
    }

    [[nodiscard]] const std::vector<PlayedSound>& playedSounds() const { return m_playedSounds; }

    // 设置指定位置为空气
    void setAir(const BlockPos& pos)
    {
        m_blocks.erase(pos);
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<PlayedSound> m_playedSounds;
};

class TurtleLayEggTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    TurtleLayEggTestWorld m_world;
};

TEST_F(TurtleLayEggTest, LayEggTimer_ResetsAfterDuration)
{
    // 创建海龟
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);
    turtle.setWorld(&m_world);
    turtle.setPosition(0.5f, 64.0f, 0.5f);
    turtle.setHasEgg(true);
    turtle.startLayEgg();

    EXPECT_TRUE(turtle.isLayingEgg());
    EXPECT_TRUE(turtle.hasEgg());

    // 运行 200 ticks（LAY_EGG_DURATION）
    for (i32 i = 0; i < 200; ++i) {
        turtle.tick();
    }

    // 200 ticks 后，计时器应该归零
    // 注意：layEgg() 会尝试放置方块，但由于没有沙子，只会重置状态
    EXPECT_FALSE(turtle.isLayingEgg());
    EXPECT_FALSE(turtle.hasEgg());
}

TEST_F(TurtleLayEggTest, LayEggTimer_DoesNotTriggerBeforeDuration)
{
    // 创建海龟
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);
    turtle.setWorld(&m_world);
    turtle.setPosition(0.5f, 64.0f, 0.5f);
    turtle.setHasEgg(true);
    turtle.startLayEgg();

    // 运行 199 ticks（比 LAY_EGG_DURATION 少 1）
    for (i32 i = 0; i < 199; ++i) {
        turtle.tick();
    }

    // 状态应该保持
    EXPECT_TRUE(turtle.isLayingEgg());
    EXPECT_TRUE(turtle.hasEgg());
}

TEST_F(TurtleLayEggTest, LayEggTimer_StateChangesOnlyWhenComplete)
{
    // 创建海龟
    TurtleEntity turtle(LegacyEntityType::Unknown, 1);
    turtle.setWorld(&m_world);
    turtle.setPosition(0.5f, 64.0f, 0.5f);
    turtle.setHasEgg(true);
    turtle.startLayEgg();

    // 半途取消（通过直接设置状态）
    for (i32 i = 0; i < 100; ++i) {
        turtle.tick();
    }

    // 状态仍然保持
    EXPECT_TRUE(turtle.isLayingEgg());
    EXPECT_TRUE(turtle.hasEgg());

    // 手动取消下蛋状态
    turtle.setLayingEgg(false);

    // 再 tick 不会触发下蛋
    for (i32 i = 0; i < 200; ++i) {
        turtle.tick();
    }

    // hasEgg 仍然为 true（因为计时器被中断）
    EXPECT_TRUE(turtle.hasEgg());
}

// ========== 孵化属性测试 ==========

TEST_F(TurtleEntityTest, TurtleEggBlock_HasCorrectDefaultState)
{
    // 验证 TurtleEggBlock 的默认状态
    auto turtleEgg = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));

    const BlockState& defaultState = turtleEgg->defaultState();

    // 默认蛋数量为 1
    EXPECT_EQ(turtleEgg->getEggs(defaultState), 1);

    // 默认孵化阶段为 0
    EXPECT_EQ(turtleEgg->getHatch(defaultState), 0);
}

TEST_F(TurtleEntityTest, TurtleEggBlock_EggCountRange)
{
    auto turtleEgg = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));

    // 验证蛋数量范围限制
    BlockState state1 = turtleEgg->withEggs(1);
    EXPECT_EQ(turtleEgg->getEggs(state1), 1);

    BlockState state2 = turtleEgg->withEggs(2);
    EXPECT_EQ(turtleEgg->getEggs(state2), 2);

    BlockState state3 = turtleEgg->withEggs(3);
    EXPECT_EQ(turtleEgg->getEggs(state3), 3);

    BlockState state4 = turtleEgg->withEggs(4);
    EXPECT_EQ(turtleEgg->getEggs(state4), 4);

    // 超出范围会被 clamp
    BlockState stateUnder = turtleEgg->withEggs(0);
    EXPECT_EQ(turtleEgg->getEggs(stateUnder), 1);  // 最小值

    BlockState stateOver = turtleEgg->withEggs(5);
    EXPECT_EQ(turtleEgg->getEggs(stateOver), 4);  // 最大值
}

TEST_F(TurtleEntityTest, TurtleEggBlock_HatchRange)
{
    auto turtleEgg = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));

    // 验证孵化阶段范围限制
    BlockState state0 = turtleEgg->withHatch(0);
    EXPECT_EQ(turtleEgg->getHatch(state0), 0);

    BlockState state1 = turtleEgg->withHatch(1);
    EXPECT_EQ(turtleEgg->getHatch(state1), 1);

    BlockState state2 = turtleEgg->withHatch(2);
    EXPECT_EQ(turtleEgg->getHatch(state2), 2);

    // 超出范围会被 clamp
    BlockState stateUnder = turtleEgg->withHatch(-1);
    EXPECT_EQ(turtleEgg->getHatch(stateUnder), 0);  // 最小值

    BlockState stateOver = turtleEgg->withHatch(3);
    EXPECT_EQ(turtleEgg->getHatch(stateOver), 2);  // 最大值
}

} // namespace
} // namespace mc
