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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/pale_garden/PaleHangingMossBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "world/border/WorldBorder.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 苍白垂苔测试用世界
 *
 * 支持 BlockState 存储、TickManager 和 isSolidSide 的测试世界。
 * 继承自 IBlockReader 以支持需要 IBlockReader 参数的方法。
 */
class PaleHangingMossTestWorld final : public IBlockReader {
public:
    PaleHangingMossTestWorld() { VanillaBlocks::initialize(); }

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

    void setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        auto [it, inserted] = m_ownedStates.insert_or_assign(pos, state);
        m_blocks[pos] = &it->second;
    }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
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
        const_cast<PaleHangingMossTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
    u64 m_seed = 12345;
    u64 m_currentTick = 0;
};

/**
 * @brief 测试用固体方块（isSolidSide 对所有面返回 true）
 */
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

/**
 * @brief 测试用非固体方块（isSolidSide 对所有面返回 false）
 */
class TestNonSolidBlock final : public Block {
public:
    explicit TestNonSolidBlock(const BlockProperties& properties)
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
        return false;
    }
};

} // namespace

// ========== PaleHangingMossBlock 测试 ==========

class PaleHangingMossBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        moss_ = std::make_unique<PaleHangingMossBlock>(
            BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    }

    std::unique_ptr<PaleHangingMossBlock> moss_;
    PaleHangingMossTestWorld world_;
};

// ============================================================================
// 构造与默认状态测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(moss_, nullptr);
}

TEST_F(PaleHangingMossBlockTest, DefaultState_TipIsFalse)
{
    const BlockState& state = moss_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, WithTip_True)
{
    auto state = moss_->withTip(true);
    EXPECT_TRUE(state.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, WithTip_False)
{
    auto state = moss_->withTip(false);
    EXPECT_FALSE(state.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, IsTip_TrueState)
{
    auto state = moss_->withTip(true);
    EXPECT_TRUE(moss_->isTip(state));
}

TEST_F(PaleHangingMossBlockTest, IsTip_FalseState)
{
    auto state = moss_->withTip(false);
    EXPECT_FALSE(moss_->isTip(state));
}

// ============================================================================
// isValidPosition 测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, IsValidPosition_NoBlockAbove_ReturnsFalse)
{
    // 上方没有方块 → 不满足存活条件
    const BlockPos pos(5, 10, 5);
    EXPECT_FALSE(moss_->isValidPosition(moss_->defaultState(), world_, pos));
}

TEST_F(PaleHangingMossBlockTest, IsValidPosition_SolidBlockAbove_ReturnsTrue)
{
    // 上方有固体方块（isSolidSide 对 Direction::Down 返回 true）
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);
    world_.setBlockStateCopy(abovePos, solidBlock.defaultState());

    EXPECT_TRUE(moss_->isValidPosition(moss_->defaultState(), world_, pos));
}

TEST_F(PaleHangingMossBlockTest, IsValidPosition_NonSolidBlockAbove_ReturnsFalse)
{
    // 上方有方块但不是固体（如空气、植物等），不满足存活条件
    TestNonSolidBlock nonSolidBlock(BlockProperties(Material::PLANT).noCollision().notSolid());
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);
    world_.setBlockStateCopy(abovePos, nonSolidBlock.defaultState());

    EXPECT_FALSE(moss_->isValidPosition(moss_->defaultState(), world_, pos));
}

TEST_F(PaleHangingMossBlockTest, IsValidPosition_AnotherPaleHangingMossAbove_ReturnsTrue)
{
    // 上方是另一个苍白垂苔方块 → 允许链式悬挂
    PaleHangingMossBlock upperMoss(
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);
    world_.setBlockStateCopy(abovePos, upperMoss.defaultState());

    EXPECT_TRUE(moss_->isValidPosition(moss_->defaultState(), world_, pos));
}

