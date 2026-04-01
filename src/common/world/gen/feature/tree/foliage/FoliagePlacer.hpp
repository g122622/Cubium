#pragma once

#include "../../FeatureSpread.hpp"
#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../chunk/ChunkPos.hpp"
#include "../trunk/TrunkPlacer.hpp"
#include <vector>
#include <set>
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;
class BlockState;

/**
 * @brief 树叶放置器基类
 *
 * 负责生成树叶。接收树干放置器返回的树叶位置列表。
 *
 * 参考: net.minecraft.world.gen.foliageplacer.FoliagePlacer
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

    /**
     * @brief 放置树叶
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param trunkHeight 树干高度
     * @param foliagePositions 树叶位置列表
     * @param trunkBlocks 树干方块集合
     * @param trunkOffset 树干顶部的偏移（从树干顶部到树叶底部的距离）
     * @param foliageBlock 树叶方块状态
     * @param outFoliageBlocks 输出参数，放置的树叶方块位置集合
     */
    void placeFoliage(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const std::vector<FoliagePosition>& foliagePositions,
        const std::set<BlockPos>& trunkBlocks,
        i32 trunkOffset,
        const BlockState* foliageBlock,
        std::set<BlockPos>& outFoliageBlocks
    );

    /**
     * @brief 获取树叶高度
     * @param random 随机数生成器
     * @param trunkHeight 树干高度
     * @return 树叶层高度
     */
    [[nodiscard]] virtual i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const = 0;

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
     */
    void placeFoliageLayer(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& centerPos,
        i32 radius,
        std::set<BlockPos>& foliageBlocks,
        i32 y,
        bool trunkTop,
        const BlockState* foliageBlock
    );

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
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const = 0;

    /**
     * @brief 内部放置树叶
     *
     * 由 placeFoliage 调用，子类实现具体逻辑。
     */
    virtual void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) = 0;

    FeatureSpread m_radius;
    FeatureSpread m_offset;
};

} // namespace mc
