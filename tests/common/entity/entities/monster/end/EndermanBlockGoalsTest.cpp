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
 * @file EndermanBlockGoalsTest.cpp
 * @brief 末影人方块放置和拾取目标测试
 *
 * 测试末影人的方块操作功能：
 * - EndermanPlaceBlockGoal 放置方块目标
 * - EndermanTakeBlockGoal 拾取方块目标
 * - BlockTags::ENDERMAN_HOLDABLE 方块标签
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/BlockRaycastResult.hpp"

namespace mc {
namespace test {

// ==================== BlockTags::ENDERMAN_HOLDABLE 测试 ====================

class EndermanHoldableTagTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化方块标签
        BlockTags::initialize();
    }
};

TEST_F(EndermanHoldableTagTest, TagExists)
{
    // 标签应该存在
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft", "enderman_holdable"));
}

TEST_F(EndermanHoldableTagTest, ContainsGrassBlock)
{
    // 草方块应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "grass_block")));
}

TEST_F(EndermanHoldableTagTest, ContainsDirt)
{
    // 泥土应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "dirt")));
}

TEST_F(EndermanHoldableTagTest, ContainsSand)
{
    // 沙子应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "sand")));
}

TEST_F(EndermanHoldableTagTest, ContainsRedSand)
{
    // 红沙应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "red_sand")));
}

TEST_F(EndermanHoldableTagTest, ContainsGravel)
{
    // 沙砾应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "gravel")));
}

TEST_F(EndermanHoldableTagTest, ContainsBrownMushroom)
{
    // 棕色蘑菇应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "brown_mushroom")));
}

TEST_F(EndermanHoldableTagTest, ContainsRedMushroom)
{
    // 红色蘑菇应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "red_mushroom")));
}

TEST_F(EndermanHoldableTagTest, ContainsTNT)
{
    // TNT应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "tnt")));
}

TEST_F(EndermanHoldableTagTest, ContainsCactus)
{
    // 仙人掌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "cactus")));
}

TEST_F(EndermanHoldableTagTest, ContainsClay)
{
    // 黏土块应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "clay")));
}

TEST_F(EndermanHoldableTagTest, ContainsPumpkin)
{
    // 南瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "pumpkin")));
}

TEST_F(EndermanHoldableTagTest, ContainsCarvedPumpkin)
{
    // 雕刻南瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "carved_pumpkin")));
}

TEST_F(EndermanHoldableTagTest, ContainsMelon)
{
    // 西瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "melon")));
}

TEST_F(EndermanHoldableTagTest, ContainsMycelium)
{
    // 菌丝体应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "mycelium")));
}

// 下界方块测试（1.16新增）
TEST_F(EndermanHoldableTagTest, ContainsCrimsonFungus)
{
    // 绯红菌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "crimson_fungus")));
}

TEST_F(EndermanHoldableTagTest, ContainsCrimsonNylium)
{
    // 绯红菌岩应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "crimson_nylium")));
}

TEST_F(EndermanHoldableTagTest, ContainsWarpedFungus)
{
    // 诡异菌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "warped_fungus")));
}

TEST_F(EndermanHoldableTagTest, ContainsWarpedNylium)
{
    // 诡异菌岩应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "warped_nylium")));
}

// 小花朵测试
TEST_F(EndermanHoldableTagTest, ContainsDandelion)
{
    // 蒲公英应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "dandelion")));
}

TEST_F(EndermanHoldableTagTest, ContainsPoppy)
{
    // 虞美人应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "poppy")));
}

TEST_F(EndermanHoldableTagTest, ContainsWitherRose)
{
    // 凋零玫瑰应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "wither_rose")));
}

// 不能被拾取的方块测试
TEST_F(EndermanHoldableTagTest, DoesNotContainStone)
{
    // 石头不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(EndermanHoldableTagTest, DoesNotContainBedrock)
{
    // 基岩不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "bedrock")));
}

TEST_F(EndermanHoldableTagTest, DoesNotContainWater)
{
    // 水不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "water")));
}

// ==================== EndermanEntity 持有方块测试 ====================

class EndermanHeldBlockTest : public ::testing::Test {
protected:
    void SetUp() override { enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1)); }

    void TearDown() override { enderman.reset(); }

    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanHeldBlockTest, IsNotHoldingBlockInitially)
{
    // 初始状态不应该持有方块
    EXPECT_FALSE(enderman->isHoldingBlock());
    EXPECT_EQ(enderman->getHeldBlockState(), nullptr);
}

