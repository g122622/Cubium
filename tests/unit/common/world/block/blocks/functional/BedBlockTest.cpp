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
 * copies or the Software. AnY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file BedBlockTest.cpp
 * @brief 床方块单元测试
 *
 * 测试 BedBlock 的功能：
 * - getBedOrientation: 获取床朝向
 * - getConnectedDirection: 获取床连接方向
 * - findStandUpPosition: 计算起床位置
 * - Directions::isFacingAngle: 偏航角朝向判断
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 床方块测试用世界桩
 *
 * 最小化的 IBlockReader 实现，仅支持 getBlockState/setBlockState，
 * 用于测试 BedBlock 的起床位置计算。
 */
class BedBlockTestWorld final : public IBlockReader {
public:
    BedBlockTestWorld() = default;

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
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
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
        const_cast<BedBlockTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random;
};

// ========== getBedOrientation 测试 ==========

class BedBlockOrientationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建白色床方块
        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
    }

    std::unique_ptr<BedBlock> bed_;
    BedBlockTestWorld world;
};

TEST_F(BedBlockOrientationTest, GetBedOrientation_ReturnsFacingWhenBedExists)
{
    // 放置一个朝南的床头
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
    world.setBlockAt(BlockPos(10, 64, 10), bedHead);

    Direction result = BedBlock::getBedOrientation(world, BlockPos(10, 64, 10));
    EXPECT_EQ(result, Direction::South);
}

TEST_F(BedBlockOrientationTest, GetBedOrientation_ReturnsNoneWhenNoBed)
{
    // 空位置应返回 None
    Direction result = BedBlock::getBedOrientation(world, BlockPos(10, 64, 10));
    EXPECT_EQ(result, Direction::None);
}

TEST_F(BedBlockOrientationTest, GetBedOrientation_AllFourDirections)
{
    const Direction directions[] = {Direction::North, Direction::South, Direction::East, Direction::West};

    for (Direction dir : directions) {
        BedBlockTestWorld dirWorld;
        const BlockState* bedHead = &bed_->defaultState()
                                         .with(BlockStateProperties::HORIZONTAL_FACING(), dir)
                                         .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
        dirWorld.setBlockAt(BlockPos(0, 64, 0), bedHead);

        Direction result = BedBlock::getBedOrientation(dirWorld, BlockPos(0, 64, 0));
        EXPECT_EQ(result, dir) << "Failed for direction index " << static_cast<int>(dir);
    }
}

// ========== getConnectedDirection 测试 ==========

class BedBlockConnectedDirectionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
    }

    std::unique_ptr<BedBlock> bed_;
};

TEST_F(BedBlockConnectedDirectionTest, HeadPartConnectsToFacingDirection)
{
    // 头部朝南：连接方向为南（朝向脚部）
    const BlockState& headSouth = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
    EXPECT_EQ(BedBlock::getConnectedDirection(headSouth), Direction::South);
}

TEST_F(BedBlockConnectedDirectionTest, FootPartConnectsToOppositeDirection)
{
    // 脚部朝南：连接方向为北（朝向头部）
    const BlockState& footSouth = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    EXPECT_EQ(BedBlock::getConnectedDirection(footSouth), Direction::North);
}

TEST_F(BedBlockConnectedDirectionTest, AllDirectionsConsistent)
{
    const Direction directions[] = {Direction::North, Direction::South, Direction::East, Direction::West};

    for (Direction dir : directions) {
        const BlockState& headState = bed_->defaultState()
                                          .with(BlockStateProperties::HORIZONTAL_FACING(), dir)
                                          .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
        const BlockState& footState = bed_->defaultState()
                                          .with(BlockStateProperties::HORIZONTAL_FACING(), dir)
                                          .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);

        EXPECT_EQ(BedBlock::getConnectedDirection(headState), dir) << "Head part should connect in facing direction";
        EXPECT_EQ(BedBlock::getConnectedDirection(footState), Directions::opposite(dir))
            << "Foot part should connect in opposite direction";
    }
}

// ========== findStandUpPosition 测试 ==========

class BedBlockStandUpTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
    }

    std::unique_ptr<BedBlock> bed_;
    BedBlockTestWorld world;
};

