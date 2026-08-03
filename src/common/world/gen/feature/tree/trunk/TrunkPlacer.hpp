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

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class BlockState;

/**
 * @brief 树叶位置信息
 *
 * 用于记录树叶生成位置和属性。
 */
struct FoliagePosition {
    BlockPos pos;    ///< 树叶中心位置
    i32 radiusBonus; ///< 树叶半径加成
    bool trunkTop;   ///< 是否在树干顶部

    FoliagePosition(const BlockPos& p, i32 radiusBonus = 0, bool top = false)
        : pos(p)
        , radiusBonus(radiusBonus)
        , trunkTop(top)
    {}
};

/**
 * @brief 树干放置器基类
 *
 * 负责生成树干，返回树叶位置信息供树叶放置器使用。
 */
class TrunkPlacer {
public:
    /**
     * @brief 构造树干放置器
     * @param baseHeight 基础高度
     * @param heightRandA 高度随机值A
     * @param heightRandB 高度随机值B
     */
    TrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    virtual ~TrunkPlacer() = default;

    /**
     * @brief 获取树干高度
     * @param random 随机数生成器
     * @return baseHeight + random(0, heightRandA) + random(0, heightRandB)
     */
    [[nodiscard]] i32 getHeight(math::Random& random) const;

    /**
     * @brief 放置树干
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param height 树干高度
     * @param startPos 起始位置
     * @param trunkBlocks 树干方块集合（用于后续计算树叶距离）
     * @param trunkBlock 树干方块状态
     * @return 树叶位置列表
     */
    virtual std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) = 0;

    /**
     * @brief 获取树干放置器类型名称
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief 克隆树干放置器
     * @return 新的树干放置器副本
     */
    [[nodiscard]] virtual std::unique_ptr<TrunkPlacer> clone() const = 0;

protected:
    /**
     * @brief 放置单个树干方块
     *
     * @param world 世界区域
     * @param pos 位置
     * @param trunkBlocks 树干方块集合
     * @param trunkBlock 树干方块状态
     */
    void placeBlock(
        WorldGenRegion& world, const BlockPos& pos, std::set<BlockPos>& trunkBlocks, const BlockState* trunkBlock);

    /**
     * @brief 检查位置是否可放置树干
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否可以放置
     */
    [[nodiscard]] static bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 在位置下方放置泥土（如果不是泥土则替换）
     *
     * @param world 世界区域
     * @param pos 位置
     */
    static void placeDirtUnder(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 放置 2x2 树干层（用于巨型树木）
     *
     * @param world 世界区域
     * @param pos 树干层起始位置
     * @param trunkBlocks 树干方块集合
     * @param trunkBlock 树干方块状态
     */
    void placeTrunkLayer2x2(
        WorldGenRegion& world, const BlockPos& pos, std::set<BlockPos>& trunkBlocks, const BlockState* trunkBlock);

    i32 m_baseHeight;
    i32 m_heightRandA;
    i32 m_heightRandB;
};

} // namespace mc
