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
#include "common/util/math/random/IRandom.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/ice/IceBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

class IceTestWorld final : public test::BaseTestWorld {
public:
    IceTestWorld() = default;

    // 延迟初始化 TickManager（首次调用时初始化）
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            // 存储 BlockState 的副本到 m_ownedStates 中
            // 因为传入的 state 可能是临时对象
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_blockLight, x, y, z); }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z); }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isUltraWarm() const override { return m_isUltraWarm; }

    void setUltraWarm(bool value) { m_isUltraWarm = value; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }

    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<IceTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void advanceTick()
    {
        tickManager().tick(m_currentTick);
        ++m_currentTick;
    }

private:
    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        const auto it = lights.find(pos);
        if (it != lights.end()) {
            return it->second;
        }
        return 0;
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates; // 存储方块状态副本，避免悬空指针
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_currentTick = 0; // 当前游戏刻
    bool m_isUltraWarm = false;
};

class SequenceRandom final : public math::IRandom {
public:
    explicit SequenceRandom(std::vector<i32> values)
        : m_values(std::move(values))
    {}

    void setSeed(u64 seed) override
    {
        m_seed = seed;
        m_index = 0;
    }

    [[nodiscard]] u64 nextU64() override { return static_cast<u64>(nextValue()); }

    [[nodiscard]] u32 nextU32() override { return static_cast<u32>(nextValue()); }

    [[nodiscard]] i32 nextInt(i32 bound) override { return nextValue() % bound; }

    [[nodiscard]] i32 nextInt() override { return nextValue(); }

    [[nodiscard]] i32 nextInt(i32 min, i32 max) override { return min + (nextValue() % (max - min + 1)); }

    [[nodiscard]] bool nextBoolean() override { return (nextValue() & 1) != 0; }

    [[nodiscard]] f32 nextFloat() override
    {
        return static_cast<f32>(nextValue() & 0x00FFFFFF) / static_cast<f32>(1 << 24);
    }

    [[nodiscard]] f32 nextFloat(f32 min, f32 max) override { return min + nextFloat() * (max - min); }

    [[nodiscard]] f64 nextDouble() override
    {
        return static_cast<f64>(nextValue() & 0x001FFFFFFFFFFFFF) / static_cast<f64>(1ULL << 53);
    }

    [[nodiscard]] f64 nextDouble(f64 min, f64 max) override { return min + nextDouble() * (max - min); }

    [[nodiscard]] f32 nextGaussian(f32 mean, f32 stddev) override
    {
        return mean + stddev * static_cast<f32>(nextValue());
    }

    [[nodiscard]] i64 nextLong() override { return static_cast<i64>(nextValue()); }

    [[nodiscard]] i64 nextLong(i64 bound) override { return static_cast<i64>(nextValue() % bound); }

private:
    [[nodiscard]] i32 nextValue()
    {
        if (m_index < m_values.size()) {
            return m_values[m_index++];
        }
        return 0;
    }

    std::vector<i32> m_values;
    size_t m_index = 0;
    u64 m_seed = 0;
};

class IceBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(IceBlockTest, RandomTickTurnsIceIntoWaterInNormalDimension)
{
    IceTestWorld world;
    IceBlock ice(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = ice.defaultState();

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    ice.randomTick(world, pos, state, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTickTurnsIceIntoAirInUltraWarmDimension)
{
    IceTestWorld world;
    IceBlock ice(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(6, 64, 6);
    BlockState state = ice.defaultState();

    world.setUltraWarm(true);
    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    ice.randomTick(world, pos, state, random);

    EXPECT_EQ(world.getBlockState(pos), nullptr);
}

// FrostedIceBlock 融化测试：
// FrostedIceBlock::randomTick() 调用 tick()
// tick() 检查融化条件，如果满足则调用 slightlyMelt() 增加 AGE 或融化
// 每次调用 randomTick 都会处理一次 tick 逻辑
// 霜冰需要 AGE 从 0 增加到 3（4 次），然后才会融化成水
TEST_F(IceBlockTest, RandomTickTurnsFrostedIceIntoWaterInNormalDimension)
{
    IceTestWorld world;
    world.ensureTickManager(); // 确保 TickManager 已初始化

    // 使用已注册的 FROSTED_ICE 方块，确保状态正确初始化
    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr) << "FROSTED_ICE should be registered";
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(8, 64, 8);

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    // 霜冰需要 4 次 tick 才能融化（age 0->1->2->3->melt）
    // 使用足够高的光照确保融化
    // 每次调用 randomTick 会处理一个 tick
    // 注意：tick() 内部会调度下一次 tick，但我们直接调用 randomTick 即可
    for (int i = 0; i < 4; ++i) {
        const BlockState* currentState = world.getBlockState(pos);
        if (currentState == nullptr) break; // 已经融化
        BlockState mutableState = *currentState;
        // 使用世界的随机数生成器（固定种子，确定性测试）
        VanillaBlocks::FROSTED_ICE->randomTick(world, pos, mutableState, world.getRandom());
    }

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should have melted into water";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTickTurnsFrostedIceIntoAirInUltraWarmDimension)
{
    IceTestWorld world;
    world.ensureTickManager(); // 确保 TickManager 已初始化
    world.setUltraWarm(true);

    // 使用已注册的 FROSTED_ICE 方块
    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr) << "FROSTED_ICE should be registered";
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(10, 64, 10);

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    // 霜冰需要 4 次 tick 才能融化（age 0->1->2->3->melt）
    for (int i = 0; i < 4; ++i) {
        const BlockState* currentState = world.getBlockState(pos);
        if (currentState == nullptr) break; // 已经融化
        BlockState mutableState = *currentState;
        VanillaBlocks::FROSTED_ICE->randomTick(world, pos, mutableState, world.getRandom());
    }

    // 在超热维度（下界），霜冰融化后变成空气而非水
    EXPECT_EQ(world.getBlockState(pos), nullptr) << "Frosted ice should have melted into air in ultra-warm dimension";
}

} // namespace
