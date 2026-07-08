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

#include "UpwardsBranchingTrunkPlacer.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

UpwardsBranchingTrunkPlacer::UpwardsBranchingTrunkPlacer(i32 baseHeight,
    i32 heightRandA,
    i32 heightRandB,
    std::unique_ptr<world::gen::valueprovider::IntProvider> extraBranchSteps,
    f32 placeBranchPerLogProbability,
    std::unique_ptr<world::gen::valueprovider::IntProvider> extraBranchLength,
    const BlockTag* canGrowThrough)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
    , m_extraBranchSteps(std::move(extraBranchSteps))
    , m_placeBranchPerLogProbability(placeBranchPerLogProbability)
    , m_extraBranchLength(std::move(extraBranchLength))
    , m_canGrowThrough(canGrowThrough)
{}

namespace {

/// MC: Direction.Plane.HORIZONTAL.getRandomDirection(random)
Direction randomHorizontalDirection(math::Random& random)
{
    const auto dirs = Directions::horizontal();
    return dirs[static_cast<size_t>(random.nextInt(4))];
}

} // namespace

bool UpwardsBranchingTrunkPlacer::placeLog(WorldGenRegion& world,
    math::Random& /*random*/,
    const BlockPos& pos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // MC: placeLog 内部走 validTreePos，成功才 setBlockState 并返回 true。
    if (!validTreePos(world, pos)) {
        return false;
    }
    placeBlock(world, pos, trunkBlocks, trunkBlock);
    return true;
}

bool UpwardsBranchingTrunkPlacer::validTreePos(WorldGenRegion& world, const BlockPos& pos) const
{
    // MC: super.validTreePos || isStateAtPosition(canGrowThrough)
    if (canPlaceAt(world, pos)) {
        return true;
    }
    if (m_canGrowThrough != nullptr) {
        const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
        if (state != nullptr && m_canGrowThrough->contains(*state)) {
            return true;
        }
    }
    return false;
}

void UpwardsBranchingTrunkPlacer::placeBranch(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock,
    std::vector<FoliagePosition>& foliagePositions,
    i32 branchStartY,
    i32 trunkX,
    i32 trunkZ,
    Direction direction,
    i32 branchLengthOffset,
    i32 branchSteps)
{
    // MC: i = startY + branchLengthOffset; j = trunkX; k = trunkZ; l = branchLengthOffset
    i32 i = branchStartY + branchLengthOffset;
    i32 j = trunkX;
    i32 k = trunkZ;
    i32 l = branchLengthOffset;

    // MC: while (l < height && branchSteps > 0)
    while (l < height && branchSteps > 0) {
        if (l >= 1) {
            const i32 y = branchStartY + l;
            j += Directions::xOffset(direction);
            k += Directions::zOffset(direction);
            i = y;
            const BlockPos pos(j, y, k);
            if (placeLog(world, random, pos, trunkBlocks, trunkBlock)) {
                i = y + 1;
            }
            foliagePositions.emplace_back(pos, 0, false);
        }
        ++l;
        --branchSteps;
    }

    // MC: if (i - startY > 1) 追加两个树叶附着点（当前位置 + 下方2格）
    if (i - branchStartY > 1) {
        const BlockPos blockpos(j, i, k);
        foliagePositions.emplace_back(blockpos, 0, false);
        foliagePositions.emplace_back(blockpos.down(2), 0, false);
    }
}

std::vector<FoliagePosition> UpwardsBranchingTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    for (i32 i = 0; i < height; ++i) {
        const i32 y = startPos.y + i;
        const BlockPos pos(startPos.x, y, startPos.z);
        if (placeLog(world, random, pos, trunkBlocks, trunkBlock) && i < height - 1 &&
            random.nextFloat() < m_placeBranchPerLogProbability) {
            const Direction direction = randomHorizontalDirection(random);
            // MC: k = extraBranchLength.sample; l = max(0, k - extraBranchLength.sample - 1); i1 =
            // extraBranchSteps.sample
            const i32 k = m_extraBranchLength->sample(random);
            const i32 l = std::max(0, k - m_extraBranchLength->sample(random) - 1);
            const i32 i1 = m_extraBranchSteps->sample(random);
            placeBranch(world,
                random,
                height,
                trunkBlocks,
                trunkBlock,
                foliagePositions,
                y,
                startPos.x,
                startPos.z,
                direction,
                l,
                i1);
        }

        // MC: if (i == height - 1) 主干顶部追加树叶附着点（y+1）
        if (i == height - 1) {
            foliagePositions.emplace_back(BlockPos(startPos.x, y + 1, startPos.z), 0, false);
        }
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> UpwardsBranchingTrunkPlacer::clone() const
{
    return std::make_unique<UpwardsBranchingTrunkPlacer>(m_baseHeight,
        m_heightRandA,
        m_heightRandB,
        m_extraBranchSteps ? m_extraBranchSteps->clone() : nullptr,
        m_placeBranchPerLogProbability,
        m_extraBranchLength ? m_extraBranchLength->clone() : nullptr,
        m_canGrowThrough);
}

} // namespace mc
