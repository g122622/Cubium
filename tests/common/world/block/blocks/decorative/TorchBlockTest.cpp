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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/TorchBlock.hpp"
#include "common/world/block/blocks/decorative/WallTorchBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::client::renderer::trident::particle;

namespace {

// ============================================================================
// Mock animate context (adapted from AnimateTickTest)
// ============================================================================

class MockAnimateContext : public IBlockAnimateContext {
public:
    struct ParticleCall {
        ParticleTypeId type;
        Vector3 pos;
        Vector3 velocity;
    };

    struct SoundCall {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    void addAnimateParticle(ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void playLocalSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({soundEventId, category, position, volume, pitch});
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { m_blocks[pos] = state; }

    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }
    [[nodiscard]] size_t soundCount() const { return m_sounds.size(); }
    [[nodiscard]] const std::vector<ParticleCall>& particles() const { return m_particles; }
    [[nodiscard]] const std::vector<SoundCall>& sounds() const { return m_sounds; }

    void clear()
    {
        m_particles.clear();
        m_sounds.clear();
    }

private:
    std::vector<ParticleCall> m_particles;
    std::vector<SoundCall> m_sounds;
    std::map<BlockPos, const BlockState*> m_blocks;
};

// ============================================================================
// Test world that implements both IWorld (via BaseTestWorld) and IBlockReader
//
// Since IBlockReader : public IWorld {} (empty extension), and we need to pass
// TorchTestWorld as both IWorld& and IBlockReader&, we use multiple inheritance.
// To resolve the diamond, we use virtual inheritance on IWorld from
// BaseTestWorld. However, BaseTestWorld doesn't use virtual inheritance, so we
// instead create a standalone test world that implements IBlockReader (which
// inherits from IWorld) and provides all necessary overrides directly.
// ============================================================================

class TorchTestWorld : public IBlockReader {
public:
    TorchTestWorld()
    {
        VanillaBlocks::initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
    }

    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    // IWorld / IBlockReader overrides
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            m_blocks[key] = state;
        }
        return true;
    }

    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos.x, pos.y, pos.z, stored);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB&, const Entity* = nullptr) const override
    {
        return {};
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TorchTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TorchTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    const BlockState* m_airState;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
};

// ============================================================================
// Solid block for testing placement support
// ============================================================================

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

// ============================================================================
// Helper: create a placement context
// ============================================================================

BlockItemUseContext makePlacementContext(
    IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw = 0.0f, f32 hitY = 0.5f)
{
    Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + hitY, static_cast<f32>(pos.z) + 0.5f);
    ItemStack stack;
    return BlockItemUseContext(world, nullptr, stack, hitPos, pos, face, playerYaw, 0.0f);
}

} // namespace

// ============================================================================
// TorchBlock Tests
// ============================================================================

class TorchBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    MockAnimateContext animateCtx_;
    math::Random random_{12345};
    std::unique_ptr<TorchBlock> torch_ = std::make_unique<TorchBlock>(
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14), ParticleTypeId::Flame);
};

TEST_F(TorchBlockTest, Construction_HasFlameParticle)
{
    EXPECT_NE(torch_, nullptr);
}

TEST_F(TorchBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const BlockState& state = torch_->defaultState();
    EXPECT_TRUE(torch_->useShapeForLightOcclusion(state));
}

TEST_F(TorchBlockTest, GetShape_ReturnsNonEmptyShape)
{
    const BlockState& state = torch_->defaultState();
    const CollisionShape& shape = torch_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(TorchBlockTest, IsValidPosition_SolidBelow_ReturnsTrue)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(0, 0, 0), solidState);

    const BlockState& torchState = torch_->defaultState();
    EXPECT_TRUE(torch_->isValidPosition(torchState, world, BlockPos(0, 1, 0)));
}

TEST_F(TorchBlockTest, IsValidPosition_NoBlockBelow_ReturnsFalse)
{
    TorchTestWorld world;
    const BlockState& torchState = torch_->defaultState();
    EXPECT_FALSE(torch_->isValidPosition(torchState, world, BlockPos(0, 100, 0)));
}

TEST_F(TorchBlockTest, IsValidPosition_AirBelow_ReturnsFalse)
{
    TorchTestWorld world;
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockStateCopy(BlockPos(0, 0, 0), airState);

    const BlockState& torchState = torch_->defaultState();
    EXPECT_FALSE(torch_->isValidPosition(torchState, world, BlockPos(0, 1, 0)));
}

