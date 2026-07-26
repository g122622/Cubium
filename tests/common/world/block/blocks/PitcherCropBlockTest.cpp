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

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/illager/RavagerEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/PitcherCropBlock.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

// ============================================================================
// 测试用世界桩（基于 DoubleBlockTestWorld 模式）
// ============================================================================

/**
 * @brief 瓶草作物集成测试用世界桩
 *
 * 提供最小化的 IWorld 实现，支持方块放置/读取、光照查询，
 * 用于测试 isValidPosition、canSustain、grow、randomTick 等需要世界交互的方法。
 */
class PitcherCropTestWorld final : public IBlockReader {
public:
    PitcherCropTestWorld() = default;

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
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    // 充足光照（模拟室外环境）
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

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
    [[nodiscard]] u64 currentTick() const override { return 0; }
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
        const_cast<PitcherCropTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    world::gamerule::GameRules m_gameRules;
    math::Random m_random{12345};
};

// ============================================================================
// 基础测试夹具（不需要世界桩）
// ============================================================================

class PitcherCropBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 集成测试夹具（需要世界桩）
// ============================================================================

class PitcherCropIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    PitcherCropTestWorld world;
};

// ============================================================================
// 常量测试
// ============================================================================

TEST_F(PitcherCropBlockTest, MaxAge_Is4)
{
    EXPECT_EQ(PitcherCropBlock::MAX_AGE, 4) << "PitcherCropBlock MAX_AGE should be 4";
}

TEST_F(PitcherCropBlockTest, DoublePlantAgeIntersection_Is3)
{
    EXPECT_EQ(PitcherCropBlock::DOUBLE_PLANT_AGE_INTERSECTION, 3)
        << "DOUBLE_PLANT_AGE_INTERSECTION should be 3 (AGE>=3 becomes double)";
}

// ============================================================================
// isDouble 静态方法测试
// ============================================================================

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge0)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(0)) << "AGE 0 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge1)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(1)) << "AGE 1 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge2)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(2)) << "AGE 2 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_True_ForAge3)
{
    EXPECT_TRUE(PitcherCropBlock::isDouble(3)) << "AGE 3 should be double block";
}

TEST_F(PitcherCropBlockTest, IsDouble_True_ForAge4)
{
    EXPECT_TRUE(PitcherCropBlock::isDouble(4)) << "AGE 4 should be double block";
}

// ============================================================================
// 状态属性测试
// ============================================================================

TEST_F(PitcherCropBlockTest, DefaultState_Age0Lower)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const BlockState& state = block->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), 0) << "Default age should be 0";
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower)
        << "Default half should be Lower";
}

TEST_F(PitcherCropBlockTest, GetAge_ReturnsCorrectValue)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_EQ(block->getAge(state), age) << "getAge should return " << age;
    }
}

TEST_F(PitcherCropBlockTest, IsMaxAge_True_OnlyAtAge4)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 3; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_FALSE(block->isMaxAge(state)) << "Age " << age << " should not be max age";
    }

    const BlockState& state4 =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_TRUE(block->isMaxAge(state4)) << "Age 4 should be max age";
}

TEST_F(PitcherCropBlockTest, WithAge_ReturnsLowerHalf)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // withAge 应始终返回 Lower 半部分
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state = block->withAge(age);
        EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), age)
            << "withAge(" << age << ") should set age to " << age;
        EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower)
            << "withAge should always return Lower half";
    }
}

TEST_F(PitcherCropBlockTest, WithAge_ClampsOutOfRange)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 负值应被 clamp 到 0
    const BlockState& stateNeg = block->withAge(-1);
    EXPECT_EQ(stateNeg.get(BlockStateProperties::AGE_0_4()), 0) << "withAge(-1) should clamp to 0";

    // 超过 MAX_AGE 的值应被 clamp 到 MAX_AGE
    const BlockState& stateOver = block->withAge(10);
    EXPECT_EQ(stateOver.get(BlockStateProperties::AGE_0_4()), 4) << "withAge(10) should clamp to 4";
}

// ============================================================================
// 双格状态测试
// ============================================================================