TEST_F(PaleHangingMossBlockTest, IsValidPosition_AirAbove_ReturnsFalse)
{
    // 上方是空气 → 不满足存活条件
    // (getBlockState 返回 nullptr，即空气位置)
    const BlockPos pos(5, 10, 5);
    // 默认世界中未设置方块的位置返回 nullptr
    EXPECT_FALSE(moss_->isValidPosition(moss_->defaultState(), world_, pos));
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, GetShape_TipState_ReturnsTipShape)
{
    // TIP=true 时应使用末端形状（更短）
    auto tipState = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    const CollisionShape& tipShape = moss_->getShape(tipState);
    EXPECT_FALSE(tipShape.isEmpty());
    EXPECT_EQ(tipShape.boxCount(), 1u);

    // 末端形状: box(2, 0, 2, 14, 10, 14) — 使用方块局部坐标
    const auto& box = tipShape.boxes()[0];
    EXPECT_FLOAT_EQ(box.minX, 2.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 2.0f);
    EXPECT_FLOAT_EQ(box.maxX, 14.0f);
    EXPECT_FLOAT_EQ(box.maxY, 10.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 14.0f);
}

TEST_F(PaleHangingMossBlockTest, GetShape_BodyState_ReturnsBodyShape)
{
    // TIP=false 时应使用身体形状（更长）
    auto bodyState = moss_->defaultState().with(BlockStateProperties::TIP(), false);
    const CollisionShape& bodyShape = moss_->getShape(bodyState);
    EXPECT_FALSE(bodyShape.isEmpty());
    EXPECT_EQ(bodyShape.boxCount(), 1u);

    // 身体形状: box(2, 0, 2, 14, 16, 14) — 使用方块局部坐标
    const auto& box = bodyShape.boxes()[0];
    EXPECT_FLOAT_EQ(box.minX, 2.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 2.0f);
    EXPECT_FLOAT_EQ(box.maxX, 14.0f);
    EXPECT_FLOAT_EQ(box.maxY, 16.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 14.0f);
}

TEST_F(PaleHangingMossBlockTest, GetShape_TipShorterThanBody)
{
    // 末端形状比身体形状短（maxY 更小）
    auto tipState = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    auto bodyState = moss_->defaultState().with(BlockStateProperties::TIP(), false);

    const CollisionShape& tipShape = moss_->getShape(tipState);
    const CollisionShape& bodyShape = moss_->getShape(bodyState);

    EXPECT_LT(tipShape.boxes()[0].maxY, bodyShape.boxes()[0].maxY);
}

TEST_F(PaleHangingMossBlockTest, GetCollisionShape_AlwaysEmpty)
{
    // 苍白垂苔没有碰撞箱
    auto tipState = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    auto bodyState = moss_->defaultState().with(BlockStateProperties::TIP(), false);

    EXPECT_TRUE(moss_->getCollisionShape(tipState).isEmpty());
    EXPECT_TRUE(moss_->getCollisionShape(bodyState).isEmpty());
}

// ============================================================================
// 光照测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    auto tipState = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    auto bodyState = moss_->defaultState().with(BlockStateProperties::TIP(), false);

    EXPECT_TRUE(moss_->useShapeForLightOcclusion(tipState));
    EXPECT_TRUE(moss_->useShapeForLightOcclusion(bodyState));
}