TEST_F(BedBlockStandUpTest, FindStandUpPosition_ReturnsFallbackWhenBedSurrounded)
{
    // 床头位于 (10, 64, 10)，朝北
    // 周围所有位置都放满床头方块（有 blocksMovement），应该返回床头正上方
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(10, 64, 10), bedHead);

    // 用床头方块填满周围所有可能位置（Y=65 和 Y=66）
    // 床方块 blocksMovement = true，所以不会被视为安全站立空间
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            world.setBlockAt(BlockPos(10 + dx, 65, 10 + dz), bedHead);
            world.setBlockAt(BlockPos(10 + dx, 66, 10 + dz), bedHead);
        }
    }

    Vector3 result = BedBlock::findStandUpPosition(world, BlockPos(10, 64, 10), Direction::North, 0.0f);

    // 床头正上方: (10.5, 65.1, 10.5)
    EXPECT_FLOAT_EQ(result.x, 10.5f);
    EXPECT_FLOAT_EQ(result.z, 10.5f);
    // Y 应回退到 65.1 (bedPos.up() + 0.1)
    EXPECT_FLOAT_EQ(result.y, 65.1f);
}

TEST_F(BedBlockStandUpTest, FindStandUpPosition_OpenSpaceNearBed)
{
    // 床头位于 (10, 64, 10)，朝北
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(10, 64, 10), bedHead);

    // 周围不放任何方块（全部为空气），应该找到安全的起床位置
    Vector3 result = BedBlock::findStandUpPosition(world, BlockPos(10, 64, 10), Direction::North, 0.0f);

    // 结果应该在床附近的合理范围内（偏移不超过3格）
    EXPECT_GE(result.x, 7.5f);
    EXPECT_LE(result.x, 13.5f);
    EXPECT_GE(result.z, 7.5f);
    EXPECT_LE(result.z, 13.5f);
    // Y 应该接近床的高度
    EXPECT_NEAR(result.y, 64.1f, 1.0f);
}

TEST_F(BedBlockStandUpTest, FindStandUpPosition_AllDirectionsReturnValidPosition)
{
    const Direction directions[] = {Direction::North, Direction::South, Direction::East, Direction::West};

    for (Direction dir : directions) {
        BedBlockTestWorld dirWorld;
        const BlockState* bedHead = &bed_->defaultState()
                                         .with(BlockStateProperties::HORIZONTAL_FACING(), dir)
                                         .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
        dirWorld.setBlockAt(BlockPos(50, 64, 50), bedHead);

        Vector3 result = BedBlock::findStandUpPosition(dirWorld, BlockPos(50, 64, 50), dir, 0.0f);

        // 每个方向都应返回有效位置
        EXPECT_GT(result.y, 0.0f) << "Failed for direction index " << static_cast<int>(dir);
    }
}

TEST_F(BedBlockStandUpTest, FindStandUpPosition_SouthFacing)
{
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), bedHead);

    Vector3 result = BedBlock::findStandUpPosition(world, BlockPos(0, 64, 0), Direction::South, 0.0f);

    // 应该返回有效位置
    EXPECT_GT(result.y, 0.0f);
}

TEST_F(BedBlockStandUpTest, FindStandUpPosition_EastFacing)
{
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), bedHead);

    Vector3 result = BedBlock::findStandUpPosition(world, BlockPos(0, 64, 0), Direction::East, 0.0f);

    EXPECT_GT(result.y, 0.0f);
}

TEST_F(BedBlockStandUpTest, FindStandUpPosition_WestFacing)
{
    const BlockState* bedHead = &bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), bedHead);

    Vector3 result = BedBlock::findStandUpPosition(world, BlockPos(0, 64, 0), Direction::West, 0.0f);

    EXPECT_GT(result.y, 0.0f);
}

// ========== Directions::isFacingAngle 测试 ==========

class DirectionIsFacingAngleTest : public ::testing::Test {};

TEST_F(DirectionIsFacingAngleTest, SouthFacingYaw0)
{
    // MC 约定：yaw=0 面向南方，South 方向的 normal 为 (0, 0, 1)
    EXPECT_TRUE(Directions::isFacingAngle(Direction::South, 0.0f));
}

TEST_F(DirectionIsFacingAngleTest, NorthFacingYaw180)
{
    // yaw=180 面向北方
    EXPECT_TRUE(Directions::isFacingAngle(Direction::North, 180.0f));
}

TEST_F(DirectionIsFacingAngleTest, EastFacingYaw270)
{
    // yaw=-90 (270) 面向东方
    EXPECT_TRUE(Directions::isFacingAngle(Direction::East, -90.0f));
}

TEST_F(DirectionIsFacingAngleTest, WestFacingYaw90)
{
    // yaw=90 面向西方
    EXPECT_TRUE(Directions::isFacingAngle(Direction::West, 90.0f));
}

