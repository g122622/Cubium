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

/**
 * @file LlamaFollowCaravanGoalTest.cpp
 * @brief 羊驼商队跟随目标测试
 *
 * 测试 LlamaFollowCaravanGoal 的核心逻辑：
 * - _firstIsLeashed() 递归拴绳链检查
 * - shouldExecute() 两阶段搜索与拴绳前置条件
 * - shouldContinueExecuting() 商队链完整性检查
 * - tick() 栅栏拴绳检查
 * - 商队链表操作（joinCaravan/leaveCaravan）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试用 Mock World
// ============================================================================

/**
 * @brief 支持实体查询的测试世界
 *
 * 覆写 getEntitiesInAABB() 返回预设的实体列表，
 * 用于测试 LlamaFollowCaravanGoal 的搜索逻辑。
 */
class CaravanTestWorld final : public mc::test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(0); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_nearbyEntities;
    }

    [[nodiscard]] Entity* getEntityByUuid(const std::string&) const override { return nullptr; }

    void setNearbyEntities(std::vector<Entity*> entities) { m_nearbyEntities = std::move(entities); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("CaravanTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("CaravanTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    std::vector<Entity*> m_nearbyEntities;
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 创建羊驼实体并设置世界，放置在指定位置
 *
 * 注意：直接构造的 LlamaEntity 不会通过注册表工厂初始化 typeId，
 * 导致 LlamaFollowCaravanGoal::shouldExecute() 中的类型过滤
 * (type != VanillaEntityTypeKeys::LLAMA && type != VanillaEntityTypeKeys::TRADER_LLAMA)
 * 会跳过该实体。这里显式设置 typeId 为 minecraft:llama。
 */
std::unique_ptr<LlamaEntity> createLlama(
    EntityInstanceId id, CaravanTestWorld& world, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto llama = std::make_unique<LlamaEntity>(id, mc::test::testEcsRegistry());
    llama->setTypeId(entity::EntityTypeKeys::LLAMA);
    llama->setWorld(&world);
    llama->setPosition(x, y, z);
    return llama;
}

/**
 * @brief 创建 LlamaFollowCaravanGoal 实例
 */
std::unique_ptr<entity::ai::goal::LlamaFollowCaravanGoal> createCaravanGoal(LlamaEntity* llama)
{
    return std::make_unique<entity::ai::goal::LlamaFollowCaravanGoal>(llama, 2.1f);
}

// ============================================================================
// 商队链表操作测试
// ============================================================================

TEST(LlamaCaravanTest, JoinCaravan_SetsHeadAndTail)
{
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);

    // llama2 加入 llama1 的商队
    llama2->joinCaravan(llama1.get());

    EXPECT_TRUE(llama2->isInCaravan());
    EXPECT_EQ(llama2->getCaravanHead(), llama1.get());
    EXPECT_TRUE(llama1->hasCaravanTail());
    EXPECT_EQ(llama1->getCaravanTail(), llama2.get());
}

TEST(LlamaCaravanTest, LeaveCaravan_ClearsHeadAndTail)
{
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);

    llama2->joinCaravan(llama1.get());
    llama2->leaveCaravan();

    EXPECT_FALSE(llama2->isInCaravan());
    EXPECT_EQ(llama2->getCaravanHead(), nullptr);
    EXPECT_FALSE(llama1->hasCaravanTail());
    EXPECT_EQ(llama1->getCaravanTail(), nullptr);
}

TEST(LlamaCaravanTest, ThreeLlamaCaravanChain)
{
    // 三只羊驼形成链式商队：llama1 <- llama2 <- llama3
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);

    llama2->joinCaravan(llama1.get());
    llama3->joinCaravan(llama2.get());

    // llama2 跟随 llama1
    EXPECT_TRUE(llama2->isInCaravan());
    EXPECT_EQ(llama2->getCaravanHead(), llama1.get());

    // llama3 跟随 llama2
    EXPECT_TRUE(llama3->isInCaravan());
    EXPECT_EQ(llama3->getCaravanHead(), llama2.get());

    // llama1 是链头，没有头领
    EXPECT_FALSE(llama1->isInCaravan());
    EXPECT_EQ(llama1->getCaravanHead(), nullptr);

    // 链表尾部关系
    EXPECT_EQ(llama1->getCaravanTail(), llama2.get());
    EXPECT_EQ(llama2->getCaravanTail(), llama3.get());
    EXPECT_EQ(llama3->getCaravanTail(), nullptr);
}