TEST_F(PaleHangingMossBlockTest, IsOpaque_AlwaysFalse)
{
    auto tipState = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    auto bodyState = moss_->defaultState().with(BlockStateProperties::TIP(), false);

    EXPECT_FALSE(moss_->isOpaque(tipState));
    EXPECT_FALSE(moss_->isOpaque(bodyState));
}

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, UpdatePostPlacement_NoSupport_SchedulesTick)
{
    // 当支撑失效时，updatePostPlacement 应调度 tick 以销毁方块
    world_.ensureTickManager();
    const BlockPos pos(5, 10, 5);

    // 不设置上方方块（无支撑）
    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    BlockState updatedState =
        moss_->updatePostPlacement(state, Direction::Up, *world_.getBlockState(pos), world_, pos, BlockPos(5, 11, 5));

    // TIP 属性应该更新（下方没有苍白垂苔 → TIP=true）
    EXPECT_TRUE(updatedState.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, UpdatePostPlacement_MossBelow_TipIsFalse)
{
    // 下方有另一个苍白垂苔 → TIP=false
    world_.ensureTickManager();
    PaleHangingMossBlock lowerMoss(
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));

    const BlockPos pos(5, 10, 5);
    const BlockPos belowPos(5, 9, 5);
    const BlockPos abovePos(5, 11, 5);

    // 上方设置固体方块以提供支撑
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockStateCopy(abovePos, solidBlock.defaultState());

    // 下方设置另一个苍白垂苔
    world_.setBlockStateCopy(belowPos, lowerMoss.defaultState());

    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    BlockState updatedState =
        moss_->updatePostPlacement(state, Direction::Down, lowerMoss.defaultState(), world_, pos, belowPos);

    // 下方有苍白垂苔 → 不是末端
    EXPECT_FALSE(updatedState.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, UpdatePostPlacement_NoMossBelow_TipIsTrue)
{
    // 下方没有苍白垂苔 → TIP=true
    world_.ensureTickManager();
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);

    // 上方设置固体方块以提供支撑
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockStateCopy(abovePos, solidBlock.defaultState());

    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), false);
    BlockState updatedState =
        moss_->updatePostPlacement(state, Direction::Down, *world_.getBlockState(pos), world_, pos, BlockPos(5, 9, 5));

    // 下方没有苍白垂苔 → 是末端
    EXPECT_TRUE(updatedState.get(BlockStateProperties::TIP()));
}

TEST_F(PaleHangingMossBlockTest, UpdatePostPlacement_SolidBlockBelow_TipIsTrue)
{
    // 下方是固体方块（不是苍白垂苔）→ TIP=true
    world_.ensureTickManager();
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));

    const BlockPos pos(5, 10, 5);
    const BlockPos belowPos(5, 9, 5);
    const BlockPos abovePos(5, 11, 5);

    // 上方设置固体方块以提供支撑
    world_.setBlockStateCopy(abovePos, solidBlock.defaultState());
    // 下方设置固体方块（非苍白垂苔）
    world_.setBlockStateCopy(belowPos, solidBlock.defaultState());

    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), false);
    BlockState updatedState =
        moss_->updatePostPlacement(state, Direction::Down, solidBlock.defaultState(), world_, pos, belowPos);

    // 下方不是苍白垂苔 → 是末端
    EXPECT_TRUE(updatedState.get(BlockStateProperties::TIP()));
}

// ============================================================================
// tick 测试 - 支撑失效时销毁方块
// ============================================================================

TEST_F(PaleHangingMossBlockTest, Tick_NoSupport_ReplacesWithAir)
{
    // tick 时检查存活条件，不满足则替换为空气
    world_.ensureTickManager();
    const BlockPos pos(5, 10, 5);

    // 不设置上方方块（无支撑）
    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    world_.setBlockStateCopy(pos, state);

    math::Random& rng = world_.getRandom();
    moss_->tick(world_, pos, state, rng);

    // 方块应该被替换为空气
    const BlockState* finalState = world_.getBlockAt(pos);
    // 空气位置返回 nullptr 或 isAir
    EXPECT_TRUE(finalState == nullptr || finalState->isAir()) << "Block should be destroyed (air) when unsupported";
}

TEST_F(PaleHangingMossBlockTest, Tick_WithSolidSupport_NotDestroyed)
{
    // 有固体支撑时 tick 不应销毁方块
    world_.ensureTickManager();
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);

    // 上方设置固体方块
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockStateCopy(abovePos, solidBlock.defaultState());

    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    world_.setBlockStateCopy(pos, state);

    math::Random& rng = world_.getRandom();
    moss_->tick(world_, pos, state, rng);

    // 方块应该仍然存在
    const BlockState* finalState = world_.getBlockAt(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(dynamic_cast<const PaleHangingMossBlock*>(&finalState->getBlock()) != nullptr)
        << "Block should still exist when supported";
}