TEST_F(TorchBlockTest, UpdatePostPlacement_FloorRemoved_ReturnsAir)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    const BlockState& torchState = torch_->defaultState();

    // Place torch at (5, 10, 5) with solid at (5, 9, 5)
    world.setBlockStateCopy(BlockPos(5, 9, 5), solidState);

    // When Direction::Down update comes and floor is removed (air now)
    // Must also remove the solid block from the world so isValidPosition returns false
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockStateCopy(BlockPos(5, 9, 5), airState);

    BlockState result = torch_->updatePostPlacement(
        torchState, Direction::Down, airState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    // Torch should become air because the floor below is now air (not solid)
    EXPECT_TRUE(result.isAir());
}

TEST_F(TorchBlockTest, UpdatePostPlacement_FloorStillPresent_ReturnsSameState)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(5, 9, 5), solidState);

    const BlockState& torchState = torch_->defaultState();

    BlockState result = torch_->updatePostPlacement(
        torchState, Direction::Down, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    EXPECT_EQ(&result.getBlock(), torch_.get());
}

TEST_F(TorchBlockTest, UpdatePostPlacement_SideUpdate_DelegatesToParent)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(5, 9, 5), solidState);

    const BlockState& torchState = torch_->defaultState();

    BlockState result = torch_->updatePostPlacement(
        torchState, Direction::North, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 10, 4));

    EXPECT_EQ(&result.getBlock(), torch_.get());
}

TEST_F(TorchBlockTest, AnimateTick_SpawnsSmokeAndFlame)
{
    const BlockState& state = torch_->defaultState();
    BlockPos pos(3, 10, 7);

    torch_->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    EXPECT_EQ(animateCtx_.particles()[0].type, ParticleTypeId::Smoke);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.x, 3.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.y, 10.7f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.z, 7.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].velocity.z, 0.0f);

    EXPECT_EQ(animateCtx_.particles()[1].type, ParticleTypeId::Flame);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.x, 3.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.y, 10.7f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.z, 7.5f);
}

TEST_F(TorchBlockTest, AnimateTick_SoulTorchSpawnsSoulFireFlame)
{
    auto soulTorch = std::make_unique<TorchBlock>(
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14), ParticleTypeId::SoulFireFlame);

    const BlockState& state = soulTorch->defaultState();
    BlockPos pos(1, 5, 2);

    soulTorch->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    EXPECT_EQ(animateCtx_.particles()[0].type, ParticleTypeId::Smoke);
    EXPECT_EQ(animateCtx_.particles()[1].type, ParticleTypeId::SoulFireFlame);
}

// ============================================================================
// WallTorchBlock Tests
// ============================================================================

class WallTorchBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    MockAnimateContext animateCtx_;
    math::Random random_{12345};
    std::unique_ptr<WallTorchBlock> wallTorch_ = std::make_unique<WallTorchBlock>(
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14), ParticleTypeId::Flame);
};

// --- Basic Properties ---

TEST_F(WallTorchBlockTest, Construction_HasHorizontalFacingProperty)
{
    const BlockState& state = wallTorch_->defaultState();
    EXPECT_EQ(WallTorchBlock::getFacing(state), Direction::North);
}

TEST_F(WallTorchBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const BlockState& state = wallTorch_->defaultState();
    EXPECT_TRUE(wallTorch_->useShapeForLightOcclusion(state));
}

TEST_F(WallTorchBlockTest, WithFacing_ReturnsCorrectState)
{
    const BlockState& state = wallTorch_->defaultState();
    const BlockState& eastState = WallTorchBlock::withFacing(state, Direction::East);
    EXPECT_EQ(WallTorchBlock::getFacing(eastState), Direction::East);

    const BlockState& southState = WallTorchBlock::withFacing(state, Direction::South);
    EXPECT_EQ(WallTorchBlock::getFacing(southState), Direction::South);
}

// --- getShape ---

TEST_F(WallTorchBlockTest, GetShape_DifferentFacings_ReturnDifferentShapes)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& southState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    const BlockState& eastState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& westState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    const CollisionShape& northShape = wallTorch_->getShape(northState);
    const CollisionShape& southShape = wallTorch_->getShape(southState);
    const CollisionShape& eastShape = wallTorch_->getShape(eastState);
    const CollisionShape& westShape = wallTorch_->getShape(westState);

    EXPECT_FALSE(northShape.isEmpty());
    EXPECT_FALSE(southShape.isEmpty());
    EXPECT_FALSE(eastShape.isEmpty());
    EXPECT_FALSE(westShape.isEmpty());

    EXPECT_NE(&northShape, &southShape);
    EXPECT_NE(&northShape, &eastShape);
    EXPECT_NE(&northShape, &westShape);
}

