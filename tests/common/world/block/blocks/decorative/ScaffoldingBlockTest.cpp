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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/misc/MiscEntities.hpp"
#include "entity/utils/ItemDropHelper.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/decorative/ScaffoldingBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

class ScaffoldingTestWorld final : public mc::test::BaseTestWorld {
public:
    ScaffoldingTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    // 存储 BlockState 的副本并返回指针
    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

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
        if (m_setBlockStateFail) {
            return false;
        }
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
        } else {
            // 存储 BlockState 的副本
            m_storedStates.push_back(std::make_unique<BlockState>(*state));
            m_blocks[packPos(x, y, z)] = m_storedStates.back().get();
        }
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    // 存储 BlockState 并设置
    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos, stored);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (m_spawnEntityFail || !entity) {
            return EntityInstanceId(0);
        }
        EntityInstanceId id = EntityInstanceId(++m_nextEntityId);
        m_entities.push_back(std::move(entity));
        m_spawnedEntityCount++;
        return id;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<ScaffoldingTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    void setSeed(u64 seed) { m_seed = seed; }
    void setSpawnEntityFail(bool fail) { m_spawnEntityFail = fail; }
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntityCount; }
    void setSetBlockStateFail(bool fail) { m_setBlockStateFail = fail; }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;     // 存储指针
    std::vector<std::unique_ptr<BlockState>> m_storedStates; // 存储 BlockState 副本
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 12345;
    u32 m_nextEntityId = 0;
    size_t m_spawnedEntityCount = 0;
    bool m_setBlockStateFail = false;
    bool m_spawnEntityFail = false;
};

class TestSolidBlock final : public Block {
public:
    explicit TestSolidBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool isSolidSide(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return true;
    }
};

} // namespace

class ScaffoldingBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        scaffolding_ =
            std::make_unique<ScaffoldingBlock>(BlockProperties(Material::AIR).hardness(0.0f).resistance(0.0f));
    }
    std::unique_ptr<ScaffoldingBlock> scaffolding_;
};

TEST_F(ScaffoldingBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(scaffolding_, nullptr);
}

