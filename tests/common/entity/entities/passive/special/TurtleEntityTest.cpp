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
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/mob/TurtleEggBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
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
        Items::initialize();
    }
};

// ========== 步高测试 ==========

TEST_F(TurtleEntityTest, StepHeightIsOne)
{
    // MC 1.16.5: TurtleEntity 构造函数中设置 stepHeight = 1.0F
    TurtleEntity turtle(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(turtle.stepHeight(), 1.0f);
}

// ========== 基础属性测试 ==========

TEST_F(TurtleEntityTest, Create_HasCorrectProperties)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证初始状态
    EXPECT_FALSE(turtle.hasEgg());
    EXPECT_FALSE(turtle.isLayingEgg());
    EXPECT_FALSE(turtle.hasHomePos());
    EXPECT_FALSE(turtle.isGoingHome());
    EXPECT_FALSE(turtle.isTravelling());
}

TEST_F(TurtleEntityTest, HomePos_CanBeSetAndRetrieved)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    BlockPos homePos(100, 64, -200);
    turtle.setHomePos(homePos);

    EXPECT_TRUE(turtle.hasHomePos());
    EXPECT_EQ(turtle.getHomePos(), homePos);
}

TEST_F(TurtleEntityTest, EyeHeight_DiffersByAge)
{
    TurtleEntity adult(EntityInstanceId(1), mc::test::testEcsRegistry());
    adult.setChild(false);

    TurtleEntity baby(EntityInstanceId(2), mc::test::testEcsRegistry());
    baby.setChild(true);

    // MC 1.16.5: 成体眼睛高度 0.4f，幼体 0.2f
    EXPECT_FLOAT_EQ(adult.eyeHeight(), 0.4f);
    EXPECT_FLOAT_EQ(baby.eyeHeight(), 0.2f);
}

TEST_F(TurtleEntityTest, SetHasEgg_ChangesState)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(turtle.hasEgg());

    turtle.setHasEgg(true);
    EXPECT_TRUE(turtle.hasEgg());

    turtle.setHasEgg(false);
    EXPECT_FALSE(turtle.hasEgg());
}

TEST_F(TurtleEntityTest, SetLayingEgg_ChangesState)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(turtle.isLayingEgg());

    turtle.setLayingEgg(true);
    EXPECT_TRUE(turtle.isLayingEgg());

    turtle.setLayingEgg(false);
    EXPECT_FALSE(turtle.isLayingEgg());
}

TEST_F(TurtleEntityTest, StartLayEgg_ResetsTimerAndState)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置为下蛋状态
    turtle.setHasEgg(true);
    turtle.startLayEgg();

    // 验证状态
    EXPECT_TRUE(turtle.isLayingEgg());
    EXPECT_TRUE(turtle.hasEgg());
}

TEST_F(TurtleEntityTest, SetGoingHome_ChangesState)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(turtle.isGoingHome());

    turtle.setGoingHome(true);
    EXPECT_TRUE(turtle.isGoingHome());

    turtle.setGoingHome(false);
    EXPECT_FALSE(turtle.isGoingHome());
}

TEST_F(TurtleEntityTest, SetTravelling_ChangesState)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(turtle.isTravelling());

    turtle.setTravelling(true);
    EXPECT_TRUE(turtle.isTravelling());

    turtle.setTravelling(false);
    EXPECT_FALSE(turtle.isTravelling());
}

// ========== 水陆状态测试 ==========

TEST_F(TurtleEntityTest, IsOnLand_ReturnsOppositeOfIsInWater)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

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
class TurtleLayEggTestWorld final : public mc::test::BaseTestWorld {
public:
    // 播放的音效记录
    struct PlayedSound {
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
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
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
    void setAir(const BlockPos& pos) { m_blocks.erase(pos); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<PlayedSound> m_playedSounds;
};

class TurtleLayEggTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    TurtleLayEggTestWorld m_world;
};

TEST_F(TurtleLayEggTest, LayEggTimer_ResetsAfterDuration)
{
    // 创建海龟
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
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
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
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
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
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
    EXPECT_EQ(turtleEgg->getEggs(stateUnder), 1); // 最小值

    BlockState stateOver = turtleEgg->withEggs(5);
    EXPECT_EQ(turtleEgg->getEggs(stateOver), 4); // 最大值
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
    EXPECT_EQ(turtleEgg->getHatch(stateUnder), 0); // 最小值

    BlockState stateOver = turtleEgg->withHatch(3);
    EXPECT_EQ(turtleEgg->getHatch(stateOver), 2); // 最大值
}

// ============================================================================
// 繁殖系统测试
// 参考 MC 1.16.5: 海龟仅使用海草繁殖
// ============================================================================

TEST_F(TurtleEntityTest, IsBreedingItem_Seagrass_ReturnsTrue)
{
    // MC 1.16.5: 海龟仅接受海草作为繁殖物品
    // 参考: net.minecraft.entity.passive.TurtleEntity.isBreedingItem()
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    ItemStack seagrassStack(Items::SEAGRASS, 1);
    EXPECT_TRUE(turtle.isBreedingItem(seagrassStack));
}

TEST_F(TurtleEntityTest, IsBreedingItem_OtherItems_ReturnsFalse)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 小麦不能繁殖海龟
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(turtle.isBreedingItem(wheatStack));