TEST_F(DirectionIsFacingAngleTest, NonFacingDirectionReturnsFalse)
{
    // yaw=0 面向南方，不应朝向北方
    EXPECT_FALSE(Directions::isFacingAngle(Direction::North, 0.0f));
}

TEST_F(DirectionIsFacingAngleTest, VerticalDirectionReturnsFalse)
{
    // 垂直方向不应受偏航角影响
    EXPECT_FALSE(Directions::isFacingAngle(Direction::Up, 0.0f));
    EXPECT_FALSE(Directions::isFacingAngle(Direction::Down, 0.0f));
}

TEST_F(DirectionIsFacingAngleTest, SouthFacingYaw45)
{
    // yaw=45 是西南方向，仍然大致朝南
    EXPECT_TRUE(Directions::isFacingAngle(Direction::South, 45.0f));
    // 但不朝东
    EXPECT_FALSE(Directions::isFacingAngle(Direction::East, 45.0f));
}

// ========== onBlockPlacedBy 测试 ==========

class BedBlockPlacedByTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // playerWillDestroy 需要 airState，所以必须初始化方块注册表
        VanillaBlocks::initialize();

        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
    }

    void TearDown() override { bed_.reset(); }

    std::unique_ptr<BedBlock> bed_;
    BedBlockTestWorld world;
};