TEST_F(EndermanHeldBlockTest, SetHeldBlockStateSetsHoldingFlag)
{
    // 设置方块状态应该设置持有标志
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);

        EXPECT_TRUE(enderman->isHoldingBlock());
        EXPECT_EQ(enderman->getHeldBlockState(), &defaultState);
    }
}

TEST_F(EndermanHeldBlockTest, SetHeldBlockStateNullClearsHoldingFlag)
{
    // 设置为空应该清除持有标志
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);
        EXPECT_TRUE(enderman->isHoldingBlock());

        enderman->setHeldBlockState(nullptr);
        EXPECT_FALSE(enderman->isHoldingBlock());
        EXPECT_EQ(enderman->getHeldBlockState(), nullptr);
    }
}

// ==================== EndermanPlaceBlockGoal 测试 ====================

class EndermanPlaceBlockGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::EndermanPlaceBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanPlaceBlockGoal> goal;
};

TEST_F(EndermanPlaceBlockGoalTest, ShouldExecuteReturnsFalseWhenNotHolding)
{
    // 不持有时不应该执行
    EXPECT_FALSE(enderman->isHoldingBlock());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(EndermanPlaceBlockGoalTest, TypeNameReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "EndermanPlaceBlockGoal");
}

TEST_F(EndermanPlaceBlockGoalTest, ResetTaskDoesNotThrow)
{
    // resetTask 不应该抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== EndermanTakeBlockGoal 测试 ====================

class EndermanTakeBlockGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanTakeBlockGoal> goal;
};

TEST_F(EndermanTakeBlockGoalTest, ShouldExecuteReturnsFalseWhenAlreadyHolding)
{
    // 已经持有时不应该执行拾取
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);
        EXPECT_TRUE(enderman->isHoldingBlock());
        EXPECT_FALSE(goal->shouldExecute());
    }
}

TEST_F(EndermanTakeBlockGoalTest, TypeNameReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "EndermanTakeBlockGoal");
}

TEST_F(EndermanTakeBlockGoalTest, ResetTaskDoesNotThrow)
{
    // resetTask 不应该抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== 带方块存储的测试世界 ====================

namespace {

/**
 * @brief 末影人目标测试用的模拟世界
 *
 * 支持方块读写、游戏事件记录、实体查询存根。
 * 用于测试 canPlaceBlock、射线检测和游戏事件发射。
 */
class EndermanTestWorld : public IBlockReader {
public:
    EndermanTestWorld()
        : m_tickManager()
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }

    // ========== 方块存储 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        i64 k = packKey(x, y, z);
        auto it = m_blocks.find(k);
        if (it != m_blocks.end()) {
            return it->second;
        }
        // 未设置的位置返回空气方块状态（与真实世界行为一致）
        return BlockRegistry::instance().airState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        i64 k = packKey(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(k);
        } else {
            m_blocks[k] = state;
        }
        m_lastSetBlockPos = BlockPos(x, y, z);
        m_lastSetBlockState = state;
        return true;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { setBlockState(pos.x, pos.y, pos.z, state); }

    // ========== 游戏事件记录 ==========

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEvents.push_back({event.id(), pos, context.sourceEntity(), context.affectedState()});
    }

    [[nodiscard]] bool hasGameEvent(const char* eventId, const BlockPos& pos) const
    {
        for (const auto& ev : m_gameEvents) {
            if (std::strcmp(ev.eventId, eventId) == 0 && ev.pos == pos) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_t gameEventCount() const { return m_gameEvents.size(); }

    void clearGameEvents() { m_gameEvents.clear(); }

    // ========== 游戏规则（默认启用 mobGriefing）==========

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // ========== IWorld 存根实现 ==========

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return m_tickManager; }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // 最近一次 setBlockState 的参数
    BlockPos m_lastSetBlockPos{0, 0, 0};
    const BlockState* m_lastSetBlockState = nullptr;

private:
    static i64 packKey(i32 x, i32 y, i32 z)
    {
        return static_cast<i64>(x) | (static_cast<i64>(y) << 16) | (static_cast<i64>(z) << 32);
    }

    struct RecordedGameEvent {
        const char* eventId;
        BlockPos pos;
        const Entity* sourceEntity;
        const BlockState* affectedState;
    };

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<RecordedGameEvent> m_gameEvents;
    world::gamerule::GameRules m_gameRules;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
    test::DummyTickManager m_tickManager;
};

} // anonymous namespace

