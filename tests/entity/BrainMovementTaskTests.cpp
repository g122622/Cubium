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

#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {
namespace {

// ========== WalkTarget 构造测试 ==========

TEST(WalkTargetVariantsTest, Vector3Constructor)
{
    WalkTarget wt(Vector3(10.5f, 64.0f, 20.5f), 1.0f, 1);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_FLOAT_EQ(wt.getSpeed(), 1.0f);
    EXPECT_EQ(wt.getDistance(), 1);
}

TEST(WalkTargetVariantsTest, BlockPosConstructor)
{
    WalkTarget wt(BlockPos(10, 64, 20), 0.8f, 3);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_EQ(wt.getTarget()->getBlockPos(), BlockPos(10, 64, 20));
    EXPECT_FLOAT_EQ(wt.getSpeed(), 0.8f);
    EXPECT_EQ(wt.getDistance(), 3);
}

TEST(WalkTargetVariantsTest, BlockPosTargetCenterPosition)
{
    BlockPos bp(3, 70, -5);
    WalkTarget wt(bp, 1.0f, 1);
    auto target = wt.getTarget();
    ASSERT_NE(target, nullptr);
    // BlockPosTarget 的 position 返回方块中心
    EXPECT_FLOAT_EQ(target->getPosition().x, 3.5f);
    EXPECT_FLOAT_EQ(target->getPosition().y, 70.5f);
    EXPECT_FLOAT_EQ(target->getPosition().z, -4.5f);
}

TEST(WalkTargetVariantsTest, Vector3ConstructorConvertsToBlockCenter)
{
    // Vector3 构造函数会先转换为 BlockPos（截断），然后 BlockPosTarget 返回方块中心
    Vector3 exactPos(10.7f, 64.3f, 20.9f);
    WalkTarget wt(exactPos, 0.5f, 2);
    auto target = wt.getTarget();
    ASSERT_NE(target, nullptr);
    // 10.7 → BlockPos(10,64,20) → 中心 (10.5, 64.5, 20.5)
    EXPECT_FLOAT_EQ(target->getPosition().x, 10.5f);
    EXPECT_FLOAT_EQ(target->getPosition().y, 64.5f);
    EXPECT_FLOAT_EQ(target->getPosition().z, 20.5f);
    EXPECT_FLOAT_EQ(wt.getSpeed(), 0.5f);
    EXPECT_EQ(wt.getDistance(), 2);
}

TEST(WalkTargetVariantsTest, SharedPositionTargetIsNotCopied)
{
    auto sharedTarget = std::make_shared<BlockPosTarget>(BlockPos(1, 2, 3));
    WalkTarget wt1(sharedTarget, 1.0f, 1);
    WalkTarget wt2(sharedTarget, 0.5f, 2);
    // 两个 WalkTarget 共享相同的 IPositionTarget
    EXPECT_EQ(wt1.getTarget(), wt2.getTarget());
}

TEST(WalkTargetVariantsTest, PositionTargetConstructor)
{
    auto target = std::make_shared<BlockPosTarget>(BlockPos(5, 70, 8));
    WalkTarget wt(target, 1.2f, 2);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_EQ(wt.getTarget()->getBlockPos(), BlockPos(5, 70, 8));
    EXPECT_FLOAT_EQ(wt.getSpeed(), 1.2f);
    EXPECT_EQ(wt.getDistance(), 2);
}

// ========== 记忆状态需求测试 ==========

// 使用 Brain<int> 验证记忆条件逻辑（无需完整实体）
// 注意：Task<E> 的模板参数 E 必须是拥有 brain()、navigator() 等方法的实体类型，
// 因此不能使用 int 实例化任务。此处只测试记忆模块的注册和状态检查。

TEST(MovementTasksMemoryTest, WalkTargetMemoryIntegration)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    // 初始状态：WALK_TARGET 不存在
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));

    // 设置 WALK_TARGET
    WalkTarget walkTarget(BlockPos(10, 64, 20), 1.0f, 2);
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, walkTarget);

    // WALK_TARGET 存在
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));

    auto stored = brain.getMemory(MemoryModuleTypes::WALK_TARGET);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->getTarget()->getBlockPos(), BlockPos(10, 64, 20));
    EXPECT_FLOAT_EQ(stored->getSpeed(), 1.0f);
    EXPECT_EQ(stored->getDistance(), 2);

    // 清除 WALK_TARGET
    brain.removeMemory(MemoryModuleTypes::WALK_TARGET);
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));
}

TEST(MovementTasksMemoryTest, AvoidTargetMemoryIntegration)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::AVOID_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);

    // AVOID_TARGET 不存在
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::AVOID_TARGET));

    // 验证记忆的注册和状态检查
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

TEST(MovementTasksMemoryTest, AttackTargetMemoryIntegration)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // ATTACK_TARGET 不存在
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED));
}

TEST(MovementTasksMemoryTest, HidingPlaceMemoryIntegration)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::HIDING_PLACE);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::HOME);
    brain.registerMemory(MemoryModuleTypes::NEAREST_BED);
    brain.registerMemory(MemoryModuleTypes::HURT_BY);
    brain.registerMemory(MemoryModuleTypes::HEARD_BELL_TIME);

    // 初始状态
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::HIDING_PLACE));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::HIDING_PLACE, MemoryModuleStatus::VALUE_ABSENT));

    // 设置 HIDING_PLACE
    brain.setMemory(MemoryModuleTypes::HIDING_PLACE, BlockPos(5, 64, 10));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::HIDING_PLACE));

    auto hidingPlace = brain.getMemory(MemoryModuleTypes::HIDING_PLACE);
    ASSERT_TRUE(hidingPlace.has_value());
    EXPECT_EQ(*hidingPlace, BlockPos(5, 64, 10));
}

TEST(MovementTasksMemoryTest, CantReachWalkTargetMemoryWithTTL)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    // 设置带 TTL 的不可达标记
    brain.setMemoryWithTTL(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE, static_cast<i64>(100), 200);
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE));

    auto value = brain.getMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 100);
}

TEST(MovementTasksMemoryTest, ActivityRegistrationWithMemoryRequirements)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::AVOID_TARGET);
    brain.registerMemory(MemoryModuleTypes::HIDING_PLACE);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    // 验证所有移动任务相关的记忆模块可以正确注册
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::HIDING_PLACE, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::PATH, MemoryModuleStatus::REGISTERED));

    // VALUE_ABSENT 在已注册但未设置值时为真
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

TEST(MovementTasksMemoryTest, HomeAndNearestBedMemoryForHiding)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::HOME);
    brain.registerMemory(MemoryModuleTypes::NEAREST_BED);

    // HOME 记忆使用 GlobalPos 类型
    GlobalPos homePos(DimensionId(0), BlockPos(100, 64, 200));
    brain.setMemory(MemoryModuleTypes::HOME, homePos);

    auto storedHome = brain.getMemory(MemoryModuleTypes::HOME);
    ASSERT_TRUE(storedHome.has_value());
    EXPECT_EQ(storedHome->getPos(), BlockPos(100, 64, 200));

    // NEAREST_BED 记忆使用 BlockPos 类型
    brain.setMemory(MemoryModuleTypes::NEAREST_BED, BlockPos(50, 65, 100));
    auto storedBed = brain.getMemory(MemoryModuleTypes::NEAREST_BED);
    ASSERT_TRUE(storedBed.has_value());
    EXPECT_EQ(*storedBed, BlockPos(50, 65, 100));
}

} // namespace
} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
