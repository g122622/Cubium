#include "BlobFoliagePlacer.hpp"
#include <cmath>

namespace mc {

BlobFoliagePlacer::BlobFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

void BlobFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 从上到下生成树叶层
    // 参考 MC BlobFoliagePlacer.func_230372_a_
    i32 startOffset = offset;

    for (i32 y = startOffset; y >= startOffset - foliageHeight; --y) {
        // 计算当前层的半径
        // 半径随高度递减
        i32 layerRadius = std::max(radius + foliagePos.radiusBonus - 1 - y / 2, 0);

        placeFoliageLayer(world,
            random,
            foliagePos.pos,
            layerRadius,
            foliageBlocks,
            foliagePos.pos.y + y,
            foliagePos.trunkTop,
            foliageBlock);
    }
}

bool BlobFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    // 参考 MC BlobFoliagePlacer.func_230373_a_
    // 跳过角落的方块，使树叶看起来更自然

    // 计算到中心的距离
    i32 absDx = std::abs(dx);
    i32 absDz = std::abs(dz);

    if (trunkTop) {
        // 如果是树干顶部，使用更宽松的规则
        absDx = std::min(absDx, std::abs(dx - 1));
        absDz = std::min(absDz, std::abs(dz - 1));
    }

    // 角落位置的方块有概率跳过
    // 当 dx == radius 且 dz == radius 时，有50%概率跳过
    if (absDx == radius && absDz == radius) {
        return random.nextInt(0, 1) == 0 && dy != 0;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> BlobFoliagePlacer::clone() const
{
    return std::make_unique<BlobFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