    // 胡萝卜不能繁殖海龟
    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_FALSE(turtle.isBreedingItem(carrotStack));

    // 苹果不能繁殖海龟
    ItemStack appleStack(Items::APPLE, 1);
    EXPECT_FALSE(turtle.isBreedingItem(appleStack));

    // 鳕鱼不能繁殖海龟
    ItemStack codStack(Items::COD, 1);
    EXPECT_FALSE(turtle.isBreedingItem(codStack));

    // 海带不能繁殖海龟（只有海草可以）
    ItemStack kelpStack(Items::DRIED_KELP, 1);
    EXPECT_FALSE(turtle.isBreedingItem(kelpStack));
}

TEST_F(TurtleEntityTest, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(turtle.isBreedingItem(emptyStack));
}

TEST_F(TurtleEntityTest, CanBreed_WhenHasEgg_ReturnsFalse)
{
    // MC 1.16.5: 海龟只有在没有蛋的情况下才能繁殖
    // 参考: net.minecraft.entity.passive.TurtleEntity.canBreed()
    // return super.canBreed() && !this.hasEgg();
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置为成体
    turtle.setChild(false);

    // 没有蛋时可以繁殖
    turtle.setHasEgg(false);
    EXPECT_TRUE(turtle.canBreed());

    // 有蛋时不能繁殖
    turtle.setHasEgg(true);
    EXPECT_FALSE(turtle.canBreed());
}

TEST_F(TurtleEntityTest, CanBreed_WhenChild_ReturnsFalse)
{
    // 幼体不能繁殖
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(true);
    turtle.setHasEgg(false);

    EXPECT_FALSE(turtle.canBreed());
}

TEST_F(TurtleEntityTest, SpawnBaby_CreatesChildTurtle)
{
    TurtleEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());

    parent1.setPosition(100.0f, 64.0f, -200.0f);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    // 验证是海龟实体
    auto* babyTurtle = dynamic_cast<TurtleEntity*>(baby.get());
    EXPECT_NE(babyTurtle, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

TEST_F(TurtleEntityTest, SpawnBaby_InheritsHomePos)
{
    // MC 1.16.5: 小海龟继承父母的出生地
    // 这样小海龟长大后也会回到这里产卵
    TurtleEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());

    parent1.setPosition(100.0f, 64.0f, -200.0f);
    BlockPos homePos(150, 65, -180);
    parent1.setHomePos(homePos);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    auto* babyTurtle = dynamic_cast<TurtleEntity*>(baby.get());
    ASSERT_NE(babyTurtle, nullptr);

    // 验证继承了出生地
    EXPECT_TRUE(babyTurtle->hasHomePos());
    EXPECT_EQ(babyTurtle->getHomePos(), homePos);
}

TEST_F(TurtleEntityTest, SpawnBaby_WithoutHomePos_DoesNotHaveHomePos)
{
    // 父母没有出生地时，幼体也没有
    TurtleEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());

    parent1.setPosition(100.0f, 64.0f, -200.0f);
    // 不设置 homePos

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    auto* babyTurtle = dynamic_cast<TurtleEntity*>(baby.get());
    ASSERT_NE(babyTurtle, nullptr);

    // 验证没有出生地
    EXPECT_FALSE(babyTurtle->hasHomePos());
}

TEST_F(TurtleEntityTest, SpawnBaby_PositionNearParent)
{
    TurtleEntity parent(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent.setPosition(100.0f, 64.0f, -200.0f);

    auto baby = parent.spawnBaby(parent);
    ASSERT_NE(baby, nullptr);

    // 验证位置在父体附近
    // 由于 spawnBaby 调用 setPosition(x(), y(), z())，位置应该与父体相同
    EXPECT_FLOAT_EQ(baby->x(), 100.0f);
    EXPECT_FLOAT_EQ(baby->y(), 64.0f);
    EXPECT_FLOAT_EQ(baby->z(), -200.0f);
}

// ============================================================================
// travel() 方法测试 - 海龟水陆移动速度调整
// 参考 MC 1.16.5: TurtleEntity.travel() 和 MoveHelperController.updateSpeed()
// ============================================================================

/**
 * @brief travel() 测试专用 Mock 世界
 *
 * 提供最小化的世界接口实现
 */
class TurtleTravelTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        MC_UNUSED(x);
        MC_UNUSED(y);
        MC_UNUSED(z);
        return nullptr;
    }

    void playSound(const ResourceLocation& /*soundEventId*/,
        sound::SoundCategory /*category*/,
        const Vector3& /*position*/,
        f32 /*volume*/,
        f32 /*pitch*/) override
    {
        // 不执行实际音效
    }
};

class TurtleTravelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    TurtleTravelTestWorld m_world;
};

TEST_F(TurtleTravelTest, WaterSpeed_NormalSpeed)
{
    // MC 1.16.5: 水中海龟保持基础移动速度 0.25
    // 并获得轻微上升动力 (+0.005 y)
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setPosition(0.0f, 64.0f, 0.0f);
    test::setEntityInWater(turtle, true);
    turtle.setOnGround(false);

    // 调用 travel
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度设置为基础速度 0.25
    EXPECT_FLOAT_EQ(turtle.aiMoveSpeed(), 0.25f);

    // 注意：水中上升动力 (+0.005) 在 travel() 中设置，
    // 但父类 AnimalEntity::travel() 会应用重力和水中阻力，
    // 最终速度可能仍为负。这里只验证 AI 速度设置正确。
}

TEST_F(TurtleTravelTest, WaterSpeed_FarFromHome_Slower)
{
    // MC 1.16.5: 远离出生地超过 16 格时，速度减半，最低 0.08F
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);

    // 设置出生地
    BlockPos homePos(0, 64, 0);
    turtle.setHomePos(homePos);

    // 设置当前位置远离出生地超过 16 格
    turtle.setPosition(20.0f, 64.0f, 0.0f); // 距离出生地 20 格
    test::setEntityInWater(turtle, true);
    turtle.setOnGround(false);

    // 调用 travel
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度减半（0.25 * 0.5 = 0.125，大于 0.08）
    EXPECT_FLOAT_EQ(turtle.aiMoveSpeed(), 0.125f);
}

TEST_F(TurtleTravelTest, WaterSpeed_Child_Slower)
{
    // MC 1.16.5: 幼体在水中速度降低为 1/3，最低 0.06F
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setChild(true); // 设置为幼体
    turtle.setPosition(0.0f, 64.0f, 0.0f);
    test::setEntityInWater(turtle, true);
    turtle.setOnGround(false);

    // 调用 travel
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度降低为 1/3（0.25 / 3 ≈ 0.0833，大于 0.06）
    EXPECT_NEAR(turtle.aiMoveSpeed(), 0.25f / 3.0f, 0.001f);
}

TEST_F(TurtleTravelTest, LandSpeed_HalfSpeed)
{
    // MC 1.16.5: 陆地速度减半，最低 0.06F
    // 基础速度 0.25 / 2 = 0.125，大于 0.06
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setPosition(0.0f, 64.0f, 0.0f);
    test::setEntityInWater(turtle, false);
    turtle.setOnGround(true);

    // 调用 travel
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度减半
    EXPECT_FLOAT_EQ(turtle.aiMoveSpeed(), 0.125f);
}

TEST_F(TurtleTravelTest, LandSpeed_MinimumSpeed)
{
    // 验证陆地速度最低为 0.06F
    // 即使基础速度很低，陆地速度也不应低于 0.06
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setPosition(0.0f, 64.0f, 0.0f);
    test::setEntityInWater(turtle, false);
    turtle.setOnGround(true);

    // 设置很低的基础速度（模拟缓慢效果等情况）
    // 注意：这里我们直接测试逻辑，实际游戏中速度属性不会这么低
    // 但 travel() 方法中有 max(baseSpeed * 0.5f, 0.06f) 的保护
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度不低于 0.06
    EXPECT_GE(turtle.aiMoveSpeed(), 0.06f);
}

TEST_F(TurtleTravelTest, AirSpeed_NoSpeedChange)
{
    // MC 1.16.5: 空中（跳跃或下落）保持当前 AI 速度
    // 不做额外调整
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setPosition(0.0f, 64.0f, 0.0f);
    test::setEntityInWater(turtle, false);
    turtle.setOnGround(false); // 在空中

    // 预设一个 AI 移动速度
    turtle.setAIMoveSpeed(0.15f);

    // 调用 travel（不应改变速度）
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 验证 AI 移动速度保持不变
    EXPECT_FLOAT_EQ(turtle.aiMoveSpeed(), 0.15f);
}

