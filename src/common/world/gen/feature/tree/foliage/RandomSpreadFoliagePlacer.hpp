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
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <set>

namespace mc {

/**
 * @brief 随机散布树叶放置器
 *
 * 在每个树叶附着点周围，按 leafPlacementAttempts 次随机偏移散布树叶方块。
 * 每次尝试以 attachment.pos 为中心：
 *   - 水平偏移：nextInt(radius) - nextInt(radius)，对称三角分布，范围 [-(radius-1), radius-1]
 *   - 垂直偏移：nextInt(foliageHeight) - nextInt(foliageHeight)，范围 [-(foliageHeight-1), foliageHeight-1]
 *
 * 与球形 / 锥形树叶放置器不同，本放置器不走 placeFoliageLayer / shouldSkip 路径，
 * 而是直接在 placeFoliageInternal 中按尝试次数随机散布坐标。
 *
 * 主要用于杜鹃树（azalea_tree）等需要"蓬松散布"树冠的场景。
 */
class RandomSpreadFoliagePlacer : public FoliagePlacer {
public:
    /**
     * @brief 构造随机散布树叶放置器
     * @param radius 水平半径配置（基类要求，散布时取采样值）
     * @param offset 偏移配置（基类要求，本放置器未使用，仅为接口一致性保留）
     * @param foliageHeight 树叶垂直散布高度提供器，每次生成独立采样
     * @param leafPlacementAttempts 每个附着点的叶片放置尝试次数
     */
    RandomSpreadFoliagePlacer(const FeatureSpread& radius,
        const FeatureSpread& offset,
        std::unique_ptr<world::gen::valueprovider::IntProvider> foliageHeight,
        i32 leafPlacementAttempts);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;

    [[nodiscard]] const char* name() const override { return "random_spread"; }

    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

    /**
     * @brief 获取叶片放置尝试次数
     */
    [[nodiscard]] i32 leafPlacementAttempts() const noexcept { return m_leafPlacementAttempts; }

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

    /// 本放置器不走 layer 路径，shouldSkip 始终返回 false
    [[nodiscard]] bool shouldSkip(
        math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const override;

private:
    std::unique_ptr<world::gen::valueprovider::IntProvider> m_foliageHeight;
    i32 m_leafPlacementAttempts;
};

} // namespace mc