TEST_F(ScaffoldingBlockTest, DefaultState_HasCorrectValues)
{
    const BlockState& state = scaffolding_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::DISTANCE_0_7()), 7);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_FALSE(state.get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockTest, IsLadder_AlwaysReturnsTrue)
{
    ScaffoldingTestWorld world;
    BlockPos pos(0, 0, 0);
    const BlockState& state = scaffolding_->defaultState();
    EXPECT_TRUE(scaffolding_->isLadder(state, &world, &pos));
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceZero_ReturnsEmpty)
{
    auto state = scaffolding_->defaultState().with(BlockStateProperties::DISTANCE_0_7(), 0);
    const CollisionShape& shape = scaffolding_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceNonZeroWithBottom_ReturnsBaseShape)
{
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 3)
                     .with(BlockStateProperties::BOTTOM(), true);
    const CollisionShape& shape = scaffolding_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceNonZeroWithoutBottom_ReturnsEmpty)
{
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 3)
                     .with(BlockStateProperties::BOTTOM(), false);
    const CollisionShape& shape = scaffolding_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, IsWaterlogged_WorksCorrectly)
{
    auto state = scaffolding_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(ScaffoldingBlockTest, Distance_CanBeSetToAllValidValues)
{
    for (i32 i = 0; i <= 7; ++i) {
        auto state = scaffolding_->defaultState().with(BlockStateProperties::DISTANCE_0_7(), i);
        EXPECT_EQ(state.get(BlockStateProperties::DISTANCE_0_7()), i);
    }
}

TEST_F(ScaffoldingBlockTest, Bottom_CanBeToggled)
{
    auto state = scaffolding_->defaultState().with(BlockStateProperties::BOTTOM(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::BOTTOM()));
    state = state.with(BlockStateProperties::BOTTOM(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockTest, Shape_ChangesWithBottomProperty)
{
    auto stateWithBottom = scaffolding_->defaultState()
                               .with(BlockStateProperties::DISTANCE_0_7(), 5)
                               .with(BlockStateProperties::BOTTOM(), true);
    auto stateWithoutBottom = scaffolding_->defaultState()
                                  .with(BlockStateProperties::DISTANCE_0_7(), 5)
                                  .with(BlockStateProperties::BOTTOM(), false);
    const CollisionShape& shapeWithBottom = scaffolding_->getShape(stateWithBottom);
    const CollisionShape& shapeWithoutBottom = scaffolding_->getShape(stateWithoutBottom);
    EXPECT_NE(&shapeWithBottom, &shapeWithoutBottom);
}

class ScaffoldingBlockIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        scaffolding_ =
            std::make_unique<ScaffoldingBlock>(BlockProperties(Material::AIR).hardness(0.0f).resistance(0.0f));
        world_.ensureTickManager();
    }
    std::unique_ptr<ScaffoldingBlock> scaffolding_;
    ScaffoldingTestWorld world_;
};

TEST_F(ScaffoldingBlockIntegrationTest, Tick_DistanceSevenFromSix_CreatesFallingBlockEntity)
{
    BlockPos pos(0, 100, 0);
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 6)
                     .with(BlockStateProperties::BOTTOM(), true);
    world_.setBlockState(pos, world_.storeBlockState(state));
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, pos, state, rng);
    const BlockState* finalState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
    EXPECT_EQ(world_.spawnedEntityCount(), 1);
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_DistanceSevenFromSeven_RemovesBlockButNoEntity)
{
    BlockPos pos(0, 100, 0);
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 7)
                     .with(BlockStateProperties::BOTTOM(), true);
    world_.setBlockState(pos, world_.storeBlockState(state));
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, pos, state, rng);
    const BlockState* finalState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_DistanceNotSeven_UpdatesState)
{
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    BlockPos scaffoldingPos(0, 65, 0);
    BlockPos solidPos(0, 64, 0);
    // 先存储固体方块的状态
    world_.setBlockState(solidPos, world_.storeBlockState(solidBlock.defaultState()));
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 7)
                     .with(BlockStateProperties::BOTTOM(), true);
    world_.setBlockState(scaffoldingPos, world_.storeBlockState(state));
    size_t entityCountBefore = world_.spawnedEntityCount();
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, scaffoldingPos, state, rng);
    const BlockState* finalState = world_.getBlockState(scaffoldingPos.x, scaffoldingPos.y, scaffoldingPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_FALSE(finalState->isAir());
    EXPECT_EQ(world_.spawnedEntityCount(), entityCountBefore);
    EXPECT_EQ(finalState->get(BlockStateProperties::DISTANCE_0_7()), 0);
    EXPECT_FALSE(finalState->get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_BlockReplaced_DoesNothing)
{
    BlockPos pos(0, 100, 0);
    auto scaffoldingState = scaffolding_->defaultState().with(BlockStateProperties::DISTANCE_0_7(), 7);
    world_.setBlockState(pos, &VanillaBlocks::AIR->defaultState());
    size_t entityCountBefore = world_.spawnedEntityCount();
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, pos, scaffoldingState, rng);
    EXPECT_EQ(world_.spawnedEntityCount(), entityCountBefore);
    const BlockState* finalState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_StateUnchanged_NoUpdate)
{
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    BlockPos scaffoldingPos(0, 65, 0);
    BlockPos solidPos(0, 64, 0);
    world_.setBlockState(solidPos, world_.storeBlockState(solidBlock.defaultState()));
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 0)
                     .with(BlockStateProperties::BOTTOM(), false);
    world_.setBlockState(scaffoldingPos, world_.storeBlockState(state));
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, scaffoldingPos, state, rng);
    const BlockState* finalState = world_.getBlockState(scaffoldingPos.x, scaffoldingPos.y, scaffoldingPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->get(BlockStateProperties::DISTANCE_0_7()), 0);
    EXPECT_FALSE(finalState->get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_SpawnEntityFail_RestoresBlock)
{
    BlockPos pos(0, 100, 0);
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 6)
                     .with(BlockStateProperties::BOTTOM(), true);
    world_.setBlockState(pos, world_.storeBlockState(state));
    world_.setSpawnEntityFail(true);
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, pos, state, rng);
    const BlockState* finalState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_FALSE(finalState->isAir());
    EXPECT_EQ(finalState->get(BlockStateProperties::DISTANCE_0_7()), 6);
}

