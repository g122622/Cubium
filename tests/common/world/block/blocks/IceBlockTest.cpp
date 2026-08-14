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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
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

// ============================================================================
// 测试用世界桩
// ============================================================================

class IceTestWorld final : public mc::test::BaseTestWorld {
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

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_blockLight, x, y, z); }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z); }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isUltraWarm() const override { return m_isUltraWarm; }
    [[nodiscard]] DimensionId dimension() const override { return m_dimension; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setUltraWarm(bool value) { m_isUltraWarm = value; }
    void setDimension(DimensionId dim) { m_dimension = dim; }
    void setDayTime(i64 time) { m_dayTime = time; }
    void setRaining(bool value) { m_isRaining = value; }
    void setThundering(bool value) { m_isThundering = value; }

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
    DimensionId m_dimension = DimensionManager::OVERWORLD;
    i64 m_dayTime = 6000; // 默认正午（6000 ticks），天空减暗 = 0
    bool m_isRaining = false;
    bool m_isThundering = false;
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

// ============================================================================
// IceBlock 融化测试
// ============================================================================

TEST_F(IceBlockTest, RandomTurnsIceIntoWaterInNormalDimension)
{
    IceTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTickTurnsIceIntoAirInUltraWarmDimension)
{
    IceTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(6, 64, 6);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setUltraWarm(true);
    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    EXPECT_EQ(world.getBlockState(pos), nullptr);
}

// MC 原版: IceBlock.randomTick 仅检查方块光照，不考虑天空光照
// 条件: blockLight > 11 - opacity（冰的 opacity=2，即 blockLight > 9）
TEST_F(IceBlockTest, RandomTick_MeltsWithBlockLightAbove9)
{
    // 冰的不透明度为2，融化条件 blockLight > 11 - 2 = 9
    // blockLight = 10 > 9，应该融化
    IceTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(1, 64, 1);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 10); // > 9，满足融化条件
    world.setSkyLightAt(pos, 0);    // 天空光照不影响

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Ice should have melted into water";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTick_DoesNotMeltWithBlockLightAt9)
{
    // blockLight = 9 == 11 - 2，不满足 > 条件（严格大于）
    IceTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(2, 64, 2);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 9); // == 11 - 2，不满足 >
    world.setSkyLightAt(pos, 0);

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Ice should not melt at block light = 9";
    EXPECT_TRUE(finalState->is(VanillaBlocks::ICE)) << "Ice should remain at block light = 9";
}

TEST_F(IceBlockTest, RandomTick_DoesNotMeltWithOnlySkyLight)
{
    // MC 原版: IceBlock.randomTick 仅使用方块光照 (LightLayer.BLOCK)
    // 天空光照不影响冰融化
    IceTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(3, 64, 3);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 0); // 方块光照 = 0
    world.setSkyLightAt(pos, 15);  // 天空光照不影响

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Ice should not melt with only sky light";
    EXPECT_TRUE(finalState->is(VanillaBlocks::ICE)) << "Ice should not melt with only sky light";
}

TEST_F(IceBlockTest, RandomTick_MeltsImmediatelyNoProbability)
{
    // MC 原版: IceBlock.randomTick 不使用随机概率，条件满足时立即融化
    // 旧实现有 1/40 的概率门，这是不正确的
    IceTestWorld world;
    // 即使 random 返回 0 也不应影响融化（无概率门）
    SequenceRandom random({999}); // 任意值都不应阻止融化
    const BlockPos pos(5, 64, 5);
    const BlockState& state = VanillaBlocks::ICE->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);

    BlockState mutableState = state;
    VanillaBlocks::ICE->randomTick(world, pos, mutableState, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Ice should melt immediately regardless of random";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

// ============================================================================
// FrostedIceBlock 测试
// ============================================================================

TEST_F(IceBlockTest, FrostedIceTick_MeltsWithHighLightInOverworld)
{
    // FrostedIceBlock.tick() 在主世界使用 getMaxLocalRawBrightness
    // 正午(dayTime=6000), 晴天: skyDarkening=0, skyLight=15 → maxLocalRawBrightness=15
    // 条件: lightLevel > 11 - AGE(0) - opacity(2) = 9, 15 > 9 满足
    IceTestWorld world;
    world.ensureTickManager();
    world.setDayTime(6000); // 正午，天空减暗 = 0

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(8, 64, 8);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);
    world.setSkyLightAt(pos, 15);

    // 霜冰需要 4 次 tick 才能融化（age 0->1->2->3->melt）
    for (int i = 0; i < 4; ++i) {
        const BlockState* currentState = world.getBlockState(pos);
        if (currentState == nullptr) break;
        BlockState mutableState = *currentState;
        VanillaBlocks::FROSTED_ICE->tick(world, pos, mutableState, world.getRandom());
    }

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should have melted into water";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, FrostedIceTick_UsesOnlyBlockLightInEndDimension)
{
    // FrostedIceBlock.tick() 在无天空光照的维度仅使用方块光照
    // MC 原版使用 dimension() == Level.END 判断，等效于 !hasSkyLight()
    // 设置末地维度（无天空光照），blockLight=0, skyLight=15 → 不应融化
    IceTestWorld world;
    world.ensureTickManager();
    world.setDimension(DimensionManager::THE_END); // hasSkyLight() = false
    world.setDayTime(6000);

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(9, 64, 9);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 0); // 方块光照 = 0
    world.setSkyLightAt(pos, 15);  // 末地仅检查方块光照，天空光照不应影响

    // 触发 tick，应该不融化
    BlockState mutableState = state;
    VanillaBlocks::FROSTED_ICE->tick(world, pos, mutableState, world.getRandom());

    const BlockState* currentState = world.getBlockState(pos);
    ASSERT_NE(currentState, nullptr) << "Frosted ice should not melt with only sky light in The End";
    EXPECT_TRUE(currentState->is(VanillaBlocks::FROSTED_ICE))
        << "Frosted ice should not melt with only sky light in The End";
}