TEST(LlamaCaravanTest, JoinCaravan_OverwritesPreviousTail)
{
    // llama1 已有 llama2 作为尾部，llama3 加入会覆盖 llama1 的尾部
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);

    llama2->joinCaravan(llama1.get());
    // llama3 也加入 llama1（在 shouldExecute 中不会发生，因为 llama1.hasCaravanTail() 为 true）
    // 但 joinCaravan 本身不做此检查，直接覆盖
    llama3->joinCaravan(llama1.get());

    // llama1 的尾部现在是 llama3
    EXPECT_EQ(llama1->getCaravanTail(), llama3.get());
    // llama2 仍在商队中但 llama1 不再指向它
    EXPECT_TRUE(llama2->isInCaravan());
    EXPECT_EQ(llama2->getCaravanHead(), llama1.get());
}

// ============================================================================
// shouldExecute 测试
// ============================================================================

TEST(LlamaCaravanTest, ShouldExecute_ReturnsFalseWhenLeashed)
{
    // 被拴住的羊驼不能发起加入商队
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);
    auto goal = createCaravanGoal(llama.get());

    // 将羊驼拴在栅栏上
    llama->setLeashedToFence(BlockPos(0, 64, 0));
    EXPECT_TRUE(llama->isLeashed());

    EXPECT_FALSE(goal->shouldExecute());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsFalseWhenAlreadyInCaravan)
{
    // 已在商队中的羊驼不能加入新商队
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);
    auto goal = createCaravanGoal(llama3.get());

    // llama3 已加入 llama2 的商队
    llama3->joinCaravan(llama2.get());

    EXPECT_FALSE(goal->shouldExecute());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsFalseWhenNoNearbyLlamas)
{
    // 附近没有羊驼时不能加入商队
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);
    auto goal = createCaravanGoal(llama.get());

    // 世界中没有其他羊驼
    world.setNearbyEntities({});

    EXPECT_FALSE(goal->shouldExecute());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsFalseWhenCandidateNotLeashedAndNoLeashedInChain)
{
    // 候选羊驼未被拴住，且链上没有拴住的羊驼，商队无效
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 5.0f, 64.0f, 0.0f);
    auto llama3 = createLlama(EntityInstanceId(3), world, 10.0f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama3.get());

    // llama1 未被拴住，没有商队
    // llama2 在 llama1 的商队中（llama1 也未被拴住）
    llama2->joinCaravan(llama1.get());

    // 搜索范围：llama2 在商队中且无尾部
    world.setNearbyEntities({llama1.get(), llama2.get()});

    // llama2 在商队中且无尾部，但 llama1（链头）未被拴住
    // shouldExecute 应该返回 false（_firstIsLeashed 检查链头未被拴住）
    EXPECT_FALSE(goal->shouldExecute());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsTrueWhenCandidateIsLeashed)
{
    // 第二阶段搜索：被拴住的羊驼可以作为商队头领
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 5.0f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama2.get());

    // llama1 被拴在栅栏上
    llama1->setLeashedToFence(BlockPos(0, 64, 0));

    // llama1 未在商队中、被拴住、无尾部 -> 第二阶段候选
    world.setNearbyEntities({llama1.get()});

    EXPECT_TRUE(goal->shouldExecute());

    // 验证 llama2 加入了 llama1 的商队
    EXPECT_TRUE(llama2->isInCaravan());
    EXPECT_EQ(llama2->getCaravanHead(), llama1.get());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsTrueWhenChainHeadIsLeashed)
{
    // 第一阶段搜索：商队链头的羊驼被拴住
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 5.0f, 64.0f, 0.0f);
    auto llama3 = createLlama(EntityInstanceId(3), world, 10.0f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama3.get());

    // llama1 被拴住，llama2 跟随 llama1
    llama1->setLeashedToFence(BlockPos(0, 64, 0));
    llama2->joinCaravan(llama1.get());

    // llama2 在商队中且无尾部 -> 第一阶段候选
    world.setNearbyEntities({llama1.get(), llama2.get()});

    EXPECT_TRUE(goal->shouldExecute());

    // 验证 llama3 加入了 llama2 的商队
    EXPECT_TRUE(llama3->isInCaravan());
    EXPECT_EQ(llama3->getCaravanHead(), llama2.get());
}

