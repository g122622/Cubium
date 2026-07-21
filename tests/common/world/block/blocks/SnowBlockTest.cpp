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

#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

// ============================================================================
// 测试用世界桩
// ============================================================================

/**
 * @brief SnowBlock 测试用的世界桩
 *
 * 继承 IBlockReader（IWorld 的子类），提供可控的方块状态和光照。
 * 使用 IBlockReader 而非 BaseTestWorld 是因为 isValidPosition 需要 IBlockReader& 参数。
 */
class SnowTestWorld final : public IBlockReader {
public:
    SnowTestWorld() = default;

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

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_blockLight, x, y, z); }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z); }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<SnowTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }
    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }
    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

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
    std::map<BlockPos, BlockState> m_ownedStates;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
    u64 m_currentTick = 0;
};

// ============================================================================
// 确定性随机数生成器
// ============================================================================

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

// ============================================================================
// 测试夹具
// ============================================================================

class SnowBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// LAYERS 属性测试
// ============================================================================

TEST_F(SnowBlockTest, DefaultState_HasOneLayer)
{
    // 默认状态应为 1 层
    const BlockState& defaultState = VanillaBlocks::SNOW->defaultState();
    EXPECT_EQ(defaultState.get(SnowBlock::LAYERS()), 1);
}

TEST_F(SnowBlockTest, LAYERS_PropertyRangeIs1To8)
{
    // LAYERS 属性范围应为 1-8
    const IntegerProperty& layers = SnowBlock::LAYERS();
    EXPECT_EQ(layers.minValue(), 1);
    EXPECT_EQ(layers.maxValue(), 8);
}

TEST_F(SnowBlockTest, SetLayers_ValidValue)
{
    // 设置 LAYERS 属性为有效值
    const BlockState& defaultState = VanillaBlocks::SNOW->defaultState();
    const BlockState* state8 = &defaultState.with(SnowBlock::LAYERS(), 8);
    ASSERT_NE(state8, nullptr);
    EXPECT_EQ(state8->get(SnowBlock::LAYERS()), 8);

    const BlockState* state4 = &defaultState.with(SnowBlock::LAYERS(), 4);
    ASSERT_NE(state4, nullptr);
    EXPECT_EQ(state4->get(SnowBlock::LAYERS()), 4);
}

// ============================================================================
// randomTick 融化测试
// ============================================================================

TEST_F(SnowBlockTest, RandomTick_MeltsWhenBlockLightAbove11)
{
    // MC 原版: SnowLayerBlock.randomTick 仅检查方块光照 (LightLayer.BLOCK)
    // 条件: blockLight > 11，即方块光照 >= 12 时雪融化
    // 方块光照 > 11 时，雪层应融化并变为空气
    SnowTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = VanillaBlocks::SNOW->defaultState();

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 12);
    world.setSkyLightAt(pos, 0);

    VanillaBlocks::SNOW->randomTick(world, pos, state, random);

    // 雪层应被替换为空气
    const BlockState* finalState = world.getBlockState(pos);
    EXPECT_EQ(finalState, nullptr) << "Snow should melt with block light > 11";
}

TEST_F(SnowBlockTest, RandomTick_DoesNotMeltWhenBlockLightAtOrBelow11)
{
    // 方块光照 <= 11 时，雪层不应融化（即使天空光照很高）
    SnowTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = VanillaBlocks::SNOW->defaultState();

    // 下方需要支撑方块，否则雪层因为 isValidPosition 失败而被移除
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(pos.down(), stoneState);

    world.setBlockAt(pos, &state);
    world.setBlockLightAt(pos, 11);
    world.setSkyLightAt(pos, 15); // 高天空光照不应导致雪融化

    VanillaBlocks::SNOW->randomTick(world, pos, state, random);

    // 雪层不应融化（方块光照恰好 == 11 不融化，需要 > 11；天空光照不影响）
    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Snow should still exist at block light level 11";
    EXPECT_TRUE(finalState->is(VanillaBlocks::SNOW)) << "Snow should not melt at block light level 11";
}

TEST_F(SnowBlockTest, RandomTick_DoesNotMeltWithOnlySkyLight)
{
    // MC 原版: SnowLayerBlock.randomTick 只检查方块光照，不检查天空光照
    // 即使天空光照 = 15，方块光照 = 0 时雪也不应融化
    SnowTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = VanillaBlocks::SNOW->defaultState();

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(pos.down(), stoneState);

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 0);

    VanillaBlocks::SNOW->randomTick(world, pos, state, random);

    // 雪层不应仅因天空光照而融化
    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Snow should still exist with only sky light";
    EXPECT_TRUE(finalState->is(VanillaBlocks::SNOW)) << "Snow should not melt with only sky light";
}