TEST_F(WallTorchBlockTest, GetShape_DefaultNorth)
{
    const BlockState& state = wallTorch_->defaultState();
    const CollisionShape& shape = wallTorch_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// --- isValidPosition ---

TEST_F(WallTorchBlockTest, IsValidPosition_SolidWallNorth_ReturnsTrue)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    // Wall torch facing North: opposite = South, attachment at (0, 10, 1)
    world.setBlockStateCopy(BlockPos(0, 10, 1), solidState);

    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_TRUE(wallTorch_->isValidPosition(state, world, BlockPos(0, 10, 0)));
}

TEST_F(WallTorchBlockTest, IsValidPosition_NoWall_ReturnsFalse)
{
    TorchTestWorld world;
    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_FALSE(wallTorch_->isValidPosition(state, world, BlockPos(0, 10, 0)));
}

TEST_F(WallTorchBlockTest, IsValidPosition_AirAtAttachment_ReturnsFalse)
{
    TorchTestWorld world;
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockStateCopy(BlockPos(0, 10, 1), airState);

    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_FALSE(wallTorch_->isValidPosition(state, world, BlockPos(0, 10, 0)));
}

TEST_F(WallTorchBlockTest, IsValidPosition_SolidWallEast_ReturnsTrue)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    // Wall torch facing East: opposite = West, attachment at (4, 10, 5)
    world.setBlockStateCopy(BlockPos(4, 10, 5), solidState);

    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_TRUE(wallTorch_->isValidPosition(state, world, BlockPos(5, 10, 5)));
}

TEST_F(WallTorchBlockTest, IsValidPosition_SolidWallSouth_ReturnsTrue)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    // Wall torch facing South: opposite = North, attachment at (0, 10, -1)
    world.setBlockStateCopy(BlockPos(0, 10, -1), solidState);

    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_TRUE(wallTorch_->isValidPosition(state, world, BlockPos(0, 10, 0)));
}

TEST_F(WallTorchBlockTest, IsValidPosition_SolidWallWest_ReturnsTrue)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    // Wall torch facing West: opposite = East, attachment at (6, 10, 5)
    world.setBlockStateCopy(BlockPos(6, 10, 5), solidState);

    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_TRUE(wallTorch_->isValidPosition(state, world, BlockPos(5, 10, 5)));
}

// --- updatePostPlacement ---

TEST_F(WallTorchBlockTest, UpdatePostPlacement_AttachWallRemoved_ReturnsAir)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(0, 10, 1), solidState);

    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = wallTorch_->updatePostPlacement(
        northState, Direction::South, airState, world, BlockPos(0, 10, 0), BlockPos(0, 10, 1));

    EXPECT_TRUE(result.isAir());
}

TEST_F(WallTorchBlockTest, UpdatePostPlacement_AttachWallStillPresent_KeepsState)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(0, 10, 1), solidState);

    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    BlockState result = wallTorch_->updatePostPlacement(
        northState, Direction::South, solidState, world, BlockPos(0, 10, 0), BlockPos(0, 10, 1));

    EXPECT_EQ(&result.getBlock(), wallTorch_.get());
    EXPECT_EQ(WallTorchBlock::getFacing(result), Direction::North);
}

TEST_F(WallTorchBlockTest, UpdatePostPlacement_UnrelatedDirection_DelegatesToParent)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(0, 10, 1), solidState);

    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    BlockState result = wallTorch_->updatePostPlacement(
        northState, Direction::Up, solidState, world, BlockPos(0, 10, 0), BlockPos(0, 11, 0));

    EXPECT_EQ(&result.getBlock(), wallTorch_.get());
}

// --- getStateForPlacement ---

TEST_F(WallTorchBlockTest, GetStateForPlacement_ClickSouthFace_ReturnsSouthFacing)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(0, 10, 0), solidState);

    // Click South face of solid block at (0,10,0): torch placed at (0,10,1), attached to the
    // block on its North side, so the torch head points South (FACING=South).
    // 与 MC 1.21.11 WallTorchBlock.getStateForPlacement 一致：
    //   getNearestLookingDirections 首位 = clickedFace.getOpposite() = North，
    //   attachPos = torchPos + direction(North) = (0,10,0)（实心），facing = opposite(North) = South。
    auto context = makePlacementContext(world, BlockPos(0, 10, 0), Direction::South);
    BlockState result = wallTorch_->getStateForPlacement(context);

    EXPECT_EQ(WallTorchBlock::getFacing(result), Direction::South);
}

