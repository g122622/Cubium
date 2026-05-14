#pragma once

#include "FoliagePlacer.hpp"

namespace mc {

/**
 * @brief 球形树叶放置器
 *
 * 用于橡树、白桦等生成球形树冠的树木。
 * 从上到下生成多层树叶，每层半径逐渐减小。
 *
 * 参考: net.minecraft.world.gen.foliageplacer.BlobFoliagePlacer
 */
class BlobFoliagePlacer : public FoliagePlacer {
public:
    /**
     * @brief 构造球形树叶放置器
     * @param radius 半径配置
     * @param offset 偏移配置
     * @param height 树叶高度
     */
    BlobFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const override
    {
        return m_height;
    }

    [[nodiscard]] const char* name() const override { return "BlobFoliagePlacer"; }

    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const override;

private:
    i32 m_height;
};

} // namespace mc
