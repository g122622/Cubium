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

#pragma once

#include "FoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <memory>
#include <set>

namespace mc {

/**
 * @brief 云杉树叶放置器
 *
 * 生成尖顶形状的树叶。
 */
class SpruceFoliagePlacer : public FoliagePlacer {
public:
    SpruceFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "spruce"; }
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

    void _placeFoliageLayer(WorldGenRegion& world,
        math::Random& random,
        const FoliagePosition& foliagePos,
        i32 radius,
        i32 yOffset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        bool trunkTop);
};

} // namespace mc