TEST_F(PitcherCropBlockTest, Age0Through2_AreSingleBlock)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 2; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_FALSE(PitcherCropBlock::isDouble(block->getAge(state))) << "AGE " << age << " should be single block";
    }
}

TEST_F(PitcherCropBlockTest, Age3And4_AreDoubleBlock)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 3; age <= 4; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_TRUE(PitcherCropBlock::isDouble(block->getAge(state))) << "AGE " << age << " should be double block";
    }
}

// ============================================================================
// 状态空间测试
// ============================================================================

TEST_F(PitcherCropBlockTest, StateSpace_HasAllStates)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    // 验证所有 5*2=10 种状态都可访问
    for (i32 age = 0; age <= 4; ++age) {
        for (auto half : {BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::DoubleBlockHalf::Upper}) {
            const BlockState& state = block->defaultState()
                                          .with(BlockStateProperties::AGE_0_4(), age)
                                          .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), half);
            EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), age);
            EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), half);
        }
    }
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(PitcherCropBlockTest, ShapeExists_ForAllAgesAndHalves)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 4; ++age) {
        for (auto half : {BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::DoubleBlockHalf::Upper}) {
            const BlockState& state = block->defaultState()
                                          .with(BlockStateProperties::AGE_0_4(), age)
                                          .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), half);
            const CollisionShape& shape = block->getShape(state);
            // 下半部分形状应该始终非空
            if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
                EXPECT_FALSE(shape.isEmpty()) << "Lower half shape should not be empty for age=" << age;
            }
            // 上半部分形状在 AGE 3-4 时应该非空（双格植物的上半部分有可视形状）
            if (half == BlockStateProperties::DoubleBlockHalf::Upper && age >= 3) {
                EXPECT_FALSE(shape.isEmpty())
                    << "Upper half shape should not be empty for age=" << age << " (double block)";
            }
        }
    }
}

TEST_F(PitcherCropBlockTest, CollisionShape_UpperHalfIsEmpty)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 上半部分无碰撞
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
        const CollisionShape& shape = block->getCollisionShape(state);
        EXPECT_TRUE(shape.isEmpty()) << "Upper half collision shape should be empty for age " << age;
    }
}

TEST_F(PitcherCropBlockTest, CollisionShape_LowerHalfIsNotEmpty)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 下半部分有碰撞
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        const CollisionShape& shape = block->getCollisionShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Lower half collision shape should not be empty for age " << age;
    }
}

// ============================================================================
// IGrowable 接口测试
// ============================================================================

TEST_F(PitcherCropBlockTest, TicksRandomly_ReturnsTrue)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(block->ticksRandomly()) << "PitcherCropBlock should tick randomly for growth";
}

TEST_F(PitcherCropBlockTest, CanGrow_Precondition_MaxAgeNotReached)
{
    // 验证 isMaxAge 在 age<4 时返回 false，这是 canGrow 返回 true 的前提
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 3; ++age) {
        const BlockState& state =
            block->defaultState()
                .with(BlockStateProperties::AGE_0_4(), age)
                .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_FALSE(block->isMaxAge(state)) << "Age " << age << " should not be max age";
    }
}

// ============================================================================
// 掉落物品测试
// ============================================================================

TEST_F(PitcherCropBlockTest, GetCropItem_ReturnsPitcherPlantItemId)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    ASSERT_NE(Items::PITCHER_PLANT, nullptr) << "Items::PITCHER_PLANT should be initialized";
    u32 cropItemId = block->getCropItem();
    EXPECT_EQ(cropItemId, Items::PITCHER_PLANT->itemId()) << "getCropItem() should return PITCHER_PLANT item ID";
}

TEST_F(PitcherCropBlockTest, GetSeedItem_ReturnsPitcherPodItemId)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    ASSERT_NE(Items::PITCHER_POD, nullptr) << "Items::PITCHER_POD should be initialized";
    u32 seedItemId = block->getSeedItem();
    EXPECT_EQ(seedItemId, Items::PITCHER_POD->itemId()) << "getSeedItem() should return PITCHER_POD item ID";
}