TEST_F(PaleHangingMossBlockTest, Tick_WithMossChainSupport_NotDestroyed)
{
    // 上方是另一个苍白垂苔（链式悬挂）时 tick 不应销毁方块
    world_.ensureTickManager();
    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);

    // 上方放置另一个苍白垂苔
    PaleHangingMossBlock upperMoss(
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    world_.setBlockStateCopy(abovePos, upperMoss.defaultState());

    auto state = moss_->defaultState().with(BlockStateProperties::TIP(), true);
    world_.setBlockStateCopy(pos, state);

    math::Random& rng = world_.getRandom();
    moss_->tick(world_, pos, state, rng);

    // 方块应该仍然存在
    const BlockState* finalState = world_.getBlockAt(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(dynamic_cast<const PaleHangingMossBlock*>(&finalState->getBlock()) != nullptr)
        << "Block should still exist when chained with another moss";
}

// ============================================================================
// 链式悬挂场景测试
// ============================================================================

TEST_F(PaleHangingMossBlockTest, Chain_TopMossSupportedBySolid_MiddleMossSupportedByTopMoss)
{
    // 三格链式悬挂：固体 → 苍白垂苔A → 苍白垂苔B → 苍白垂苔C
    // 中间和底部的苍白垂苔应满足存活条件
    const BlockPos topPos(5, 13, 5);
    const BlockPos midPos(5, 12, 5);
    const BlockPos bottomPos(5, 11, 5);
    const BlockPos solidPos(5, 14, 5);

    // 顶部设置固体方块
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockStateCopy(solidPos, solidBlock.defaultState());

    // 中间和底部设置苍白垂苔
    PaleHangingMossBlock chainMoss(
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    world_.setBlockStateCopy(topPos, chainMoss.defaultState());
    world_.setBlockStateCopy(midPos, chainMoss.defaultState());

    // 中间的苍白垂苔应该能存活（上方有苍白垂苔）
    EXPECT_TRUE(moss_->isValidPosition(moss_->defaultState(), world_, bottomPos));

    // 顶部的苍白垂苔也应该能存活（上方有固体方块）
    EXPECT_TRUE(chainMoss.isValidPosition(chainMoss.defaultState(), world_, topPos));
}

TEST_F(PaleHangingMossBlockTest, Chain_BottomMossIsTip_MiddleIsNotTip)
{
    // 链式场景：固体方块 → 顶部苍白垂苔（非末端）→ 底部苍白垂苔（末端）
    world_.ensureTickManager();
    const BlockPos topPos(5, 11, 5);
    const BlockPos bottomPos(5, 10, 5);
    const BlockPos solidPos(5, 12, 5);

    // 顶部设置固体方块
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world_.setBlockStateCopy(solidPos, solidBlock.defaultState());

    // 顶部放置苍白垂苔
    PaleHangingMossBlock chainMoss(
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    world_.setBlockStateCopy(topPos, chainMoss.defaultState());

    // 底部也放置苍白垂苔
    world_.setBlockStateCopy(bottomPos, moss_->defaultState());

    // 底部的苍白垂苔 → 下方无苍白垂苔 → 是末端 (TIP=true)
    auto bottomState = moss_->defaultState().with(BlockStateProperties::TIP(), false);
    BlockState updatedBottomState =
        moss_->updatePostPlacement(bottomState, Direction::Up, chainMoss.defaultState(), world_, bottomPos, topPos);
    EXPECT_TRUE(updatedBottomState.get(BlockStateProperties::TIP()));

    // 顶部的苍白垂苔 → 下方有苍白垂苔 → 不是末端 (TIP=false)
    auto topState = chainMoss.defaultState().with(BlockStateProperties::TIP(), true);
    BlockState updatedTopState =
        chainMoss.updatePostPlacement(topState, Direction::Down, moss_->defaultState(), world_, topPos, bottomPos);
    EXPECT_FALSE(updatedTopState.get(BlockStateProperties::TIP()));
}