TEST(LlamaCaravanTest, ShouldExecute_ReturnsFalseWhenCandidateTooClose)
{
    // 距离太近（< 2格）不加入
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 0.5f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama2.get());

    // llama1 被拴住
    llama1->setLeashedToFence(BlockPos(0, 64, 0));

    world.setNearbyEntities({llama1.get()});

    // 距离 < 2 格（0.5^2 = 0.25 < MIN_JOIN_DISTANCE_SQ = 4.0）
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// shouldContinueExecuting 测试
// ============================================================================

TEST(LlamaCaravanTest, ShouldContinueExecuting_ReturnsFalseWhenChainHeadNotLeashed)
{
    // 商队链头未被拴住时，继续执行返回 false
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);
    auto goal = createCaravanGoal(llama3.get());

    // 构建商队：llama1 <- llama2 <- llama3
    // llama1 未被拴住
    llama2->joinCaravan(llama1.get());
    llama3->joinCaravan(llama2.get());

    // _firstIsLeashed(llama3, 0) 应该返回 false
    // 因为链头 llama1 未被拴住
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST(LlamaCaravanTest, ShouldContinueExecuting_ReturnsTrueWhenChainHeadIsLeashed)
{
    // 商队链头被拴住时，继续执行返回 true
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);
    auto goal = createCaravanGoal(llama3.get());

    // 构建商队：llama1(拴住) <- llama2 <- llama3
    llama1->setLeashedToFence(BlockPos(0, 64, 0));
    llama2->joinCaravan(llama1.get());
    llama3->joinCaravan(llama2.get());

    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST(LlamaCaravanTest, ShouldContinueExecuting_ReturnsFalseWhenHeadIsDead)
{
    // 头领被移除时，继续执行返回 false
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto goal = createCaravanGoal(llama2.get());

    llama1->setLeashedToFence(BlockPos(0, 64, 0));
    llama2->joinCaravan(llama1.get());

    // 移除头领
    llama1->remove();

    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST(LlamaCaravanTest, ShouldContinueExecuting_ReturnsFalseWhenNotInCaravan)
{
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);
    auto goal = createCaravanGoal(llama.get());

    // 未加入商队
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

// ============================================================================
// tick() 栅栏拴绳检查测试
// ============================================================================

TEST(LlamaCaravanTest, Tick_DoesNotMoveWhenLeashedToFence)
{
    // 被拴在栅栏柱上的羊驼在 tick() 中不应移动
    // tick() 在 isLeashed() && leaseFencePos().has_value() 时早返回
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 5.0f, 64.0f, 0.0f);
    auto llama3 = createLlama(EntityInstanceId(3), world, 10.0f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama3.get());

    // llama1 被拴住，构建商队链
    llama1->setLeashedToFence(BlockPos(0, 64, 0));
    llama2->joinCaravan(llama1.get());
    llama3->joinCaravan(llama2.get());

    // llama3 被拴在栅栏上
    llama3->setLeashedToFence(BlockPos(10, 64, 0));

    // tick() 不应崩溃，且 llama3 应保持原位（栅栏拴绳早返回）
    EXPECT_NO_THROW(goal->tick());

    // llama3 的位置应保持不变（tick() 因栅栏拴绳而早返回）
    EXPECT_FLOAT_EQ(llama3->x(), 10.0f);
    EXPECT_FLOAT_EQ(llama3->y(), 64.0f);
    EXPECT_FLOAT_EQ(llama3->z(), 0.0f);
}

TEST(LlamaCaravanTest, Tick_DoesNotCrashWhenLeashedToEntity)
{
    // 被拴在实体上（非栅栏）的羊驼在 tick() 中应执行正常的跟随逻辑
    // leashFencePos() 为空，tick() 不会早返回
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world, 0.0f, 64.0f, 0.0f);
    auto llama2 = createLlama(EntityInstanceId(2), world, 5.0f, 64.0f, 0.0f);
    auto goal = createCaravanGoal(llama2.get());

    // llama1 被拴在实体上（非栅栏）
    llama1->setLeashedToEntity("wandering-trader-uuid");
    llama2->joinCaravan(llama1.get());

    // llama2 被拴在实体上（非栅栏）——leaseFencePos() 为空
    llama2->setLeashedToEntity("some-other-uuid");

    // tick() 不应崩溃
    // 注意：由于 navigator() 在测试中为 nullptr，不会实际移动，但不会因栅栏检查早返回
    EXPECT_NO_THROW(goal->tick());
}

