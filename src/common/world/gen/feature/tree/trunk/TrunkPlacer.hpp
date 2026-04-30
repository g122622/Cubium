#pragma once

#include "../../FeatureSpread.hpp"
#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../chunk/ChunkPos.hpp"
#include <vector>
#include <set>
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;
class BlockState;

/**
 * @brief 树叶位置信息
 *
 * 参考: net.minecraft.world.gen.foliageplacer.FoliagePlacer.Foliage
 */
struct FoliagePosition {
    BlockPos pos;           ///< 树叶中心位置
    i32 radiusBonus;        ///< 树叶半径加成（MC: field_236761_b_）
    bool trunkTop;          ///< 是否在树干顶部（MC: field_236762_c_）

    FoliagePosition(const BlockPos& p, i32 radiusBonus = 0, bool top = false)
        : pos(p), radiusBonus(radiusBonus), trunkTop(top) {}
};

/**
 * @brief 树干放置器基类
 *
 * 负责生成树干，返回树叶位置信息供树叶放置器使用。
 *
 * 参考: net.minecraft.world.gen.trunkplacer.AbstractTrunkPlacer
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
    virtual std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) = 0;

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
        WorldGenRegion& world,
        const BlockPos& pos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    );

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
        WorldGenRegion& world,
        const BlockPos& pos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    );

    i32 m_baseHeight;
    i32 m_heightRandA;
    i32 m_heightRandB;
};

} // namespace mc
