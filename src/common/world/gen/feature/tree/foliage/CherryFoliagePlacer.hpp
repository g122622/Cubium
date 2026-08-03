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
 * @brief 樱花树叶放置器
 *
 * 生成樱花树特有的下垂伞形树冠，具有角落孔洞和底部垂叶效果。
 * 树冠从上到下逐渐变宽，底部两层有悬挂的垂叶。
 */
class CherryFoliagePlacer : public FoliagePlacer {
public:
    /**
     * @brief 构造樱花树叶放置器
     * @param radius 半径配置
     * @param offset 偏移配置
     * @param height 树叶高度
     * @param wideBottomLayerHoleChance 底层宽孔洞概率（0.25）
     * @param cornerHoleChance 角落孔洞概率（0.5）
     * @param hangingLeavesChance 垂叶概率（1/6）
     * @param hangingLeavesExtensionChance 垂叶延伸概率（1/3）
     */
    CherryFoliagePlacer(const FeatureSpread& radius,
        const FeatureSpread& offset,
        i32 height,
        f32 wideBottomLayerHoleChance,
        f32 cornerHoleChance,
        f32 hangingLeavesChance,
        f32 hangingLeavesExtensionChance);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;

    [[nodiscard]] const char* name() const override { return "cherry"; }

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
    /**
     * @brief 放置带垂叶的树叶层
     *
     * 在指定高度放置树叶层，然后在下方放置垂叶。
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param centerPos 中心位置
     * @param radius 半径
     * @param y Y坐标
     * @param foliageBlocks 树叶方块集合
     * @param foliageBlock 树叶方块状态
     * @param hangingChance 垂叶概率
     * @param extensionChance 垂叶延伸概率
     */
    void placeLeavesRowWithHangingLeavesBelow(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& centerPos,
        i32 radius,
        i32 y,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        f32 hangingChance,
        f32 extensionChance);

    i32 m_height;
    f32 m_wideBottomLayerHoleChance;
    f32 m_cornerHoleChance;
    f32 m_hangingLeavesChance;
    f32 m_hangingLeavesExtensionChance;
};

} // namespace mc
