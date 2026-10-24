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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/cave/FrogspawnBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/block/registry/WildBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/// 青蛙卵测试世界：稀疏方块存储 + 可注入的流体查询 + 实体生成记录。
class FrogspawnTestWorld final : public mc::test::BaseTestWorld {
public:
    FrogspawnTestWorld()
    {
        m_tickManager = std::make_unique<world::tick::TickManager>(*this);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        return it != m_blocks.end() ? it->second.get() : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
            return true;
        }
        m_blocks[packPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    /// 设置 pos 处流体（nullptr 表示空流体）
    void setFluid(const BlockPos& pos, const fluid::FluidState* state)
    {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_fluids.find(packPos(x, y, z));
        if (it != m_fluids.end()) {
            return it->second;
        }
        return fluid::Fluids::EMPTY() != nullptr ? &fluid::Fluids::EMPTY()->defaultState() : nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        return *m_tickManager;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return *m_tickManager;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(entity->entityType());
        return EntityInstanceId(static_cast<u32>(m_spawned.size()));
    }

    [[nodiscard]] const std::vector<const entity::EntityType*>& spawned() const
    {
        return m_spawned;
    }

    void set(const BlockPos& pos, const BlockState& state)
    {
        setBlockState(pos.x, pos.y, pos.z, &state);
    }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ z;
    }

    std::unique_ptr<world::tick::TickManager> m_tickManager;
    std::unordered_map<i64, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    std::vector<const entity::EntityType*> m_spawned;
};

/// 固定序列随机数：按顺序返回预设整数，耗尽后返回 0；浮点按 nextInt(1000)/1000 派生。
class ScriptedRandom final : public math::IRandom {
public:
    explicit ScriptedRandom(std::vector<i32> values)
        : m_values(std::move(values))
    {}

    void setSeed(u64) override {}

    [[nodiscard]] i32 nextInt() override
    {
        return _take();
    }

    [[nodiscard]] i32 nextInt(i32 bound) override
    {
        return _take() % bound;
    }

    [[nodiscard]] i32 nextInt(i32 min, i32 max) override
    {
        MC_ASSERT_RELEASE(min <= max);
        return min + (_take() % (max - min + 1));
    }

    [[nodiscard]] bool nextBoolean() override
    {
        return (_take() & 1) != 0;
    }

    [[nodiscard]] f32 nextFloat() override
    {
        return static_cast<f32>(nextDouble());
    }

    [[nodiscard]] f32 nextFloat(f32 min, f32 max) override
    {
        return min + nextFloat() * (max - min);
    }

    [[nodiscard]] f64 nextDouble() override
    {
        return static_cast<f64>(_take() % 1000) / 1000.0;
    }

    [[nodiscard]] u32 nextU32() override
    {
        return static_cast<u32>(_take());
    }

    [[nodiscard]] u64 nextU64() override
    {
        return static_cast<u64>(_take());
    }

    math::PositionalRandomFactory forkPositional() override
    {
        return math::PositionalRandomFactory(0u);
    }

private:
    i32 _take()
    {
        if (m_index >= m_values.size()) {
            return 0;
        }
        return m_values[m_index++];
    }

    std::vector<i32> m_values;
    size_t m_index = 0;
};

class FrogspawnBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    /// 在 pos 下方放水、pos 与上方留空：可存活的标准环境
    void prepareWaterBelow(const BlockPos& pos)
    {
        world.setFluid(pos.down(), &fluid::Fluids::WATER()->defaultState());
        world.setFluid(pos, &fluid::Fluids::EMPTY()->defaultState());
        world.setFluid(pos.up(), &fluid::Fluids::EMPTY()->defaultState());
    }

    FrogspawnTestWorld world;
    FrogspawnBlock* block = static_cast<FrogspawnBlock*>(block_registry::WildBlocks::FROGSPAWN);
};

} // namespace

// 青蛙卵注册为专用方块类型
TEST_F(FrogspawnBlockTest, RegisteredAsDedicatedBlockType)
{
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:frogspawn"));
}

