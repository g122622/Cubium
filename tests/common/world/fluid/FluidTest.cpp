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

#include "common/world/fluid/Fluid.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/fluid/fluids/EmptyFluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <unordered_map>

using namespace mc::fluid;
using namespace mc;

namespace {

class FlowingFluidTestWorld final : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
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
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FlowingFluidTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FlowingFluidTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override
    {
        throw std::runtime_error("FlowingFluidTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        throw std::runtime_error("FlowingFluidTestWorld::getRandom not implemented");
    }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        throw std::runtime_error("FlowingFluidTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        throw std::runtime_error("FlowingFluidTestWorld::worldBorder not implemented");
    }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
};

} // namespace

// ============================================================================
// FluidState Tests
// ============================================================================

TEST(FluidStateTest, EmptyFluidStateIsEmpty)
{
    // EmptyFluid的状态应该标记为空
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    // EmptyFluid重写isEmpty返回true
    EXPECT_TRUE(state.isEmpty());
    EXPECT_FALSE(state.isSource());
    EXPECT_EQ(state.getLevel(), 0);
}

TEST(FluidStateTest, GetFluidReturnsOwner)
{
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    EXPECT_EQ(&state.getFluid(), &emptyFluid);
}

TEST(FluidStateTest, FluidId)
{
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    EXPECT_EQ(state.fluidId(), emptyFluid.fluidId());
}

TEST(FluidStateTest, WaterBlockStateMapsToShallowFluidHeight)
{
    FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    const BlockState& shallowWater = VanillaBlocks::WATER->defaultState().with(BlockStateProperties::LEVEL_0_15(), 7);
    const FluidState* fluidState = shallowWater.getFluidState();

    ASSERT_NE(fluidState, nullptr);
    EXPECT_FALSE(fluidState->isSource());
    EXPECT_EQ(fluidState->getLevel(), 1);
    EXPECT_NEAR(fluidState->getHeight(), 1.0f / 9.0f, 1e-6f);
}

TEST(FluidStateTest, ActualHeightBecomesFullWhenSameFluidAbove)
{
    FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    FlowingFluidTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& shallowWater = VanillaBlocks::WATER->defaultState().with(BlockStateProperties::LEVEL_0_15(), 7);
    world.setBlockState(pos.x, pos.y, pos.z, &shallowWater);
    world.setBlockState(pos.x, pos.y + 1, pos.z, &VanillaBlocks::WATER->defaultState());

    const FluidState* fluidState = world.getFluidState(pos.x, pos.y, pos.z);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_NEAR(fluidState->getActualHeight(world, pos), 1.0f, 1e-6f);
}

// ============================================================================
// FluidRegistry Tests
// ============================================================================

TEST(FluidRegistryTest, InitializeRegistersEmptyFluid)
{
    FluidRegistry& registry = FluidRegistry::instance();

    // 初始化
    registry.initialize();

    // EmptyFluid应该被注册为ID 0
    Fluid* emptyFluid = registry.getFluid(FluidRegistry::EMPTY_ID);
    EXPECT_NE(emptyFluid, nullptr);
    EXPECT_EQ(emptyFluid->fluidLocation(), ResourceLocation("minecraft:empty"));
}

TEST(FluidRegistryTest, GetFluidByInvalidIdReturnsNull)
{
    FluidRegistry& registry = FluidRegistry::instance();

    Fluid* fluid = registry.getFluid(99999);
    EXPECT_EQ(fluid, nullptr);
}

TEST(FluidRegistryTest, GetFluidByInvalidResourceLocationReturnsNull)
{
    FluidRegistry& registry = FluidRegistry::instance();

    Fluid* fluid = registry.getFluid(ResourceLocation("minecraft:nonexistent"));
    EXPECT_EQ(fluid, nullptr);
}