// ==================== canPlaceBlock isValidPosition 测试 ====================

class EndermanCanPlaceBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();

        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.0f, 64.0f, 0.0f);

        goal = std::make_unique<entity::ai::goal::EndermanPlaceBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    EndermanTestWorld world;
    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanPlaceBlockGoal> goal;
};

TEST_F(EndermanCanPlaceBlockTest, PlaceBlockEmitsBlockPlaceEvent)
{
    // 直接测试放置方块的逻辑，不依赖随机概率。
    // PlaceBlockGoal::shouldExecute() 有 1/2000 概率，直接遍历实体 ID 效率太低。
    // 因此我们直接调用 tick() 来绕过 shouldExecute() 的概率检查，
    // 并依赖随机坐标恰好命中有效放置位置。
    // 由于放置范围只有 2x2x2，我们在范围内填充多个可放置位置来增加命中率。

    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);
    const BlockState* stoneState = &stoneBlock->defaultState();

    // 设置末影人持有泥土
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);

    // 在末影人周围的多个位置放置支撑方块，使更多位置可以放置
    // 末影人在 (0.5, 64, 0.5)，放置范围 x ∈ [-1,0,1], y ∈ {64,65}, z ∈ [-1,0,1]
    for (int x = -1; x <= 1; ++x) {
        for (int z = -1; z <= 1; ++z) {
            // 在 y=63 放支撑，y=64 为空气（可放置）
            world.setBlockAt(BlockPos(x, 63, z), stoneState);
        }
    }
    world.clearGameEvents();

    // 直接调用 tick() 绕过 shouldExecute() 的 1/2000 概率，并用 setSeed(K) 确定性控制
    // 实体 RNG：tick() 内 nextDouble 决定放置坐标。下方 3x3 已填支撑、y=64 为空气可放置，
    // 几乎任意命中都会成功；setSeed 使每次试验确定，消除 id^now() 非确定性导致的 flaky。
    bool placed = false;
    for (uint64_t k = 1; k <= 200; ++k) {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.5f, 64.0f, 0.5f);
        enderman->setHeldBlockState(&dirtBlock->defaultState());
        enderman->getRandom().setSeed(k);
        goal = std::make_unique<entity::ai::goal::EndermanPlaceBlockGoal>(enderman.get());

        // 重置持有的方块状态
        enderman->setHeldBlockState(&dirtBlock->defaultState());

        // 直接调用 tick()（绕过 shouldExecute 的 1/2000 概率）
        goal->tick();

        if (!enderman->isHoldingBlock()) {
            placed = true;
            // 验证发出了 BLOCK_PLACE 事件
            EXPECT_GT(world.gameEventCount(), 0u);
            break;
        }
    }

    // 应该在 200 次内成功放置
    EXPECT_TRUE(placed);
}