TEST_F(BedBlockPlacedByTest, PlacesHeadBlockAtFacingOffset)
{
    // 放置脚部朝南 → 床头应出现在 (5, 64, 6)
    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    world.setBlockAt(BlockPos(5, 64, 5), &footState);

    bed_->onBlockPlacedBy(world, BlockPos(5, 64, 5), footState, ItemStack{});

    // 检查头部位置 (5, 64, 6) 是否被设为 Head
    const BlockState* headState = world.getBlockState(5, 64, 6);
    ASSERT_NE(headState, nullptr);
    EXPECT_TRUE(headState->hasProperty(BlockStateProperties::BED_PART()));
    EXPECT_EQ(headState->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
    EXPECT_EQ(headState->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(BedBlockPlacedByTest, PlacesHeadBlockNorthFacing)
{
    // 放置脚部朝北 → 床头应出现在 (5, 64, 4)
    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    world.setBlockAt(BlockPos(5, 64, 5), &footState);

    bed_->onBlockPlacedBy(world, BlockPos(5, 64, 5), footState, ItemStack{});

    const BlockState* headState = world.getBlockState(5, 64, 4);
    ASSERT_NE(headState, nullptr);
    EXPECT_EQ(headState->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
    EXPECT_EQ(headState->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BedBlockPlacedByTest, PlacesHeadBlockEastFacing)
{
    // 放置脚部朝东 → 床头应出现在 (6, 64, 5)
    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    world.setBlockAt(BlockPos(5, 64, 5), &footState);

    bed_->onBlockPlacedBy(world, BlockPos(5, 64, 5), footState, ItemStack{});

    const BlockState* headState = world.getBlockState(6, 64, 5);
    ASSERT_NE(headState, nullptr);
    EXPECT_EQ(headState->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
}

TEST_F(BedBlockPlacedByTest, PlacesHeadBlockWestFacing)
{
    // 放置脚部朝西 → 床头应出现在 (4, 64, 5)
    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    world.setBlockAt(BlockPos(5, 64, 5), &footState);

    bed_->onBlockPlacedBy(world, BlockPos(5, 64, 5), footState, ItemStack{});

    const BlockState* headState = world.getBlockState(4, 64, 5);
    ASSERT_NE(headState, nullptr);
    EXPECT_EQ(headState->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
}

TEST_F(BedBlockPlacedByTest, HeadBlockPreservesFacingDirection)
{
    // 朝南脚部的头部也应朝南
    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    world.setBlockAt(BlockPos(0, 64, 0), &footState);

    bed_->onBlockPlacedBy(world, BlockPos(0, 64, 0), footState, ItemStack{});

    const BlockState* headState = world.getBlockState(0, 64, 1);
    ASSERT_NE(headState, nullptr);
    EXPECT_EQ(headState->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

// ========== playerWillDestroy 测试 ==========

class BedBlockDestroyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // playerWillDestroy 需要 BlockRegistry::airState()，所以必须初始化
        VanillaBlocks::initialize();

        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
    }

    void TearDown() override { bed_.reset(); }

    std::unique_ptr<BedBlock> bed_;
    BedBlockTestWorld world;
};

TEST_F(BedBlockDestroyTest, CreativeModeDestroysHeadWhenFootBroken)
{
    // 创造模式：破坏脚部时，头部应被设为空气
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);
    ASSERT_TRUE(player.isCreative());

    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    const BlockState& headState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    // 放置脚部在 (0, 64, 0)，头部在 (0, 64, 1)
    world.setBlockAt(BlockPos(0, 64, 0), &footState);
    world.setBlockAt(BlockPos(0, 64, 1), &headState);

    // 破坏脚部
    bed_->playerWillDestroy(world, BlockPos(0, 64, 0), footState, player);

    // 头部应已被设为空气
    const BlockState* headAfter = world.getBlockState(0, 64, 1);
    EXPECT_TRUE(headAfter == nullptr || headAfter->isAir());
}

TEST_F(BedBlockDestroyTest, SurvivalModeDoesNotDestroyHeadWhenFootBroken)
{
    // 生存模式：破坏脚部时，头部不应被移除
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);
    ASSERT_FALSE(player.isCreative());

    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    const BlockState& headState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), &footState);
    world.setBlockAt(BlockPos(0, 64, 1), &headState);

    bed_->playerWillDestroy(world, BlockPos(0, 64, 0), footState, player);

    // 头部应仍然存在
    const BlockState* headAfter = world.getBlockState(0, 64, 1);
    ASSERT_NE(headAfter, nullptr);
    EXPECT_EQ(headAfter->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
}

TEST_F(BedBlockDestroyTest, DestroyingHeadDoesNotAffectFoot)
{
    // 破坏头部不应影响脚部（即使创造模式）
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    const BlockState& headState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), &footState);
    world.setBlockAt(BlockPos(0, 64, 1), &headState);

    // 破坏头部
    bed_->playerWillDestroy(world, BlockPos(0, 64, 1), headState, player);

    // 脚部应不受影响
    const BlockState* footAfter = world.getBlockState(0, 64, 0);
    ASSERT_NE(footAfter, nullptr);
    EXPECT_EQ(footAfter->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Foot);
}

TEST_F(BedBlockDestroyTest, CreativeModeDoesNotRemoveHeadIfNoHeadPresent)
{
    // 创造模式破坏脚部但头部位置没有床头方块 → 不应崩溃
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);

    world.setBlockAt(BlockPos(0, 64, 0), &footState);
    // 不放头部

    EXPECT_NO_THROW(bed_->playerWillDestroy(world, BlockPos(0, 64, 0), footState, player));
}

TEST_F(BedBlockDestroyTest, CreativeModeDestroysHeadAllDirections)
{
    // 验证四个方向的创造模式破坏均正确移除头部
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    struct TestCase {
        Direction facing;
        BlockPos footPos;
        BlockPos headPos;
    };

    TestCase cases[] = {
        {Direction::North, BlockPos(5, 64, 5), BlockPos(5, 64, 4)},
        {Direction::South, BlockPos(5, 64, 5), BlockPos(5, 64, 6)},
        {Direction::East, BlockPos(5, 64, 5), BlockPos(6, 64, 5)},
        {Direction::West, BlockPos(5, 64, 5), BlockPos(4, 64, 5)},
    };

    for (const auto& tc : cases) {
        BedBlockTestWorld dirWorld;
        const BlockState& foot = bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), tc.facing)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
        const BlockState& head = bed_->defaultState()
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), tc.facing)
                                     .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

        dirWorld.setBlockAt(tc.footPos, &foot);
        dirWorld.setBlockAt(tc.headPos, &head);

        bed_->playerWillDestroy(dirWorld, tc.footPos, foot, player);

        const BlockState* headAfter = dirWorld.getBlockState(tc.headPos.x, tc.headPos.y, tc.headPos.z);
        EXPECT_TRUE(headAfter == nullptr || headAfter->isAir())
            << "Head should be removed for direction " << static_cast<int>(tc.facing);
    }
}

TEST_F(BedBlockDestroyTest, AdventureModeDoesNotDestroyHead)
{
    // 冒险模式也不应自动移除头部
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Adventure);
    ASSERT_FALSE(player.isCreative());

    const BlockState& footState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot);
    const BlockState& headState = bed_->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                      .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);

    world.setBlockAt(BlockPos(0, 64, 0), &footState);
    world.setBlockAt(BlockPos(0, 64, 1), &headState);

    bed_->playerWillDestroy(world, BlockPos(0, 64, 0), footState, player);

    const BlockState* headAfter = world.getBlockState(0, 64, 1);
    ASSERT_NE(headAfter, nullptr);
    EXPECT_EQ(headAfter->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Head);
}

} // namespace
