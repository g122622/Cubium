#include "StraightTrunkPlacer.hpp"

namespace mc {

StraightTrunkPlacer::StraightTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> StraightTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& /*random*/,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 在起始位置下方放置泥土
    placeDirtUnder(world, startPos);

    // 生成垂直树干
    for (i32 i = 0; i < height; ++i) {
        BlockPos pos = startPos.up(i);
        if (canPlaceAt(world, pos)) {
            placeBlock(world, pos, trunkBlocks, trunkBlock);
        }
    }

    // 返回树叶位置（在树干顶部）
    std::vector<FoliagePosition> foliagePositions;
    BlockPos topPos = startPos.up(height);
    foliagePositions.emplace_back(topPos, 0, false);

    return foliagePositions;
}

} // namespace mc
