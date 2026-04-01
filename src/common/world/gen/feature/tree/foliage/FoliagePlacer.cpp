#include "FoliagePlacer.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

FoliagePlacer::FoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : m_radius(radius)
    , m_offset(offset)
{
}

void FoliagePlacer::placeFoliage(
    WorldGenRegion& world,
    math::Random& random,
    i32 trunkHeight,
    const std::vector<FoliagePosition>& foliagePositions,
    const std::set<BlockPos>& /*trunkBlocks*/,
    i32 /*trunkOffset*/,
    const BlockState* foliageBlock,
    std::set<BlockPos>& outFoliageBlocks
) {
    for (const auto& foliagePos : foliagePositions) {
        i32 radius = m_radius.get(random);
        i32 offset = m_offset.get(random);
        i32 foliageHeight = getFoliageHeight(random, trunkHeight);

        placeFoliageInternal(
            world, random, trunkHeight, foliagePos,
            foliageHeight, radius, offset, outFoliageBlocks, foliageBlock
        );
    }
}

void FoliagePlacer::placeFoliageLayer(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& centerPos,
    i32 radius,
    std::set<BlockPos>& foliageBlocks,
    i32 y,
    bool trunkTop,
    const BlockState* foliageBlock
) {
    // 遍历半径范围内的所有方块
    i32 radiusOffset = trunkTop ? 1 : 0;
    BlockPos pos;

    for (i32 dx = -radius; dx <= radius + radiusOffset; ++dx) {
        for (i32 dz = -radius; dz <= radius + radiusOffset; ++dz) {
            // 检查是否跳过该位置
            if (shouldSkip(random, dx, 0, dz, radius, trunkTop)) {
                continue;
            }

            pos.x = centerPos.x + dx;
            pos.y = y;
            pos.z = centerPos.z + dz;

            // 检查位置是否在有效范围内
            if (pos.y < 0 || pos.y >= 256) {
                continue;
            }

            // 检查是否可以放置树叶
            const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
            if (state == nullptr || state->isAir()) {
                // 空气可以放置
            } else if (state->is(VanillaBlocks::OAK_LEAVES) ||
                       state->is(VanillaBlocks::SPRUCE_LEAVES) ||
                       state->is(VanillaBlocks::BIRCH_LEAVES) ||
                       state->is(VanillaBlocks::JUNGLE_LEAVES) ||
                       state->is(VanillaBlocks::ACACIA_LEAVES) ||
                       state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                // 树叶可以替换
            } else {
                continue;
            }

            // 放置树叶
            if (foliageBlock != nullptr) {
                world.setBlock(pos.x, pos.y, pos.z, foliageBlock);
                foliageBlocks.insert(pos);
            }
        }
    }
}

} // namespace mc