TEST(LlamaCaravanTest, Tick_DoesNotMoveWhenNotInCaravan)
{
    // 不在商队中的羊驼调用 tick() 应安全返回
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);
    auto goal = createCaravanGoal(llama.get());

    EXPECT_NO_THROW(goal->tick());
}

TEST(LlamaCaravanTest, Tick_LeashFencePosEmptyWhenLeashedToEntity)
{
    // 验证被拴在实体上时 leaseFencePos() 为空（tick() 不会因栅栏检查早返回）
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);

    llama->setLeashedToEntity("entity-uuid");
    EXPECT_TRUE(llama->isLeashed());
    EXPECT_FALSE(llama->leashFencePos().has_value());
}

TEST(LlamaCaravanTest, Tick_LeashFencePosSetWhenLeashedToFence)
{
    // 验证被拴在栅栏上时 leaseFencePos() 有值（tick() 会因栅栏检查早返回）
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);

    llama->setLeashedToFence(BlockPos(10, 64, 10));
    EXPECT_TRUE(llama->isLeashed());
    EXPECT_TRUE(llama->leashFencePos().has_value());
    EXPECT_EQ(llama->leashFencePos().value(), BlockPos(10, 64, 10));
}

// ============================================================================
// 递归深度限制测试（MAX_CARAVAN_LENGTH = 8）
// ============================================================================

