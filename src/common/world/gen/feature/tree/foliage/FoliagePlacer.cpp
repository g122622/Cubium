#include "FoliagePlacer.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../../../core/Types.hpp"
#include "../../../../../core/Constants.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

FoliagePlacer::FoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : m_radius(radius)
    , m_offset(offset)
{
}

bool FoliagePlacer::shouldSkip(
    math::Random& /*random*/,
    i32 dx, i32 /*dy*/, i32 dz,
    i32 radius,
    bool trunkTop
) const {
    // 基类默认实现：跳过角落
    // 参考 MC FoliagePlacer.func_230373_a_
    if (trunkTop) {
        i32 absDx = std::abs(dx);
        i32 absDz = std::abs(dz);
        absDx = std::min(absDx, std::abs(dx - 1));
        absDz = std::min(absDz, std::abs(dz - 1));
        return absDx == radius && absDz == radius;
    }
    return std::abs(dx) == radius && std::abs(dz) == radius;
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

    // 子类只负责计算并收集树叶坐标，这里统一执行实际放置。
    // 允许覆盖空气或已有树叶，避免覆盖实心方块。
    if (foliageBlock == nullptr) {
        return;
    }

    for (const auto& pos : outFoliageBlocks) {
        if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
            continue;
        }

        const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
        if (state == nullptr || state->isAir() ||
            state->is(VanillaBlocks::OAK_LEAVES) ||
            state->is(VanillaBlocks::SPRUCE_LEAVES) ||
            state->is(VanillaBlocks::BIRCH_LEAVES) ||
            state->is(VanillaBlocks::JUNGLE_LEAVES) ||
            state->is(VanillaBlocks::ACACIA_LEAVES) ||
            state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
            world.setBlock(pos, foliageBlock);
        }
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
            if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
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
                world.setBlock(pos, foliageBlock);
                foliageBlocks.insert(pos);
            }
        }
    }
}

} // namespace mc