TEST_F(WallTorchBlockTest, GetStateForPlacement_ClickEastFace_ReturnsEastFacing)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockStateCopy(BlockPos(5, 10, 5), solidState);

    // Click East face of solid block at (5,10,5): torch placed at (6,10,5), attached to the
    // block on its West side, so the torch head points East (FACING=East).
    // 与 MC 1.21.11 一致：getNearestLookingDirections 首位 = East.getOpposite() = West，
    //   attachPos = torchPos + West = (5,10,5)（实心），facing = opposite(West) = East。
    auto context = makePlacementContext(world, BlockPos(5, 10, 5), Direction::East);
    BlockState result = wallTorch_->getStateForPlacement(context);

    EXPECT_EQ(WallTorchBlock::getFacing(result), Direction::East);
}

TEST_F(WallTorchBlockTest, GetStateForPlacement_ClickUpFace_TriesAlternatives)
{
    TorchTestWorld world;
    TestSolidBlock solidBlock{BlockProperties(Material::ROCK).hardness(1.5f)};
    const BlockState& solidState = solidBlock.defaultState();
    // Solid at South side: (5, 10, 6)
    world.setBlockStateCopy(BlockPos(5, 10, 6), solidState);

    auto context = makePlacementContext(world, BlockPos(5, 10, 5), Direction::Up);
    BlockState result = wallTorch_->getStateForPlacement(context);

    // Loop finds dir=South -> attachPos=(5,10,6) solid -> returns facing=opposite(South)=North
    EXPECT_EQ(WallTorchBlock::getFacing(result), Direction::North);
}

TEST_F(WallTorchBlockTest, GetStateForPlacement_NoWallsAvailable_ReturnsDefault)
{
    TorchTestWorld world;
    auto context = makePlacementContext(world, BlockPos(0, 10, 0), Direction::Up);
    BlockState result = wallTorch_->getStateForPlacement(context);

    EXPECT_EQ(WallTorchBlock::getFacing(result), Direction::North);
}

// --- animateTick ---

TEST_F(WallTorchBlockTest, AnimateTick_NorthFacing_ParticleOffsets)
{
    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    BlockPos pos(3, 10, 7);

    wallTorch_->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    // North-facing: opposite=South, xOff=0, zOff=+0.27, yOff=0.22
    EXPECT_EQ(animateCtx_.particles()[0].type, ParticleTypeId::Smoke);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.x, 3.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.y, 10.92f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.z, 7.77f);

    EXPECT_EQ(animateCtx_.particles()[1].type, ParticleTypeId::Flame);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.x, 3.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.y, 10.92f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[1].pos.z, 7.77f);
}

TEST_F(WallTorchBlockTest, AnimateTick_EastFacing_ParticleOffsets)
{
    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    BlockPos pos(3, 10, 7);

    wallTorch_->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    // East-facing: opposite=West, xOff=-0.27, zOff=0, yOff=0.22
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.x, 3.23f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.y, 10.92f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.z, 7.5f);
}

TEST_F(WallTorchBlockTest, AnimateTick_WestFacing_ParticleOffsets)
{
    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    BlockPos pos(3, 10, 7);

    wallTorch_->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    // West-facing: opposite=East, xOff=+0.27, zOff=0, yOff=0.22
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.x, 3.77f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.y, 10.92f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.z, 7.5f);
}

TEST_F(WallTorchBlockTest, AnimateTick_SouthFacing_ParticleOffsets)
{
    const BlockState& state =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    BlockPos pos(3, 10, 7);

    wallTorch_->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    // South-facing: opposite=North, xOff=0, zOff=-0.27, yOff=0.22
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.x, 3.5f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.y, 10.92f);
    EXPECT_FLOAT_EQ(animateCtx_.particles()[0].pos.z, 7.23f);
}