TEST_F(EndermanCanPlaceBlockTest, TakeBlockEmitsBlockDestroyEvent)
{
    // 在末影人附近放置泥土
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);
    const BlockState* dirtState = &dirtBlock->defaultState();
    world.setBlockAt(BlockPos(1, 64, 0), dirtState);

    // 末影人没有持有方块
    ASSERT_FALSE(enderman->isHoldingBlock());

    enderman->setPosition(0.5f, 64.0f, 0.5f);
    world.clearGameEvents();

    // 直接调用 tick() 绕过 shouldExecute() 的 1/20 概率，并用 setSeed(K) 确定性控制
    // 实体 RNG：tick() 内 3 次 nextDouble 决定 (x,y,z)，setSeed 完全重置状态故每个 K
    // 是独立确定性试验。48 个候选位置中 (1,64,0) 占其一，期望 ~48 次内命中，5000 次充裕。
    // 旧实现依赖 id^now() 非确定种子 + shouldExecute 概率门，5000 次 P(0 成功)≈0.55% 导致 flaky。
    bool pickedUp = false;
    for (uint64_t k = 1; k <= 5000; ++k) {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.5f, 64.0f, 0.5f);
        enderman->getRandom().setSeed(k);
        auto takeGoal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());

        // 重置方块状态（前一次迭代可能被拾取设为空气）
        world.setBlockAt(BlockPos(1, 64, 0), dirtState);
        world.clearGameEvents();

        takeGoal->tick();
        if (enderman->isHoldingBlock()) {
            pickedUp = true;
            // 验证发出了 BLOCK_DESTROY 事件
            EXPECT_TRUE(world.hasGameEvent("block_destroy", BlockPos(1, 64, 0)));
            break;
        }
    }

    // 应该在 5000 次内成功拾取
    EXPECT_TRUE(pickedUp);
}

// ==================== isValidPosition 拒绝测试 ====================

/**
 * @brief 测试仙人掌不能放在非沙地上
 *
 * 仙人掌的 isValidPosition 要求下方是沙子或仙人掌，
 * 如果放在石头上方，canPlaceBlock 应该拒绝。
 * 此测试通过直接验证 Block::isValidPosition 来确认逻辑。
 */
TEST_F(EndermanCanPlaceBlockTest, CactusCannotSurviveOnStone)
{
    auto* cactusBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "cactus"));
    if (cactusBlock == nullptr) {
        GTEST_SKIP() << "Cactus block not registered";
    }

    // 放置石头作为支撑
    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);
    world.setBlockAt(BlockPos(0, 63, 0), &stoneBlock->defaultState());
    world.setBlockAt(BlockPos(0, 64, 0), nullptr); // 目标位置是空气

    // 直接验证 isValidPosition 返回 false
    auto& blockReader = static_cast<IBlockReader&>(world);
    const BlockState& cactusState = cactusBlock->defaultState();
    EXPECT_FALSE(cactusBlock->isValidPosition(cactusState, blockReader, BlockPos(0, 64, 0)));
}

/**
 * @brief 测试仙人掌可以放在沙子上
 */
TEST_F(EndermanCanPlaceBlockTest, CactusCanSurviveOnSand)
{
    auto* cactusBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "cactus"));
    if (cactusBlock == nullptr) {
        GTEST_SKIP() << "Cactus block not registered";
    }

    auto* sandBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "sand"));
    if (sandBlock == nullptr) {
        GTEST_SKIP() << "Sand block not registered";
    }

    world.setBlockAt(BlockPos(0, 63, 0), &sandBlock->defaultState());
    world.setBlockAt(BlockPos(0, 64, 0), nullptr); // 目标位置是空气

    auto& blockReader = static_cast<IBlockReader&>(world);
    const BlockState& cactusState = cactusBlock->defaultState();
    EXPECT_TRUE(cactusBlock->isValidPosition(cactusState, blockReader, BlockPos(0, 64, 0)));
}

// ==================== hasEnoughSolidSide 测试 ====================

/**
 * @brief 测试石头方块的顶面有足够的固体支撑
 */
TEST_F(EndermanCanPlaceBlockTest, StoneHasEnoughSolidSideUp)
{
    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);
    world.setBlockAt(BlockPos(0, 63, 0), &stoneBlock->defaultState());

    EXPECT_TRUE(Block::hasEnoughSolidSide(world, BlockPos(0, 63, 0), Direction::Up));
}

/**
 * @brief 测试空气方块的顶面没有足够的固体支撑
 */
TEST_F(EndermanCanPlaceBlockTest, AirDoesNotHaveEnoughSolidSideUp)
{
    // 空气位置（未设置方块 = nullptr = 空气）
    EXPECT_FALSE(Block::hasEnoughSolidSide(world, BlockPos(0, 63, 0), Direction::Up));
}

/**
 * @brief 测试基岩方块的顶面有足够的固体支撑
 */