TEST_F(IceBlockTest, FrostedIceTick_MeltsInEndWithHighBlockLight)
{
    // FrostedIceBlock.tick() 在无天空光照的维度，blockLight=15 应导致融化
    IceTestWorld world;
    world.ensureTickManager();
    world.setDimension(DimensionManager::THE_END); // hasSkyLight() = false
    world.setDayTime(6000);

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(10, 64, 10);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);
    world.setSkyLightAt(pos, 0);

    // 霜冰需要 4 次 tick 才能融化
    for (int i = 0; i < 4; ++i) {
        const BlockState* currentState = world.getBlockState(pos);
        if (currentState == nullptr) break;
        BlockState mutableState = *currentState;
        VanillaBlocks::FROSTED_ICE->tick(world, pos, mutableState, world.getRandom());
    }

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should have melted with high block light in The End";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, FrostedIceRandomTick_InheritsIceBlockBehavior)
{
    // MC 原版: FrostedIceBlock 继承自 IceBlock，不重写 randomTick
    // randomTick 使用 IceBlock 的逻辑：仅检查方块光照 > 11 - opacity
    IceTestWorld world;
    world.ensureTickManager();

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(11, 64, 11);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 15);
    world.setSkyLightAt(pos, 0);

    // randomTick 应该像 IceBlock 一样立即融化
    BlockState mutableState = state;
    VanillaBlocks::FROSTED_ICE->randomTick(world, pos, mutableState, world.getRandom());

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should have melted via randomTick with high block light";
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, FrostedIceRandomTick_DoesNotMeltWithOnlySkyLight)
{
    // randomTick 仅检查方块光照，天空光照不影响
    IceTestWorld world;
    world.ensureTickManager();

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(12, 64, 12);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 0);
    world.setSkyLightAt(pos, 15);

    BlockState mutableState = state;
    VanillaBlocks::FROSTED_ICE->randomTick(world, pos, mutableState, world.getRandom());

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should not melt with only sky light via randomTick";
    EXPECT_TRUE(finalState->is(VanillaBlocks::FROSTED_ICE))
        << "Frosted ice should not melt with only sky light via randomTick";
}

TEST_F(IceBlockTest, FrostedIceRandomTick_DoesNotMeltWithLowBlockLight)
{
    // FrostedIce 的 opacity=2, randomTick 条件: blockLight > 11 - 2 = 9
    // blockLight = 9 不满足 > 9
    IceTestWorld world;
    world.ensureTickManager();

    ASSERT_NE(VanillaBlocks::FROSTED_ICE, nullptr);
    const BlockState& state = VanillaBlocks::FROSTED_ICE->defaultState();
    const BlockPos pos(13, 64, 13);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 9); // == 11 - opacity, 不满足 >
    world.setSkyLightAt(pos, 0);

    BlockState mutableState = state;
    VanillaBlocks::FROSTED_ICE->randomTick(world, pos, mutableState, world.getRandom());

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Frosted ice should not melt at block light = 9";
    EXPECT_TRUE(finalState->is(VanillaBlocks::FROSTED_ICE)) << "Frosted ice should not melt at block light = 9";
}

} // namespace