TEST_F(PitcherCropBlockTest, CropAndSeedItemsAreDifferent)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    u32 cropItemId = block->getCropItem();
    u32 seedItemId = block->getSeedItem();
    EXPECT_NE(cropItemId, seedItemId) << "Crop item (pitcher_plant) and seed item (pitcher_pod) should be different";
}

// ============================================================================
// canSustain 集成测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, CanSustain_OnFarmland_ReturnsTrue)
{
    // 瓶草作物在耕地上 should 返回 true
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos cropPos(5, 10, 5);
    const BlockPos farmlandPos = cropPos.down();

    // 在下方放置耕地
    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    const BlockState* farmlandState = world.getBlockState(farmlandPos);
    ASSERT_NE(farmlandState, nullptr);

    bool canSustain = block->canSustain(*farmlandState, world, farmlandPos);
    EXPECT_TRUE(canSustain) << "PitcherCropBlock should sustain on farmland";
}

TEST_F(PitcherCropIntegrationTest, CanSustain_OnDirt_ReturnsFalse)
{
    // 瓶草作物在泥土上不能种植（只有耕地可以）
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos cropPos(5, 10, 5);
    const BlockPos dirtPos = cropPos.down();

    // 在下方放置泥土
    world.setBlockAt(dirtPos, &VanillaBlocks::DIRT->defaultState());

    const BlockState* dirtState = world.getBlockState(dirtPos);
    ASSERT_NE(dirtState, nullptr);

    bool canSustain = block->canSustain(*dirtState, world, dirtPos);
    EXPECT_FALSE(canSustain) << "PitcherCropBlock should NOT sustain on dirt (only farmland)";
}

TEST_F(PitcherCropIntegrationTest, CanSustain_OnGrassBlock_ReturnsFalse)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos cropPos(5, 10, 5);
    const BlockPos grassPos = cropPos.down();

    world.setBlockAt(grassPos, &VanillaBlocks::GRASS_BLOCK->defaultState());

    const BlockState* grassState = world.getBlockState(grassPos);
    ASSERT_NE(grassState, nullptr);

    bool canSustain = block->canSustain(*grassState, world, grassPos);
    EXPECT_FALSE(canSustain) << "PitcherCropBlock should NOT sustain on grass block";
}

// ============================================================================
// isValidPosition 集成测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, IsValidPosition_LowerOnFarmlandWithLight_ReturnsTrue)
{
    // 下半部分在耕地上，光照充足 → 有效位置
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos cropPos(5, 10, 5);
    const BlockPos farmlandPos = cropPos.down();

    // 放置耕地
    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    const BlockState& lowerState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    bool valid = block->isValidPosition(lowerState, static_cast<IBlockReader&>(world), cropPos);
    EXPECT_TRUE(valid) << "Lower half on farmland with sufficient light should be valid";
}

TEST_F(PitcherCropIntegrationTest, IsValidPosition_LowerOnDirt_ReturnsFalse)
{
    // 下半部分在泥土上（非耕地）→ 无效位置
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos cropPos(5, 10, 5);
    const BlockPos dirtPos = cropPos.down();

    // 放置泥土
    world.setBlockAt(dirtPos, &VanillaBlocks::DIRT->defaultState());

    const BlockState& lowerState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    bool valid = block->isValidPosition(lowerState, static_cast<IBlockReader&>(world), cropPos);
    EXPECT_FALSE(valid) << "Lower half on dirt should be invalid (needs farmland)";
}

TEST_F(PitcherCropIntegrationTest, IsValidPosition_UpperAboveLower_ReturnsTrue)
{
    // 上半部分位于下半部分之上 → 有效位置
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos = lowerPos.up();

    // 在下方放置下半部分（AGE=3，双格状态）
    const BlockState& lowerState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 3)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(lowerPos, &lowerState);

    const BlockState& upperState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 3)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    bool valid = block->isValidPosition(upperState, static_cast<IBlockReader&>(world), upperPos);
    EXPECT_TRUE(valid) << "Upper half above matching lower half should be valid";
}