TEST_F(ScaffoldingBlockIntegrationTest, CalculateDistance_DirectSupport_ReturnsZero)
{
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    BlockPos scaffoldingPos(0, 65, 0);
    BlockPos solidPos(0, 64, 0);
    world_.setBlockState(solidPos, world_.storeBlockState(solidBlock.defaultState()));
    world_.setBlockState(scaffoldingPos, &scaffolding_->defaultState());
    i32 distance = ScaffoldingBlock::calculateDistance(world_, scaffoldingPos);
    EXPECT_EQ(distance, 0);
}

TEST_F(ScaffoldingBlockIntegrationTest, CalculateDistance_NoSupport_ReturnsSeven)
{
    BlockPos pos(0, 100, 0);
    i32 distance = ScaffoldingBlock::calculateDistance(world_, pos);
    EXPECT_EQ(distance, 7);
}

TEST_F(ScaffoldingBlockIntegrationTest, CalculateDistance_AboveAnotherScaffolding_InheritsDistance)
{
    BlockPos bottomPos(0, 64, 0);
    BlockPos topPos(0, 65, 0);
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockState(BlockPos(0, 63, 0), world_.storeBlockState(solidBlock.defaultState()));
    auto bottomState = scaffolding_->defaultState().with(BlockStateProperties::DISTANCE_0_7(), 0);
    world_.setBlockState(bottomPos, world_.storeBlockState(bottomState));
    world_.setBlockState(topPos, &scaffolding_->defaultState());
    i32 distance = ScaffoldingBlock::calculateDistance(world_, topPos);
    EXPECT_EQ(distance, 0);
}

TEST_F(ScaffoldingBlockIntegrationTest, CalculateDistance_HorizontalScaffoldingSupport)
{
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    BlockPos solidPos(0, 63, 0);
    BlockPos scaffoldingAt64_0(0, 64, 0);
    BlockPos scaffoldingAt65_0(0, 65, 0);
    BlockPos testPos(1, 65, 0);
    world_.setBlockState(solidPos, world_.storeBlockState(solidBlock.defaultState()));
    auto scaffoldingState = scaffolding_->defaultState().with(BlockStateProperties::DISTANCE_0_7(), 0);
    world_.setBlockState(scaffoldingAt64_0, world_.storeBlockState(scaffoldingState));
    world_.setBlockState(scaffoldingAt65_0, world_.storeBlockState(scaffoldingState));
    i32 distance = ScaffoldingBlock::calculateDistance(world_, testPos);
    EXPECT_EQ(distance, 1);
}

TEST_F(ScaffoldingBlockIntegrationTest, Tick_WaterloggedStatePreserved_InFallingState)
{
    BlockPos pos(0, 100, 0);
    auto state = scaffolding_->defaultState()
                     .with(BlockStateProperties::DISTANCE_0_7(), 6)
                     .with(BlockStateProperties::WATERLOGGED(), true);
    world_.setBlockState(pos, world_.storeBlockState(state));
    math::Random& rng = world_.getRandom();
    scaffolding_->tick(world_, pos, state, rng);
    const BlockState* finalState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
    EXPECT_EQ(world_.spawnedEntityCount(), 1);
}