TEST_F(EndermanCanPlaceBlockTest, BedrockHasEnoughSolidSideUp)
{
    auto* bedrockBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "bedrock"));
    if (bedrockBlock == nullptr) {
        GTEST_SKIP() << "Bedrock block not registered";
    }
    world.setBlockAt(BlockPos(0, 63, 0), &bedrockBlock->defaultState());

    EXPECT_TRUE(Block::hasEnoughSolidSide(world, BlockPos(0, 63, 0), Direction::Up));
}

// ==================== 射线检测测试 ====================

class EndermanRaycastTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }

    EndermanTestWorld world;
};

/**
 * @brief 测试射线检测：无阻挡时命中目标方块
 */
TEST_F(EndermanRaycastTest, RayHitsTargetBlockWithoutObstruction)
{
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);
    world.setBlockAt(BlockPos(2, 64, 0), &dirtBlock->defaultState());

    // 从 (0.5, 64.5, 0.5) 射向 (2.5, 64.5, 0.5)，中间无阻挡
    Vector3f startPos(0.5f, 64.5f, 0.5f);
    Vector3f targetPos(2.5f, 64.5f, 0.5f);
    Vector3f direction = (targetPos - startPos).normalized();
    f32 distance = static_cast<f32>((targetPos - startPos).length());

    Ray ray(startPos, direction);
    RaycastContext context(ray, distance);
    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));
}

/**
 * @brief 测试射线检测：有阻挡时不能命中目标方块
 */
TEST_F(EndermanRaycastTest, RayBlockedByInterveningBlock)
{
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);
    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);

    // 目标方块在 (2, 64, 0)
    world.setBlockAt(BlockPos(2, 64, 0), &dirtBlock->defaultState());
    // 阻挡方块在 (1, 64, 0)
    world.setBlockAt(BlockPos(1, 64, 0), &stoneBlock->defaultState());

    // 从 (0.5, 64.5, 0.5) 射向 (2.5, 64.5, 0.5)，中间有石头阻挡
    Vector3f startPos(0.5f, 64.5f, 0.5f);
    Vector3f targetPos(2.5f, 64.5f, 0.5f);
    Vector3f direction = (targetPos - startPos).normalized();
    f32 distance = static_cast<f32>((targetPos - startPos).length());

    Ray ray(startPos, direction);
    RaycastContext context(ray, distance);
    BlockRaycastResult result = raycastBlocks(context, world);

    // 射线应该命中中间的石头而不是目标泥土
    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(1, 64, 0));
    EXPECT_NE(result.blockPos(), BlockPos(2, 64, 0));
}

/**
 * @brief 测试射线检测：空世界无命中
 */
TEST_F(EndermanRaycastTest, RayMissesInEmptyWorld)
{
    // 空世界中射线不会命中任何方块
    Vector3f startPos(0.5f, 64.5f, 0.5f);
    Vector3f direction(1.0f, 0.0f, 0.0f);
    Ray ray(startPos, direction);
    RaycastContext context(ray, 10.0f);
    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_FALSE(result.isHit());
    EXPECT_TRUE(result.isMiss());
}

// ==================== 拾取方块射线检测集成测试 ====================

class EndermanTakeBlockRaycastTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();

        // 使用 EntityInstanceId(1) 作为默认实体 ID。
        // 注意：Entity 构造用 id ^ high_resolution_clock::now() 作 RNG 种子（非确定），
        // 需要确定性的用例在 tick() 前用 getRandom().setSeed(k) 重置。
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.5f, 64.0f, 0.5f);

        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    EndermanTestWorld world;
    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanTakeBlockGoal> goal;
};

/**
 * @brief 测试末影人能拾取可见方块（无阻挡）
 *
 * 用 setSeed(K) 确定性控制实体 RNG，直接调用 tick() 绕过 shouldExecute() 的 1/20 概率，
 * 在 48 个候选位置中遍历 K 直到 tick() 的 3 次 nextDouble 命中 (1,64,0)。
 */