TEST_F(PitcherCropIntegrationTest, IsValidPosition_UpperWithoutLower_ReturnsFalse)
{
    // 上半部分下方不是同类型方块的下半部分 → 无效位置
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos upperPos(5, 10, 5);

    const BlockState& upperState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 3)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    // 下方为空
    bool valid = block->isValidPosition(upperState, static_cast<IBlockReader&>(world), upperPos);
    EXPECT_FALSE(valid) << "Upper half without lower half below should be invalid";
}

// ============================================================================
// placeAt 静态方法集成测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, PlaceAt_Age0_SingleBlock)
{
    // AGE=0 应只放置下半部分
    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    // 放置耕地
    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    bool placed = PitcherCropBlock::placeAt(world, pos, 0, 2);
    EXPECT_TRUE(placed) << "placeAt with age=0 should succeed";

    // 验证下半部分存在
    const BlockState* lowerState = world.getBlockState(pos);
    ASSERT_NE(lowerState, nullptr);
    EXPECT_TRUE(lowerState->is(TrailsBlocks::PITCHER_CROP)) << "Block at pos should be pitcher crop";
    EXPECT_EQ(lowerState->get(BlockStateProperties::AGE_0_4()), 0) << "Age should be 0";
    EXPECT_EQ(lowerState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower)
        << "Should be lower half";

    // 验证上方没有上半部分
    const BlockState* upperState = world.getBlockState(pos.up());
    EXPECT_EQ(upperState, nullptr) << "No upper half should exist for AGE=0";
}

TEST_F(PitcherCropIntegrationTest, PlaceAt_Age3_DoubleBlock)
{
    // AGE=3 应放置下半和上半两部分
    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    // 放置耕地
    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    bool placed = PitcherCropBlock::placeAt(world, pos, 3, 2);
    EXPECT_TRUE(placed) << "placeAt with age=3 should succeed";

    // 验证下半部分
    const BlockState* lowerState = world.getBlockState(pos);
    ASSERT_NE(lowerState, nullptr);
    EXPECT_TRUE(lowerState->is(TrailsBlocks::PITCHER_CROP));
    EXPECT_EQ(lowerState->get(BlockStateProperties::AGE_0_4()), 3);
    EXPECT_EQ(lowerState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);

    // 验证上半部分
    const BlockState* upperState = world.getBlockState(pos.up());
    ASSERT_NE(upperState, nullptr) << "Upper half should exist for AGE=3";
    EXPECT_TRUE(upperState->is(TrailsBlocks::PITCHER_CROP));
    EXPECT_EQ(upperState->get(BlockStateProperties::AGE_0_4()), 3);
    EXPECT_EQ(upperState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(PitcherCropIntegrationTest, PlaceAt_Age4_DoubleBlock)
{
    // AGE=4 (最大) 应放置双格
    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    bool placed = PitcherCropBlock::placeAt(world, pos, 4, 2);
    EXPECT_TRUE(placed);

    const BlockState* lowerState = world.getBlockState(pos);
    ASSERT_NE(lowerState, nullptr);
    EXPECT_EQ(lowerState->get(BlockStateProperties::AGE_0_4()), 4);

    const BlockState* upperState = world.getBlockState(pos.up());
    ASSERT_NE(upperState, nullptr) << "Upper half should exist for AGE=4";
    EXPECT_EQ(upperState->get(BlockStateProperties::AGE_0_4()), 4);
}

// ============================================================================
// grow (骨粉) 集成测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, Grow_Age0To1_SingleBlock)
{
    // 骨粉催熟 AGE 0→1，仍为单格
    auto* block = const_cast<PitcherCropBlock*>(dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP));
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    // 放置 AGE=0 的作物
    const BlockState& state0 =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(pos, &state0);

    math::Random rng(42);
    block->grow(world, rng, pos, state0);

    // 验证年龄增长到 1
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->get(BlockStateProperties::AGE_0_4()), 1) << "Age should grow from 0 to 1";
    EXPECT_EQ(newState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower)
        << "Should still be lower half";

    // AGE 1 仍为单格，上方不应有方块
    const BlockState* upperState = world.getBlockState(pos.up());
    EXPECT_EQ(upperState, nullptr) << "No upper half should exist for AGE=1";
}

