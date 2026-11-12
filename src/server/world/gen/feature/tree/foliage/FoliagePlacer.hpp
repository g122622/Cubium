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
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/world/gen/feature/FeatureSpread.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include "server/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class BlockState;

/**
 * @brief 树叶落位登记接口
 *
 * 树叶在真正的 setBlock 之前就需要判定「是否已经由本棵树落位」，因此把集合判断与写入
 * 拆成两个回调，由树特征注入（原版由 `FoliageSetter` 的 `isSet` / `set` 两个方法提供）。
 * `tryPlaceLeaf` 先调用 `isSet` 判定，仅在需要落位时才调用 `set`；该顺序决定了每个叶片
 * 是否消耗一次状态采样，所以两者不能合并。
 */
class ISet {
public:
    virtual ~ISet() = default;

    /**
     * @brief 该位置是否已经记录过树叶状态
     * @param pos 查询位置
     * @return 是否已记录
     */
    [[nodiscard]] virtual bool isSet(const BlockPos& pos) const = 0;

    /**
     * @brief 记录一个树叶状态
     * @param pos 树叶位置
     * @param state 树叶状态（保存副本，生命周期与原版一致）
     */
    virtual void set(const BlockPos& pos, const BlockState& state) = 0;
};

/** @brief 每棵树在空间检查前采样一次的树冠尺寸。 */
struct FoliageShape {
    i32 height;
    i32 radius;
};

/**
 * @brief 树叶放置器基类
 *
 * 负责生成树叶。接收树干放置器返回的树叶位置列表。
 *
 * placeFoliage 主流程：
 * 1. 树高确定后先采样树冠高度与半径，空间检查及树干生成后，各附着点仅采样 offset
 * 2. 各子类在形状遍历中逐个候选位置调用 tryPlaceLeaf：命中时当场采样状态并落位
 *
 * 状态采样与落位必须在形状遍历中进行（而不是先收集坐标、最后统一落位）：加权状态提供者
 * 会消耗随机数，采样时机一旦挪到遍历之后，同一棵树上后续附着点与装饰器的取点会整体错开。
 *
 * 当 foliageProvider 非空时，每个叶片独立采样（用于杜鹃树混合杜鹃叶/开花杜鹃叶等场景）；
 * 否则使用单一 foliageBlock。
 */
class FoliagePlacer {
public:
    /**
     * @brief 构造树叶放置器
     * @param radius 半径配置
     * @param offset 偏移配置
     */
    FoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset);

    virtual ~FoliagePlacer() = default;

    /** @brief 按高度、半径顺序采样树冠尺寸。 */
    [[nodiscard]] FoliageShape sampleShape(math::IRandom& random, i32 trunkHeight) const;

    /** @brief 使用已采样的树冠尺寸逐附着点采样状态并落位树叶，附着点分别采样偏移。 */
    void placeFoliage(WorldGenRegion& world,
        math::IRandom& random,
        i32 trunkHeight,
        const std::vector<FoliagePosition>& foliagePositions,
        const FoliageShape& shape,
        const BlockState* foliageBlock,
        const world::gen::feature::state::BlockStateProvider* foliageProvider,
        std::set<BlockPos>& outFoliageBlocks,
        ISet* outLeafPositionSet);

    /**
     * @brief 获取树叶高度
     * @param random 随机数生成器
     * @param trunkHeight 树干高度
     * @return 树叶层高度
     */
    [[nodiscard]] virtual i32 getFoliageHeight(math::IRandom& random, i32 trunkHeight) const = 0;

    /**
     * @brief 获取树叶放置器类型名称
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief 克隆树叶放置器
     * @return 新的树叶放置器副本
     */
    [[nodiscard]] virtual std::unique_ptr<FoliagePlacer> clone() const = 0;

protected:
    /**
     * @brief 放置单层树叶
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param centerPos 中心位置
     * @param radius 半径
     * @param foliageBlocks 树叶方块集合
     * @param y Y坐标
     * @param trunkTop 是否是树干顶部
     * @param foliageBlock 树叶方块状态
     * @param foliageProvider 树叶状态提供者（非空时逐叶片采样，优先于 foliageBlock）
     * @param leafPositionSet 已落位树叶登记（为空时不登记，仅把坐标写入 foliageBlocks）
     */
    void placeFoliageLayer(WorldGenRegion& world,
        math::IRandom& random,
        const BlockPos& centerPos,
        i32 radius,
        std::set<BlockPos>& foliageBlocks,
        i32 y,
        bool trunkTop,
        const BlockState* foliageBlock,
        const world::gen::feature::state::BlockStateProvider* foliageProvider,
        ISet* leafPositionSet);

    /**
     * @brief 尝试在指定位置落位一片树叶
     *
     * 持久树叶与已由本棵树落位的叶片会被跳过；命中的位置在此处采样树叶状态并立即
     * 写入世界，同时把坐标记入 `foliageBlocks` 与 `leafPositionSet`。
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 树叶位置
     * @param foliageBlock 固定树叶状态（无状态提供者时使用）
     * @param foliageProvider 树叶状态提供者（非空时逐叶片采样）
     * @param foliageBlocks 树叶方块集合
     * @param leafPositionSet 已落位树叶登记
     * @return 是否真的落位了树叶
     */
    bool tryPlaceLeaf(WorldGenRegion& world,
        math::IRandom& random,
        const BlockPos& pos,
        const BlockState* foliageBlock,
        const world::gen::feature::state::BlockStateProvider* foliageProvider,
        std::set<BlockPos>& foliageBlocks,
        ISet* leafPositionSet);

    /**
     * @brief 检查是否应该跳过该位置的树叶
     *
     * @param random 随机数生成器
     * @param dx X偏移
     * @param dy Y偏移
     * @param dz Z偏移
     * @param radius 半径
     * @param trunkTop 是否是树干顶部
     * @return 是否跳过
     */
    [[nodiscard]] virtual bool shouldSkip(
        math::IRandom& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const;

    /**
     * @brief 内部放置树叶
     *
     * 由 placeFoliage 调用，子类实现具体逻辑。
     * 子类在遍历候选位置时调用 tryPlaceLeaf 直接落位（原版行为：状态采样与落位发生在
     * 形状遍历中，顺序影响随机流），因此子类必须把 leafPositionSet 一路透传下来。
     */
    virtual void placeFoliageInternal(WorldGenRegion& world,
        math::IRandom& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        const world::gen::feature::state::BlockStateProvider* foliageProvider,
        ISet* leafPositionSet) = 0;

    FeatureSpread m_radius;
    FeatureSpread m_offset;
};

} // namespace mc