// 取内置流体的默认状态必须走 fluidId 路径（Fluids::XXX()->defaultState()）。
// 历史上存在一个陷阱式 API Fluid::getFluidState(u32 stateId)：stateId 在各 Fluid 的
// StateContainer 内独立从 0 分配、不全局唯一，按 stateId 反查会错乱（stateId=0 实际指向
// flowing_lava 而非 EMPTY）。该反查 API 与底层 m_statesById 表已删除，此处回归断言
// fluidId 路径的正确性：EMPTY/WATER/LAVA 等内置访问器返回的默认状态语义正确。
TEST(FluidRegistryTest, BuiltinFluidDefaultStatesViaFluidIdPath)
{
    FluidRegistry& registry = FluidRegistry::instance();
    // registry.initialize() 现已内含 Fluids::initialize()——注册表就绪即保证
    // Fluids::EMPTY() 等内置访问器指针缓存可用，调用方无需单独刷缓存。
    registry.initialize();

    // EMPTY 默认状态：isEmpty() 为 true，owner 是 EmptyFluid。
    const FluidState& emptyState = Fluids::EMPTY()->defaultState();
    EXPECT_TRUE(emptyState.isEmpty());
    EXPECT_EQ(emptyState.getFluid().fluidLocation(), ResourceLocation("minecraft:empty"));

    // WATER 默认状态：是源头、非空、owner 是 water。
    const FluidState& waterState = Fluids::WATER()->defaultState();
    EXPECT_FALSE(waterState.isEmpty());
    EXPECT_TRUE(waterState.isSource());
    EXPECT_EQ(waterState.getFluid().fluidLocation(), ResourceLocation("minecraft:water"));

    // LAVA 默认状态：是源头、非空、owner 是 lava。
    const FluidState& lavaState = Fluids::LAVA()->defaultState();
    EXPECT_FALSE(lavaState.isEmpty());
    EXPECT_TRUE(lavaState.isSource());
    EXPECT_EQ(lavaState.getFluid().fluidLocation(), ResourceLocation("minecraft:lava"));

    // FLOWING_WATER / FLOWING_LAVA 默认状态：非源头（流动版本）。
    EXPECT_FALSE(Fluids::FLOWING_WATER()->defaultState().isSource());
    EXPECT_FALSE(Fluids::FLOWING_LAVA()->defaultState().isSource());

    // fluidId 与 ResourceLocation 双路径查找必须返回同一流体指针。
    EXPECT_EQ(registry.getFluid(FluidRegistry::EMPTY_ID), Fluids::EMPTY());
    EXPECT_EQ(registry.getFluid(FluidRegistry::WATER_ID), Fluids::WATER());
    EXPECT_EQ(registry.getFluid(FluidRegistry::LAVA_ID), Fluids::LAVA());
    EXPECT_EQ(registry.getFluid(ResourceLocation("minecraft:empty")), Fluids::EMPTY());
    EXPECT_EQ(registry.getFluid(ResourceLocation("minecraft:water")), Fluids::WATER());
    EXPECT_EQ(registry.getFluid(ResourceLocation("minecraft:lava")), Fluids::LAVA());
}

TEST(FluidRegistryTest, FluidCountAfterInitialization)
{
    FluidRegistry& registry = FluidRegistry::instance();
    registry.initialize();

    // The registry registers exactly 5 fluids
    EXPECT_EQ(registry.fluidCount(), 5u);
}

// ============================================================================
// FluidProperties Tests
// ============================================================================

TEST(FluidPropertiesTest, LevelPropertyHasCorrectRange)
{
    auto& level = FluidProperties::LEVEL_1_8();

    EXPECT_EQ(level.name(), "level");
    EXPECT_EQ(level.minValue(), 1);
    EXPECT_EQ(level.maxValue(), 8);
}

TEST(FluidPropertiesTest, FallingPropertyExists)
{
    auto& falling = FluidProperties::FALLING();

    EXPECT_EQ(falling.name(), "falling");
}

// ============================================================================
// Fluid Base Class Tests
// ============================================================================

TEST(FluidTest, DefaultTickDoesNothing)
{
    // Fluid基类的tick方法应该可以被安全调用
    EmptyFluid emptyFluid;
    // 创建一个模拟的world和pos - 需要实际的IWorld实现来测试
    // emptyFluid.tick(world, pos, state);
    // 这里只验证方法存在
}

TEST(FluidTest, DefaultRandomTickDoesNothing)
{
    EmptyFluid emptyFluid;
    // 与tick类似，需要实际的IWorld和IRandom实现
}

TEST(FluidTest, DefaultTicksRandomlyReturnsFalse)
{
    EmptyFluid emptyFluid;
    EXPECT_FALSE(emptyFluid.ticksRandomly());
}