TEST_F(PitcherCropIntegrationTest, Grow_Age2To3_CreatesDoubleBlock)
{
    // 骨粉催熟 AGE 2→3，变为双格植物
    auto* block = const_cast<PitcherCropBlock*>(dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP));
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    // 放置 AGE=2 的作物
    const BlockState& state2 =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 2)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(pos, &state2);

    math::Random rng(42);
    block->grow(world, rng, pos, state2);

    // 验证下半部分年龄增长到 3
    const BlockState* lowerState = world.getBlockState(pos);
    ASSERT_NE(lowerState, nullptr);
    EXPECT_EQ(lowerState->get(BlockStateProperties::AGE_0_4()), 3) << "Age should grow from 2 to 3";
    EXPECT_EQ(lowerState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);

    // 验证上半部分被放置
    const BlockState* upperState = world.getBlockState(pos.up());
    ASSERT_NE(upperState, nullptr) << "Upper half should be created when AGE reaches 3";
    EXPECT_EQ(upperState->get(BlockStateProperties::AGE_0_4()), 3);
    EXPECT_EQ(upperState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(PitcherCropIntegrationTest, Grow_AtMaxAge_NoGrowth)
{
    // AGE=4 时骨粉不应再生长
    auto* block = const_cast<PitcherCropBlock*>(dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP));
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    // 放置 AGE=4 的下半部分
    const BlockState& state4Lower =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(pos, &state4Lower);

    // 放置上半部分
    const BlockState& state4Upper =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockAt(pos.up(), &state4Upper);

    math::Random rng(42);
    block->grow(world, rng, pos, state4Lower);

    // 年龄不应变化
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->get(BlockStateProperties::AGE_0_4()), 4) << "Age should not exceed MAX_AGE=4";
}

TEST_F(PitcherCropIntegrationTest, Grow_FromUpperHalf_FindsLowerAndGrows)
{
    // 对上半部分使用骨粉，应找到下半部分并催熟
    auto* block = const_cast<PitcherCropBlock*>(dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP));
    ASSERT_NE(block, nullptr);

    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos = lowerPos.up();
    const BlockPos farmlandPos = lowerPos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    // 放置 AGE=3 的双格作物
    const BlockState& state3Lower =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 3)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState& state3Upper =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 3)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockAt(lowerPos, &state3Lower);
    world.setBlockAt(upperPos, &state3Upper);

    // 对上半部分使用骨粉
    math::Random rng(42);
    block->grow(world, rng, upperPos, state3Upper);

    // 验证下半部分年龄增长到 4
    const BlockState* newLowerState = world.getBlockState(lowerPos);
    ASSERT_NE(newLowerState, nullptr);
    EXPECT_EQ(newLowerState->get(BlockStateProperties::AGE_0_4()), 4)
        << "Age should grow from 3 to 4 when bonemealing upper half";
}

// ============================================================================
// canGrow 集成测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, CanGrow_WhenNotMaxAge_ReturnsTrue)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    const BlockState& state =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 1)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(pos, &state);

    bool canGrow = block->canGrow(static_cast<IBlockReader&>(world), pos, state, false);
    EXPECT_TRUE(canGrow) << "PitcherCropBlock should be able to grow when age < MAX_AGE";
}

TEST_F(PitcherCropIntegrationTest, CanGrow_WhenMaxAge_ReturnsFalse)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockPos farmlandPos = pos.down();

    world.setBlockAt(farmlandPos, &VanillaBlocks::FARMLAND->defaultState());

    const BlockState& state =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(pos, &state);

    bool canGrow = block->canGrow(static_cast<IBlockReader&>(world), pos, state, false);
    EXPECT_FALSE(canGrow) << "PitcherCropBlock should not be able to grow when at MAX_AGE";
}

// ============================================================================
// getPlantType 测试
// ============================================================================

TEST_F(PitcherCropIntegrationTest, GetPlantType_ReturnsCrop)
{
    // PitcherCropBlock::getPlantType 应返回 PlantType::Crop
    // 这确保 canSustain 通过 canSustainPlant 正确路由到 FarmlandBlock
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockPos pos(5, 10, 5);
    const BlockState& state = block->defaultState();

    PlantType plantType = block->getPlantType(static_cast<IBlockReader&>(world), pos);
    EXPECT_EQ(plantType, PlantType::Crop) << "PitcherCropBlock::getPlantType should return PlantType::Crop";
}