TEST_F(SnowBlockTest, RandomTick_DoesNotMeltWhenLightIsZero)
{
    // 光照为 0 时，雪层不应融化
    SnowTestWorld world;
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = VanillaBlocks::SNOW->defaultState();

    // 下方需要支撑方块
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(pos.down(), stoneState);

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 0);
    world.setBlockLightAt(pos, 0);

    VanillaBlocks::SNOW->randomTick(world, pos, state, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr) << "Snow should still exist at light level 0";
    EXPECT_TRUE(finalState->is(VanillaBlocks::SNOW)) << "Snow should not melt at light level 0";
}

// ============================================================================
// isValidPosition 测试
// ============================================================================

TEST_F(SnowBlockTest, IsValidPosition_SolidBelow_ReturnsTrue)
{
    // 下方有固体方块时，雪层可以放置
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(pos.down(), stoneState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_AirBelow_ReturnsFalse)
{
    // 下方为空气时，雪层不能放置
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    // 下方不设置任何方块（空气）

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_FALSE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_IceBelow_ReturnsFalse)
{
    // 下方为冰时，雪层不能放置（SNOW_LAYER_CANNOT_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* iceState = &VanillaBlocks::ICE->defaultState();
    world.setBlockAt(pos.down(), iceState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_FALSE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_PackedIceBelow_ReturnsFalse)
{
    // 下方为浮冰时，雪层不能放置（SNOW_LAYER_CANNOT_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* packedIceState = &VanillaBlocks::PACKED_ICE->defaultState();
    world.setBlockAt(pos.down(), packedIceState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_FALSE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_BarrierBelow_ReturnsFalse)
{
    // 下方为屏障时，雪层不能放置（SNOW_LAYER_CANNOT_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* barrierState = &VanillaBlocks::BARRIER->defaultState();
    world.setBlockAt(pos.down(), barrierState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_FALSE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_HoneyBlockBelow_ReturnsTrue)
{
    // 下方为蜂蜜块时，雪层可以放置（SNOW_LAYER_CAN_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* honeyBlockState = &VanillaBlocks::HONEY_BLOCK->defaultState();
    world.setBlockAt(pos.down(), honeyBlockState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_SoulSandBelow_ReturnsTrue)
{
    // 下方为灵魂沙时，雪层可以放置（SNOW_LAYER_CAN_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* soulSandState = &VanillaBlocks::SOUL_SAND->defaultState();
    world.setBlockAt(pos.down(), soulSandState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_MudBelow_ReturnsTrue)
{
    // 下方为泥巴时，雪层可以放置（SNOW_LAYER_CAN_SURVIVE_ON）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* mudState = &VanillaBlocks::MUD->defaultState();
    world.setBlockAt(pos.down(), mudState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_FullLayerSnowBelow_ReturnsTrue)
{
    // 下方为 8 层雪层时，雪层可以放置
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snow8State = &VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 8);
    world.setBlockAt(pos.down(), snow8State);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_PartialLayerSnowBelow_ReturnsFalse)
{
    // 下方为非满层（如 1 层）雪层时，雪层不能放置（没有完整上表面）
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    // 默认雪层为 1 层
    const BlockState* snow1State = &VanillaBlocks::SNOW->defaultState();
    world.setBlockAt(pos.down(), snow1State);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_FALSE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

TEST_F(SnowBlockTest, IsValidPosition_SnowBlockBelow_ReturnsTrue)
{
    // 下方为雪块（SnowBlock，不同于雪层 SnowLayer）时，雪层可以放置
    // 雪块有完整的上表面碰撞箱
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snowBlockState = &VanillaBlocks::SNOW_BLOCK->defaultState();
    world.setBlockAt(pos.down(), snowBlockState);

    BlockState snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->isValidPosition(snowState, world, pos));
}

// ============================================================================
// canSurviveAt 静态方法测试
// ============================================================================

TEST_F(SnowBlockTest, CanSurviveAt_SolidBelow_ReturnsTrue)
{
    // 下方有固体方块时，雪层可以存活
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(pos.down(), stoneState);

    EXPECT_TRUE(SnowBlock::canSurviveAt(world, pos));
}

TEST_F(SnowBlockTest, CanSurviveAt_AirBelow_ReturnsFalse)
{
    // 下方为空气时，雪层不能存活
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);

    EXPECT_FALSE(SnowBlock::canSurviveAt(world, pos));
}

TEST_F(SnowBlockTest, CanSurviveAt_IceBelow_ReturnsFalse)
{
    // 下方为冰时，雪层不能存活
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* iceState = &VanillaBlocks::ICE->defaultState();
    world.setBlockAt(pos.down(), iceState);

    EXPECT_FALSE(SnowBlock::canSurviveAt(world, pos));
}

TEST_F(SnowBlockTest, CanSurviveAt_HoneyBlockBelow_ReturnsTrue)
{
    // 下方为蜂蜜块时，雪层可以存活
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* honeyBlockState = &VanillaBlocks::HONEY_BLOCK->defaultState();
    world.setBlockAt(pos.down(), honeyBlockState);

    EXPECT_TRUE(SnowBlock::canSurviveAt(world, pos));
}

TEST_F(SnowBlockTest, CanSurviveAt_FullLayerSnowBelow_ReturnsTrue)
{
    // 下方为 8 层雪层时，雪层可以存活
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snow8State = &VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 8);
    world.setBlockAt(pos.down(), snow8State);

    EXPECT_TRUE(SnowBlock::canSurviveAt(world, pos));
}

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(SnowBlockTest, UpdatePostPlacement_LostSupport_ReturnsAir)
{
    // 当下方方块变为空气时，雪层应返回空气状态
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();

    // 下方无方块（空气），雪层失去支撑
    BlockState result =
        VanillaBlocks::SNOW->updatePostPlacement(*snowState, Direction::Down, *airState, world, pos, pos.down());

    // 返回的应该是空气
    EXPECT_TRUE(result.isAir()) << "Snow should become air when support is lost";
}

TEST_F(SnowBlockTest, UpdatePostPlacement_StillSupported_ReturnsOriginalState)
{
    // 当下方方块仍然提供支撑时，雪层应保持原状态
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    // 下方有石方块
    world.setBlockAt(pos.down(), stoneState);

    BlockState result =
        VanillaBlocks::SNOW->updatePostPlacement(*snowState, Direction::Down, *stoneState, world, pos, pos.down());

    // 返回的应该还是雪层
    EXPECT_TRUE(result.is(VanillaBlocks::SNOW)) << "Snow should remain when still supported";
}

TEST_F(SnowBlockTest, UpdatePostPlacement_SideUpdate_DoesNotBreakSnow)
{
    // 侧面方块变化不应导致雪层破裂
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    // 下方有石方块
    world.setBlockAt(pos.down(), stoneState);

    BlockState result = VanillaBlocks::SNOW->updatePostPlacement(
        *snowState, Direction::North, VanillaBlocks::AIR->defaultState(), world, pos, pos.north());

    // 侧面变化不影响雪层
    EXPECT_TRUE(result.is(VanillaBlocks::SNOW)) << "Side neighbor update should not break snow";
}

TEST_F(SnowBlockTest, UpdatePostPlacement_BelowBecomesIce_ReturnsAir)
{
    // 下方方块变为冰时（SNOW_LAYER_CANNOT_SURVIVE_ON），雪层应返回空气
    SnowTestWorld world;
    const BlockPos pos(4, 65, 4);
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    const BlockState* iceState = &VanillaBlocks::ICE->defaultState();

    // 下方被替换为冰
    world.setBlockAt(pos.down(), iceState);

    BlockState result =
        VanillaBlocks::SNOW->updatePostPlacement(*snowState, Direction::Down, *iceState, world, pos, pos.down());

    // 冰上不能放雪层
    EXPECT_TRUE(result.isAir()) << "Snow should break when below block becomes ice";
}

// ============================================================================
// ticksRandomly 测试
// ============================================================================

TEST_F(SnowBlockTest, TicksRandomly_ReturnsTrue)
{
    // 雪层应响应随机刻
    EXPECT_TRUE(VanillaBlocks::SNOW->ticksRandomly());
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(SnowBlockTest, GetShape_Layer1_HeightIs2Pixels)
{
    // layers=1 渲染形状高度应为 2 像素（0.125）
    const BlockState& defaultState = VanillaBlocks::SNOW->defaultState();
    ASSERT_EQ(defaultState.get(SnowBlock::LAYERS()), 1);

    const CollisionShape& shape = VanillaBlocks::SNOW->getShape(defaultState);
    ASSERT_EQ(shape.boxCount(), 1u);

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.maxY, 2.0f / 16.0f) << "Layer 1 render shape maxY should be 2/16 (0.125)";
}

TEST_F(SnowBlockTest, GetShape_Layer4_HeightIsHalfBlock)
{
    // layers=4 渲染形状高度应为 8 像素（0.5，半方块）
    const BlockState& state4 = VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 4);
    const CollisionShape& shape = VanillaBlocks::SNOW->getShape(state4);
    ASSERT_EQ(shape.boxCount(), 1u);

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.maxY, 8.0f / 16.0f) << "Layer 4 render shape maxY should be 8/16 (0.5)";
}

TEST_F(SnowBlockTest, GetShape_Layer8_IsFullBlock)
{
    // layers=8 渲染形状应为完整方块（FullBlock 类型），让 ChunkMesher 走完整方块路径
    const BlockState& state8 = VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 8);
    const CollisionShape& shape = VanillaBlocks::SNOW->getShape(state8);

    EXPECT_TRUE(shape.isFullBlock()) << "Layer 8 render shape should be FullBlock";
}

TEST_F(SnowBlockTest, GetShape_Layers1To7_AreNotFullBlock)
{
    // layers 1-7 渲染形状应为 SimpleBox（非 FullBlock），以便按层数渲染矮高度
    const BlockState& base = VanillaBlocks::SNOW->defaultState();
    for (i32 layers = 1; layers <= 7; ++layers) {
        const BlockState& state = base.with(SnowBlock::LAYERS(), layers);
        const CollisionShape& shape = VanillaBlocks::SNOW->getShape(state);
        EXPECT_FALSE(shape.isFullBlock()) << "Layer " << layers << " should not be FullBlock";
        EXPECT_EQ(shape.boxCount(), 1u) << "Layer " << layers << " should have exactly 1 box";
        EXPECT_FLOAT_EQ(shape.boxes().front().maxY, static_cast<f32>(layers * 2) / 16.0f)
            << "Layer " << layers << " maxY should be " << (layers * 2) << "/16";
    }
}

TEST_F(SnowBlockTest, GetCollisionShape_Layer1_IsEmpty)
{
    // layers=1 碰撞形状应为空（无碰撞，实体可踩过）
    const BlockState& defaultState = VanillaBlocks::SNOW->defaultState();
    const CollisionShape& shape = VanillaBlocks::SNOW->getCollisionShape(defaultState);

    EXPECT_TRUE(shape.isEmpty()) << "Layer 1 collision shape should be empty (no collision)";
}

TEST_F(SnowBlockTest, GetCollisionShape_Layer8_HeightIs14Pixels)
{
    // layers=8 碰撞形状高度应为 14 像素（0.875），比渲染形状矮 1 层
    const BlockState& state8 = VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 8);
    const CollisionShape& shape = VanillaBlocks::SNOW->getCollisionShape(state8);

    ASSERT_EQ(shape.boxCount(), 1u);
    EXPECT_FLOAT_EQ(shape.boxes().front().maxY, 14.0f / 16.0f)
        << "Layer 8 collision shape maxY should be 14/16 (0.875)";
}