TEST(FluidTest, IsEquivalentTo)
{
    EmptyFluid emptyFluid1;
    EmptyFluid emptyFluid2;

    // 同一对象应该是等效的
    EXPECT_TRUE(emptyFluid1.isEquivalentTo(emptyFluid1));

    // 不同对象不是等效的（默认实现比较指针）
    EXPECT_FALSE(emptyFluid1.isEquivalentTo(emptyFluid2));
}

TEST(FluidFlowBehaviorTest, SourceWaterFlowsDownwardIntoAir)
{
    FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    FlowingFluidTestWorld world;
    const BlockPos sourcePos(0, 64, 0);
    world.setBlockState(sourcePos.x, sourcePos.y, sourcePos.z, &VanillaBlocks::WATER->defaultState());

    const FluidState* sourceStatePtr = world.getFluidState(sourcePos.x, sourcePos.y, sourcePos.z);
    ASSERT_NE(sourceStatePtr, nullptr);
    ASSERT_FALSE(sourceStatePtr->isEmpty());

    FluidState sourceState = *sourceStatePtr;
    Fluid& sourceFluid = const_cast<Fluid&>(sourceState.getFluid());
    sourceFluid.tick(world, sourcePos, sourceState);

    const BlockState* belowState = world.getBlockState(sourcePos.x, sourcePos.y - 1, sourcePos.z);
    ASSERT_NE(belowState, nullptr);
    EXPECT_TRUE(belowState->is(VanillaBlocks::WATER));
}

TEST(FluidFlowBehaviorTest, SourceWaterSpreadsHorizontallyAsFlowingNotSource)
{
    FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    FlowingFluidTestWorld world;
    const BlockPos sourcePos(0, 64, 0);
    const BlockPos targetPos(1, 64, 0);

    world.setBlockState(sourcePos.x, sourcePos.y, sourcePos.z, &VanillaBlocks::WATER->defaultState());
    world.setBlockState(sourcePos.x, sourcePos.y - 1, sourcePos.z, &VanillaBlocks::STONE->defaultState());
    world.setBlockState(targetPos.x, targetPos.y - 1, targetPos.z, &VanillaBlocks::STONE->defaultState());
    world.setBlockState(sourcePos.x - 1, sourcePos.y, sourcePos.z, &VanillaBlocks::STONE->defaultState());
    world.setBlockState(sourcePos.x, sourcePos.y, sourcePos.z - 1, &VanillaBlocks::STONE->defaultState());
    world.setBlockState(sourcePos.x, sourcePos.y, sourcePos.z + 1, &VanillaBlocks::STONE->defaultState());

    const FluidState* sourceStatePtr = world.getFluidState(sourcePos.x, sourcePos.y, sourcePos.z);
    ASSERT_NE(sourceStatePtr, nullptr);
    ASSERT_FALSE(sourceStatePtr->isEmpty());

    FluidState sourceState = *sourceStatePtr;
    Fluid& sourceFluid = const_cast<Fluid&>(sourceState.getFluid());
    sourceFluid.tick(world, sourcePos, sourceState);

    const FluidState* targetFluid = world.getFluidState(targetPos.x, targetPos.y, targetPos.z);
    ASSERT_NE(targetFluid, nullptr);
    EXPECT_FALSE(targetFluid->isEmpty());
    EXPECT_FALSE(targetFluid->isSource());
    EXPECT_EQ(targetFluid->getLevel(), 7);
}

// ============================================================================
// StateContainer Tests for Fluid
// ============================================================================

TEST(FluidStateTest, StateWithProperties)
{
    EmptyFluid emptyFluid;
    const FluidState& baseState = emptyFluid.defaultState();

    // EmptyFluid没有属性，所以baseState就是唯一状态
    EXPECT_EQ(emptyFluid.stateContainer().stateCount(), static_cast<size_t>(1));
}

// ============================================================================
// ResourceLocation Hash Tests
// ============================================================================

TEST(ResourceLocationHashTest, CanBeUsedInUnorderedMap)
{
    std::unordered_map<ResourceLocation, int> map;

    map[ResourceLocation("minecraft:water")] = 1;
    map[ResourceLocation("minecraft:lava")] = 2;

    EXPECT_EQ(map[ResourceLocation("minecraft:water")], 1);
    EXPECT_EQ(map[ResourceLocation("minecraft:lava")], 2);
}