// ============================================================================
// 注册测试
// ============================================================================

TEST_F(PitcherCropBlockTest, PitcherCropBlock_IsRegistered)
{
    ASSERT_NE(TrailsBlocks::PITCHER_CROP, nullptr) << "PITCHER_CROP should be registered";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_IsPitcherCropBlock)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* pitcherCrop = dynamic_cast<const PitcherCropBlock*>(block);
    EXPECT_NE(pitcherCrop, nullptr) << "PITCHER_CROP should be registered as PitcherCropBlock, not SimpleBlock";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_ImplementsIGrowable)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* growable = dynamic_cast<const IGrowable*>(block);
    EXPECT_NE(growable, nullptr) << "PitcherCropBlock should implement IGrowable";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_InheritsDoublePlantBlock)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* doublePlant = dynamic_cast<const DoublePlantBlock*>(block);
    EXPECT_NE(doublePlant, nullptr) << "PitcherCropBlock should inherit from DoublePlantBlock";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_DoesNotInheritCropBlock)
{
    // PitcherCropBlock 继承自 DoublePlantBlock，不是 CropBlock
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* cropBlock = dynamic_cast<const CropBlock*>(block);
    EXPECT_EQ(cropBlock, nullptr) << "PitcherCropBlock should NOT inherit from CropBlock";
}

// ============================================================================
// onEntityCollision 集成测试（Ravager 破坏作物逻辑）
// ============================================================================
//
// 测试场景：参考 MC Java PitcherCropBlock.entityInside
// - Ravager 进入瓶草作物方块 + mobGriefing=true → 方块被破坏
// - Ravager 进入瓶草作物方块 + mobGriefing=false → 方块保留
// - 非 Ravager 实体（如 Pig）进入 → 方块保留
// - 客户端世界 → 不执行破坏（PitcherCropTestWorld 默认 isClientSide=false，
//   通过 isClientSide=false 模拟服务端，本测试主要验证服务端逻辑）

class PitcherCropOnEntityCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
        // 注册所有原版实体类型，确保 RavagerEntity 的 typeId 可查询
        entity::VanillaEntities::registerAll();
    }

    PitcherCropTestWorld world;
};

// 辅助：在指定位置放置一个 AGE=4 的双格瓶草作物（下半部分 + 上半部分）
const BlockState placeMaturePitcherCrop(PitcherCropTestWorld& world, const BlockPos& lowerPos)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    EXPECT_NE(block, nullptr);

    // 放置耕地
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::FARMLAND->defaultState());

    // 下半部分 AGE=4
    const BlockState lowerState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(lowerPos, &lowerState);

    // 上半部分 AGE=4
    const BlockState upperState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockAt(lowerPos.up(), &upperState);

    return lowerState;
}

// 辅助：构造 RavagerEntity 并设置 typeId
std::unique_ptr<RavagerEntity> createRavager(PitcherCropTestWorld& world)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1));
    ravager->setWorld(&world);
    // 显式设置 typeId 字符串，使 entity.entityType() 懒查询返回 RAVAGER 类型指针
    // （RavagerEntity 构造函数不会自动设置 typeId，需通过 EntityType::create 或显式 setTypeId）
    ravager->setTypeId("minecraft:ravager");
    return ravager;
}

// 辅助：构造 PigEntity 作为非 Ravager 对照
std::unique_ptr<PigEntity> createPig(PitcherCropTestWorld& world)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(2));
    pig->setWorld(&world);
    pig->setTypeId("minecraft:pig");
    return pig;
}

// ---------- Ravager + mobGriefing=true → 方块被破坏 ----------