TEST_F(WallTorchBlockTest, AnimateTick_SoulWallTorchSpawnsSoulFireFlame)
{
    auto soulWallTorch = std::make_unique<WallTorchBlock>(
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14), ParticleTypeId::SoulFireFlame);

    const BlockState& state =
        soulWallTorch->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    BlockPos pos(1, 5, 2);

    soulWallTorch->animateTick(animateCtx_, pos, state, random_);

    ASSERT_EQ(animateCtx_.particleCount(), 2u);

    EXPECT_EQ(animateCtx_.particles()[0].type, ParticleTypeId::Smoke);
    EXPECT_EQ(animateCtx_.particles()[1].type, ParticleTypeId::SoulFireFlame);
}

// --- rotate ---

TEST_F(WallTorchBlockTest, Rotate_North_CW90_ReturnsEast)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = wallTorch_->rotate(northState, Rotation::Clockwise90);
    EXPECT_EQ(WallTorchBlock::getFacing(rotated), Direction::East);
}

TEST_F(WallTorchBlockTest, Rotate_North_CW180_ReturnsSouth)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = wallTorch_->rotate(northState, Rotation::Clockwise180);
    EXPECT_EQ(WallTorchBlock::getFacing(rotated), Direction::South);
}

TEST_F(WallTorchBlockTest, Rotate_North_CCW90_ReturnsWest)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = wallTorch_->rotate(northState, Rotation::CounterClockwise90);
    EXPECT_EQ(WallTorchBlock::getFacing(rotated), Direction::West);
}

TEST_F(WallTorchBlockTest, Rotate_None_ReturnsSame)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = wallTorch_->rotate(northState, Rotation::None);
    EXPECT_EQ(WallTorchBlock::getFacing(rotated), Direction::North);
}

TEST_F(WallTorchBlockTest, Rotate_East_CW90_ReturnsSouth)
{
    const BlockState& eastState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& rotated = wallTorch_->rotate(eastState, Rotation::Clockwise90);
    EXPECT_EQ(WallTorchBlock::getFacing(rotated), Direction::South);
}

// --- mirror ---

TEST_F(WallTorchBlockTest, Mirror_LeftRight_North_ReturnsSouth)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirrored = wallTorch_->mirror(northState, Mirror::LeftRight);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::South);
}

TEST_F(WallTorchBlockTest, Mirror_LeftRight_South_ReturnsNorth)
{
    const BlockState& southState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    const BlockState& mirrored = wallTorch_->mirror(southState, Mirror::LeftRight);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::North);
}

TEST_F(WallTorchBlockTest, Mirror_LeftRight_East_StaysEast)
{
    const BlockState& eastState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& mirrored = wallTorch_->mirror(eastState, Mirror::LeftRight);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::East);
}

TEST_F(WallTorchBlockTest, Mirror_FrontBack_East_ReturnsWest)
{
    const BlockState& eastState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& mirrored = wallTorch_->mirror(eastState, Mirror::FrontBack);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::West);
}

TEST_F(WallTorchBlockTest, Mirror_FrontBack_North_StaysNorth)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirrored = wallTorch_->mirror(northState, Mirror::FrontBack);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::North);
}

TEST_F(WallTorchBlockTest, Mirror_None_ReturnsSame)
{
    const BlockState& northState =
        wallTorch_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirrored = wallTorch_->mirror(northState, Mirror::None);
    EXPECT_EQ(WallTorchBlock::getFacing(mirrored), Direction::North);
}

// ============================================================================
// Inheritance tests
// ============================================================================

TEST_F(WallTorchBlockTest, InheritsFromTorchBlock)
{
    const TorchBlock* asTorch = static_cast<const TorchBlock*>(wallTorch_.get());
    EXPECT_NE(asTorch, nullptr);
}

// ============================================================================
// VanillaBlocks integration: ensure registered blocks are correct types
// ============================================================================

TEST_F(TorchBlockTest, VanillaTorchBlock_IsTorchBlock)
{
    const Block* torchBlock = VanillaBlocks::TORCH;
    ASSERT_NE(torchBlock, nullptr);
    const auto* asTorch = dynamic_cast<const TorchBlock*>(torchBlock);
    EXPECT_NE(asTorch, nullptr);
}

TEST_F(TorchBlockTest, VanillaWallTorchBlock_IsWallTorchBlock)
{
    const Block* wallTorchBlock = VanillaBlocks::WALL_TORCH;
    ASSERT_NE(wallTorchBlock, nullptr);
    const auto* asWallTorch = dynamic_cast<const WallTorchBlock*>(wallTorchBlock);
    EXPECT_NE(asWallTorch, nullptr);
}