TEST(LlamaCaravanTest, FirstIsLeashed_DeepChainStillFindsLeashedHead)
{
    // 商队链有 9 只羊驼时（从 llama3 到 llama0 的深度为 0~7），
    // 仍在递归深度限制内，应该能找到被拴住的链头
    CaravanTestWorld world;

    // 创建 9 只羊驼
    std::vector<std::unique_ptr<LlamaEntity>> llamas;
    for (int i = 0; i < 9; ++i) {
        llamas.push_back(createLlama(EntityInstanceId(i + 1), world));
    }

    // 第 1 只被拴住
    llamas[0]->setLeashedToFence(BlockPos(0, 64, 0));

    // 构建链：llama[0](拴住) <- llama[1] <- ... <- llama[8]
    for (int i = 1; i < 9; ++i) {
        llamas[i]->joinCaravan(llamas[i - 1].get());
    }

    // 测试最后一只羊驼的 goal
    auto goal = createCaravanGoal(llamas[8].get());

    // 链长度为 9，_firstIsLeashed 从 llama[8] 开始 depth=0，
    // 递归到 llama[1] 时 depth=7，检查 llama[0].isLeashed()=true
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST(LlamaCaravanTest, FirstIsLeashed_TooDeepChainReturnsFalse)
{
    // 商队链超过限制（10 只，从 llama9 到 llama0 的深度 0~8，
    // 在 depth=8 时 llama[1] 还在商队中，检查 llama[0].isLeashed()，
    // 但在 depth=8 时 depth > 8 为 false，所以检查执行。
    // 实际上 depth=8 时不超限（depth > 8 为 false），
    // 所以 10 只链也能找到。只有 11+ 只时 depth 才会超过 8。
    // 让我们用 11 只羊驼来真正超限
    CaravanTestWorld world;

    // 创建 11 只羊驼
    std::vector<std::unique_ptr<LlamaEntity>> llamas;
    for (int i = 0; i < 11; ++i) {
        llamas.push_back(createLlama(EntityInstanceId(i + 1), world));
    }

    // 第 1 只被拴住
    llamas[0]->setLeashedToFence(BlockPos(0, 64, 0));

    // 构建链：llama[0](拴住) <- llama[1] <- ... <- llama[10]
    for (int i = 1; i < 11; ++i) {
        llamas[i]->joinCaravan(llamas[i - 1].get());
    }

    // 测试最后一只羊驼的 goal
    auto goal = createCaravanGoal(llamas[10].get());

    // 链长度为 11，_firstIsLeashed 从 llama[10] 开始 depth=0，
    // 递归到 depth=8 时检查 llama[2]（仍在商队中），
    // 继续递归 _firstIsLeashed(llama[2], 9)，depth=9 > 8，返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST(LlamaCaravanTest, FirstIsLeashed_ExactMaxLength)
{
    // 商队链恰好 9 只时（8 跳 + 1 链头），递归深度恰好不超限
    CaravanTestWorld world;

    // 创建 9 只羊驼（MAX_CARAVAN_LENGTH + 1 = 链头 + 8 跟随者）
    std::vector<std::unique_ptr<LlamaEntity>> llamas;
    for (int i = 0; i < 9; ++i) {
        llamas.push_back(createLlama(EntityInstanceId(i + 1), world));
    }

    // 第 1 只被拴住
    llamas[0]->setLeashedToFence(BlockPos(0, 64, 0));

    // 构建链：llama[0](拴住) <- llama[1] <- ... <- llama[8]
    for (int i = 1; i < 9; ++i) {
        llamas[i]->joinCaravan(llamas[i - 1].get());
    }

    // 测试最后一只羊驼的 goal
    auto goal = createCaravanGoal(llamas[8].get());

    // 链长度恰好为 9，递归深度不超限，链头被拴住
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

// ============================================================================
// 栅栏拴绳检查测试
// ============================================================================

TEST(LlamaCaravanTest, ShouldContinueExecuting_LeashedToFence_StillValid)
{
    // 被拴在栅栏上的羊驼仍在商队中时，shouldContinueExecuting 仍返回 true
    // （tick 中会跳过移动，但 shouldContinueExecuting 不会因此返回 false）
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto llama3 = createLlama(EntityInstanceId(3), world);
    auto goal = createCaravanGoal(llama3.get());

    // llama1 被拴住，llama2 跟随 llama1，llama3 跟随 llama2
    llama1->setLeashedToFence(BlockPos(0, 64, 0));
    llama2->joinCaravan(llama1.get());
    llama3->joinCaravan(llama2.get());

    // llama3 也被拴在栅栏上
    llama3->setLeashedToFence(BlockPos(5, 64, 0));

    // shouldContinueExecuting 应该仍然返回 true
    // 因为链头 llama1 被拴住
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

// ============================================================================
// 拴绳实体检查测试
// ============================================================================

TEST(LlamaCaravanTest, ShouldContinueExecuting_ChainHeadLeashedToEntity)
{
    // 链头被拴在实体上时，商队有效
    CaravanTestWorld world;
    auto llama1 = createLlama(EntityInstanceId(1), world);
    auto llama2 = createLlama(EntityInstanceId(2), world);
    auto goal = createCaravanGoal(llama2.get());

    llama1->setLeashedToEntity("wandering-trader-uuid");
    llama2->joinCaravan(llama1.get());

    EXPECT_TRUE(goal->shouldContinueExecuting());
}

// ============================================================================
// TraderLlama 拴绳集成测试
// ============================================================================

TEST(LlamaCaravanTest, TraderLlamaCanBeLeashedToFence)
{
    CaravanTestWorld world;
    auto traderLlama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    traderLlama->setWorld(&world);

    EXPECT_FALSE(traderLlama->isLeashed());

    traderLlama->setLeashedToFence(BlockPos(10, 64, 10));
    EXPECT_TRUE(traderLlama->isLeashed());
    EXPECT_TRUE(traderLlama->leashFencePos().has_value());
    EXPECT_EQ(traderLlama->leashFencePos().value(), BlockPos(10, 64, 10));
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST(LlamaFollowCaravanGoalConstantsTest, ConstantsAreCorrect)
{
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::SEARCH_RADIUS, 9.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::SEARCH_HEIGHT, 4.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MIN_JOIN_DISTANCE_SQ, 4.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MAX_FOLLOW_DISTANCE_SQ, 676.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::CARAVAN_FOLLOW_DISTANCE, 2.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MAX_CARAVAN_LENGTH, 8);
}

// ============================================================================
// Goal 类型名称测试
// ============================================================================

TEST(LlamaFollowCaravanGoalTypeTest, TypeNameIsCorrect)
{
    CaravanTestWorld world;
    auto llama = createLlama(EntityInstanceId(1), world);
    auto goal = createCaravanGoal(llama.get());

    EXPECT_EQ(goal->getTypeName(), "LlamaFollowCaravanGoal");
}

} // namespace
} // namespace mc