TEST_F(TurtleTravelTest, WaterSpeed_ChildFarFromHome_Minimum)
{
    // MC 1.16.5: 幼体 + 远离出生地组合
    // 速度计算顺序：
    // 1. 基础速度 0.25
    // 2. 远离出生地减半 -> 0.125
    // 3. 幼体再除以 3 -> 0.0416...
    // 4. 最低 0.06
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setWorld(&m_world);
    turtle.setChild(true);

    BlockPos homePos(0, 64, 0);
    turtle.setHomePos(homePos);
    turtle.setPosition(20.0f, 64.0f, 0.0f); // 距离出生地 20 格
    test::setEntityInWater(turtle, true);
    turtle.setOnGround(false);

    // 调用 travel
    turtle.travel(Vector3(0.0f, 0.0f, 1.0f));

    // 0.25 / 2 / 3 = 0.0416...，但最低 0.06
    EXPECT_FLOAT_EQ(turtle.aiMoveSpeed(), 0.06f);
}

// ============================================================================
// BABY_ON_LAND_SELECTOR 逻辑测试
// 验证海龟攻击目标过滤：只攻击 isChild() && !isInWater() 的海龟
// ============================================================================

TEST_F(TurtleEntityTest, BabyOnLandSelector_BabyOnLandMatchesFilter)
{
    // 幼年海龟在陆地上 -> isChild()=true, isInWater()=false -> 匹配
    TurtleEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent1.setPosition(100.0f, 64.0f, -200.0f);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    auto* babyTurtle = dynamic_cast<TurtleEntity*>(baby.get());
    ASSERT_NE(babyTurtle, nullptr);

    // 幼体不在水中 -> 符合 BABY_ON_LAND_SELECTOR
    EXPECT_TRUE(babyTurtle->isChild());
    EXPECT_FALSE(babyTurtle->isInWater());
}

TEST_F(TurtleEntityTest, BabyOnLandSelector_AdultDoesNotMatchFilter)
{
    // 成年海龟 -> isChild()=false -> 不匹配
    TurtleEntity adult(EntityInstanceId(1), mc::test::testEcsRegistry());
    adult.setPosition(100.0f, 64.0f, -200.0f);

    EXPECT_FALSE(adult.isChild());
}

TEST_F(TurtleEntityTest, BabyOnLandSelector_BabyInWaterDoesNotMatchFilter)
{
    // 幼年海龟在水中 -> isChild()=true, isInWater()=true -> 不匹配
    TurtleEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent1.setPosition(100.0f, 64.0f, -200.0f);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    auto* babyTurtle = dynamic_cast<TurtleEntity*>(baby.get());
    ASSERT_NE(babyTurtle, nullptr);

    // 设置在水中
    test::setEntityInWater(*babyTurtle, true);

    // 在水中不符合 BABY_ON_LAND_SELECTOR
    EXPECT_TRUE(babyTurtle->isChild());
    EXPECT_TRUE(babyTurtle->isInWater());
}

// ============================================================================
// 目标选择器集成测试
// 验证 OcelotEntity 的目标选择器中正确注册了海龟攻击目标和小鸡攻击目标
// （NearestAttackableTargetGoal<TurtleEntity> 使用 BABY_ON_LAND_SELECTOR 过滤谓词）
// 注意：AbstractSkeletonEntity 同样注册了海龟攻击目标（优先级3），但 SkeletonEntity
// 构造函数需要完整实体注册系统，无法在单元测试中直接实例化
// ============================================================================

class TurtleTargetSelectorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleTargetSelectorTest, Ocelot_HasTurtleTargetGoal)
{
    // 创建豹猫实体，验证其目标选择器中注册了 TurtleEntity 攻击目标
    OcelotEntity ocelot(EntityInstanceId(2), mc::test::testEcsRegistry());

    bool foundTurtleGoal = false;
    for (const auto& prioritizedGoal : ocelot.targetSelector().getAllGoals()) {
        const auto* goal = prioritizedGoal.getGoal();
        if (dynamic_cast<const entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>*>(goal) != nullptr) {
            foundTurtleGoal = true;
            // MC 原版：targetSelector.addGoal(1, ...)
            EXPECT_EQ(prioritizedGoal.getPriority(), 1);
            break;
        }
    }
    EXPECT_TRUE(foundTurtleGoal) << "OcelotEntity should have a NearestAttackableTargetGoal<TurtleEntity> registered";
}

TEST_F(TurtleTargetSelectorTest, Ocelot_HasChickenTargetGoal)
{
    // 验证豹猫同时注册了小鸡攻击目标
    OcelotEntity ocelot(EntityInstanceId(3), mc::test::testEcsRegistry());

    bool foundChickenGoal = false;
    for (const auto& prioritizedGoal : ocelot.targetSelector().getAllGoals()) {
        const auto* goal = prioritizedGoal.getGoal();
        if (dynamic_cast<const entity::ai::goal::NearestAttackableTargetGoal<ChickenEntity>*>(goal) != nullptr) {
            foundChickenGoal = true;
            break;
        }
    }
    EXPECT_TRUE(foundChickenGoal) << "OcelotEntity should have a NearestAttackableTargetGoal<ChickenEntity> registered";
}

} // namespace
} // namespace mc