TEST_F(PitcherCropOnEntityCollisionTest, Ravager_WithMobGriefing_BreaksLowerHalf)
{
    // 默认 mobGriefing=true（GameRules 默认值）
    const BlockPos lowerPos(5, 10, 5);

    // 放置成熟的双格瓶草作物
    const BlockState lowerState = placeMaturePitcherCrop(world, lowerPos);

    // 创建 Ravager
    auto ravager = createRavager(world);

    // 对下半部分调用 onEntityCollision
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    block->onEntityCollision(lowerState, world, lowerPos, *ravager);

    // 验证下半部分被设为空气（破坏成功）
    const BlockState* afterState = world.getBlockState(lowerPos);
    if (afterState != nullptr) {
        EXPECT_TRUE(afterState->isAir()) << "Lower half should be air after Ravager collision";
    }
    // 注：afterState 可能为 nullptr（PitcherCropTestWorld 在 setBlockState 设为 air 时 erase）
}

TEST_F(PitcherCropOnEntityCollisionTest, Ravager_WithMobGriefing_BreaksUpperHalf)
{
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos = lowerPos.up();

    placeMaturePitcherCrop(world, lowerPos);

    // 重新获取上半部分状态副本（避免 setBlockState 后引用失效）
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    const BlockState upperState =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 4)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    auto ravager = createRavager(world);

    // 对上半部分调用 onEntityCollision
    block->onEntityCollision(upperState, world, upperPos, *ravager);

    // 验证上半部分被设为空气
    const BlockState* afterState = world.getBlockState(upperPos);
    if (afterState != nullptr) {
        EXPECT_TRUE(afterState->isAir()) << "Upper half should be air after Ravager collision";
    }
}

// ---------- Ravager + mobGriefing=false → 方块保留 ----------

TEST_F(PitcherCropOnEntityCollisionTest, Ravager_WithoutMobGriefing_KeepsBlock)
{
    // 关闭 mobGriefing
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false);

    const BlockPos lowerPos(5, 10, 5);
    const BlockState lowerState = placeMaturePitcherCrop(world, lowerPos);

    auto ravager = createRavager(world);

    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    block->onEntityCollision(lowerState, world, lowerPos, *ravager);

    // 验证方块保留（仍是瓶草作物，AGE 仍为 4）
    const BlockState* afterState = world.getBlockState(lowerPos);
    ASSERT_NE(afterState, nullptr) << "Block should not be destroyed when mobGriefing is false";
    EXPECT_TRUE(afterState->is(TrailsBlocks::PITCHER_CROP))
        << "Block should still be pitcher crop when mobGriefing is false";
    EXPECT_EQ(afterState->get(BlockStateProperties::AGE_0_4()), 4);
}

// ---------- 非 Ravager 实体（如 Pig）→ 方块保留 ----------

TEST_F(PitcherCropOnEntityCollisionTest, NonRavagerEntity_KeepsBlock)
{
    const BlockPos lowerPos(5, 10, 5);
    const BlockState lowerState = placeMaturePitcherCrop(world, lowerPos);

    auto pig = createPig(world);

    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    block->onEntityCollision(lowerState, world, lowerPos, *pig);

    // 验证方块保留
    const BlockState* afterState = world.getBlockState(lowerPos);
    ASSERT_NE(afterState, nullptr) << "Block should not be destroyed by non-Ravager entity";
    EXPECT_TRUE(afterState->is(TrailsBlocks::PITCHER_CROP)) << "Block should still be pitcher crop after Pig collision";
    EXPECT_EQ(afterState->get(BlockStateProperties::AGE_0_4()), 4);
}

// ---------- 未成熟作物（AGE=0）也会被 Ravager 破坏 ----------

TEST_F(PitcherCropOnEntityCollisionTest, Ravager_BreaksImmatureCrop)
{
    // 瓶草作物无论成熟度都会被 Ravager 破坏（与 MC Java 行为一致）
    const BlockPos lowerPos(5, 10, 5);

    // 放置耕地
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::FARMLAND->defaultState());

    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 放置 AGE=0 的单格作物
    const BlockState state0 =
        block->defaultState()
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockAt(lowerPos, &state0);

    auto ravager = createRavager(world);
    block->onEntityCollision(state0, world, lowerPos, *ravager);

    // 验证方块被破坏
    const BlockState* afterState = world.getBlockState(lowerPos);
    if (afterState != nullptr) {
        EXPECT_TRUE(afterState->isAir()) << "Immature crop should be broken by Ravager";
    }
}

} // namespace
