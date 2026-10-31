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
 */

#include "common/entity/ai/pathfinding/WorldRegion.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/WalkNodeEvaluator.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/util/core/CoordConverter.hpp"
#include <array>
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

namespace {

// 使用真实区块存储，保留未加载区块与未分配区块段返回空指针的语义。
class _ChunkWorld final : public test::BaseChunkBackedTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const i32 chunkX = util::core::CoordConverter::blockToChunk(x);
        const i32 chunkZ = util::core::CoordConverter::blockToChunk(z);
        const ChunkData* chunk = m_chunkManager.tryToGetChunkInMem(chunkX, chunkZ);
        return chunk != nullptr
            ? chunk->getBlockState(x - chunkX * world::CHUNK_WIDTH, y, z - chunkZ * world::CHUNK_WIDTH)
            : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : nullptr;
    }
};

// 底层在任意高度都返回水，确保区域按所属世界的高度主动裁剪读取。
class _BoundedWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return &VanillaBlocks::WATER->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return VanillaBlocks::WATER->defaultState().getFluidState();
    }

    [[nodiscard]] i32 getMinBuildHeight() const override
    {
        return world::MIN_BUILD_HEIGHT + world::CHUNK_SECTION_HEIGHT;
    }

    [[nodiscard]] i32 getMaxBuildHeight() const override
    {
        return world::MAX_BUILD_HEIGHT - world::CHUNK_SECTION_HEIGHT;
    }
};

} // namespace

TEST(WorldRegionTest, UnloadedChunkReturnsAirAndEmptyFluid)
{
    _ChunkWorld world;
    WorldRegion region(world);

    EXPECT_EQ(world.getBlockState(-17, 64, 63), nullptr);
    EXPECT_EQ(region.getBlockState(-17, 64, 63), &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(region.getFluidState(-17, 64, 63), &fluid::Fluids::EMPTY()->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, -17, 64, 63), PathType::Open);
    EXPECT_DOUBLE_EQ(WalkNodeEvaluator::getFloorLevel(region, -17, 64, 63), 63.0);
}

TEST(WorldRegionTest, EmptyChunkSectionReturnsAirAndEmptyFluid)
{
    _ChunkWorld world;
    world.ensureChunk(-1, 3);
    WorldRegion region(world);

    ASSERT_EQ(world.getBlockState(-16, 64, 63), nullptr);
    EXPECT_EQ(region.getBlockState(-16, 64, 63), &VanillaBlocks::AIR->defaultState());
    EXPECT_EQ(region.getFluidState(-16, 64, 63), &fluid::Fluids::EMPTY()->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, -16, 64, 63), PathType::Open);
}

TEST(WorldRegionTest, LoadedBlockAndFluidStatesArePreserved)
{
    _ChunkWorld world;
    ChunkData& chunk = world.ensureChunk(0, 0);
    const std::array blocks{VanillaBlocks::STONE, VanillaBlocks::WATER, VanillaBlocks::LAVA};
    const std::array types{PathType::Blocked, PathType::Water, PathType::Lava};
    WorldRegion region(world);

    for (size_t index = 0; index < blocks.size(); ++index) {
        const i32 x = static_cast<i32>(index);
        const BlockState& state = blocks[index]->defaultState();
        chunk.setBlockState(x, 64, 0, &state);
        EXPECT_EQ(region.getBlockState(x, 64, 0), &state);
        EXPECT_EQ(region.getFluidState(x, 64, 0), state.getFluidState());
        EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, x, 64, 0), types[index]);
    }
}

TEST(WorldRegionTest, BuildHeightUsesWorldBoundsAndExclusiveUpperLimit)
{
    _BoundedWorld world;
    WorldRegion region(world);

    EXPECT_EQ(region.minBuildHeight(), world.getMinBuildHeight());
    EXPECT_EQ(region.maxBuildHeight(), world.getMaxBuildHeight());
    for (const i32 y : {world.getMinBuildHeight() - 1, world.getMaxBuildHeight()}) {
        EXPECT_EQ(region.getBlockState(0, y, 0), &VanillaBlocks::AIR->defaultState());
        EXPECT_EQ(region.getFluidState(0, y, 0), &fluid::Fluids::EMPTY()->defaultState());
    }
    for (const i32 y : {world.getMinBuildHeight(), world.getMaxBuildHeight() - 1}) {
        EXPECT_EQ(region.getBlockState(0, y, 0), &VanillaBlocks::WATER->defaultState());
        EXPECT_EQ(region.getFluidState(0, y, 0), VanillaBlocks::WATER->defaultState().getFluidState());
    }
}

TEST(WorldRegionTest, NeighbourScanCrossesUnloadedChunkBoundary)
{
    _ChunkWorld world;
    ChunkData& chunk = world.ensureChunk(-1, 3);
    chunk.setBlockState(0, 63, world::CHUNK_WIDTH - 1, &VanillaBlocks::STONE->defaultState());
    WorldRegion region(world);

    // 邻域同时越过负 X 与正 Z 区块边界，仍须完成危险检测。
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, -16, 64, 63), PathType::Walkable);
    chunk.setBlockState(1, 64, world::CHUNK_WIDTH - 1, &VanillaBlocks::FIRE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, -16, 64, 63), PathType::DangerFire);
}

TEST(WorldRegionTest, PathFinderWalksAlongLoadedChunkEdge)
{
    _ChunkWorld world;
    ChunkData& chunk = world.ensureChunk(-1, 3);
    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        chunk.setBlockState(x, 63, world::CHUNK_WIDTH - 1, &VanillaBlocks::STONE->defaultState());
    }
    WorldRegion region(world);
    ecs::EntityRegistry registry{"pathfinding-region-test"};
    PigEntity pig(1, registry);
    pig.setPosition(-15.5f, 64.0f, 63.5f);
    pig.setOnGround(true);
    PathFinder finder(std::make_unique<WalkNodeEvaluator>(), PathFinder::DEFAULT_VISITED_NODES);

    const auto path =
        finder.findPath(region, pig, Vector3d(-15.5, 64.0, 63.5), {BlockPos(-12, 64, 63)}, 16.0f, 0, 1.0f);

    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->canReach());
    ASSERT_NE(path->endNode(), nullptr);
    EXPECT_EQ(path->endNode()->x(), -12);
    EXPECT_EQ(path->endNode()->y(), 64);
    EXPECT_EQ(path->endNode()->z(), 63);
}