TEST_F(EndermanTakeBlockRaycastTest, CanPickUpVisibleBlock)
{
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);

    // 在 (1, 64, 0) 放置泥土
    world.setBlockAt(BlockPos(1, 64, 0), &dirtBlock->defaultState());
    world.clearGameEvents();

    // setSeed 完全重置 RNG 状态，每个 K 是独立确定性试验；48 个位置中 (1,64,0) 占其一。
    bool pickedUp = false;
    for (uint64_t k = 1; k <= 5000; ++k) {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.5f, 64.0f, 0.5f);
        enderman->getRandom().setSeed(k);
        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());

        // 重置方块状态（可能被前一次迭代修改）
        world.setBlockAt(BlockPos(1, 64, 0), &dirtBlock->defaultState());
        world.clearGameEvents();

        goal->tick();
        if (enderman->isHoldingBlock()) {
            pickedUp = true;
            break;
        }
    }

    // 应该在 5000 次内成功拾取
    EXPECT_TRUE(pickedUp);

    // 拾取后应该发出了 BLOCK_DESTROY 事件
    if (pickedUp) {
        EXPECT_GT(world.gameEventCount(), 0u);
    }
}

/**
 * @brief 测试末影人不能拾取不可达方块（被墙挡住）
 *
 * 当目标方块和末影人之间有一堵墙时，射线被阻挡，末影人不能拾取。
 * 遍历实体 ID 寻找使得 shouldExecute() 返回 true 的组合，
 * 并验证即使随机位置选中了被阻挡的方块，也不会穿墙拾取。
 */
TEST_F(EndermanTakeBlockRaycastTest, CannotPickUpBlockedBlock)
{
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirtBlock, nullptr);
    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);

    // 将末影人放在 (5.5, 64, 5.5)，目标泥土在 (7, 64, 5)，中间有石头墙在 (6, 64, 5)
    enderman->setPosition(5.5f, 64.0f, 5.5f);
    world.setBlockAt(BlockPos(7, 64, 5), &dirtBlock->defaultState());
    world.setBlockAt(BlockPos(6, 64, 5), &stoneBlock->defaultState()); // 阻挡

    ASSERT_FALSE(enderman->isHoldingBlock());

    // 用 setSeed(K) 确定性控制 RNG，直接调 tick() 绕过 shouldExecute()，确保每次迭代
    // 真正进入 tick 路径；射线被 (6,64,5) 石头阻挡，末影人不应拾取 (7,64,5) 的泥土。
    for (uint64_t k = 1; k <= 500; ++k) {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(5.5f, 64.0f, 5.5f);
        enderman->getRandom().setSeed(k);
        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());

        // 确保方块状态正确
        world.setBlockAt(BlockPos(7, 64, 5), &dirtBlock->defaultState());
        world.setBlockAt(BlockPos(6, 64, 5), &stoneBlock->defaultState());

        goal->tick();

        // 末影人不应该拾取被阻挡的方块
        EXPECT_FALSE(enderman->isHoldingBlock());

        // 被阻挡的泥土方块应该仍然存在
        const BlockState* blockedState = world.getBlockState(7, 64, 5);
        ASSERT_NE(blockedState, nullptr);
        EXPECT_FALSE(blockedState->isAir());
    }
}

/**
 * @brief 测试末影人不会拾取非 ENDERMAN_HOLDABLE 方块
 *
 * 遍历实体 ID，验证无论随机种子如何，石头都不会被拾取。
 */
TEST_F(EndermanTakeBlockRaycastTest, CannotPickUpNonHoldableBlock)
{
    auto* stoneBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneBlock, nullptr);

    // 石头不在 ENDERMAN_HOLDABLE 标签中，放在末影人旁边
    world.setBlockAt(BlockPos(1, 64, 0), &stoneBlock->defaultState());

    // 用 setSeed(K) 确定性控制 RNG，直接调 tick() 绕过 shouldExecute()，确保每次迭代
    // 真正进入 tick 路径；石头不可拾取，末影人应始终不持有方块。
    for (uint64_t k = 1; k <= 500; ++k) {
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1));
        enderman->setWorld(&world);
        enderman->setPosition(0.5f, 64.0f, 0.5f);
        enderman->getRandom().setSeed(k);
        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());

        // 确保石头还在
        world.setBlockAt(BlockPos(1, 64, 0), &stoneBlock->defaultState());

        goal->tick();

        // 末影人应该不会拾取石头
        ASSERT_FALSE(enderman->isHoldingBlock());
    }
}

} // namespace test
} // namespace mc