// 下方为水且上方无流体时可存活
TEST_F(FrogspawnBlockTest, CanSurvive_RequiresWaterBelowAndClearAbove)
{
    const BlockPos pos(3, 64, 3);
    auto& reader = static_cast<IBlockReader&>(world);
    const BlockState& state = block->defaultState();

    // 初始无流体：不可存活
    EXPECT_FALSE(block->isValidPosition(state, reader, pos));

    prepareWaterBelow(pos);
    EXPECT_TRUE(block->isValidPosition(state, reader, pos));

    // 上方被水占据：不可存活
    world.setFluid(pos.up(), &fluid::Fluids::WATER()->defaultState());
    EXPECT_FALSE(block->isValidPosition(state, reader, pos));

    // 恢复上方为空，但下方换成流动水或岩浆：不可存活
    world.setFluid(pos.up(), &fluid::Fluids::EMPTY()->defaultState());
    world.setFluid(pos.down(), &fluid::Fluids::LAVA()->defaultState());
    EXPECT_FALSE(block->isValidPosition(state, reader, pos));
}

// 邻居变化时立即校验存活：仍可存活则保持原状态，否则变为空气
TEST_F(FrogspawnBlockTest, UpdatePostPlacement_RemovesWhenInvalid)
{
    const BlockPos pos(3, 64, 3);
    prepareWaterBelow(pos);
    const BlockState& state = block->defaultState();

    BlockState kept = block->updatePostPlacement(state, Direction::Down, state, world, pos, pos.down());
    EXPECT_EQ(&kept.getBlock(), block);

    world.setFluid(pos.down(), &fluid::Fluids::EMPTY()->defaultState());
    BlockState removed = block->updatePostPlacement(state, Direction::Down, state, world, pos, pos.down());
    EXPECT_EQ(&removed.getBlock(), VanillaBlocks::AIR);
}

// 孵化 tick：移除青蛙卵并生成 2~5 只持久化的蝌蚪，位置在 0.2~0.8 方块范围内
TEST_F(FrogspawnBlockTest, Tick_HatchesTadpolesInWaterRange)
{
    const BlockPos pos(3, 64, 3);
    prepareWaterBelow(pos);
    world.set(pos, block->defaultState());

    // 注入值 3：(3 % 4) + 2 = 5 只；随后两次 nextDouble 派生位置偏移、一次朝向
    ScriptedRandom random({3, 200, 800, 180});
    BlockState state = block->defaultState();
    block->tick(world, pos, state, random);

    const BlockState* current = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(&current->getBlock(), VanillaBlocks::AIR) << "孵化后青蛙卵应消失";
    const auto& spawned = world.spawned();
    ASSERT_EQ(spawned.size(), 5u);
    for (const auto* type : spawned) {
        ASSERT_NE(type, nullptr);
        EXPECT_EQ(type, entity::VanillaEntityTypeKeys::TADPOLE);
    }
}

// 孵化 tick 时已不可存活：直接消失且不生成蝌蚪
TEST_F(FrogspawnBlockTest, Tick_WhenInvalid_ClearsWithoutSpawning)
{
    const BlockPos pos(3, 64, 3);
    world.set(pos, block->defaultState());

    ScriptedRandom random({4});
    BlockState state = block->defaultState();
    block->tick(world, pos, state, random);

    const BlockState* current = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(&current->getBlock(), VanillaBlocks::AIR);
    EXPECT_TRUE(world.spawned().empty());
}

// _hatchDelay 在闭区间内取值，经随机数取值落在 [3600, 12000) 等价原版范围
TEST_F(FrogspawnBlockTest, HatchDelay_WithinVanillaRange)
{
    // 闭区间 [3600, 11999]：下界与上界返回值均在原版 [3600, 12000) 内
    ScriptedRandom lower({0});
    ScriptedRandom upper({8399});
    EXPECT_EQ(block->getHatchDelayForTest(lower), FrogspawnBlock::MIN_HATCH_TICK_DELAY);
    EXPECT_EQ(block->getHatchDelayForTest(upper), FrogspawnBlock::MAX_HATCH_TICK_DELAY - 1);
}

// 放置回调在服务端调度孵化 tick，不抛出异常且不修改方块
TEST_F(FrogspawnBlockTest, OnBlockAdded_SchedulesWithoutSideEffects)
{
    const BlockPos pos(3, 64, 3);
    prepareWaterBelow(pos);
    world.set(pos, block->defaultState());
    const BlockState& state = block->defaultState();

    block->onBlockAdded(world, pos, state, false);

    const BlockState* current = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(&current->getBlock(), block);
}