TEST_F(SnowBlockTest, GetCollisionShape_LessThanShape)
{
    // 碰撞形状应比渲染形状矮 1 层：layers=4 时碰撞 6px < 渲染 8px
    const BlockState& state4 = VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 4);
    const CollisionShape& renderShape = VanillaBlocks::SNOW->getShape(state4);
    const CollisionShape& collisionShape = VanillaBlocks::SNOW->getCollisionShape(state4);

    ASSERT_EQ(renderShape.boxCount(), 1u);
    ASSERT_EQ(collisionShape.boxCount(), 1u);
    EXPECT_LT(collisionShape.boxes().front().maxY, renderShape.boxes().front().maxY)
        << "Collision shape should be shorter than render shape";
    EXPECT_FLOAT_EQ(collisionShape.boxes().front().maxY, 6.0f / 16.0f)
        << "Layer 4 collision maxY should be 6/16 (one layer below 8/16)";
}

TEST_F(SnowBlockTest, GetBlockSupportShape_EqualsShape)
{
    // 支撑形状应与渲染形状一致
    const BlockState& state5 = VanillaBlocks::SNOW->defaultState().with(SnowBlock::LAYERS(), 5);
    const CollisionShape& shape = VanillaBlocks::SNOW->getShape(state5);
    const CollisionShape& supportShape = VanillaBlocks::SNOW->getBlockSupportShape(state5);

    ASSERT_EQ(shape.boxCount(), 1u);
    ASSERT_EQ(supportShape.boxCount(), 1u);
    EXPECT_FLOAT_EQ(supportShape.boxes().front().maxY, shape.boxes().front().maxY)
        << "Support shape should equal render shape";
    EXPECT_FLOAT_EQ(supportShape.boxes().front().maxY, 10.0f / 16.0f) << "Layer 5 support maxY should be 10/16";
}

TEST_F(SnowBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    // 雪层应使用形状进行光照遮挡检测
    const BlockState& defaultState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(VanillaBlocks::SNOW->useShapeForLightOcclusion(defaultState));
}

} // namespace
